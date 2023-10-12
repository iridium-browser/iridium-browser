# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
from typing import TYPE_CHECKING, Optional, cast

from crossbench.browsers.chromium.chromium import Chromium
from crossbench.probes.probe import Probe, ProbeMissingDataError, ProbeScope
from crossbench.probes.results import LocalProbeResult, ProbeResult

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.runner.run import Run
  from crossbench.runner.groups import (BrowsersRunGroup, RepetitionsRunGroup,
                                        StoriesRunGroup)


class V8RCSProbe(Probe):
  """
  Chromium-only Probe to extract runtime-call-stats data that can be used
  to analyze precise counters and time spent in various VM components in V8:
  https://v8.dev/tools/head/callstats.html
  """
  NAME = "v8.rcs"

  def is_compatible(self, browser: Browser) -> bool:
    return isinstance(browser, Chromium)

  def attach(self, browser: Browser) -> None:
    assert isinstance(browser, Chromium), "Expected Chromium-based browser."
    super().attach(browser)
    chromium = cast(Chromium, browser)
    chromium.js_flags.update(("--runtime-call-stats", "--allow-natives-syntax"))

  @property
  def result_path_name(self) -> str:
    return f"{self.name}.txt"

  def get_scope(self, run: Run) -> V8RCSProbeScope:
    return V8RCSProbeScope(self, run)

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    merged_result_path = group.get_local_probe_result_path(self)
    result_files = (run.results[self].file for run in group.runs)
    result_file = self.runner_platform.concat_files(
        inputs=result_files, output=merged_result_path)
    return LocalProbeResult(file=[result_file])

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    merged_result_path = group.get_local_probe_result_path(self)
    with merged_result_path.open("w", encoding="utf-8") as merged_file:
      for repetition_group in group.repetitions_groups:
        merged_repetitions_file = repetition_group.results[self].file
        if not merged_repetitions_file.exists():
          logging.info("Probe %s: skipping non-existing results file: %s",
                       self.NAME, merged_repetitions_file)
          continue
        merged_file.write(f"\n== Page: {repetition_group.story.name}\n")
        with merged_repetitions_file.open(encoding="utf-8") as f:
          merged_file.write(f.read())
    return LocalProbeResult(file=[merged_result_path])

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    # We put all the fils by in a toplevel v8.rcs folder
    merged_result_path = group.get_local_probe_result_path(self).with_suffix("")
    merged_result_path.mkdir()
    files = []
    for story_group in group.story_groups:
      story_group_file = story_group.results[self].file
      # Be permissive and skip failed probes
      if not story_group_file.exists():
        logging.info("Probe %s: skipping non-existing results file: %s",
                     self.NAME, story_group_file)
      dest_file = merged_result_path / f"{story_group.path.name}.rcs.txt"
      dest_file.symlink_to(story_group_file)
      files.append(dest_file)
    return LocalProbeResult(file=files)


class V8RCSProbeScope(ProbeScope[V8RCSProbe]):
  _rcs_table: Optional[str] = None

  def setup(self, run: Run) -> None:
    pass

  def start(self, run: Run) -> None:
    pass

  def stop(self, run: Run) -> None:
    with run.actions("Extract RCS") as actions:
      self._rcs_table = actions.js("return %GetAndResetRuntimeCallStats();")

  def tear_down(self, run: Run) -> ProbeResult:
    if not self._rcs_table:
      raise ProbeMissingDataError(
          "Chrome didn't produce any RCS data. "
          "Use Chrome Canary or make sure to enable the "
          "v8_enable_runtime_call_stats compile-time flag.")
    rcs_file = self.result_path
    with rcs_file.open("a") as f:
      f.write(self._rcs_table)
    return LocalProbeResult(file=(rcs_file,))
