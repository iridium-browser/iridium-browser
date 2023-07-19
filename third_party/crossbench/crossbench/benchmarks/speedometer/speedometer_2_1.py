# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Final

from .speedometer import ProbeClsTupleT, SpeedometerBenchmark, SpeedometerProbe
from .speedometer_2 import Speedometer2Story


class Speedometer21Probe(SpeedometerProbe):
  NAME: Final[str] = "speedometer_2.1"


class Speedometer21Story(Speedometer2Story):
  NAME: Final[str] = "speedometer_2.1"
  URL: Final[str] = "https://browserbench.org/Speedometer2.1/"
  PROBES: Final[ProbeClsTupleT] = (Speedometer21Probe,)


class Speedometer21Benchmark(SpeedometerBenchmark):
  """
  Benchmark runner for Speedometer 2.1
  """
  NAME: Final[str] = "speedometer_2.1"
  DEFAULT_STORY_CLS = Speedometer21Story
