# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
import itertools
from typing import TYPE_CHECKING, List, Optional, Tuple

import crossbench.probes.helper as probes_helper
from crossbench.benchmarks.benchmark import PressBenchmark
from crossbench.probes import metric
from crossbench.probes.json import JsonResultProbe
from crossbench.probes.results import ProbeResult
from crossbench.stories.press_benchmark import PressBenchmarkStory

if TYPE_CHECKING:
  from crossbench.runner.actions import Actions
  from crossbench.runner.groups import BrowsersRunGroup, StoriesRunGroup
  from crossbench.runner.run import Run
  from crossbench.types import JSON


def _probe_skip_data_segments(path: Tuple[str, ...]) -> Optional[str]:
  name = path[-1]
  if name.startswith("segment") or name == "data":
    return None
  return "/".join(path)


class MotionMark12Probe(JsonResultProbe):
  """
  MotionMark-specific Probe.
  Extracts all MotionMark times and scores.
  """
  NAME = "motionmark_1.2"
  IS_GENERAL_PURPOSE = False
  JS = """
    return window.benchmarkRunnerClient.results.results;
  """

  def to_json(self, actions: Actions) -> JSON:
    return actions.js(self.JS)

  def flatten_json_data(self, json_data: List) -> JSON:
    assert isinstance(json_data, list) and len(json_data) == 1, (
        "Motion12MarkProbe requires a results list.")
    return probes_helper.Flatten(
        json_data[0], key_fn=_probe_skip_data_segments).data

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    merged = metric.MetricsMerger.merge_json_list(
        story_group.results[self].json
        for story_group in group.repetitions_groups)
    return self.write_group_result(group, merged, write_csv=True)

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    return self.merge_browsers_csv_list(group)


class MotionMark12Story(PressBenchmarkStory):
  NAME = "motionmark_1.2"
  PROBES = (MotionMark12Probe,)
  URL = "https://browserbench.org/MotionMark1.2/developer.html"
  URL_LOCAL = "http://localhost:8000/developer.html"
  ALL_STORIES = {
      "MotionMark": (
          "Multiply",
          "Canvas Arcs",
          "Leaves",
          "Paths",
          "Canvas Lines",
          "Images",
          "Design",
          "Suits",
      ),
      "HTML suite": (
          "CSS bouncing circles",
          "CSS bouncing clipped rects",
          "CSS bouncing gradient circles",
          "CSS bouncing blend circles",
          "CSS bouncing filter circles",
          # "CSS bouncing SVG images",
          "CSS bouncing tagged images",
          "Focus 2.0",
          "DOM particles, SVG masks",
          # "Composited Transforms",
      ),
      "Canvas suite": (
          "canvas bouncing clipped rects",
          "canvas bouncing gradient circles",
          # "canvas bouncing SVG images",
          # "canvas bouncing PNG images",
          "Stroke shapes",
          "Fill shapes",
          "Canvas put/get image data",
      ),
      "SVG suite": (
          "SVG bouncing circles",
          "SVG bouncing clipped rects",
          "SVG bouncing gradient circles",
          # "SVG bouncing SVG images",
          # "SVG bouncing PNG images",
      ),
      "Leaves suite": (
          "Translate-only Leaves",
          "Translate + Scale Leaves",
          "Translate + Opacity Leaves",
      ),
      "Multiply suite": (
          "Multiply: CSS opacity only",
          "Multiply: CSS display only",
          "Multiply: CSS visibility only",
      ),
      "Text suite": (
          "Design: Latin only (12 items)",
          "Design: CJK only (12 items)",
          "Design: RTL and complex scripts only (12 items)",
          "Design: Latin only (6 items)",
          "Design: CJK only (6 items)",
          "Design: RTL and complex scripts only (6 items)",
      ),
      "Suits suite": (
          "Suits: clip only",
          "Suits: shape only",
          "Suits: clip, shape, rotation",
          "Suits: clip, shape, gradient",
          "Suits: static",
      ),
      "3D Graphics": (
          "Triangles (WebGL)",
          # "Triangles (WebGPU)",
      ),
      "Basic canvas path suite": (
          "Canvas line segments, butt caps",
          "Canvas line segments, round caps",
          "Canvas line segments, square caps",
          "Canvas line path, bevel join",
          "Canvas line path, round join",
          "Canvas line path, miter join",
          "Canvas line path with dash pattern",
          "Canvas quadratic segments",
          "Canvas quadratic path",
          "Canvas bezier segments",
          "Canvas bezier path",
          "Canvas arcTo segments",
          "Canvas arc segments",
          "Canvas rects",
          "Canvas ellipses",
          "Canvas line path, fill",
          "Canvas quadratic path, fill",
          "Canvas bezier path, fill",
          "Canvas arcTo segments, fill",
          "Canvas arc segments, fill",
          "Canvas rects, fill",
          "Canvas ellipses, fill",
      )
  }
  SUBSTORIES = tuple(itertools.chain.from_iterable(ALL_STORIES.values()))

  @classmethod
  def default_story_names(cls) -> Tuple[str, ...]:
    return cls.ALL_STORIES["MotionMark"]

  @property
  def substory_duration(self) -> dt.timedelta:
    return dt.timedelta(seconds=35)

  def setup(self, run: Run) -> None:
    with run.actions("Setup") as actions:
      actions.show_url(self._url)
      actions.wait_js_condition(
          """return document.querySelector("tree > li") !== undefined""", 0.1,
          10)
      num_enabled = actions.js(
          """
        let benchmarks = arguments[0];
        const list = document.querySelectorAll(".tree li");
        let counter = 0;
        for (const row of list) {
          const name = row.querySelector("label.tree-label").textContent.trim();
          let checked = benchmarks.includes(name);
          const labels = row.querySelectorAll("input[type=checkbox]");
          for (const label of labels) {
            if (checked) {
              label.click()
              counter++;
            }
          }
        }
        return counter
        """,
          arguments=[self._substories])
      assert num_enabled > 0, "No tests were enabled"
      actions.wait(0.1)

  def run(self, run: Run) -> None:
    with run.actions("Running") as actions:
      actions.js("window.benchmarkController.startBenchmark()")
      actions.wait(self.fast_duration)
    with run.actions("Waiting for completion") as actions:
      actions.wait_js_condition(
          """
          return window.benchmarkRunnerClient.results._results != undefined
          """, self.substory_duration / 4, self.slow_duration)


class MotionMarkBenchmark(PressBenchmark):

  @classmethod
  def short_base_name(cls) -> str:
    return "mm"

  @classmethod
  def base_name(cls) -> str:
    return "motionmark"


class MotionMark12Benchmark(MotionMarkBenchmark):
  """
  Benchmark runner for MotionMark 1.2.

  See https://browserbench.org/MotionMark1.2/ for more details.
  """

  NAME = "motionmark_1.2"
  DEFAULT_STORY_CLS = MotionMark12Story

  @classmethod
  def version(cls) -> Tuple[int, ...]:
    return (1, 2)
