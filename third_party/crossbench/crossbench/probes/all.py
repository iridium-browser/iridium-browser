# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Tuple, Type

from crossbench.probes import internal
from crossbench.probes.debugger import DebuggerProbe
from crossbench.probes.json import JsonResultProbe
from crossbench.probes.performance_entries import PerformanceEntriesProbe
from crossbench.probes.power_sampler import PowerSamplerProbe
from crossbench.probes.probe import Probe
from crossbench.probes.profiling.browser_profiling import BrowserProfilingProbe
from crossbench.probes.profiling.system_profiling import ProfilingProbe
from crossbench.probes.system_stats import SystemStatsProbe
from crossbench.probes.tracing import TracingProbe
from crossbench.probes.v8.builtins_pgo import V8BuiltinsPGOProbe
from crossbench.probes.v8.log import V8LogProbe
from crossbench.probes.v8.rcs import V8RCSProbe
from crossbench.probes.v8.turbolizer import V8TurbolizerProbe
from crossbench.probes.video import VideoProbe

ABSTRACT_PROBES: Tuple[Type[Probe], ...] = (Probe, JsonResultProbe)

# Probes that are not user-configurable
# Order matters, not alpha-sorted:
# Internal probes depend on each other, for instance the ResultsSummaryProbe
# reads the values of the other internal probes and thus needs to be the first
# to be initialized and the last to be teared down to write out a summary
# result of all the other probes.
INTERNAL_PROBES: Tuple[Type[internal.InternalProbe], ...] = (
    internal.ResultsSummaryProbe,
    internal.DurationsProbe,
    internal.ErrorsProbe,
    internal.LogProbe,
    internal.SystemDetailsProbe,
)
# ResultsSummaryProbe should always be processed last, and thus must be the
# first probe to be added to any browser.
assert INTERNAL_PROBES[0] == internal.ResultsSummaryProbe
assert INTERNAL_PROBES[1] == internal.DurationsProbe

# Probes that can be used on arbitrary stories and may be user configurable.
GENERAL_PURPOSE_PROBES: Tuple[Type[Probe], ...] = (
    DebuggerProbe,
    PerformanceEntriesProbe,
    PowerSamplerProbe,
    ProfilingProbe,
    BrowserProfilingProbe,
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
