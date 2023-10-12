# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import json
import logging
from typing import TYPE_CHECKING, Iterable, Optional

from crossbench.probes import probe
from crossbench.probes.json import JsonResultProbe, JsonResultProbeScope
from crossbench.probes.metric import MetricsMerger
from crossbench.probes.results import (EmptyProbeResult, ProbeResult,
                                       ProbeResultDict)

if TYPE_CHECKING:
  from crossbench.runner.actions import Actions
  from crossbench.runner.groups import (BrowsersRunGroup, RepetitionsRunGroup,
                                        StoriesRunGroup)
  from crossbench.runner.run import Run
  from crossbench.types import JsonDict, JSON


class InternalProbe(probe.Probe):
  IS_GENERAL_PURPOSE = False


class InternalJsonResultProbe(JsonResultProbe, InternalProbe):
  IS_GENERAL_PURPOSE = False
  FLATTEN = False

  def get_scope(self, run: Run) -> InternalJsonResultProbeScope:
    return InternalJsonResultProbeScope(self, run)


class InternalJsonResultProbeScope(JsonResultProbeScope[InternalJsonResultProbe]
                                  ):

  def stop(self, run: Run) -> None:
    # Only extract data in the late TearDown phase.
    pass

  def tear_down(self, run: Run) -> ProbeResult:
    self._json_data = self.extract_json(run)
    return super().tear_down(run)


class LogProbe(InternalProbe):
  """
  Runner-internal meta-probe: Collects the python logging data from the runner
  itself.
  """
  NAME = "cb.log"

  def get_scope(self, run: Run) -> LogProbeScope:
    return LogProbeScope(self, run)


class LogProbeScope(probe.ProbeScope[LogProbe]):

  def __init__(self, probe_instance: LogProbe, run: Run) -> None:
    super().__init__(probe_instance, run)
    self._log_handler: Optional[logging.Handler] = None

  def setup(self, run: Run) -> None:
    log_formatter = logging.Formatter(
        "%(asctime)s [%(threadName)-12.12s] [%(levelname)-5.5s] "
        "[%(name)s]  %(message)s")
    self._log_handler = logging.FileHandler(self.result_path)
    self._log_handler.setFormatter(log_formatter)
    self._log_handler.setLevel(logging.DEBUG)
    logging.getLogger().addHandler(self._log_handler)

  def start(self, run: Run) -> None:
    pass

  def stop(self, run: Run) -> None:
    pass

  def tear_down(self, run: Run) -> ProbeResult:
    assert self._log_handler
    logging.getLogger().removeHandler(self._log_handler)
    self._log_handler = None
    return ProbeResult(file=(self.result_path,))


class SystemDetailsProbe(InternalJsonResultProbe):
  """
  Runner-internal meta-probe: Collects the browser's system/platform details.
  """
  NAME = "cb.system.details"

  def to_json(self, actions: Actions) -> JSON:
    return actions.run.browser_platform.system_details()

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    return EmptyProbeResult()


class ErrorsProbe(InternalJsonResultProbe):
  """
  Runner-internal meta-probe: Collects all errors from running stories and/or
  from merging probe data.
  """
  NAME = "cb.errors"

  def to_json(self, actions: Actions) -> JSON:
    return actions.run.exceptions.to_json()

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    return self._merge_group(group, (run.results for run in group.runs))

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    return self._merge_group(
        group, (rep_group.results for rep_group in group.repetitions_groups))

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    return self._merge_group(
        group, (story_group.results for story_group in group.story_groups))

  def _merge_group(self, group,
                   results_iter: Iterable[ProbeResultDict]) -> ProbeResult:
    merged_errors = []

    for results in results_iter:
      result = results[self]
      if not result:
        continue
      source_file = result.json
      assert source_file.is_file()
      with source_file.open(encoding="utf-8") as f:
        repetition_errors = json.load(f)
        assert isinstance(repetition_errors, list)
        merged_errors.extend(repetition_errors)

    group_errors = group.exceptions.to_json()
    assert isinstance(group_errors, list)
    merged_errors.extend(group_errors)

    if not merged_errors:
      return EmptyProbeResult()

    return self.write_group_result(group, merged_errors)


class DurationsProbe(InternalJsonResultProbe):
  """
  Runner-internal meta-probe: Collects timing information for various components
  of the runner (and the times spent in individual stories as well).
  """
  NAME = "cb.durations"

  def to_json(self, actions: Actions) -> JSON:
    return actions.run.durations.to_json()

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    merged = MetricsMerger.merge_json_list(
        (repetitions_group.results[self].json
         for repetitions_group in group.repetitions_groups),
        merge_duplicate_paths=True)
    return self.write_group_result(
        group,
        merged,
    )

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    merged = MetricsMerger.merge_json_list(
        (story_group.results[self].json for story_group in group.story_groups),
        merge_duplicate_paths=True)
    return self.write_group_result(group, merged)


class ResultsSummaryProbe(InternalJsonResultProbe):
  """
  Runner-internal meta-probe: Collects a summary results.json with all the Run
  information, including all paths to the results of all attached Probes.
  """
  NAME = "cb.results"
  # Given that this is  a meta-Probe that summarizes the data from other
  # probes we exclude it from the default results lists.
  PRODUCES_DATA = False

  @property
  def is_attached(self) -> bool:
    return True

  def to_json(self, actions: Actions) -> JsonDict:
    run = actions.run
    return {
        "name": run.name,
        "cwd": str(run.out_dir),
        "story": run.story.details_json(),
        "browser": run.get_browser_details_json(),
        "durations": run.durations.to_json(),
        "probes": run.results.to_json(),
        "success": run.is_success,
        "errors": run.exceptions.error_messages()
    }

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    repetitions = []
    browser = None

    for run in group.runs:
      source_file = run.results[self].json
      assert source_file.is_file()
      with source_file.open(encoding="utf-8") as f:
        repetition_data = json.load(f)
      if browser is None:
        browser = repetition_data["browser"]
        del browser["log"]
      repetitions.append({
          "cwd": repetition_data["cwd"],
          "probes": repetition_data["probes"],
          "success": repetition_data["success"],
          "errors": repetition_data["errors"],
      })

    merged_data: JsonDict = {
        "cwd": str(group.path),
        "story": group.story.details_json(),
        "browser": browser,
        "repetitions": repetitions,
        "probes": group.results.to_json(),
        "success": group.is_success,
        "errors": group.exceptions.error_messages(),
    }
    return self.write_group_result(group, merged_data)

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    stories = {}
    browser = None

    for repetitions_group in group.repetitions_groups:
      source_file = repetitions_group.results[self].json
      assert source_file.is_file()
      with source_file.open(encoding="utf-8") as f:
        merged_story_data = json.load(f)
      if browser is None:
        browser = merged_story_data["browser"]
      story_info = merged_story_data["story"]
      stories[story_info["name"]] = {
          "cwd": merged_story_data["cwd"],
          "duration": story_info["duration"],
          "probes": merged_story_data["probes"],
          "errors": merged_story_data["errors"],
      }

    merged_data: JsonDict = {
        "cwd": str(group.path),
        "browser": browser,
        "stories": stories,
        "probes": group.results.to_json(),
        "success": group.is_success,
        "errors": group.exceptions.error_messages(),
    }
    return self.write_group_result(group, merged_data)

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    browsers = {}
    for story_group in group.story_groups:
      source_file = story_group.results[self].json
      assert source_file.is_file()
      with source_file.open(encoding="utf-8") as f:
        merged_browser_data = json.load(f)
      browser_info = merged_browser_data["browser"]
      browsers[browser_info["unique_name"]] = {
          "cwd": merged_browser_data["cwd"],
          "probes": merged_browser_data["probes"],
          "errors": merged_browser_data["errors"],
      }

    merged_data: JsonDict = {
        "cwd": str(group.path),
        "browsers": browsers,
        "probes": group.results.to_json(),
        "success": group.is_success,
        "errors": group.exceptions.error_messages(),
    }
    return self.write_group_result(group, merged_data)
