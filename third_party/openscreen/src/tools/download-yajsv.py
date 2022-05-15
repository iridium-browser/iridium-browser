#!/usr/bin/env python3
# Copyright 2020 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
This script is used to download YAJSV (yet another json schema validator). It
runs as a gclient hook.
"""

import argparse
import curlish
import os
import stat
import sys

RELEASES_DOWNLOAD_URL = 'https://github.com/neilpa/yajsv/releases/download/'
VERSION = 'v1.4.0'
YAJSV_FLAVOR_DICT = {
    'linux32': 'yajsv.linux.386',
    'linux64': 'yajsv.linux.amd64',
    'mac64': 'yajsv.darwin.amd64'
}

PLATFORM_MAP = {'linux2': 'linux', 'darwin': 'mac'}


def get_bitness():
    # According to the python docs, this is more reliable than
    # querying platform.architecture().
    if sys.maxsize > 2**32:
        return '64'
    return '32'


def get_platform():
    return PLATFORM_MAP.get(sys.platform, sys.platform)


def get_flavor():
    return "{}{}".format(get_platform(), get_bitness())


def main():
    parser = argparse.ArgumentParser(description='Download a YAJSV release.')
    parser.add_argument('--flavor',
                        help='Flavor to download (currently one of {})'.format(
                            ', '.join(YAJSV_FLAVOR_DICT.keys())))
    args = parser.parse_args()

    flavor = args.flavor
    if not flavor:
        flavor = get_flavor()
        if flavor in YAJSV_FLAVOR_DICT:
            print('flavor not provided, defaulting to ' + flavor)

    if flavor not in YAJSV_FLAVOR_DICT:
        print('could not find an appropriate flavor, "{}" is invalid'.format(
            flavor))
        return 1

    output_path = os.path.abspath(
        os.path.join(os.path.dirname(os.path.relpath(__file__)), 'yajsv'))
    download_url = '{}{}/{}'.format(RELEASES_DOWNLOAD_URL, VERSION,
                                    YAJSV_FLAVOR_DICT[flavor])
    result = curlish.curlish(download_url, output_path)

    # YAJSV isn't useful if it's not executable.
    if result:
        current_mode = os.stat(output_path).st_mode
        os.chmod(output_path, current_mode | stat.S_IEXEC)

    return 0 if result else 1


if __name__ == '__main__':
    sys.exit(main())
