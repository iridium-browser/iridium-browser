# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Final

from .speedometer import ProbeClsTupleT, SpeedometerBenchmark, SpeedometerProbe
from .speedometer_2 import Speedometer2Story


class Speedometer20Probe(SpeedometerProbe):
  NAME: Final[str] = "speedometer_2.0"


class Speedometer20Story(Speedometer2Story):
  NAME: Final[str] = "speedometer_2.0"
  URL: Final[str] = "https://browserbench.org/Speedometer2.0"
  PROBES: Final[ProbeClsTupleT] = (Speedometer20Probe,)


class Speedometer20Benchmark(SpeedometerBenchmark):
  """
  Benchmark runner for Speedometer 2.0
  """
  NAME: Final[str] = "speedometer_2.0"
  DEFAULT_STORY_CLS = Speedometer20Story
