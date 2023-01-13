#!/usr/bin/env python3
# Copyright 2022 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""This script is a wrapper around the ninja binary that is pulled to
third_party as part of gclient sync. It will automatically find the ninja
binary when run inside a gclient source tree, so users can just type
"ninja" on the command line."""

import os
import subprocess
import sys

import gclient_paths

DEPOT_TOOLS_ROOT = os.path.abspath(os.path.dirname(__file__))


def fallbackToLegacyNinja(ninja_args):
  print(
      'depot_tools/ninja.py: Fallback to a deprecated legacy ninja binary. '
      'Note that this ninja binary will be removed soon.\n'
      'Please install ninja to your project using DEPS. '
      'If your project does not have DEPS, Please install ninja in your PATH.\n'
      'See also https://crbug.com/1340825',
      file=sys.stderr)

  exe_name = ''
  if sys.platform == 'linux':
    exe_name = 'ninja-linux64'
  elif sys.platform == 'darwin':
    exe_name = 'ninja-mac'
  elif sys.platform in ['win32', 'cygwin']:
    exe_name = 'ninja.exe'
  else:
    print('depot_tools/ninja.py: %s is not supported platform' % sys.platform)
    return 1

  ninja_path = os.path.join(DEPOT_TOOLS_ROOT, exe_name)
  return subprocess.call([ninja_path] + ninja_args)


def findNinjaInPath():
  env_path = os.getenv('PATH')
  if not env_path:
    return
  exe = 'ninja'
  if sys.platform in ['win32', 'cygwin']:
    exe += '.exe'
  for bin_dir in env_path.split(os.pathsep):
    if bin_dir.rstrip(os.sep).endswith('depot_tools'):
      # skip depot_tools to avoid calling ninja.py infitely.
      continue
    ninja_path = os.path.join(bin_dir, exe)
    if os.path.isfile(ninja_path):
      return ninja_path


def fallback(ninja_args):
  # Try to find ninja in PATH.
  ninja_path = findNinjaInPath()
  if ninja_path:
    return subprocess.call([ninja_path] + ninja_args)

  # TODO(crbug.com/1340825): remove raw binaries from depot_tools.
  return fallbackToLegacyNinja(ninja_args)


def main(args):
  # Get gclient root + src.
  primary_solution_path = gclient_paths.GetPrimarySolutionPath()
  gclient_root_path = gclient_paths.FindGclientRoot(os.getcwd())
  for base_path in [primary_solution_path, gclient_root_path]:
    if not base_path:
      continue
    ninja_path = os.path.join(base_path, 'third_party', 'ninja',
                              'ninja' + gclient_paths.GetExeSuffix())
    if os.path.isfile(ninja_path):
      return subprocess.call([ninja_path] + args[1:])

  return fallback(args[1:])


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv))
  except KeyboardInterrupt:
    sys.exit(1)
