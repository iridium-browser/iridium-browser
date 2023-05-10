#!/usr/bin/env python3
# Copyright 2023 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""This script is a wrapper around the ninja.py script that also
handles the client lifecycle safely. It will automatically start
reproxy before running ninja and stop reproxy when ninja stops
for any reason eg. build completes, keyboard interupt etc."""

import hashlib
import os
import subprocess
import sys

import ninja
import gclient_paths


def find_reclient_bin_dir():
  tools_path = gclient_paths.GetBuildtoolsPath()
  if not tools_path:
    return None

  reclient_bin_dir = os.path.join(tools_path, 'reclient')
  if os.path.isdir(reclient_bin_dir):
    return reclient_bin_dir
  return None


def find_reclient_cfg():
  tools_path = gclient_paths.GetBuildtoolsPath()
  if not tools_path:
    return None

  reclient_cfg = os.path.join(tools_path, 'reclient_cfgs', 'reproxy.cfg')
  if os.path.isfile(reclient_cfg):
    return reclient_cfg
  return None


def run(cmd_args):
  if os.environ.get('NINJA_SUMMARIZE_BUILD') == '1':
    print(' '.join(cmd_args))
  return subprocess.call(cmd_args)


def start_reproxy(reclient_cfg, reclient_bin_dir):
  return run([
      os.path.join(reclient_bin_dir, 'bootstrap'),
      '--re_proxy=' + os.path.join(reclient_bin_dir, 'reproxy'),
      '--cfg=' + reclient_cfg
  ])


def stop_reproxy(reclient_cfg, reclient_bin_dir):
  return run([
      os.path.join(reclient_bin_dir, 'bootstrap'), '--shutdown',
      '--cfg=' + reclient_cfg
  ])


def find_rel_ninja_out_dir(args):
  # Ninja uses getopt_long, which allow to intermix non-option arguments.
  # To leave non supported parameters untouched, we do not use getopt.
  for index, arg in enumerate(args[1:]):
    if arg == '-C':
      # + 1 to get the next argument and +1 because we trimmed off args[0]
      return args[index + 2]
    if arg.startswith('-C'):
      # Support -Cout/Default
      return arg[2:]
  return '.'


def set_reproxy_path_flags(out_dir):
  """Helper to setup the logs and cache directories for reclient

  Creates the following directory structure:
  out_dir/
    .reproxy_tmp/
      logs/
      cache/

  The following env vars are set if not already set:
    RBE_output_dir=out_dir/.reproxy_tmp/logs
    RBE_proxy_log_dir=out_dir/.reproxy_tmp/logs
    RBE_log_dir=out_dir/.reproxy_tmp/logs
    RBE_cache_dir=out_dir/.reproxy_tmp/cache
  *Nix Only:
    RBE_server_address=unix://out_dir/.reproxy_tmp/reproxy.sock
  Windows Only:
    RBE_server_address=pipe://md5(out_dir/.reproxy_tmp)/reproxy.pipe
  """
  tmp_dir = os.path.abspath(os.path.join(out_dir, '.reproxy_tmp'))
  os.makedirs(tmp_dir, exist_ok=True)
  log_dir = os.path.join(tmp_dir, 'logs')
  os.makedirs(log_dir, exist_ok=True)
  os.environ.setdefault("RBE_output_dir", log_dir)
  os.environ.setdefault("RBE_proxy_log_dir", log_dir)
  os.environ.setdefault("RBE_log_dir", log_dir)
  cache_dir = os.path.join(tmp_dir, 'cache')
  os.makedirs(cache_dir, exist_ok=True)
  os.environ.setdefault("RBE_cache_dir", cache_dir)
  if sys.platform.startswith('win'):
    pipe_dir = hashlib.md5(tmp_dir.encode()).hexdigest()
    os.environ.setdefault("RBE_server_address",
                          "pipe://%s/reproxy.pipe" % pipe_dir)
  else:
    os.environ.setdefault("RBE_server_address",
                          "unix://%s/reproxy.sock" % tmp_dir)


def main(argv):
  # If use_remoteexec is set, but the reclient binaries or configs don't
  # exist, display an error message and stop.  Otherwise, the build will
  # attempt to run with rewrapper wrapping actions, but will fail with
  # possible non-obvious problems.
  # As of January 2023, dev builds with reclient are not supported, so
  # indicate that use_goma should be swapped for use_remoteexec.  This
  # message will be changed when dev builds are fully supported.
  reclient_bin_dir = find_reclient_bin_dir()
  reclient_cfg = find_reclient_cfg()
  if reclient_bin_dir is None or reclient_cfg is None:
    print(("Build is configured to use reclient but necessary binaries "
           "or config files can't be found.  Developer builds with "
           "reclient are not yet supported.  Try regenerating your "
           "build with use_goma in place of use_remoteexec for now."),
          file=sys.stderr)
    return 1
  try:
    set_reproxy_path_flags(find_rel_ninja_out_dir(argv))
  except OSError:
    print("Error creating reproxy_tmp in output dir", file=sys.stderr)
    return 1
  reproxy_ret_code = start_reproxy(reclient_cfg, reclient_bin_dir)
  if reproxy_ret_code != 0:
    return reproxy_ret_code
  try:
    return ninja.main(argv)
  except KeyboardInterrupt:
    print("Caught User Interrupt", file=sys.stderr)
    # Suppress python stack trace if ninja is interrupted
    return 1
  finally:
    print("Shutting down reproxy...", file=sys.stderr)
    stop_reproxy(reclient_cfg, reclient_bin_dir)


if __name__ == '__main__':
  sys.exit(main(sys.argv))
