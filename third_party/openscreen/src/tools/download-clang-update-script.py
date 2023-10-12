#!/usr/bin/env python3
# Copyright 2020 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This script is used to download the clang update script. It runs as a gclient
hook.

It's equivalent to using curl to download the latest update script.
"""

import argparse
import curlish
import sys

SCRIPT_DOWNLOAD_URL = ('https://raw.githubusercontent.com/chromium/'
                       'chromium/main/tools/clang/scripts/update.py')


def main():
    parser = argparse.ArgumentParser(description='Download a file.')
    parser.add_argument('--output', help='Path to file to create/overwrite.')
    args = parser.parse_args()

    if not args.output:
        print('usage: download-clang-update-script.py --output=/a/b/update.py')
        return 1

    return 0 if curlish.curlish(SCRIPT_DOWNLOAD_URL, args.output) else 1


if __name__ == '__main__':
    sys.exit(main())
