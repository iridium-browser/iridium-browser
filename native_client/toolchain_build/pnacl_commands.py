#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runnables for toolchain_build_pnacl.py
"""

from __future__ import print_function

import base64
import os
import shutil
import stat
import subprocess
import sys
import tarfile
import zipfile

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.file_tools
import pynacl.http_download
import pynacl.platform
import pynacl.repo_tools


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(SCRIPT_DIR)

# User-facing tools
DRIVER_TOOLS = ['pnacl-' + tool + '.py' for tool in
                    ('abicheck', 'ar', 'as', 'clang', 'clang++', 'compress',
                     'dis', 'driver', 'finalize', 'ld', 'nm', 'opt',
                     'ranlib', 'readelf', 'strip', 'translate')]
# Utilities used by the driver
DRIVER_UTILS = [name + '.py' for name in
                    ('artools', 'driver_env', 'driver_log', 'driver_temps',
                     'driver_tools', 'elftools', 'filetype', 'ldtools',
                     'loader', 'nativeld', 'pathtools', 'shelltools')]

def CMakePlatformName():
    return {
        'linux': 'linux',
        'linux2': 'linux',
        'darwin': 'macos',
        'win32': 'windows'
    }[sys.platform]


def CMakeArch():
    return 'universal' if pynacl.platform.IsMac() else 'x86_64'

WASM_STORAGE_BASE = 'https://wasm.storage.googleapis.com/'
PREBUILT_CMAKE_VERSION = '3.21.3'
PREBUILT_CMAKE_BASE_NAME = 'cmake-%s-%s-%s' % (
    PREBUILT_CMAKE_VERSION, CMakePlatformName(), CMakeArch())
PREBUILT_CMAKE_DIR = os.path.join(NACL_DIR, PREBUILT_CMAKE_BASE_NAME)

def PrebuiltCMakeBin():
    if pynacl.platform.IsMac():
        bin_dir = os.path.join('CMake.app', 'Contents', 'bin')
    else:
        bin_dir = 'bin'
    suffix = '.exe' if pynacl.platform.IsWindows() else ''
    return os.path.join(PREBUILT_CMAKE_DIR, bin_dir, 'cmake' + suffix)


def SyncArchive(out_dir, name, url, create_out_dir=False):
    """Download and extract an archive (zip, tar.gz or tar.xz) file from a URL.

    The extraction happens in the prebuilt dir. If create_out_dir is True,
    out_dir will be created and the archive will be extracted inside. Otherwise
    the archive is expected to contain a top-level directory with all the
    files; this is expected to be 'out_dir', so if 'out_dir' already exists
    then the download will be skipped.
    """
    stamp_file = os.path.join(out_dir, 'stamp.txt')
    if os.path.isdir(out_dir):
        if os.path.isfile(stamp_file):
            with open(stamp_file) as f:
                stamp_url = f.read().strip()
            if stamp_url == url:
                print('%s directory already exists' % name)
                return
        print('%s directory exists but is not up-to-date' % name)
    print('Downloading %s from %s' % (name, url))

    if create_out_dir:
        os.makedirs(out_dir)
        work_dir = out_dir
    else:
        work_dir = os.path.dirname(out_dir)

    basename = url.split('/')[-1]
    download_target = os.path.join(work_dir, basename)
    pynacl.http_download.HttpDownload(url, download_target)
    try:
        print('Extracting into %s' % work_dir)
        ext = os.path.splitext(url)[-1]
        if ext == '.zip':
            with zipfile.ZipFile(download_target, 'r') as zip:
                zip.extractall(path=work_dir)
        elif ext == '.xz':
            subprocess.check_call(
                ['tar', '-xvf', download_target], cwd=work_dir)
        else:
            with open(download_target, 'rb') as f:
                tarfile.open(fileobj=f).extractall(path=work_dir)
    except Exception as e:
        print('Error downloading/extracting %s: %s' % (url, e))
        raise

    with open(stamp_file, 'w') as f:
        f.write(url + '\n')


def SyncPrebuiltCMake():
    extension = '.zip' if pynacl.platform.IsWindows() else '.tar.gz'
    url = WASM_STORAGE_BASE + PREBUILT_CMAKE_BASE_NAME + extension
    SyncArchive(PREBUILT_CMAKE_DIR, 'cmake', url)


# Install a remote_toolchains_inputs file for reclient
def InstallRemoteToolchainInputs(source_file_name, dstdir):
  source = os.path.join(NACL_DIR, 'toolchain_build', source_file_name)
  destination = os.path.join(dstdir, 'remote_toolchain_inputs')
  shutil.copy(source, destination)
  os.chmod(destination,
           stat.S_IRUSR | stat.S_IXUSR | stat.S_IWUSR | stat.S_IRGRP |
           stat.S_IWGRP | stat.S_IXGRP)


def InstallDriverScripts(logger, subst, srcdir, dstdir, host_windows=False,
                         host_64bit=False, extra_config=[]):
  srcdir = subst.SubstituteAbsPaths(srcdir)
  dstdir = subst.SubstituteAbsPaths(dstdir)
  logger.debug('Installing Driver Scripts: %s -> %s', srcdir, dstdir)

  pynacl.file_tools.MakeDirectoryIfAbsent(os.path.join(dstdir, 'pydir'))
  for name in DRIVER_TOOLS + DRIVER_UTILS:
    source = os.path.join(srcdir, name)
    destination = os.path.join(dstdir, 'pydir')
    logger.debug('  Installing: %s -> %s', source, destination)
    shutil.copy(source, destination)
  # Install redirector sh/bat scripts
  for name in DRIVER_TOOLS:
    # Chop the .py off the name
    source = os.path.join(srcdir, 'redirect.sh')
    destination = os.path.join(dstdir, os.path.splitext(name)[0])
    logger.debug('  Installing: %s -> %s', source, destination)
    shutil.copy(source, destination)
    os.chmod(destination,
             stat.S_IRUSR | stat.S_IXUSR | stat.S_IWUSR | stat.S_IRGRP |
             stat.S_IWGRP | stat.S_IXGRP)

    if host_windows:
      # Windows gets both sh and bat extensions so it works w/cygwin and without
      win_source = os.path.join(srcdir, 'redirect.bat')
      win_dest = destination + '.bat'
      logger.debug('  Installing: %s -> %s', win_source, win_dest)
      shutil.copy(win_source, win_dest)
      os.chmod(win_dest,
               stat.S_IRUSR | stat.S_IXUSR | stat.S_IWUSR | stat.S_IRGRP |
               stat.S_IWGRP | stat.S_IXGRP)
  # Install the driver.conf file
  driver_conf = os.path.join(dstdir, 'driver.conf')
  logger.debug('  Installing: %s', driver_conf)
  with open(driver_conf, 'w') as f:
    print('HAS_FRONTEND=1', file=f)
    print('HOST_ARCH=x86_64' if host_64bit else 'HOST_ARCH=x86_32', file=f)
    for line in extra_config:
      print(subst.Substitute(line), file=f)
  logger.debug(' Installing remote_toolchain_inputs')
  source_file_name = 'pnacl_remote_toolchain_inputs.txt'
  if host_windows:
    source_file_name = 'pnacl_remote_toolchain_inputs_windows.txt'
  InstallRemoteToolchainInputs(source_file_name, dstdir)


def InstallRemoteToolchainInputsSaigo(logger, subst, dstdir):
  dstdir = subst.SubstituteAbsPaths(dstdir)
  logger.debug(' Installing remote_toolchain_inputs for saigo')
  InstallRemoteToolchainInputs('saigo_remote_toolchain_inputs.txt', dstdir)


def CheckoutGitBundleForTrybot(repo, destination):
  # For testing LLVM, Clang, etc. changes on the trybots, look for a
  # Git bundle file created by llvm_change_try_helper.sh.
  bundle_file = os.path.join(NACL_DIR, 'pnacl', 'not_for_commit',
                             '%s_bundle' % repo)
  base64_file = '%s.b64' % bundle_file
  if os.path.exists(base64_file):
    input_fh = open(base64_file, 'r')
    output_fh = open(bundle_file, 'wb')
    base64.decode(input_fh, output_fh)
    input_fh.close()
    output_fh.close()
    subprocess.check_call(
        pynacl.repo_tools.GitCmd() + ['fetch'],
        cwd=destination
    )
    subprocess.check_call(
        pynacl.repo_tools.GitCmd() + ['bundle', 'unbundle', bundle_file],
        cwd=destination
    )
    commit_id_file = os.path.join(NACL_DIR, 'pnacl', 'not_for_commit',
                                  '%s_commit_id' % repo)
    commit_id = open(commit_id_file, 'r').readline().strip()
    subprocess.check_call(
        pynacl.repo_tools.GitCmd() + ['checkout', commit_id],
        cwd=destination
    )

def CmdCheckoutGitBundleForTrybot(logger, subst, repo, destination):
  destination = subst.SubstituteAbsPaths(destination)
  logger.debug('Checking out Git Bundle for Trybot: %s [%s]', destination, repo)
  return CheckoutGitBundleForTrybot(repo, destination)


def WriteREVFile(logger, subst, dstfile, base_url, repos, revisions):
  # Install the REV file with repo info for all the components
  rev_file = subst.SubstituteAbsPaths(dstfile)
  logger.debug('Installing: %s', rev_file)
  with open(rev_file, 'w') as f:
    url, rev = pynacl.repo_tools.GitRevInfo(NACL_DIR)
    print('[GIT] %s: %s' % (url, rev), file=f)

    for name, revision in revisions.items():
      repo = base_url + repos[name]
      print('[GIT] %s: %s' % (repo, revision), file=f)
