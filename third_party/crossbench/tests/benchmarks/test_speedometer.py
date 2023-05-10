# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

import pytest

from crossbench.benchmarks import speedometer
from tests.benchmarks import speedometer_helper


class Speedometer20TestCase(speedometer_helper.Speedometer2BaseTestCase):

  @property
  def benchmark_cls(self):
    return speedometer.Speedometer20Benchmark

  @property
  def story_cls(self):
    return speedometer.Speedometer20Story

  @property
  def probe_cls(self):
    return speedometer.Speedometer20Probe

  @property
  def name(self):
    return "speedometer_2.0"


class Speedometer21TestCase(speedometer_helper.Speedometer2BaseTestCase):

  @property
  def benchmark_cls(self):
    return speedometer.Speedometer21Benchmark

  @property
  def story_cls(self):
    return speedometer.Speedometer21Story

  @property
  def probe_cls(self):
    return speedometer.Speedometer21Probe

  @property
  def name(self):
    return "speedometer_2.1"


if __name__ == "__main__":
  sys.exit(pytest.main([__file__]))
