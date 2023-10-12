# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import json
import logging
import pathlib
import datetime as dt
from collections import defaultdict
from typing import TYPE_CHECKING, Any, Dict, List, Tuple, Type

from crossbench.benchmarks import PressBenchmark
from crossbench.probes import metric as cb_metric
from crossbench.probes.json import JsonResultProbe
from crossbench.probes.results import ProbeResult, ProbeResultDict
from crossbench.stories.press_benchmark import PressBenchmarkStory

if TYPE_CHECKING:
  from crossbench.runner.run import Run
  from crossbench.runner.actions import Actions
  from crossbench.runner.groups import (StoriesRunGroup, BrowsersRunGroup)


class JetStream2Probe(JsonResultProbe, metaclass=abc.ABCMeta):
  """
  JetStream2-specific Probe.
  Extracts all JetStream2 times and scores.
  """
  IS_GENERAL_PURPOSE: bool = False
  FLATTEN: bool = False
  JS: str = """
  let results = Object.create(null);
  for (let benchmark of JetStream.benchmarks) {
    const data = { score: benchmark.score };
    if ("worst4" in benchmark) {
      data.firstIteration = benchmark.firstIteration;
      data.average = benchmark.average;
      data.worst4 = benchmark.worst4;
    } else if ("runTime" in benchmark) {
      data.runTime = benchmark.runTime;
      data.startupTime = benchmark.startupTime;
    }
    results[benchmark.plan.name] = data;
  };
  return results;
"""

  def to_json(self, actions: Actions) -> Dict[str, float]:
    data = actions.js(self.JS)
    assert len(data) > 0, "No benchmark data generated"
    return data

  def process_json_data(self, json_data: Dict[str, Any]) -> Dict[str, Any]:
    assert "Total" not in json_data, (
        "JSON result data already contains a ['Total'] entry.")
    json_data["Total"] = self._compute_total_metrics(json_data)
    return json_data

  def _compute_total_metrics(self, json_data: Dict[str,
                                                   Any]) -> Dict[str, float]:
    # Manually add all total scores
    accumulated_metrics = defaultdict(list)
    for _, metrics in json_data.items():
      for metric, value in metrics.items():
        accumulated_metrics[metric].append(value)
    total: Dict[str, float] = {}
    for metric, values in accumulated_metrics.items():
      total[metric] = cb_metric.geomean(values)
    return total

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    merged = cb_metric.MetricsMerger.merge_json_list(
        story_group.results[self].json
        for story_group in group.repetitions_groups)
    return self.write_group_result(group, merged, write_csv=True)

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    return self.merge_browsers_json_list(group).merge(
        self.merge_browsers_csv_list(group))

  def log_run_result(self, run: Run) -> None:
    self._log_result(run.results, single_result=True)

  def log_browsers_result(self, group: BrowsersRunGroup) -> None:
    self._log_result(group.results, single_result=False)

  def _log_result(self, result_dict: ProbeResultDict,
                  single_result: bool) -> None:
    if self not in result_dict:
      return
    results_json: pathlib.Path = result_dict[self].json
    logging.info("-" * 80)
    logging.critical("JetStream results:")
    if not single_result:
      logging.critical("  %s", result_dict[self].csv)
    logging.info("- " * 40)

    with results_json.open(encoding="utf-8") as f:
      data = json.load(f)
      if single_result:
        logging.critical("Score %s", data["Total"]["score"])
      else:
        self._log_result_metrics(data)

  def _extract_result_metrics_table(self, metrics: Dict[str, Any],
                                    table: Dict[str, List[str]]) -> None:
    for metric_key, metric_value in metrics.items():
      parts = metric_key.split("/")
      if len(parts) != 2 or parts[0] == "Total" or parts[1] != "score":
        continue
      table[metric_key].append(
          cb_metric.format_metric(metric_value["average"],
                                  metric_value["stddev"]))
      # Separate runs don't produce a score
    if "Total/score" in metrics:
      metric_value = metrics["Total/score"]
      table["Score"].append(
          cb_metric.format_metric(metric_value["average"],
                                  metric_value["stddev"]))


class JetStream2Story(PressBenchmarkStory, metaclass=abc.ABCMeta):
  URL_LOCAL: str = "http://localhost:8000/"
  SUBSTORIES: Tuple[str, ...] = (
      "WSL",
      "UniPoker",
      "uglify-js-wtb",
      "typescript",
      "tsf-wasm",
      "tagcloud-SP",
      "string-unpack-code-SP",
      "stanford-crypto-sha256",
      "stanford-crypto-pbkdf2",
      "stanford-crypto-aes",
      "splay",
      "segmentation",
      "richards-wasm",
      "richards",
      "regexp",
      "regex-dna-SP",
      "raytrace",
      "quicksort-wasm",
      "prepack-wtb",
      "pdfjs",
      "OfflineAssembler",
      "octane-zlib",
      "octane-code-load",
      "navier-stokes",
      "n-body-SP",
      "multi-inspector-code-load",
      "ML",
      "mandreel",
      "lebab-wtb",
      "json-stringify-inspector",
      "json-parse-inspector",
      "jshint-wtb",
      "HashSet-wasm",
      "hash-map",
      "gcc-loops-wasm",
      "gbemu",
      "gaussian-blur",
      "float-mm.c",
      "FlightPlanner",
      "first-inspector-code-load",
      "espree-wtb",
      "earley-boyer",
      "delta-blue",
      "date-format-xparb-SP",
      "date-format-tofte-SP",
      "crypto-sha1-SP",
      "crypto-md5-SP",
      "crypto-aes-SP",
      "crypto",
      "coffeescript-wtb",
      "chai-wtb",
      "cdjs",
      "Box2D",
      "bomb-workers",
      "Basic",
      "base64-SP",
      "babylon-wtb",
      "Babylon",
      "async-fs",
      "Air",
      "ai-astar",
      "acorn-wtb",
      "3d-raytrace-SP",
      "3d-cube-SP",
  )

  @property
  def substory_duration(self) -> dt.timedelta:
    return dt.timedelta(seconds=2)

  def setup(self, run: Run) -> None:
    with run.actions("Setup") as actions:
      actions.show_url(self._url)
      if self._substories != self.SUBSTORIES:
        actions.wait_js_condition(("return JetStream && JetStream.benchmarks "
                                   "&& JetStream.benchmarks.length > 0;"), 0.1,
                                  10)
        actions.js(
            """
        let benchmarks = arguments[0];
        JetStream.benchmarks = JetStream.benchmarks.filter(
            benchmark => benchmarks.includes(benchmark.name));
        """,
            arguments=[self._substories])
      actions.wait_js_condition(
          """
        return document.querySelectorAll("#results>.benchmark").length > 0;
      """, 1, self.duration + dt.timedelta(seconds=30))

  def run(self, run: Run) -> None:
    with run.actions("Running") as actions:
      actions.js("JetStream.start()")
      actions.wait(self.fast_duration)
    with run.actions("Waiting for completion") as actions:
      actions.wait_js_condition(
          """
        let summaryElement = document.getElementById("result-summary");
        return (summaryElement.classList.contains("done"));
        """, self.substory_duration, self.slow_duration)


ProbeClsTupleT = Tuple[Type[JetStream2Probe], ...]


class JetStreamBenchmark(PressBenchmark, metaclass=abc.ABCMeta):

  @classmethod
  def short_base_name(cls) -> str:
    return "js"

  @classmethod
  def base_name(cls) -> str:
    return "jetstream"


class JetStream2Benchmark(JetStreamBenchmark):
  pass
