#!/usr/bin/env python3
# Copyright 2020 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Converts a data file, e.g. a JSON file, into a C++ raw string that
can be #included.
"""

import argparse
import os
import sys

FORMAT_STRING = """#pragma once

namespace openscreen {{
namespace {0} {{

constexpr char {1}[] = R"(
        {2}
)";

}} // namspace {0}
}} // namespace openscreen
"""


def ToCamelCase(snake_case):
    """Converts snake_case to TitleCamelCase."""
    return ''.join(x.title() for x in snake_case.split('_'))


def GetVariableName(path):
    """Converts a snake case file name into a kCamelCase variable name."""
    file_name = os.path.splitext(os.path.split(path)[1])[0]
    return 'k' + ToCamelCase(file_name)


def Convert(namespace, input_path, output_path):
    """Takes an input file, such as a JSON file, and converts it into a C++
       data file, in the form of a character array constant in a header."""
    if not os.path.exists(input_path):
        print('\tERROR: failed to generate, invalid path supplied: ' +
              input_path)
        return 1

    content = False
    with open(input_path, 'r') as f:
        content = f.read()

    with open(output_path, 'w') as f:
        f.write(
            FORMAT_STRING.format(namespace, GetVariableName(input_path),
                                 content))


def main():
    parser = argparse.ArgumentParser(
        description='Convert a file to a C++ data file')
    parser.add_argument(
        'namespace',
        help='Namespace to scope data variable (nested under openscreen)')
    parser.add_argument('input_path', help='Path to file to convert')
    parser.add_argument('output_path', help='Output path of converted file')
    args = parser.parse_args()

    input_path = os.path.abspath(args.input_path)
    output_path = os.path.abspath(args.output_path)
    Convert(args.namespace, input_path, output_path)


if __name__ == '__main__':
    sys.exit(main())
