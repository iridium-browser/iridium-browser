#!/usr/bin/env python3

# Copyright 2019 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Replacement for the deprecated sysroot_ld_path.sh implementation in Chrome.
"""
Reads etc/ld.so.conf and/or etc/ld.so.conf.d/*.conf and returns the
appropriate linker flags.
"""

from __future__ import print_function
import argparse
import glob
import os
import sys

LD_SO_CONF_REL_PATH = "etc/ld.so.conf"
LD_SO_CONF_D_REL_PATH = "etc/ld.so.conf.d"


def parse_args(args):
    p = argparse.ArgumentParser(__doc__)
    p.add_argument('sysroot_path', nargs=1, help='Path to sysroot root folder')
    return os.path.abspath(p.parse_args(args).sysroot_path[0])


def process_entry(sysroot_path, entry):
    assert (entry.startswith('/'))
    print(os.path.join(sysroot_path, entry.strip()[1:]))

def process_ld_conf_file(sysroot_path, conf_file_path):
    with open(conf_file_path, 'r') as f:
        for line in f.readlines():
            if line.startswith('#'):
                continue
            process_entry(sysroot_path, line)


def process_ld_conf_folder(sysroot_path, ld_conf_path):
    files = glob.glob(os.path.join(ld_conf_path, '*.conf'))
    for file in files:
        process_ld_conf_file(sysroot_path, file)


def process_ld_conf_files(sysroot_path):
    conf_path = os.path.join(sysroot_path, LD_SO_CONF_REL_PATH)
    conf_d_path = os.path.join(sysroot_path, LD_SO_CONF_D_REL_PATH)

    if os.path.isdir(conf_path):
        process_ld_conf_folder(sysroot_path, conf_path)
    elif os.path.isdir(conf_d_path):
        process_ld_conf_folder(sysroot_path, conf_d_path)


def main(args):
    sysroot_path = parse_args(args)
    process_ld_conf_files(sysroot_path)


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except Exception as e:
        sys.stderr.write(str(e) + '\n')
        sys.exit(1)
