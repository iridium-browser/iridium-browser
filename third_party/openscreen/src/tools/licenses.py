#!/usr/bin/env python3
# Copyright 2020 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Utility for checking and processing licensing information in third_party
directories. Copied and forked from Chrome's tools/licenses.py.

Usage: licenses.py <command>

Commands:
  scan     scan third_party directories, verifying that we have licensing info
  credits  generate about:credits on stdout

(You can also import this as a module.)
"""
from __future__ import print_function

import argparse
import codecs
import logging
import os
import shutil
import re
import subprocess
import sys
import tempfile
from typing import List

# TODO(issuetracker.google.com/173766869): Remove Python2 checks/compatibility.
if sys.version_info.major == 2:
    import cgi as html
else:
    import html

_REPOSITORY_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

# Paths from the root of the tree to directories to skip.
PRUNE_PATHS = set([
    # Used for development and test, not in the shipping product.
    os.path.join('third_party', 'llvm-build'),
])

# Directories we don't scan through.
PRUNE_DIRS = ('.git')

# Directories where we check out directly from upstream, and therefore can't
# provide a README.openscreen.  Please prefer a README.openscreen wherever
# possible.
SPECIAL_CASES = {
    os.path.join('third_party', 'googletest'): {
        "Name": "gtest",
        "URL": "http://code.google.com/p/googletest",
        "License": "BSD",
        "Shipped": "no",
    },
    os.path.join('third_party', 'clang-format'): {
        "Name": "clang format",
        "URL":
        "https://chromium.googlesource.com/external/github.com/llvm/llvm-project/clang/tools/clang-format",
        "Shipped": "no",
        "License": "Apache 2.0",
        "License File": "NOT_SHIPPED",
    },
}

# The delimiter used to separate license files specified in the 'License File'
# field.
LICENSE_FILE_DELIMITER = ","

# Deprecated special value for 'License File' field used to indicate
# that the library is not shipped so the license file should not be used in
# about:credits.
# This value is still supported, but the preferred method is to set the
# 'Shipped' field to 'no' in the library's README.chromium/README.openscreen.
NOT_SHIPPED = "NOT_SHIPPED"

# Valid values for the 'Shipped' field used to indicate whether the library is
# shipped and consequently whether the license file should be used in
# about:credits.
YES = "yes"
NO = "no"


def MakeDirectory(dir_path):
    try:
        os.makedirs(dir_path)
    except OSError:
        pass


def WriteDepfile(depfile_path, first_gn_output, inputs=None):
    assert depfile_path != first_gn_output  # http://crbug.com/646165
    assert not isinstance(inputs, string_types)  # Easy mistake to make
    inputs = inputs or []
    MakeDirectory(os.path.dirname(depfile_path))
    # Ninja does not support multiple outputs in depfiles.
    with open(depfile_path, 'w') as depfile:
        depfile.write(first_gn_output.replace(' ', '\\ '))
        depfile.write(': ')
        depfile.write(' '.join(i.replace(' ', '\\ ') for i in inputs))
        depfile.write('\n')


class LicenseError(Exception):
    """We raise this exception when a directory's licensing info isn't
    fully filled out."""
    pass


def AbsolutePath(path, filename, root):
    """Convert a README path to be absolute based on the source root."""
    if filename.startswith('/'):
        # Absolute-looking paths are relative to the source root
        # (which is the directory we're run from).
        absolute_path = os.path.join(root, filename[1:])
    else:
        absolute_path = os.path.join(root, path, filename)
    if os.path.exists(absolute_path):
        return absolute_path
    return None


def ParseDir(path,
             root,
             require_license_file=True,
             optional_keys=None,
             enable_warnings=False):
    """Examine a third_party/foo component and extract its metadata."""
    # Parse metadata fields out of README.chromium.
    # We examine "LICENSE" for the license file by default.
    metadata = {
        "License File": ["LICENSE"],  # Relative paths to license text.
        "Name": None,  # Short name (for header on about:credits).
        "URL": None,  # Project home page.
        "License": None,  # Software license.
        "Shipped": None,  # Whether the package is in the shipped product.
    }

    if optional_keys is None:
        optional_keys = []

    readme_path = ""
    if path in SPECIAL_CASES:
        readme_path = f"licenses.py SPECIAL_CASES entry for {path}"
        metadata.update(SPECIAL_CASES[path])
    else:
        # Try to find README.chromium.
        readme_path = os.path.join(root, path, 'README.chromium')
        if not os.path.exists(readme_path):
            readme_path = os.path.join(root, path, 'README.openscreen')

        if not os.path.exists(readme_path):
            raise LicenseError(
                "missing README.chromium, README.openscreen, or "
                "licenses.py SPECIAL_CASES entry in %s\n" % path)

        for line in open(readme_path):
            line = line.strip()
            if not line:
                break
            for key in list(metadata.keys()) + optional_keys:
                field = key + ": "
                if line.startswith(field):
                    value = line[len(field):]
                    # Multiple license files can be specified.
                    if key == "License File":
                        licenses = value.split(LICENSE_FILE_DELIMITER)
                        metadata[key] = [
                            license.strip() for license in licenses
                        ]
                    else:
                        metadata[key] = value

    if enable_warnings:
        # Check for the deprecated special value in the "License File" field.
        if NOT_SHIPPED in metadata["License File"]:
            logging.warning(f"{readme_path} is using deprecated {NOT_SHIPPED} "
                            "value in 'License File' field - remove this and "
                            f"instead specify 'Shipped: {NO}'.")

            # Check the "Shipped" and "License File" fields do not contradict.
            if metadata["Shipped"] == YES:
                logging.warning(
                    f"Contradictory metadata for {readme_path} - "
                    f"'Shipped: {YES}' but 'License File' includes "
                    f"'{NOT_SHIPPED}'")

    # If the "Shipped" field isn't present, set it based on the value of the
    # "License File" field.
    if not metadata["Shipped"]:
        shipped = YES
        if NOT_SHIPPED in metadata["License File"]:
            shipped = NO
        metadata["Shipped"] = shipped

    # Check that all expected metadata is present.
    errors = []
    for key, value in metadata.items():
        if not value:
            errors.append("couldn't find '" + key + "' line "
                          "in README file or licenses.py SPECIAL_CASES")

    # For the modules that are in the shipping product, we need their license in
    # about:credits, so update the license files to be the full paths.
    license_paths = process_license_files(root, path, metadata["License File"])
    if (metadata["Shipped"] == YES and require_license_file
            and not license_paths):
        errors.append("License file not found. "
                      "Either add a file named LICENSE, "
                      "import upstream's COPYING if available, "
                      "or add a 'License File:' line to README.chromium or "
                      "README.openscreen with the appropriate paths.")
    metadata["License File"] = license_paths

    if errors:
        raise LicenseError("Errors in %s:\n %s\n" %
                           (path, ";\n ".join(errors)))
    return metadata


def process_license_files(
    root: str,
    path: str,
    license_files: List[str],
) -> List[str]:
    """
    Convert a list of license file paths which were specified in a
    README.chromium / README.openscreen to be absolute paths based on the source
    root.

    Args:
        root: the repository source root.
        path: the relative path from root.
        license_files: list of values specified in the 'License File' field.

    Returns: absolute paths to license files that exist.
    """
    license_paths = []
    for file_path in license_files:
        if file_path == NOT_SHIPPED:
            continue

        license_path = AbsolutePath(path, file_path, root)
        # Check that the license file exists.
        if license_path is not None:
            license_paths.append(license_path)

    # If there are no license files at all, check for the COPYING license file.
    if not license_paths:
        license_path = AbsolutePath(path, "COPYING", root)
        # Check that the license file exists.
        if license_path is not None:
            license_paths.append(license_path)

    return license_paths


def ContainsFiles(path, root):
    """Determines whether any files exist in a directory or in any of its
    subdirectories."""
    for _, dirs, files in os.walk(os.path.join(root, path)):
        if files:
            return True
        for prune_dir in PRUNE_DIRS:
            if prune_dir in dirs:
                dirs.remove(prune_dir)
    return False


def FilterDirsWithFiles(dirs_list, root):
    # If a directory contains no files, assume it's a DEPS directory for a
    # project not used by our current configuration and skip it.
    return [x for x in dirs_list if ContainsFiles(x, root)]


def FindThirdPartyDirs(prune_paths, root):
    """Find all third_party directories underneath the source root."""
    third_party_dirs = set()
    for path, dirs, files in os.walk(root):
        path = path[len(root) + 1:]  # Pretty up the path.

        # .gitignore ignores /out*/, so do the same here.
        if path in prune_paths or path.startswith('out'):
            dirs[:] = []
            continue

        # Prune out directories we want to skip.
        # (Note that we loop over PRUNE_DIRS so we're not iterating over a
        # list that we're simultaneously mutating.)
        for skip in PRUNE_DIRS:
            if skip in dirs:
                dirs.remove(skip)

        if os.path.basename(path) == 'third_party':
            # Add all subdirectories that are not marked for skipping.
            for dir in dirs:
                dirpath = os.path.join(path, dir)
                if dirpath not in prune_paths:
                    third_party_dirs.add(dirpath)

            # Don't recurse into any subdirs from here.
            dirs[:] = []
            continue

    return third_party_dirs


def FindThirdPartyDirsWithFiles(root):
    third_party_dirs = FindThirdPartyDirs(PRUNE_PATHS, root)
    return FilterDirsWithFiles(third_party_dirs, root)


# Many builders do not contain 'gn' in their PATH, so use the GN binary from
# //buildtools.
def _GnBinary():
    exe = 'gn'
    if sys.platform.startswith('linux'):
        subdir = 'linux64'
    elif sys.platform == 'darwin':
        subdir = 'mac'
    elif sys.platform == 'win32':
        subdir, exe = 'win', 'gn.exe'
    else:
        raise RuntimeError("Unsupported platform '%s'." % sys.platform)

    return os.path.join(_REPOSITORY_ROOT, 'buildtools', subdir, exe)


def GetThirdPartyDepsFromGNDepsOutput(gn_deps, target_os):
    """Returns third_party/foo directories given the output of "gn desc deps".

    Note that it always returns the direct sub-directory of third_party
    where README.chromium/README.openscreen and LICENSE files are, so that it
    can be passed to ParseDir(). e.g.:
        third_party/cld_3/src/src/BUILD.gn -> third_party/cld_3

    It returns relative paths from _REPOSITORY_ROOT, not absolute paths.
    """
    third_party_deps = set()
    for absolute_build_dep in gn_deps.split():
        relative_build_dep = os.path.relpath(absolute_build_dep,
                                             _REPOSITORY_ROOT)
        m = re.search(
            r'^((.+[/\\])?third_party[/\\][^/\\]+[/\\])(.+[/\\])?BUILD\.gn$',
            relative_build_dep)
        if not m:
            continue
        third_party_path = m.group(1)
        if any(third_party_path.startswith(p + os.sep) for p in PRUNE_PATHS):
            continue
        third_party_deps.add(third_party_path[:-1])
    return third_party_deps


def FindThirdPartyDeps(gn_out_dir, gn_target, target_os):
    if not gn_out_dir:
        raise RuntimeError("--gn-out-dir is required if --gn-target is used.")

    # Generate gn project in temp directory and use it to find dependencies.
    # Current gn directory cannot be used when we run this script in a gn action
    # rule, because gn doesn't allow recursive invocations due to potential side
    # effects.
    tmp_dir = None
    try:
        tmp_dir = tempfile.mkdtemp(dir=gn_out_dir)
        shutil.copy(os.path.join(gn_out_dir, "args.gn"), tmp_dir)
        subprocess.check_output([_GnBinary(), "gen", tmp_dir])
        gn_deps = subprocess.check_output([
            _GnBinary(), "desc", tmp_dir, gn_target, "deps", "--as=buildfile",
            "--all"
        ])
        if isinstance(gn_deps, bytes):
            gn_deps = gn_deps.decode("utf-8")
    finally:
        if tmp_dir and os.path.exists(tmp_dir):
            shutil.rmtree(tmp_dir)

    return GetThirdPartyDepsFromGNDepsOutput(gn_deps, target_os)


def ScanThirdPartyDirs(root=None):
    """Scan a list of directories and report on any problems we find."""
    if root is None:
        root = os.getcwd()
    third_party_dirs = FindThirdPartyDirsWithFiles(root)

    errors = []
    for path in sorted(third_party_dirs):
        try:
            ParseDir(path, root, enable_warnings=True)
        except LicenseError as e:
            errors.append((path, e.args[0]))
            continue

    return ['{}: {}'.format(path, error) for path, error in sorted(errors)]


def GenerateCredits(file_template_file,
                    entry_template_file,
                    output_file,
                    target_os,
                    gn_out_dir,
                    gn_target,
                    depfile=None,
                    enable_warnings=False):
    """Generate about:credits."""

    def EvaluateTemplate(template, env, escape=True):
        """Expand a template with variables like {{foo}} using a
        dictionary of expansions."""
        for key, val in env.items():
            if escape:
                val = html.escape(val)
            template = template.replace('{{%s}}' % key, val)
        return template

    def MetadataToTemplateEntry(metadata, entry_template):
        licenses = []
        for filepath in metadata['License File']:
            licenses.append(codecs.open(filepath, encoding='utf-8').read())
        license_content = '\n\n'.join(licenses)

        env = {
            'name': metadata['Name'],
            'url': metadata['URL'],
            'license': license_content,
        }
        return {
            'name': metadata['Name'],
            'content': EvaluateTemplate(entry_template, env),
            'license_file': metadata['License File'],
        }

    if gn_target:
        third_party_dirs = FindThirdPartyDeps(gn_out_dir, gn_target, target_os)

        # Sanity-check to raise a build error if invalid gn_... settings are
        # somehow passed to this script.
        if not third_party_dirs:
            raise RuntimeError("No deps found.")
    else:
        third_party_dirs = FindThirdPartyDirs(PRUNE_PATHS, _REPOSITORY_ROOT)

    if not file_template_file:
        # Note: this path assumes the repo root follows the form
        # chromium/src/third_party/openscreen/src
        file_template_file = os.path.abspath(
            os.path.join(_REPOSITORY_ROOT, '..', '..', '..', 'components',
                         'about_ui', 'resources', 'about_credits.tmpl'))
    if not os.path.exists(file_template_file):
        raise FileNotFoundError(
            f'about:credits template not found at {file_template_file}')

    if not entry_template_file:
        # Note: this path assumes the repo root follows the form
        # chromium/src/third_party/openscreen/src
        entry_template_file = os.path.abspath(
            os.path.join(_REPOSITORY_ROOT, '..', '..', '..', 'components',
                         'about_ui', 'resources', 'about_credits_entry.tmpl'))
    if not os.path.exists(entry_template_file):
        raise FileNotFoundError(
            f'about:credits entry template not found at {entry_template_file}')

    entry_template = open(entry_template_file).read()
    entries = []
    # Start from Chromium's LICENSE file
    chromium_license_metadata = {
        'Name': 'The Chromium Project',
        'URL': 'http://www.chromium.org',
        'Shipped': 'yes',
        'License File': [os.path.join(_REPOSITORY_ROOT, 'LICENSE')],
    }
    entries.append(
        MetadataToTemplateEntry(chromium_license_metadata, entry_template))

    entries_by_name = {}
    for path in third_party_dirs:
        try:
            metadata = ParseDir(path,
                                _REPOSITORY_ROOT,
                                enable_warnings=enable_warnings)
        except LicenseError:
            # TODO(phajdan.jr): Convert to fatal error (http://crbug.com/39240).
            continue
        if metadata['Shipped'] == NO:
            continue

        new_entry = MetadataToTemplateEntry(metadata, entry_template)
        # Skip entries that we've already seen.
        prev_entry = entries_by_name.setdefault(new_entry['name'], new_entry)
        if prev_entry is not new_entry and (
                prev_entry['content'] == new_entry['content']):
            continue

        entries.append(new_entry)

    entries.sort(key=lambda entry: (entry['name'].lower(), entry['content']))
    for entry_id, entry in enumerate(entries):
        entry['content'] = entry['content'].replace('{{id}}', str(entry_id))

    entries_contents = '\n'.join([entry['content'] for entry in entries])
    file_template = open(file_template_file).read()
    template_contents = "<!-- Generated by licenses.py; do not edit. -->"
    template_contents += EvaluateTemplate(file_template,
                                          {'entries': entries_contents},
                                          escape=False)

    if output_file:
        changed = True
        try:
            old_output = open(output_file, 'r').read()
            if old_output == template_contents:
                changed = False
        except:
            pass
        if changed:
            with open(output_file, 'w') as output:
                output.write(template_contents)
    else:
        print(template_contents)

    if depfile:
        assert output_file
        # Add in build.ninja so that the target will be considered dirty when
        # gn gen is run. Otherwise, it will fail to notice new files being
        # added. This is still not perfect, as it will fail if no build files
        # are changed, but a new README.* / LICENSE is added. This shouldn't
        # happen in practice however.
        license_file_list = []
        for entry in entries:
            license_file_list.extend(entry['license_file'])
        license_file_list = (os.path.relpath(p) for p in license_file_list)
        license_file_list = sorted(set(license_file_list))
        WriteDepfile(depfile, output_file, license_file_list + ['build.ninja'])

    return True


def _ReadFile(path):
    """Reads a file from disk.
    Args:
      path: The path of the file to read, relative to the root of the
      repository.
    Returns:
      The contents of the file as a string.
    """
    with codecs.open(os.path.join(_REPOSITORY_ROOT, path), 'r', 'utf-8') as f:
        return f.read()


def GenerateLicenseFile(output_file, gn_out_dir, gn_target, target_os,
                        enable_warnings):
    """Generate a plain-text LICENSE file which can be used when you ship a part
    of Chromium code (specified by gn_target) as a stand-alone library
    (e.g., //ios/web_view).

    The LICENSE file contains licenses of both Chromium and third-party
    libraries which gn_target depends on. """

    third_party_dirs = FindThirdPartyDeps(gn_out_dir, gn_target, target_os)

    # Start with Chromium's LICENSE file.
    content = [_ReadFile('LICENSE')]

    # Add necessary third_party.
    for directory in sorted(third_party_dirs):
        metadata = ParseDir(directory,
                            _REPOSITORY_ROOT,
                            require_license_file=True,
                            enable_warnings=enable_warnings)
        shipped = metadata['Shipped']
        license_files = metadata['License File']
        if shipped == YES and license_files:
            content.append('-' * 20)
            content.append(directory.split(os.sep)[-1])
            content.append('-' * 20)
            for license_file in license_files:
                content.append(_ReadFile(license_file))

    content_text = '\n'.join(content)

    if output_file:
        with codecs.open(output_file, 'w', 'utf-8') as output:
            output.write(content_text)
    else:
        print(content_text)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--file-template',
                        help='Template HTML to use for the license page.')
    parser.add_argument('--entry-template',
                        help='Template HTML to use for each license.')
    parser.add_argument('--target-os', help='OS that this build is targeting.')
    parser.add_argument('--gn-out-dir',
                        help='GN output directory for scanning dependencies.')
    parser.add_argument('--gn-target',
                        help='GN target to scan for dependencies.')
    parser.add_argument('--enable-warnings',
                        action='store_true',
                        help='Enables warning logs when processing directory '
                        'metadata for credits or license file generation.')
    parser.add_argument('command',
                        choices=['help', 'scan', 'credits', 'license_file'])
    parser.add_argument('output_file', nargs='?')
    parser.add_argument('--depfile',
                        help='Path to depfile (refer to `gn help depfile`)')
    args = parser.parse_args()

    if args.command == 'scan':
        if not ScanThirdPartyDirs():
            return 1
    elif args.command == 'credits':
        if not GenerateCredits(args.file_template, args.entry_template,
                               args.output_file, args.target_os,
                               args.gn_out_dir, args.gn_target, args.depfile,
                               args.enable_warnings):
            return 1
    elif args.command == 'license_file':
        try:
            GenerateLicenseFile(args.output_file, args.gn_out_dir,
                                args.gn_target, args.target_os,
                                args.enable_warnings)
        except LicenseError as e:
            print("Failed to parse README file: {}".format(e))
            return 1
    else:
        print(__doc__)
        return 1


if __name__ == '__main__':
    sys.exit(main())
