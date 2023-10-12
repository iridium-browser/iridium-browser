# Copyright 2019 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Merges multiple Apple Property List files (.plist) and perform variables
substitutions $(VARIABLE) in the Property List string values.
"""

import argparse
import json
import re
import subprocess
import sys


# Pattern representing a variable to substitue in a string value.
VARIABLE_PATTERN = re.compile(r'\$\(([^)]*)\)')


def GetCommandOutput(command):
  """Returns the output of `command` as a string."""
  return subprocess.check_output(command, encoding='utf-8')


def LoadPlist(plist_path):
  """Loads Apple Property List file at |plist_path|."""
  return json.loads(
      GetCommandOutput(['plutil', '-convert', 'json', '-o', '-', plist_path]))


def SavePlist(plist_path, content, format):
  """Saves |content| as Apple Property List in |format| at |plist_path|."""
  proc = subprocess.Popen(
      ['plutil', '-convert', format, '-o', plist_path, '-'],
      stdin=subprocess.PIPE)
  output, _ = proc.communicate(json.dumps(content).encode('utf-8'))
  if proc.returncode:
    raise subprocess.CalledProcessError(
        proc.returncode,
        ['plutil', '-convert', format, '-o', plist_path, '-'],
        output)


def MergeObjects(obj1, obj2):
  """Merges two objects (either dictionary, list, string or numbers)."""
  if type(obj1) != type(obj2):
    return obj2

  if isinstance(obj2, dict):
    result = dict(obj1)
    for key in obj2:
      value1 = obj1.get(key, None)
      value2 = obj2.get(key, None)
      result[key] = MergeObjects(value1, value2)
    return result

  if isinstance(obj2, list):
    return obj1 + obj2

  return obj2


def MergePlists(plist_paths):
  """Loads and merges all Apple Property List files at |plist_paths|."""
  plist = {}
  for plist_path in plist_paths:
    plist = MergeObjects(plist, LoadPlist(plist_path))
  return plist


def PerformSubstitutions(plist, substitutions):
  """Performs variables substitutions in |plist| given by |substitutions|."""
  if isinstance(plist, dict):
    result = dict(plist)
    for key in plist:
      result[key] = PerformSubstitutions(plist[key], substitutions)
    return result

  if isinstance(plist, list):
    return [ PerformSubstitutions(item, substitutions) for item in plist ]

  if isinstance(plist, str):
    result = plist
    while True:
      match = VARIABLE_PATTERN.search(result)
      if not match:
        break

      extent = match.span()
      expand = substitutions[match.group(1)]
      result = result[:extent[0]] + expand + result[extent[1]:]
    return result

  return plist


def PerformSubstitutionsFrom(plist, substitutions_path):
  """Performs variable substitutions in |plist| from |substitutions_path|."""
  with open(substitutions_path) as substitutions_file:
    return PerformSubstitutions(plist, json.load(substitutions_file))


def ParseArgs(argv):
  """Parses command line arguments."""
  parser = argparse.ArgumentParser(
      description=__doc__,
      formatter_class=argparse.ArgumentDefaultsHelpFormatter)

  parser.add_argument(
      '-s', '--substitutions',
      help='path to a JSON file containing variable substitutions')
  parser.add_argument(
      '-f', '--format', default='json', choices=('json', 'binary1', 'xml1'),
      help='format of the generated file')
  parser.add_argument(
      '-o', '--output', default='-',
      help='path to the result; - means stdout')
  parser.add_argument(
      'inputs', nargs='+',
      help='path of the input files to merge')

  return parser.parse_args(argv)


def main(argv):
  args = ParseArgs(argv)

  data = MergePlists(args.inputs)
  if args.substitutions:
    data = PerformSubstitutionsFrom(
        data, args.substitutions)

  SavePlist(args.output, data, args.format)


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
