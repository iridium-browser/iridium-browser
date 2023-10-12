# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from tests import run_helper

from crossbench.benchmarks.speedometer import speedometer_2_0
from crossbench.benchmarks.speedometer import speedometer_2_1
from crossbench.benchmarks.speedometer import speedometer_3_0
from tests.crossbench.benchmarks import speedometer_helper


class Speedometer20TestCase(speedometer_helper.SpeedometerBaseTestCase):

  @property
  def benchmark_cls(self):
    return speedometer_2_0.Speedometer20Benchmark

  @property
  def story_cls(self):
    return speedometer_2_0.Speedometer20Story

  @property
  def probe_cls(self):
    return speedometer_2_0.Speedometer20Probe

  @property
  def name(self):
    return "speedometer_2.0"


class Speedometer21TestCase(speedometer_helper.SpeedometerBaseTestCase):

  @property
  def benchmark_cls(self):
    return speedometer_2_1.Speedometer21Benchmark

  @property
  def story_cls(self):
    return speedometer_2_1.Speedometer21Story

  @property
  def probe_cls(self):
    return speedometer_2_1.Speedometer21Probe

  @property
  def name(self):
    return "speedometer_2.1"


class Speedometer30TestCase(speedometer_helper.SpeedometerBaseTestCase):

  @property
  def benchmark_cls(self):
    return speedometer_3_0.Speedometer30Benchmark

  @property
  def story_cls(self):
    return speedometer_3_0.Speedometer30Story

  @property
  def probe_cls(self):
    return speedometer_3_0.Speedometer30Probe

  @property
  def name(self):
    return "speedometer_3.0"

  def test_run_combined(self):
    self._run_combined(["TodoMVC-JavaScript-ES5", "TodoMVC-Backbone"])

  def test_run_separate(self):
    self._run_separate(["TodoMVC-JavaScript-ES5", "TodoMVC-Backbone"])


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
