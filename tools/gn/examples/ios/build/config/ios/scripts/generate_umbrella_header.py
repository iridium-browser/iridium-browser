# Copyright 2019 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Generates an umbrella header file that #import all public header of a
binary framework.
"""

import argparse
import os
import sys


def GenerateImport(header):
  """Returns a string for importing |header|."""
  return '#import "%s"\n' % os.path.basename(header)


def GenerateUmbrellaHeader(headers):
  """Returns a string with the content of the umbrella header."""
  return ''.join([ GenerateImport(header) for header in headers ])


def ParseArgs(argv):
  """Parses command line arguments."""
  parser = argparse.ArgumentParser(
      description=__doc__,
      formatter_class=argparse.ArgumentDefaultsHelpFormatter)

  parser.add_argument(
      'headers', nargs='+',
      help='path to the public heeaders')
  parser.add_argument(
      '-o', '--output', default='-',
      help='path of the output file to create; - means stdout')

  return parser.parse_args(argv)


def main(argv):
  args = ParseArgs(argv)

  content = GenerateUmbrellaHeader(args.headers)

  if args.output == '-':
    sys.stdout.write(content)
  else:
    with open(args.output, 'w') as output:
      output.write(content)


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
