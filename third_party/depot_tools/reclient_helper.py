# Copyright 2023 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""This helper provides a build context that handles
the reclient lifecycle safely. It will automatically start
reproxy before running ninja and stop reproxy when build stops
for any reason e.g. build completion, keyboard interrupt etc."""

import contextlib
import hashlib
import os
import shutil
import subprocess
import sys
import time

import gclient_paths
import reclient_metrics


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
      os.path.join(reclient_bin_dir,
                   'bootstrap' + gclient_paths.GetExeSuffix()), '--re_proxy=' +
      os.path.join(reclient_bin_dir, 'reproxy' + gclient_paths.GetExeSuffix()),
      '--cfg=' + reclient_cfg
  ])


def stop_reproxy(reclient_cfg, reclient_bin_dir):
  return run([
      os.path.join(reclient_bin_dir,
                   'bootstrap' + gclient_paths.GetExeSuffix()), '--shutdown',
      '--cfg=' + reclient_cfg
  ])


def find_ninja_out_dir(args):
  # Ninja uses getopt_long, which allows to intermix non-option arguments.
  # To leave non supported parameters untouched, we do not use getopt.
  for index, arg in enumerate(args[1:]):
    if arg == '-C':
      # + 1 to get the next argument and +1 because we trimmed off args[0]
      return args[index + 2]
    if arg.startswith('-C'):
      # Support -Cout/Default
      return arg[2:]
  return '.'


def find_cache_dir(tmp_dir):
  """Helper to find the correct cache directory for a build.

  tmp_dir should be a build specific temp directory within the out directory.

  If this is called from within a gclient checkout, the cache dir will be:
  <gclient_root>/.reproxy_cache/md5(tmp_dir)/
  If this is not called from within a gclient checkout, the cache dir will be:
  tmp_dir/cache
  """
  gclient_root = gclient_paths.FindGclientRoot(os.getcwd())
  if gclient_root:
    return os.path.join(gclient_root, '.reproxy_cache',
                        hashlib.md5(tmp_dir.encode()).hexdigest())
  return os.path.join(tmp_dir, 'cache')


def set_reproxy_metrics_flags(tool):
  """Helper to setup metrics collection flags for reproxy.

  The following env vars are set if not already set:
    RBE_metrics_project=chromium-reclient-metrics
    RBE_invocation_id=$AUTONINJA_BUILD_ID
    RBE_metrics_table=rbe_metrics.builds
    RBE_metrics_labels=source=developer,tool={tool}
    RBE_metrics_prefix=go.chromium.org
  """
  autoninja_id = os.environ.get("AUTONINJA_BUILD_ID")
  if autoninja_id is not None:
    os.environ.setdefault("RBE_invocation_id", autoninja_id)
  os.environ.setdefault("RBE_metrics_project", "chromium-reclient-metrics")
  os.environ.setdefault("RBE_metrics_table", "rbe_metrics.builds")
  os.environ.setdefault("RBE_metrics_labels", "source=developer,tool=" + tool)
  os.environ.setdefault("RBE_metrics_prefix", "go.chromium.org")


def remove_mdproxy_from_path():
  os.environ["PATH"] = os.pathsep.join(
      d for d in os.environ.get("PATH", "").split(os.pathsep)
      if "mdproxy" not in d)


def set_reproxy_path_flags(out_dir, make_dirs=True):
  """Helper to setup the logs and cache directories for reclient.

  Creates the following directory structure if make_dirs is true:
  If in a gclient checkout
  out_dir/
    .reproxy_tmp/
      logs/
  <gclient_root>
    .reproxy_cache/
      md5(out_dir/.reproxy_tmp)/

  If not in a gclient checkout
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
  log_dir = os.path.join(tmp_dir, 'logs')
  racing_dir = os.path.join(tmp_dir, 'racing')
  cache_dir = find_cache_dir(tmp_dir)
  if make_dirs:
    if os.path.exists(log_dir):
      try:
        # Clear log dir before each build to ensure correct metric aggregation.
        shutil.rmtree(log_dir)
      except OSError:
        print(
            "Couldn't clear logs because reproxy did "
            "not shutdown after the last build",
            file=sys.stderr)
    os.makedirs(tmp_dir, exist_ok=True)
    os.makedirs(log_dir, exist_ok=True)
    os.makedirs(cache_dir, exist_ok=True)
    os.makedirs(racing_dir, exist_ok=True)
  os.environ.setdefault("RBE_output_dir", log_dir)
  os.environ.setdefault("RBE_proxy_log_dir", log_dir)
  os.environ.setdefault("RBE_log_dir", log_dir)
  os.environ.setdefault("RBE_cache_dir", cache_dir)
  os.environ.setdefault("RBE_racing_tmp_dir", racing_dir)
  if sys.platform.startswith('win'):
    pipe_dir = hashlib.md5(tmp_dir.encode()).hexdigest()
    os.environ.setdefault("RBE_server_address",
                          "pipe://%s/reproxy.pipe" % pipe_dir)
  else:
    # unix domain socket has path length limit, so use fixed size path here.
    # ref: https://www.man7.org/linux/man-pages/man7/unix.7.html
    os.environ.setdefault(
        "RBE_server_address", "unix:///tmp/reproxy_%s.sock" %
        hashlib.sha256(tmp_dir.encode()).hexdigest())


def set_racing_defaults():
  os.environ.setdefault("RBE_local_resource_fraction", "0.2")
  os.environ.setdefault("RBE_racing_bias", "0.95")


@contextlib.contextmanager
def build_context(argv, tool):
  # If use_remoteexec is set, but the reclient binaries or configs don't
  # exist, display an error message and stop.  Otherwise, the build will
  # attempt to run with rewrapper wrapping actions, but will fail with
  # possible non-obvious problems.
  reclient_bin_dir = find_reclient_bin_dir()
  reclient_cfg = find_reclient_cfg()
  if reclient_bin_dir is None or reclient_cfg is None:
    print(('Build is configured to use reclient but necessary binaries '
           "or config files can't be found.\n"
           'Please check if `"download_remoteexec_cfg": True` custom var is set'
           ' in `.gclient`, and run `gclient sync`.'),
          file=sys.stderr)
    yield 1
    return

  ninja_out = find_ninja_out_dir(argv)

  try:
    set_reproxy_path_flags(ninja_out)
  except OSError:
    print("Error creating reproxy_tmp in output dir", file=sys.stderr)
    yield 1
    return

  if reclient_metrics.check_status(ninja_out):
    set_reproxy_metrics_flags(tool)

  if os.environ.get('RBE_instance', None):
    print('WARNING: Using RBE_instance=%s\n' %
          os.environ.get('RBE_instance', ''))

  remote_disabled = os.environ.get('RBE_remote_disabled')
  if remote_disabled not in ('1', 't', 'T', 'true', 'TRUE', 'True'):
    set_racing_defaults()

  # TODO(b/292523514) remove this once a fix is landed in reproxy
  remove_mdproxy_from_path()

  start = time.time()
  reproxy_ret_code = start_reproxy(reclient_cfg, reclient_bin_dir)
  elapsed = time.time() - start
  print('%1.3f s to start reproxy' % elapsed)
  if reproxy_ret_code != 0:
    yield reproxy_ret_code
    return
  try:
    yield
  finally:
    print("Shutting down reproxy...", file=sys.stderr)
    start = time.time()
    stop_reproxy(reclient_cfg, reclient_bin_dir)
    elapsed = time.time() - start
    print('%1.3f s to stop reproxy' % elapsed)
