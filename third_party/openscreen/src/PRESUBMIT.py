# Copyright 2018 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# pragma pylint: disable=invalid-name, import-error
"""
This file acts as a git pre-submission checker, and uses the support tooling
from depot_tools to check a variety of style and programming requirements.
"""

from collections import namedtuple
import os
import re
import sys


_REPO_PATH = os.path.dirname(os.path.realpath('__file__'))
_IMPORT_SUBFOLDERS = ['tools', os.path.join('buildtools', 'checkdeps')]

# git-cl upload is not compatible with __init__.py based subfolder imports, so
# we extend the system path instead.
sys.path.extend(os.path.join(_REPO_PATH, p) for p in _IMPORT_SUBFOLDERS)

from checkdeps import DepsChecker  # pylint: disable=wrong-import-position
import licenses  # pylint: disable=wrong-import-position

# Opt-in to using Python3 instead of Python2, as part of the ongoing Python2
# deprecation. For more information, see
# https://issuetracker.google.com/173766869.
USE_PYTHON3 = True


def _CheckLicenses(input_api, output_api):
    """Checks third party licenses and returns a list of violations."""
    # NOTE: the licenses check is confused by the fact that we don't actually
    # check ou the libraries in buildtools/third_party, so explicitly exclude
    # that folder. See https://crbug.com/1215335 for more info.
    licenses.PRUNE_PATHS.add(os.path.join('buildtools', 'third_party'))

    if any(s.LocalPath().startswith('third_party')
           for s in input_api.change.AffectedFiles()):
        return [
            output_api.PresubmitError(v)
            for v in licenses.ScanThirdPartyDirs()
        ]
    return []


def _CheckDeps(input_api, output_api):
    """Checks DEPS rules and returns a list of violations."""
    deps_checker = DepsChecker(input_api.PresubmitLocalPath())
    deps_checker.CheckDirectory(input_api.PresubmitLocalPath())
    deps_results = deps_checker.results_formatter.GetResults()
    return [output_api.PresubmitError(v) for v in deps_results]


# Arguments passed to methods by cpplint.
CpplintArgs = namedtuple("CpplintArgs", "filename clean_lines linenum error")

# A defined error to return to cpplint.
Error = namedtuple("Error", "type message")


def _CheckNoRegexMatches(regex, cpplint_args, error, include_cpp_files=True):
    """Checks that there are no matches for a specific regex.

  Args:
    regex: The regex to use for matching.
    cpplint_args: The arguments passed to us by cpplint.
    error: The error to return if we find an issue.
    include_cpp_files: Whether C++ files should be checked.
    """
    if not include_cpp_files and not cpplint_args.filename.endswith('.h'):
        return

    line = cpplint_args.clean_lines.elided[cpplint_args.linenum]
    matched = regex.match(line)
    if matched:
        cpplint_args.error(
            cpplint_args.filename, cpplint_args.linenum, error.type, 4,
            f'Error: {error.message} at {matched.group(0).strip()}')


# Matches OSP_CHECK(foo.is_value()) or OSP_DCHECK(foo.is_value())
_RE_PATTERN_VALUE_CHECK = re.compile(
    r'\s*OSP_D?CHECK\([^)]*\.is_value\(\)\);\s*')


def _CheckNoValueDchecks(filename, clean_lines, linenum, error):
    """Checks that there are no OSP_DCHECK(foo.is_value()) instances.

    filename: The name of the current file.
    clean_lines: A CleansedLines instance containing the file.
    linenum: The number of the line to check.
    error: The function to call with any errors found.
    """
    cpplint_args = CpplintArgs(filename, clean_lines, linenum, error)
    error_to_return = Error('runtime/is_value_dchecks',
                            'Unnecessary CHECK for ErrorOr::is_value()')

    _CheckNoRegexMatches(_RE_PATTERN_VALUE_CHECK, cpplint_args,
                         error_to_return)


# Matches Foo(Foo&&) when not followed by noexcept.
_RE_PATTERN_MOVE_WITHOUT_NOEXCEPT = re.compile(
    r'\s*(?P<classname>\w+)\((?P=classname)&&[^)]*\)\s*(?!noexcept)\s*[{;=]')


def _CheckNoexceptOnMove(filename, clean_lines, linenum, error):
    """Checks that move constructors are declared with 'noexcept'.

    filename: The name of the current file.
    clean_lines: A CleansedLines instance containing the file.
    linenum: The number of the line to check.
    error: The function to call with any errors found.
    """
    cpplint_args = CpplintArgs(filename, clean_lines, linenum, error)
    error_to_return = Error('runtime/noexcept',
                            'Move constructor not declared \'noexcept\'')

    # We only check headers as noexcept is meaningful on declarations, not
    # definitions.  This may skip some definitions in .cc files though.
    _CheckNoRegexMatches(_RE_PATTERN_MOVE_WITHOUT_NOEXCEPT, cpplint_args,
                         error_to_return, False)


# Gives additional debug information whenever a linting error occurs.
_CPPLINT_VERBOSE_LEVEL = 4

# - We disable c++11 header checks since Open Screen allows them.
# - We disable whitespace/braces because of various false positives.
# - There are some false positives with 'explicit' checks, but it's useful
#   enough to keep.
# - We add a custom check for 'noexcept' usage.
def _CheckChangeLintsClean(input_api, output_api):
    """Checks that all '.cc' and '.h' files pass cpplint.py."""
    cpplint = input_api.cpplint
    # Directive that allows access to a protected member _XX of a client class.
    # pylint: disable=protected-access
    cpplint._cpplint_state.ResetErrorCounts()

    cpplint._SetFilters('-build/c++11,-whitespace/braces')
    files = [
        f.AbsoluteLocalPath() for f in input_api.AffectedSourceFiles(None)
    ]

    for file_name in files:
        cpplint.ProcessFile(file_name, _CPPLINT_VERBOSE_LEVEL,
                            [_CheckNoexceptOnMove, _CheckNoValueDchecks])

    if cpplint._cpplint_state.error_count:
        if input_api.is_committing:
            res_type = output_api.PresubmitError
        else:
            res_type = output_api.PresubmitPromptWarning
        return [res_type('Changelist failed cpplint.py check.')]

    return []


def _CheckLuciCfgLint(input_api, output_api):
    """Check that the luci configs pass the linter."""
    path = os.path.join('infra', 'config', 'global', 'main.star')
    pred = lambda f : os.path.samefile(f.AbsoluteLocalPath(), path)
    if not input_api.AffectedSourceFiles(pred):
        return []

    result = []
    result.extend(input_api.RunTests([input_api.Command(
        'lucicfg lint',
        [
            'lucicfg' if not input_api.is_windows else 'lucicfg.bat', 'lint',
            path, '--log-level', 'debug' if input_api.verbose else 'warning'
        ],
        {
            'stderr': input_api.subprocess.STDOUT,
            'shell': input_api.is_windows,  # to resolve *.bat
            'cwd': input_api.PresubmitLocalPath(),
        },
        output_api.PresubmitError)]))
    return result


def _CommonChecks(input_api, output_api):
    """Performs a list of checks that should be used for both presubmission and
       upload validation.
    """
    # PanProjectChecks include:
    #   CheckLongLines (@ 80 cols)
    #   CheckChangeHasNoTabs
    #   CheckChangeHasNoStrayWhitespace
    #   CheckChangeWasUploaded (if committing)
    #   CheckChangeHasDescription
    #   CheckDoNotSubmitInDescription
    #   CheckDoNotSubmitInFiles
    #   CheckLicenses
    results = input_api.canned_checks.PanProjectChecks(input_api,
                                                       output_api,
                                                       owners_check=False)

    # No carriage return characters, files end with one EOL (\n).
    results.extend(
        input_api.canned_checks.CheckChangeHasNoCrAndHasOnlyOneEol(
            input_api, output_api))

    # Ensure code change is gender inclusive.
    results.extend(
        input_api.canned_checks.CheckGenderNeutral(input_api, output_api))

    # Ensure code change to do items uses TODO(bug) or TODO(user) format.
    #  TODO(bug) is generally preferred.
    results.extend(
        input_api.canned_checks.CheckChangeTodoHasOwner(input_api, output_api))

    # Ensure code change passes linter cleanly.
    results.extend(_CheckChangeLintsClean(input_api, output_api))

    # Ensure code change has already had clang-format ran.
    results.extend(
        input_api.canned_checks.CheckPatchFormatted(input_api,
                                                    output_api,
                                                    bypass_warnings=False))

    # Ensure code change has had GN formatting ran.
    results.extend(
        input_api.canned_checks.CheckGNFormatted(input_api, output_api))

    # Run buildtools/checkdeps on code change.
    results.extend(_CheckDeps(input_api, output_api))

    # Ensure the LUCI configs pass the linter.
    results.extend(_CheckLuciCfgLint(input_api, output_api))

    # Run tools/licenses on code change.
    results.extend(_CheckLicenses(input_api, output_api))

    return results


def CheckChangeOnUpload(input_api, output_api):
    """Checks the changelist whenever there is an upload (`git cl upload`)"""
    # We always run the OnCommit checks, as well as some additional checks.
    results = CheckChangeOnCommit(input_api, output_api)
    results.extend(
        input_api.canned_checks.CheckChangedLUCIConfigs(input_api, output_api))
    return results


def CheckChangeOnCommit(input_api, output_api):
    """Checks the changelist whenever there is commit (`git cl commit`)"""
    return _CommonChecks(input_api, output_api)
