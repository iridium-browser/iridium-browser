# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import copy
import csv
from typing import Optional
from unittest import mock


from crossbench.benchmarks.motionmark.motionmark_1_2 import (
    MotionMark12Benchmark, MotionMark12Probe, MotionMark12Story)
from crossbench.env import (HostEnvironment, HostEnvironmentConfig,
                            ValidationMode)
from crossbench.runner.runner import Runner
from tests.crossbench.benchmarks import helper
from tests import run_helper


class MotionMark2Test(helper.PressBaseBenchmarkTestCase):

  @property
  def benchmark_cls(self):
    return MotionMark12Benchmark

  EXAMPLE_PROBE_DATA = [{
      "testsResults": {
          "MotionMark": {
              "Multiply": {
                  "complexity": {
                      "complexity":
                          1169.7666313745012,
                      "stdev":
                          2.6693101402239985,
                      "bootstrap": {
                          "confidenceLow": 1154.0859381321234,
                          "confidenceHigh": 1210.464520355893,
                          "median": 1180.8987652049277,
                          "mean": 1163.0061487765158,
                          "confidencePercentage": 0.8
                      },
                      "segment1": [[1, 16.666666666666668],
                                   [1, 16.666666666666668]],
                      "segment2": [[1, 6.728874992470971],
                                   [3105, 13.858528114770454]]
                  },
                  "controller": {
                      "score": 1168.106104032434,
                      "average": 1168.106104032434,
                      "stdev": 37.027504395081785,
                      "percent": 3.1698750881669624
                  },
                  "score": 1180.8987652049277,
                  "scoreLowerBound": 1154.0859381321234,
                  "scoreUpperBound": 1210.464520355893
              }
          }
      },
      "score": 1180.8987652049277,
      "scoreLowerBound": 1154.0859381321234,
      "scoreUpperBound": 1210.464520355893
  }]

  def test_all_stories(self):
    stories = self.story_filter(["all"], separate=True).stories
    self.assertGreater(len(stories), 1)
    for story in stories:
      self.assertIsInstance(story, MotionMark12Story)
    names = set(story.name for story in stories)
    self.assertEqual(len(names), len(stories))
    self.assertEqual(len(names), len(MotionMark12Story.SUBSTORIES))

  def test_default_stories(self):
    stories = self.story_filter(["default"], separate=True).stories
    self.assertGreater(len(stories), 1)
    for story in stories:
      self.assertIsInstance(story, MotionMark12Story)
    names = set(story.name for story in stories)
    self.assertEqual(len(names), len(stories))
    self.assertEqual(
        len(names), len(MotionMark12Story.ALL_STORIES["MotionMark"]))

  def test_run_throw(self):
    self._test_run(throw=True)

  def test_run_default(self):
    self._test_run()
    for browser in self.browsers:
      urls = self.filter_data_urls(browser.url_list)
      self.assertIn(self.story_cls.URL, urls)
      self.assertNotIn(self.story_cls.URL_LOCAL, urls)

  def test_run_custom_url(self):
    custom_url = "http://test.example.com/speedometer"
    self._test_run(custom_url)
    for browser in self.browsers:
      urls = self.filter_data_urls(browser.url_list)
      self.assertIn(custom_url, urls)
      self.assertNotIn(self.story_cls.URL, urls)
      self.assertNotIn(self.story_cls.URL_LOCAL, urls)

  def _test_run(self, custom_url: Optional[str] = None, throw: bool = False):
    stories = MotionMark12Story.from_names(['Multiply'], url=custom_url)
    repetitions = 3
    # The order should match Runner.get_runs
    for _ in range(repetitions):
      for _ in stories:
        js_side_effects = [
            True,  # Page is ready
            1,  # NOF enabled benchmarks
            None,  # Start running benchmark
            True,  # Wait until done
            self.EXAMPLE_PROBE_DATA
        ]
        for browser in self.browsers:
          browser.js_side_effects += js_side_effects
    for browser in self.browsers:
      browser.js_side_effect = copy.deepcopy(browser.js_side_effects)
    benchmark = self.benchmark_cls(stories, custom_url=custom_url)
    self.assertTrue(len(benchmark.describe()) > 0)
    runner = Runner(
        self.out_dir,
        self.browsers,
        benchmark,
        env_config=HostEnvironmentConfig(),
        env_validation_mode=ValidationMode.SKIP,
        platform=self.platform,
        repetitions=repetitions,
        throw=throw)
    with mock.patch.object(
        HostEnvironment, "validate_url", return_value=True) as cm:
      runner.run()
    cm.assert_called_once()
    assert runner.is_success
    for browser in self.browsers:
      urls = self.filter_data_urls(browser.url_list)
      self.assertEqual(len(urls), repetitions)
      self.assertIn(MotionMark12Probe.JS, browser.js_list)
    with (self.out_dir /
          f"{MotionMark12Probe.NAME}.csv").open(encoding="utf-8") as f:
      csv_data = list(csv.DictReader(f, delimiter="\t"))
    self.assertListEqual(list(csv_data[0].keys()), ["label", "dev", "stable"])
    self.assertDictEqual(csv_data[1], {
        "label": "version",
        "dev": "102.22.33.44",
        "stable": "100.22.33.44",
    })


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
