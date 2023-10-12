# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING, Optional, cast

from crossbench.browsers.chromium.chromium import Chromium
from crossbench.probes.probe import Probe, ProbeScope
from crossbench.probes.results import LocalProbeResult

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.probes.results import ProbeResult
  from crossbench.runner.run import Run
  from crossbench.runner.groups import (RepetitionsRunGroup, StoriesRunGroup)


class V8BuiltinsPGOProbe(Probe):
  """
  Chromium-only Probe to extract V8 builtins PGO data.
  The resulting data is used to optimize Torque and CSA builtins.
  """
  NAME = "v8.builtins.pgo"

  def is_compatible(self, browser: Browser) -> bool:
    return isinstance(browser, Chromium)

  def attach(self, browser: Browser) -> None:
    # Use inline isinstance assert to hint that we have a Chrome browser.
    assert isinstance(browser, Chromium), "Expected Chromium-based browser."
    super().attach(browser)
    chromium = cast(Chromium, browser)
    chromium.js_flags.set("--allow-natives-syntax")

  def get_scope(self, run: Run) -> V8BuiltinsPGOProbeScope:
    return V8BuiltinsPGOProbeScope(self, run)

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    merged_result_path = group.get_local_probe_result_path(self)
    result_files = (run.results[self].file for run in group.runs)
    result_file = self.runner_platform.concat_files(
        inputs=result_files, output=merged_result_path)
    return LocalProbeResult(file=[result_file])

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    merged_result_path = group.get_local_probe_result_path(self)
    result_files = (g.results[self].file for g in group.repetitions_groups)
    result_file = self.runner_platform.concat_files(
        inputs=result_files, output=merged_result_path)
    return LocalProbeResult(file=[result_file])


class V8BuiltinsPGOProbeScope(ProbeScope[V8BuiltinsPGOProbe]):
  _pgo_counters: Optional[str] = None

  def setup(self, run: Run) -> None:
    pass

  def start(self, run: Run) -> None:
    pass

  def stop(self, run: Run) -> None:
    with run.actions("Extract Builtins PGO DATA") as actions:
      self._pgo_counters = actions.js(
          "return %GetAndResetTurboProfilingData();")

  def tear_down(self, run: Run) -> ProbeResult:
    assert self._pgo_counters is not None and self._pgo_counters, (
        "Chrome didn't produce any V8 builtins PGO data. "
        "Please make sure to set the v8_enable_builtins_profiling=true "
        "gn args.")
    pgo_file = self.result_path
    with pgo_file.open("a") as f:
      f.write(self._pgo_counters)
    return LocalProbeResult(file=[pgo_file])
