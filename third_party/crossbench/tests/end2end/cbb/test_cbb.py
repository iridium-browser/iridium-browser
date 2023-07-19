# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from crossbench.benchmarks import all as benchmarks
from .helper import CBBTestCase

import pytest
import sys


class Speedometer20CBBTestCase(CBBTestCase):
  benchmark_name = benchmarks.Speedometer20Benchmark.NAME
  __test__ = True


class Speedometer21CBBTestCase(CBBTestCase):
  benchmark_name = benchmarks.Speedometer21Benchmark.NAME
  __test__ = True


class MotionmarkCBBTestCase(CBBTestCase):
  benchmark_name = benchmarks.MotionMark12Benchmark.NAME
  __test__ = True


class Jetstream20CBBTestCase(CBBTestCase):
  benchmark_name = benchmarks.JetStream20Benchmark.NAME
  __test__ = True


class Jetstream21CBBTestCase(CBBTestCase):
  benchmark_name = benchmarks.JetStream21Benchmark.NAME
  __test__ = True


if __name__ == "__main__":
  sys.exit(pytest.main([__file__]))
