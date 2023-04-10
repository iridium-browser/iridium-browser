# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import json
import logging
import pathlib
from typing import (TYPE_CHECKING, Any, Dict, Final, List, Optional, Sequence,
                    Tuple, Type)

import crossbench.probes.helper as probes_helper
from crossbench import helper
from crossbench.benchmarks.base import PressBenchmark
from crossbench.probes.json import JsonResultProbe
from crossbench.probes.results import ProbeResult, ProbeResultDict
from crossbench.stories import PressBenchmarkStory

if TYPE_CHECKING:
  import argparse

  from crossbench.runner import (Actions, BrowsersRunGroup, Run, Runner,
                                 StoriesRunGroup)


def _probe_remove_tests_segments(path: Tuple[str, ...]) -> str:
  return "/".join(segment for segment in path if segment != "tests")


class Speedometer2Probe(JsonResultProbe, metaclass=abc.ABCMeta):
  """
  Speedometer2-specific probe (compatible with v2.0 and v2.1).
  Extracts all speedometer times and scores.
  """
  IS_GENERAL_PURPOSE: Final[bool] = False
  JS: Final[str] = "return window.suiteValues;"

  def to_json(self, actions: Actions) -> Dict[str, Any]:
    return actions.js(self.JS)

  def flatten_json_data(self, json_data: Sequence) -> Dict[str, Any]:
    # json_data may contain multiple iterations, merge those first
    assert isinstance(json_data, list)
    merged = probes_helper.ValuesMerger(
        json_data, key_fn=_probe_remove_tests_segments).to_json(
            value_fn=lambda values: values.geomean)
    return probes_helper.Flatten(merged).data

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    merged = probes_helper.ValuesMerger.merge_json_list(
        repetitions_group.results[self].json
        for repetitions_group in group.repetitions_groups)
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
    logging.info("Speedometer results:")
    if not single_result:
      relative_path = result_dict[self].csv.relative_to(pathlib.Path.cwd())
      logging.info("  %s", relative_path)
    logging.info("- " * 40)

    with results_json.open(encoding="utf-8") as f:
      data = json.load(f)
      if single_result:
        logging.info("Score %s", data["score"])
      else:
        self._log_result_metrics(data)

  def _extract_result_metrics_table(self, metrics: Dict[str, Any],
                                    table: Dict[str, List[str]]) -> None:
    for metric_key, metric in metrics.items():
      parts = metric_key.split("/")
      if len(parts) != 2 or parts[-1] != "total":
        continue
      table[metric_key].append(
          helper.format_metric(metric["average"], metric["stddev"]))
      # Separate runs don't produce a score
    if "score" in metrics:
      metric = metrics["score"]
      table["Score"].append(
          helper.format_metric(metric["average"], metric["stddev"]))


class Speedometer20Probe(Speedometer2Probe):
  __doc__ = Speedometer2Probe.__doc__
  NAME: Final[str] = "speedometer_2.0"


class Speedometer21Probe(Speedometer2Probe):
  __doc__ = Speedometer2Probe.__doc__
  NAME: Final[str] = "speedometer_2.1"


class Speedometer2Story(PressBenchmarkStory, metaclass=abc.ABCMeta):
  URL_LOCAL: Final[str] = "http://localhost:8000/InteractiveRunner.html"
  SUBSTORIES = (
      "VanillaJS-TodoMVC",
      "Vanilla-ES2015-TodoMVC",
      "Vanilla-ES2015-Babel-Webpack-TodoMVC",
      "React-TodoMVC",
      "React-Redux-TodoMVC",
      "EmberJS-TodoMVC",
      "EmberJS-Debug-TodoMVC",
      "BackboneJS-TodoMVC",
      "AngularJS-TodoMVC",
      "Angular2-TypeScript-TodoMVC",
      "VueJS-TodoMVC",
      "jQuery-TodoMVC",
      "Preact-TodoMVC",
      "Inferno-TodoMVC",
      "Elm-TodoMVC",
      "Flight-TodoMVC",
  )

  def __init__(self,
               substories: Sequence[str] = (),
               iterations: int = 10,
               url: Optional[str] = None):
    self.iterations = iterations or 10
    super().__init__(url=url, substories=substories)

  @property
  def substory_duration(self) -> float:
    return self.iterations * 0.4

  def run(self, run: Run) -> None:
    with run.actions("Setup") as actions:
      actions.navigate_to(self._url)
      actions.wait_js_condition(
          """
        return window.Suites !== undefined;
      """, 0.5, 10)
      if self._substories != self.SUBSTORIES:
        actions.js(
            """
        let substories = arguments[0];
        Suites.forEach((suite) => {
          suite.disabled = substories.indexOf(suite.name) == -1;
        });
        """,
            arguments=[self._substories])
      actions.wait(0.5)
    with run.actions("Running") as actions:
      actions.js(
          """
        // Store all the results in the benchmarkClient
        window.testDone = false;
        window.suiteValues = [];
        const benchmarkClient = {
          didRunSuites(measuredValues) {
            window.suiteValues.push(measuredValues);
          },
          didFinishLastIteration() {
            window.testDone = true;
          }
        };
        const runner = new BenchmarkRunner(Suites, benchmarkClient);
        const iterationCount = arguments[0];
        runner.runMultipleIterations(iterationCount);
        """,
          arguments=[self.iterations])
      actions.wait(self.fast_duration)
    with run.actions("Waiting for completion") as actions:
      actions.wait_js_condition("return window.testDone",
                                self.substory_duration, self.slow_duration)


ProbeClsTupleT = Tuple[Type[Speedometer2Probe], ...]


class Speedometer20Story(Speedometer2Story):
  __doc__ = Speedometer2Story.__doc__
  NAME: Final[str] = "speedometer_2.0"
  PROBES: Final[ProbeClsTupleT] = (Speedometer20Probe,)
  URL: Final[str] = ("https://browserbench.org/Speedometer2.0"
                     "/InteractiveRunner.html")


class Speedometer21Story(Speedometer2Story):
  __doc__ = Speedometer2Story.__doc__
  NAME: Final[str] = "speedometer_2.1"
  PROBES: Final[ProbeClsTupleT] = (Speedometer21Probe,)
  URL: Final[str] = ("https://browserbench.org/Speedometer2.1/"
                     "InteractiveRunner.html")


class Speedometer2Benchmark(PressBenchmark, metaclass=abc.ABCMeta):

  DEFAULT_STORY_CLS = Speedometer2Story

  @classmethod
  def add_cli_parser(cls, subparsers,
                     aliases: Sequence[str] = ()) -> argparse.ArgumentParser:
    parser = super().add_cli_parser(subparsers, aliases)
    parser.add_argument(
        "--iterations",
        default=10,
        type=int,
        help="Number of iterations each Speedometer subtest is run "
        "within the same session. \n"
        "Note: --repeat restarts the whole benchmark, --iterations runs the"
        "same test tests n-times within the same session without the setup "
        "overhead of starting up a whole new browser.")
    return parser

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    kwargs = super().kwargs_from_cli(args)
    kwargs["iterations"] = int(args.iterations)
    return kwargs

  def __init__(self,
               stories: Optional[Sequence[Speedometer2Story]] = None,
               iterations: Optional[int] = None,
               custom_url: Optional[str] = None):
    if stories is None:
      stories = self.DEFAULT_STORY_CLS.default()
    for story in stories:
      assert isinstance(story, self.DEFAULT_STORY_CLS)
      if iterations is not None:
        assert iterations >= 1
        story.iterations = iterations
    super().__init__(stories, custom_url=custom_url)


class Speedometer20Benchmark(Speedometer2Benchmark):
  """
  Benchmark runner for Speedometer 2.0
  """
  NAME: Final[str] = "speedometer_2.0"
  DEFAULT_STORY_CLS = Speedometer20Story


class Speedometer21Benchmark(Speedometer2Benchmark):
  """
  Benchmark runner for Speedometer 2.1
  """
  NAME: Final[str] = "speedometer_2.1"
  DEFAULT_STORY_CLS = Speedometer21Story
