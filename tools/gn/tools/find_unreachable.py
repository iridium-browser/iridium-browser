#!/usr/bin/env python
# Copyright 2020 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""
Finds unreachable gn targets by analysing --ide=json output
from gn gen.

Usage:
# Generate json file with targets info, will be located at out/project.json:
gn gen out --ide=json
# Lists all targets that are not reachable from //:all or //ci:test_all:
find_unreachable.py --from //:all --from //ci:test_all --json-file out/project.json
# Lists targets unreachable from //:all that aren't referenced by any other target:
find_unreachable.py --from //:all --json-file out/project.json --no-refs
"""

import argparse
import json
import sys


def find_reachable_targets(known, graph):
  reachable = set()
  to_visit = known
  while to_visit:
    next = to_visit.pop()
    if next in reachable:
      continue
    reachable.add(next)
    to_visit += graph[next]['deps']
  return reachable


def find_source_targets_from(targets, graph):
  source_targets = set(targets)
  for target in targets:
    source_targets -= set(graph[target]['deps'])
  return source_targets


def main():
  parser = argparse.ArgumentParser(description='''
    Tool to find unreachable targets.
    This can be useful to inspect forgotten targets,
    for example tests or intermediate targets in templates
    that are no longer needed.
    ''')
  parser.add_argument(
    '--json-file', required=True,
    help='JSON file from gn gen with --ide=json option')
  parser.add_argument(
    '--from', action='append', dest='roots',
    help='Known "root" targets. Can be multiple. Those targets \
        and all their recursive dependencies are considered reachable.\
        Examples: //:all, //ci:test_all')
  parser.add_argument(
    '--no-refs', action='store_true',
    help='Show only targets that aren\'t referenced by any other target')
  cmd_args = parser.parse_args()

  with open(cmd_args.json_file) as json_file:
    targets_graph = json.load(json_file)['targets']

  reachable = find_reachable_targets(cmd_args.roots, targets_graph)
  all = set(targets_graph.keys())
  unreachable = all - reachable

  result = find_source_targets_from(unreachable, targets_graph) \
    if cmd_args.no_refs else unreachable

  print '\n'.join(sorted(result))


if __name__ == '__main__':
  sys.exit(main())
