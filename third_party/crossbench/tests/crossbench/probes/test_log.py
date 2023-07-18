# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from crossbench.probes.all import V8LogProbe

import sys
import pytest


class TestV8LogProbe(unittest.TestCase):

  def test_invalid_flags(self):
    with self.assertRaises(ValueError):
      V8LogProbe(js_flags=["--no-opt"])

  def test_disabling_flag(self):
    probe = V8LogProbe(log_all=True, js_flags=["--no-log-maps"])
    self.assertSetEqual({"--log-all", "--no-log-maps"},
                        set(probe.js_flags.keys()))

  def test_conflicting_flags(self):
    with self.assertRaises(Exception):
      V8LogProbe(js_flags=["--log-maps", "--no-log-maps"])
    with self.assertRaises(Exception):
      V8LogProbe(prof=True, js_flags=["--no-prof"])

  def test_parse_invalid_config(self):
    with self.assertRaises(Exception):
      # No logging enabled
      V8LogProbe.from_config({"log_all": False})
    with self.assertRaises(Exception):
      V8LogProbe.from_config({"log_all": []})
    with self.assertRaises(Exception):
      V8LogProbe.from_config({"prof": 12})
    with self.assertRaises(Exception):
      V8LogProbe.from_config({"js_flags": [1]})
    with self.assertRaises(Exception):
      V8LogProbe.from_config({"js_flags": ["--log-all", True]})

  def test_parse_config(self):
    probe = V8LogProbe.from_config({})
    self.assertSetEqual({"--log-all"}, set(probe.js_flags.keys()))

    probe = V8LogProbe.from_config({"prof": False})
    self.assertSetEqual({"--log-all"}, set(probe.js_flags.keys()))

    probe = V8LogProbe.from_config({"prof": True})
    self.assertSetEqual({"--log-all", "--prof"}, set(probe.js_flags.keys()))

    probe = V8LogProbe.from_config({"js_flags": None})
    self.assertSetEqual({"--log-all"}, set(probe.js_flags.keys()))

    probe = V8LogProbe.from_config({"js_flags": []})
    self.assertSetEqual({"--log-all"}, set(probe.js_flags.keys()))

    probe = V8LogProbe.from_config(
        {"js_flags": ["--no-log-ic", "--no-log-maps"]})
    self.assertSetEqual({"--log-all", "--no-log-ic", "--no-log-maps"},
                        set(probe.js_flags.keys()))

    probe = V8LogProbe.from_config({
        "log_all": False,
        "js_flags": ["--no-log-ic", "--no-log-maps"]
    })
    self.assertSetEqual({"--no-log-ic", "--no-log-maps"},
                        set(probe.js_flags.keys()))


if __name__ == "__main__":
  sys.exit(pytest.main([__file__]))
