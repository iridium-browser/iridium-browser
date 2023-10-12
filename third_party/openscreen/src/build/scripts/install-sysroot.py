#!/usr/bin/env python3

# Copyright 2019 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Install Debian sysroots for cross compiling Open Screen.
"""

# The sysroot is needed to ensure that binaries that get built will run on
# the oldest stable version of Debian that we currently support.
# This script can be run manually but is more often run as part of gclient
# hooks. When run from hooks this script is a no-op on non-linux platforms.

# The sysroot image could be constructed from scratch based on the current state
# of the Debian archive but for consistency we use a pre-built root image (we
# don't want upstream changes to Debian to affect the build until we
# choose to pull them in). The sysroot images are stored in Chrome's common
# data storage, and the sysroots.json file should be kept in sync with Chrome's
# copy of it.

from __future__ import print_function

import hashlib
import json
import platform
import argparse
import os
import re
import shutil
import subprocess
import sys
try:
    # For Python 3.0 and later
    from urllib.request import urlopen
except ImportError:
    # Fall back to Python 2's urllib2
    from urllib2 import urlopen

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PARENT_DIR = os.path.dirname(SCRIPT_DIR)
URL_PREFIX = 'https://storage.googleapis.com'
URL_PATH = 'openscreen-sysroots'

VALID_ARCHS = ('arm', 'arm64')
VALID_PLATFORMS = ('stretch', 'bullseye')


class Error(Exception):
    pass


def GetSha1(filename):
    """Generates a SHA1 hash for validating download. Done in chunks to avoid
    excess memory usage."""
    BLOCKSIZE = 1024 * 1024
    sha1 = hashlib.sha1()
    with open(filename, 'rb') as f:
        chunk = f.read(BLOCKSIZE)
        while chunk:
            sha1.update(chunk)
            chunk = f.read(BLOCKSIZE)
    return sha1.hexdigest()


def GetSysrootDict(target_platform, target_arch):
    """Gets the sysroot information for a given platform and arch from the
       sysroots.json file."""
    if target_arch not in VALID_ARCHS:
        raise Error('Unknown architecture: %s' % target_arch)

    sysroots_file = os.path.join(SCRIPT_DIR, 'sysroots.json')
    sysroots = json.load(open(sysroots_file))
    sysroot_key = '%s_%s' % (target_platform, target_arch)
    if sysroot_key not in sysroots:
        raise Error('No sysroot for: %s' % (sysroot_key))
    return sysroots[sysroot_key]


def DownloadFile(url, local_path):
    """Uses urllib to download a remote file into local_path."""
    for _ in range(3):
        try:
            response = urlopen(url)
            with open(local_path, "wb") as f:
                f.write(response.read())
            break
        except Exception:
            pass
    else:
        raise Error('Failed to download %s' % url)

def ValidateFile(local_path, expected_sum):
    """Generates the SHA1 hash of a local file to compare with an expected
       hashsum."""
    sha1sum = GetSha1(local_path)
    if sha1sum != expected_sum:
        raise Error('Tarball sha1sum is wrong.'
                    'Expected %s, actual: %s' % (expected_sum, sha1sum))

def InstallSysroot(target_platform, target_arch):
    """Downloads, validates, unpacks, and installs a sysroot image."""
    sysroot_dict = GetSysrootDict(target_platform, target_arch)
    tarball_filename = sysroot_dict['Tarball']
    tarball_sha1sum = sysroot_dict['Sha1Sum']

    sysroot = os.path.join(PARENT_DIR, sysroot_dict['SysrootDir'])

    url = '%s/%s/%s/%s' % (URL_PREFIX, URL_PATH, tarball_sha1sum,
                           tarball_filename)

    stamp = os.path.join(sysroot, '.stamp')
    if os.path.exists(stamp):
        with open(stamp) as s:
            if s.read() == url:
                return

    if os.path.isdir(sysroot):
        shutil.rmtree(sysroot)
    os.mkdir(sysroot)

    tarball_path = os.path.join(sysroot, tarball_filename)
    DownloadFile(url, tarball_path)
    ValidateFile(tarball_path, tarball_sha1sum)
    subprocess.check_call(['tar', 'xf', tarball_path, '-C', sysroot])
    os.remove(tarball_path)

    with open(stamp, 'w') as s:
        s.write(url)


def parse_args(args):
    """Parses the passed in arguments into an object."""
    p = argparse.ArgumentParser()
    p.add_argument(
        'arch',
        help='Sysroot architecture: %s' % ', '.join(VALID_ARCHS))
    p.add_argument(
        'platform',
        help='Sysroot platform: %s' % ', '.join(VALID_PLATFORMS))
    p.add_argument(
        '--print-hash', action="store_true",
        help='Print the hash of the sysroot for the specified arch.')

    return p.parse_args(args)


def main(args):
    if not (sys.platform.startswith('linux') or sys.platform == 'darwin'):
        print('Unsupported platform. Only Linux and Mac OS X are supported.')
        return 1

    parsed_args = parse_args(args)
    if parsed_args.print_hash:
        print(GetSysrootDict(parsed_args.platform, parsed_args.arch)['Sha1Sum'])

    InstallSysroot(parsed_args.platform, parsed_args.arch)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except Error as e:
        sys.stderr.write('Installing sysroot error: {}\n'.format(e))
        sys.exit(1)
