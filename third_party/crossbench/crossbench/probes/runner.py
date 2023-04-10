# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import json
import logging
from typing import TYPE_CHECKING, Any, Dict

from crossbench.probes import base
from crossbench.probes.json import JsonResultProbe
from crossbench.probes.results import ProbeResult

if TYPE_CHECKING:
  from crossbench.runner import (Actions, RepetitionsRunGroup, Run,
                                 StoriesRunGroup)


class RunRunnerLogProbe(base.Probe):
  """
  Runner-internal meta-probe: Collects the python logging data from the runner
  itself.
  """
  NAME = "cb.runner.log"
  IS_GENERAL_PURPOSE = False
  FLATTEN = False

  class Scope(base.Probe.Scope):

    def __init__(self, *args, **kwargs):
      super().__init__(*args, **kwargs)
      self._log_handler = None

    def setup(self, run: Run) -> None:
      log_formatter = logging.Formatter(
          "%(asctime)s [%(threadName)-12.12s] [%(levelname)-5.5s] "
          "[%(name)s]  %(message)s")
      self._log_handler = logging.FileHandler(self.results_file)
      self._log_handler.setFormatter(log_formatter)
      self._log_handler.setLevel(logging.DEBUG)
      logging.getLogger().addHandler(self._log_handler)

    def start(self, run: Run) -> None:
      pass

    def stop(self, run: Run) -> None:
      pass

    def tear_down(self, run: Run) -> ProbeResult:
      logging.getLogger().removeHandler(self._log_handler)
      self._log_handler = None
      return ProbeResult(file=(self.results_file,))


class RunDurationsProbe(JsonResultProbe):
  """
  Runner-internal meta-probe: Collects timing information for various components
  of the runner (and the times spent in individual stories as well).
  """
  NAME = "cb.runner.durations"
  IS_GENERAL_PURPOSE = False
  FLATTEN = False

  def to_json(self, actions: Actions) -> Any:
    return actions.run.durations.to_json()

  class Scope(JsonResultProbe.Scope):

    def stop(self, run: Run) -> None:
      # Only extract data in the late TearDown phase.
      pass

    def tear_down(self, run: Run) -> ProbeResult:
      json_data = self.extract_json(run)
      return self.write_json(run, json_data)


class RunResultsSummaryProbe(JsonResultProbe):
  """
  Runner-internal meta-probe: Collects a summary results.json with all the Run
  information, including all paths to the results of all attached Probes.
  """
  NAME = "results"
  IS_GENERAL_PURPOSE = False
  # Given that this is  a meta-Probe that summarizes the data from other
  # probes we exclude it from the default results lists.
  PRODUCES_DATA = False
  FLATTEN = False

  @property
  def is_attached(self) -> bool:
    return True

  def to_json(self, actions: Actions) -> Dict[str, Any]:
    run = actions.run
    return {
        "name": run.name,
        "cwd": str(run.out_dir),
        "story": run.story.details_json(),
        "browser": run.get_browser_details_json(),
        "durations": run.durations.to_json(),
        "probes": run.results.to_json(),
        "errors": run.exceptions.to_json()
    }

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    iterations = []
    browser = None

    for run in group.runs:
      source_file = run.results[self].json
      assert source_file.is_file()
      with source_file.open(encoding="utf-8") as f:
        iteration_data = json.load(f)
      if browser is None:
        browser = iteration_data["browser"]
        del browser["log"]
      iterations.append({
          "cwd": iteration_data["cwd"],
          "probes": iteration_data["probes"],
          "errors": iteration_data["errors"],
      })

    merged_data = {
        "cwd": str(group.path),
        "story": group.story.details_json(),
        "browser": browser,
        "iterations": iterations,
        "probes": group.results.to_json(),
        "errors": group.exceptions.to_json(),
    }
    return self.write_group_result(group, merged_data)

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    stories = {}
    browser = None

    for story_group in group.repetitions_groups:
      source_file = story_group.results[self].json
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

    merged_data = {
        "cwd": str(group.path),
        "browser": browser,
        "stories": stories,
        "probes": group.results.to_json(),
        "errors": group.exceptions.to_json(),
    }
    return self.write_group_result(group, merged_data)

  class Scope(JsonResultProbe.Scope):

    def stop(self, run: Run) -> None:
      # Only extract data in the late TearDown phase.
      pass

    def tear_down(self, run: Run) -> ProbeResult:
      # Extract JSON late, When all other probes have produced data.
      json_data = self.extract_json(run)
      return self.write_json(run, json_data)
