# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING

from crossbench.browsers.chromium import Chromium
from crossbench.probes.base import Probe

if TYPE_CHECKING:
  from crossbench.probes.results import ProbeResult
  from crossbench.browsers.base import Browser
  from crossbench.runner import RepetitionsRunGroup, Run, StoriesRunGroup


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
    assert isinstance(browser, Chromium)
    super().attach(browser)
    browser.js_flags.set("--allow-natives-syntax")

  class Scope(Probe.Scope):

    def __init__(self, *args, **kwargs):
      super().__init__(*args, *kwargs)
      self._pgo_counters = None

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
      pgo_file = run.get_probe_results_file(self.probe)
      with pgo_file.open("a") as f:
        f.write(self._pgo_counters)
      return ProbeResult(file=[pgo_file])

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    merged_result_path = group.get_probe_results_file(self)
    result_files = (run.results[self].file for run in group.runs)
    result_file = self.runner_platform.concat_files(
        inputs=result_files, output=merged_result_path)
    return ProbeResult(file=[result_file])

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    merged_result_path = group.get_probe_results_file(self)
    result_files = (g.results[self].file for g in group.repetitions_groups)
    result_file = self.runner_platform.concat_files(
        inputs=result_files, output=merged_result_path)
    return ProbeResult(file=[result_file])
