# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from crossbench.runner import Timing

import datetime as dt

import sys
import pytest


class TimingTestCase(unittest.TestCase):

  def test_default_instance(self):
    t = Timing()
    self.assertEqual(t.unit, dt.timedelta(seconds=1))
    self.assertEqual(t.timedelta(10), dt.timedelta(seconds=10))
    self.assertEqual(t.timedelta(10, absolute=True), dt.timedelta(seconds=10))
    self.assertEqual(t.units(1), 1)
    self.assertEqual(t.units(dt.timedelta(seconds=1)), 1)

  def test_default_instance_slowdown(self):
    t = Timing(unit=dt.timedelta(seconds=10))
    self.assertEqual(t.unit, dt.timedelta(seconds=10))
    self.assertEqual(t.timedelta(10), dt.timedelta(seconds=100))
    self.assertEqual(t.timedelta(10, absolute=True), dt.timedelta(seconds=10))
    self.assertEqual(t.units(100), 10)
    self.assertEqual(t.units(dt.timedelta(seconds=100)), 10)

  def test_default_instance_speedup(self):
    t = Timing(unit=dt.timedelta(seconds=0.1))
    self.assertEqual(t.unit, dt.timedelta(seconds=0.1))
    self.assertEqual(t.timedelta(10), dt.timedelta(seconds=1))
    self.assertEqual(t.timedelta(10, absolute=True), dt.timedelta(seconds=10))
    self.assertEqual(t.units(1), 10)
    self.assertEqual(t.units(dt.timedelta(seconds=1)), 10)


if __name__ == "__main__":
  sys.exit(pytest.main([__file__]))
