# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Tuple, Type

from crossbench.probes.base import Probe
from crossbench.probes.json import JsonResultProbe
from crossbench.probes.performance_entries import PerformanceEntriesProbe
from crossbench.probes.power_sampler import PowerSamplerProbe
from crossbench.probes.profiling import ProfilingProbe
from crossbench.probes.runner import (RunDurationsProbe, RunResultsSummaryProbe,
                                      RunRunnerLogProbe)
from crossbench.probes.system_stats import SystemStatsProbe
from crossbench.probes.tracing import TracingProbe
from crossbench.probes.v8 import (V8BuiltinsPGOProbe, V8LogProbe, V8RCSProbe,
                                  V8TurbolizerProbe)
from crossbench.probes.video import VideoProbe

ABSTRACT_PROBES: Tuple[Type[Probe], ...] = (Probe, JsonResultProbe)

# Probes that are not user-configurable
INTERNAL_PROBES: Tuple[Type[Probe], ...] = (
    RunResultsSummaryProbe,
    RunRunnerLogProbe,
    RunDurationsProbe,
)

# Probes that can be used on arbitrary stories and may be user configurable.
GENERAL_PURPOSE_PROBES: Tuple[Type[Probe], ...] = (
    PerformanceEntriesProbe,
    PowerSamplerProbe,
    ProfilingProbe,
    SystemStatsProbe,
    TracingProbe,
    V8BuiltinsPGOProbe,
    V8LogProbe,
    V8RCSProbe,
    V8TurbolizerProbe,
    VideoProbe,
)

for probe_cls in GENERAL_PURPOSE_PROBES:
  assert probe_cls.IS_GENERAL_PURPOSE, (
      f"Probe {probe_cls} should be marked for GENERAL_PURPOSE")
  assert probe_cls.NAME

for probe_cls in INTERNAL_PROBES:
  assert not probe_cls.IS_GENERAL_PURPOSE, (
      f"Internal Probe {probe_cls} should not marked for GENERAL_PURPOSE")
  assert probe_cls.NAME
