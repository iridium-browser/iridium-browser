#!/usr/bin/env python3

# Copyright 2019 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Writes True if the argument is a directory.
"""

from __future__ import print_function

import os.path
import sys


def main():
    print(is_dir(sys.argv[1]), end='')
    return 0


def is_dir(dir_name):
    return str(os.path.isdir(dir_name))


def DoMain(args):
    """Hook to be called from gyp without starting a separate python
  interpreter."""
    return is_dir(args[0])


if __name__ == '__main__':
    sys.exit(main())
