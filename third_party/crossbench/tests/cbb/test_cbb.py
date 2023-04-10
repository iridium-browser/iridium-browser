# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from crossbench.benchmarks import speedometer, motionmark, jetstream
from tests.cbb.base import CbbTest

import pytest
import sys


class Speedometer20CBBTests(CbbTest):
  benchmark_name = speedometer.Speedometer20Benchmark.NAME
  __test__ = True


class Speedometer21CBBTests(CbbTest):
  benchmark_name = speedometer.Speedometer21Benchmark.NAME
  __test__ = True


class MotionmarkCBBTests(CbbTest):
  benchmark_name = motionmark.MotionMark12Benchmark.NAME
  __test__ = True


class Jetstream20CBBTests(CbbTest):
  benchmark_name = jetstream.JetStream20Benchmark.NAME
  __test__ = True


class Jetstream21CBBTests(CbbTest):
  benchmark_name = jetstream.JetStream21Benchmark.NAME
  __test__ = True


if __name__ == '__main__':
  sys.exit(pytest.main([__file__]))
