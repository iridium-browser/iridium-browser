#!/usr/bin/env python3
# Copyright 2023 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""This script manages the counter for how many times developers should
be notified before uploading reclient metrics."""

import json
import os
import subprocess
import sys

THIS_DIR = os.path.dirname(__file__)
CONFIG = os.path.join(THIS_DIR, 'reclient_metrics.cfg')
VERSION = 1


def default_config():
  return {
      'is-googler': is_googler(),
      'countdown': 10,
      'version': VERSION,
  }


def load_config():
  config = None
  try:
    with open(CONFIG) as f:
      raw_config = json.load(f)
      if raw_config['version'] == VERSION:
        raw_config['countdown'] = max(0, raw_config['countdown'] - 1)
        config = raw_config
  except Exception:
    pass
  if not config:
    config = default_config()
  save_config(config)
  return config


def save_config(config):
  with open(CONFIG, 'w') as f:
    json.dump(config, f)


def show_message(config, ninja_out):
  print("""
Your reclient metrics will be uploaded to the chromium build metrics database. The uploaded metrics will be used to analyze user side build performance.

We upload the contents of {ninja_out_abs}.
This contains
* Flags passed to reproxy
  * Auth related flags are filtered out by reproxy
* Start and end time of build tasks
* Aggregated durations and counts of events during remote build actions
* OS (e.g. Win, Mac or Linux)
* Number of cpu cores and the amount of RAM of the building machine

Uploading reclient metrics will be started after you run autoninja another {config_count} time(s).

If you don't want to upload reclient metrics, please run following command.
$ python3 {file_path} opt-out

If you want to allow upload reclient metrics from next autoninja run, please run the
following command.
$ python3 {file_path} opt-in

If you have questions about this, please send an email to foundry-x@google.com

You can find a more detailed explanation in
{metrics_readme_path}
or
https://chromium.googlesource.com/chromium/tools/depot_tools/+/main/reclient_metrics.README.md
""".format(
      ninja_out_abs=os.path.abspath(
          os.path.join(ninja_out, ".reproxy_tmp", "logs", "rbe_metrics.txt")),
      config_count=config.get("countdown", 0),
      file_path=__file__,
      metrics_readme_path=os.path.abspath(
          os.path.join(THIS_DIR, "reclient_metrics.README.md")),
  ))


def is_googler(config=None):
  """Check whether this user is Googler or not."""
  if config is not None and 'is-googler' in config:
    return config['is-googler']
  # Use cipd auth-info to check for googler status as
  # downloading rewrapper configs already requires cipd to be logged in
  p = subprocess.run('cipd auth-info',
                     stdout=subprocess.PIPE,
                     stderr=subprocess.PIPE,
                     text=True,
                     shell=True)
  if p.returncode != 0:
    return False
  lines = p.stdout.splitlines()
  if len(lines) == 0:
    return False
  l = lines[0]
  # |l| will be like 'Logged in as <user>@google.com.' for googlers.
  return l.startswith('Logged in as ') and l.endswith('@google.com.')


def check_status(ninja_out):
  """Checks metrics collections status and shows notice to user if needed.

  Returns True if metrics should be collected."""
  config = load_config()
  if not is_googler(config):
    return False
  if 'opt-in' in config:
    return config['opt-in']
  if config.get("countdown", 0) > 0:
    show_message(config, ninja_out)
    return False
  return True


def main(argv):
  cfg = load_config()

  if not is_googler(cfg):
    save_config(cfg)
    return 0

  if len(argv) == 2 and argv[1] == 'opt-in':
    cfg['opt-in'] = True
    cfg['countdown'] = 0
    save_config(cfg)
    print('reclient metrics upload is opted in.')
    return 0

  if len(argv) == 2 and argv[1] == 'opt-out':
    cfg['opt-in'] = False
    save_config(cfg)
    print('reclient metrics upload is opted out.')
    return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
