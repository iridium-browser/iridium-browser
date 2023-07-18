# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from crossbench.benchmarks.jetstream import (JetStream20Benchmark,
                                             JetStream21Benchmark)
from crossbench.benchmarks.loading import PageLoadBenchmark
from crossbench.benchmarks.motionmark import MotionMark12Benchmark
from crossbench.benchmarks.speedometer import (Speedometer20Benchmark,
                                               Speedometer21Benchmark,
                                               Speedometer30Benchmark)

__all__ = [
    "JetStream20Benchmark",
    "JetStream21Benchmark",
    "PageLoadBenchmark",
    "MotionMark12Benchmark",
    "Speedometer20Benchmark",
    "Speedometer21Benchmark",
    "Speedometer30Benchmark",
]
