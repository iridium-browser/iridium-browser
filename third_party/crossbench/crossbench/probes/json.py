# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import csv
import json
import logging
import pathlib
from collections import defaultdict
from typing import TYPE_CHECKING, Any, Callable, Dict, List, Optional, Union

from tabulate import tabulate

from crossbench.probes import base, helper
from crossbench.probes.results import ProbeResult

if TYPE_CHECKING:
  from crossbench.runner import (Actions, BrowsersRunGroup, RepetitionsRunGroup,
                                 Run, RunGroup)


class JsonResultProbe(base.Probe, metaclass=abc.ABCMeta):
  """
  Abstract Probe that stores a JSON result extracted by the `to_json` method

  Tje `to_json` is provided by subclasses. A typical examples includes just
  running a JS script on the page.
  Multiple JSON result files for RepetitionsRunGroups are merged with the
  ValuesMerger. Custom merging for other RunGroups can be defined in the
  subclass.
  """

  FLATTEN = True

  @property
  def results_file_name(self) -> str:
    return f"{self.name}.json"

  @abc.abstractmethod
  def to_json(self, actions: Actions) -> Any:
    """
    Override in subclasses.
    Returns json-serializable data.
    """
    return None

  def flatten_json_data(self, json_data: Any) -> Dict[str, Any]:
    return helper.Flatten(json_data).data

  def process_json_data(self, json_data) -> Any:
    return json_data

  class Scope(base.Probe.Scope):

    def __init__(self, probe: JsonResultProbe, run: Run):
      super().__init__(probe, run)
      self._json_data = None

    @property
    def probe(self) -> JsonResultProbe:
      return super().probe

    def to_json(self, actions: Actions) -> Any:
      return self.probe.to_json(actions)

    def start(self, run: Run) -> None:
      pass

    def stop(self, run: Run) -> None:
      self._json_data = self.extract_json(run)

    def tear_down(self, run: Run) -> ProbeResult:
      self._json_data = self.process_json_data(self._json_data)
      return self.write_json(run, self._json_data)

    def extract_json(self, run: Run) -> Dict:
      with run.actions(f"Extracting Probe name={self.probe.name}") as actions:
        json_data = self.to_json(actions)
        assert json_data is not None, (
            "Probe name=={self.probe.name} produced no data")
        return json_data

    def write_json(self, run: Run, json_data: Any) -> ProbeResult:
      flattened_file = None
      with run.actions(f"Writing Probe name={self.probe.name}"):
        assert json_data is not None
        raw_file = self.results_file
        if self.probe.FLATTEN:
          raw_file = raw_file.with_suffix(".json.raw")
          flattened_file = self.results_file
          flat_json_data = self.flatten_json_data(json_data)
          with flattened_file.open("w", encoding="utf-8") as f:
            json.dump(flat_json_data, f, indent=2)
        with raw_file.open("w", encoding="utf-8") as f:
          json.dump(json_data, f, indent=2)
      if flattened_file:
        return ProbeResult(json=(flattened_file,), file=(raw_file,))
      return ProbeResult(json=(raw_file,))

    def process_json_data(self, json_data: Any) -> Any:
      return self.probe.process_json_data(json_data)

    def flatten_json_data(self, json_data: Any) -> Dict[str, Any]:
      return self.probe.flatten_json_data(json_data)

  def merge_repetitions(
      self,
      group: RepetitionsRunGroup,
  ) -> ProbeResult:
    merger = helper.ValuesMerger()
    for run in group.runs:
      if self not in run.results:
        raise Exception(f"Probe {self.NAME} produced no data to merge.")
      source_file = run.results[self].json
      assert source_file.is_file()
      with source_file.open(encoding="utf-8") as f:
        merger.add(json.load(f))
    return self.write_group_result(group, merger, write_csv=True)

  def merge_browsers_json_list(self, group: BrowsersRunGroup) -> ProbeResult:
    merged_json: Dict[str, Dict[str, Any]] = {}
    for story_group in group.story_groups:
      browser_result: Dict[str, Any] = {}
      merged_json[story_group.browser.unique_name] = browser_result
      browser_result["info"] = story_group.info
      browser_json_path = story_group.results[self].json
      assert browser_json_path.is_file()
      with browser_json_path.open(encoding="utf-8") as f:
        browser_result["data"] = json.load(f)
    merged_json_path = group.get_probe_results_file(self)
    assert not merged_json_path.exists()
    with merged_json_path.open("w", encoding="utf-8") as f:
      json.dump(merged_json, f, indent=2)
    return ProbeResult(json=(merged_json_path,))

  def merge_browsers_csv_list(self, group: BrowsersRunGroup) -> ProbeResult:
    csv_list: List[pathlib.Path] = []
    headers: List[str] = []
    for story_group in group.story_groups:
      csv_list.append(story_group.results[self].csv)
      headers.append(story_group.browser.unique_name)
    merged_table = helper.merge_csv(csv_list)
    merged_json_path = group.get_probe_results_file(self, exists_ok=True)
    merged_csv_path = merged_json_path.with_suffix(".csv")
    assert not merged_csv_path.exists()
    with merged_csv_path.open("w", newline="", encoding="utf-8") as f:
      csv.writer(f, delimiter="\t").writerows(merged_table)
    return ProbeResult(csv=(merged_csv_path,))

  def write_group_result(self,
                         group: RunGroup,
                         merged_data: Union[Dict, helper.ValuesMerger],
                         write_csv: bool = False,
                         value_fn: Optional[Callable[[Any], Any]] = None
                        ) -> ProbeResult:
    merged_json_path = group.get_probe_results_file(self)
    with merged_json_path.open("w", encoding="utf-8") as f:
      if isinstance(merged_data, dict):
        json.dump(merged_data, f, indent=2)
      else:
        json.dump(merged_data.to_json(), f, indent=2)
    if not write_csv:
      return ProbeResult(json=(merged_json_path,))
    if not isinstance(merged_data, helper.ValuesMerger):
      raise ValueError("write_csv is only supported for ValuesMerger, "
                       f"but found {type(merged_data)}'.")
    if not value_fn:
      value_fn = value_geomean
    return self.write_group_csv_result(group, merged_data, merged_json_path,
                                       value_fn)

  def write_group_csv_result(self, group: RunGroup,
                             merged_data: helper.ValuesMerger,
                             merged_json_path: pathlib.Path,
                             value_fn: Callable[[Any], Any]) -> ProbeResult:
    merged_csv_path = merged_json_path.with_suffix(".csv")
    assert not merged_csv_path.exists()
    with merged_csv_path.open("w", newline="", encoding="utf-8") as f:
      writer = csv.writer(f, delimiter="\t")
      csv_data = merged_data.to_csv(value_fn, list(group.info.items()))
      writer.writerows(csv_data)
    return ProbeResult(json=(merged_json_path,), csv=(merged_csv_path,))

  def _log_result_metrics(self, data: Dict):
    table: Dict[str, List[str]] = defaultdict(list)
    for browser_result in data.values():
      for info_key in ("label", "browser", "version"):
        table[info_key].append(browser_result["info"][info_key])
      data = browser_result["data"]
      self._extract_result_metrics_table(data, table)
    flattened: List[List[str]] = list(
        [label] + values for label, values in table.items())
    logging.info(tabulate(flattened, tablefmt="plain"))

  def _extract_result_metrics_table(self, metrics: Dict[str, Any],
                                    table: Dict[str, List[str]]) -> None:
    """Add individual metrics to the table in here.
    Typically you only add score and total values for each benchmark or 
    benchmark item."""
    del metrics
    del table


def value_geomean(value):
  return value.geomean
