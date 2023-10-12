# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import abc
import argparse
import copy
import csv
import types
from dataclasses import dataclass
from typing import Dict, List, Optional, Sequence, Type
from unittest import mock

from crossbench.benchmarks.speedometer.speedometer import (SpeedometerBenchmark,
                                                           SpeedometerProbe,
                                                           SpeedometerStory)
from crossbench.env import (HostEnvironment, HostEnvironmentConfig,
                            ValidationMode)
from crossbench.runner.runner import Runner
from tests.crossbench.benchmarks import helper


class SpeedometerBaseTestCase(
    helper.PressBaseBenchmarkTestCase, metaclass=abc.ABCMeta):

  @property
  @abc.abstractmethod
  def benchmark_cls(self) -> Type[SpeedometerBenchmark]:
    pass

  @property
  @abc.abstractmethod
  def story_cls(self) -> Type[SpeedometerStory]:
    pass

  @property
  @abc.abstractmethod
  def probe_cls(self) -> Type[SpeedometerProbe]:
    pass

  @property
  @abc.abstractmethod
  def name(self) -> str:
    pass

  @dataclass
  class Namespace(argparse.Namespace):
    stories = "all"
    iterations = 10
    separate: bool = False
    custom_benchmark_url: Optional[str] = None

  def test_default_all(self):
    default_story_names = [
        story.name for story in self.story_cls.default(separate=True)
    ]
    all_story_names = [
        story.name for story in self.story_cls.all(separate=True)
    ]
    self.assertListEqual(default_story_names, all_story_names)

  def test_iterations_kwargs(self):
    args = self.Namespace()
    self.benchmark_cls.from_cli_args(args)
    with self.assertRaises(TypeError):
      args.iterations = "-10"
      self.benchmark_cls.from_cli_args(args)
    with self.assertRaises(TypeError):
      args.iterations = "1234"
      benchmark = self.benchmark_cls.from_cli_args(args)
    args.iterations = 1234
    benchmark = self.benchmark_cls.from_cli_args(args)
    for story in benchmark.stories:
      assert isinstance(story, self.story_cls)
      self.assertEqual(story.iterations, 1234)

  def test_story_filtering_cli_args_all_separate(self):
    stories = self.story_cls.default(separate=True)
    args = self.Namespace()
    args.stories = "all"
    args.separate = True
    stories_all = self.benchmark_cls.stories_from_cli_args(args)
    self.assertListEqual(
        [story.name for story in stories],
        [story.name for story in stories_all],
    )

  def test_story_filtering_cli_args_all(self):
    stories = self.story_cls.default(separate=False)
    args = self.Namespace()
    args.stories = "all"
    args.custom_benchmark_url = self.story_cls.URL_LOCAL
    args.separate = False
    args.iterations = 503
    stories_all = self.benchmark_cls.stories_from_cli_args(args)
    self.assertEqual(len(stories), 1)
    self.assertEqual(len(stories_all), 1)
    story = stories[0]
    assert isinstance(story, self.story_cls)
    self.assertEqual(story.name, self.name)
    story = stories_all[0]
    assert isinstance(story, self.story_cls)
    self.assertEqual(story.name, self.name)
    self.assertEqual(story.url, self.story_cls.URL_LOCAL)
    self.assertEqual(story.iterations, 503)

    args.custom_benchmark_url = None
    args.separate = False
    args.iterations = 701
    stories_all = self.benchmark_cls.stories_from_cli_args(args)
    self.assertEqual(len(stories_all), 1)
    story = stories_all[0]
    assert isinstance(story, self.story_cls)
    self.assertEqual(story.name, self.name)
    self.assertEqual(story.url, self.story_cls.URL)
    self.assertEqual(story.iterations, 701)

  def test_story_filtering(self):
    with self.assertRaises(ValueError):
      self.story_cls.from_names([])
    stories = self.story_cls.default(separate=False)
    self.assertEqual(len(stories), 1)

    with self.assertRaises(ValueError):
      self.story_cls.from_names([], separate=True)
    stories = self.story_cls.default(separate=True)
    self.assertEqual(len(stories), len(self.story_cls.SUBSTORIES))

  def test_story_filtering_regexp_invalid(self):
    with self.assertRaises(ValueError):
      _ = self.story_filter(  # pytype: disable=wrong-arg-types
          ".*", separate=True).stories

  def test_story_filtering_regexp(self):
    stories = self.story_cls.default(separate=True)
    stories_b = self.story_filter([".*"], separate=True).stories
    self.assertListEqual(
        [story.name for story in stories],
        [story.name for story in stories_b],
    )


  EXAMPLE_STORY_DATA = types.MappingProxyType({
      "tests": {
          "Adding100Items": {
              "tests": {
                  "Sync": 74.6000000089407,
                  "Async": 6.299999997019768
              },
              "total": 80.90000000596046
          },
          "CompletingAllItems": {
              "tests": {
                  "Sync": 22.600000008940697,
                  "Async": 5.899999991059303
              },
              "total": 28.5
          },
          "DeletingItems": {
              "tests": {
                  "Sync": 11.800000011920929,
                  "Async": 0.19999998807907104
              },
              "total": 12
          }
      },
      "total": 121.40000000596046
  })

  def _test_run(self,
                story_names: Optional[Sequence[str]] = None,
                separate: bool = False,
                custom_url: Optional[str] = None,
                throw: bool = True) -> Runner:
    repetitions = 3
    iterations = 2
    if story_names is None:
      default_story_name = self.story_cls.SUBSTORIES[0]
      self.assertTrue(default_story_name)
      story_names = [default_story_name]
    stories = self.story_cls.from_names(
        story_names, separate=separate, url=custom_url)

    # The order should match Runner.get_runs
    for _ in range(repetitions):
      for story in stories:
        speedometer_probe_results = [{
            "tests": {
                substory_name: dict(self.EXAMPLE_STORY_DATA)
                for substory_name in story.substories
            },
            "total": 1000,
            "mean": 2000,
            "geomean": 3000,
            "score": 10
        }
                                     for _ in range(iterations)]
        js_side_effects = [
            True,  # Page is ready
            None,  # _setup_substories
            None,  # _setup_benchmark_client
            None,  # _run_stories
            True,  # Wait until done
            speedometer_probe_results,
        ]
        for browser in self.browsers:
          browser.js_side_effects += js_side_effects
    for browser in self.browsers:
      browser.js_side_effect = copy.deepcopy(browser.js_side_effects)

    benchmark = self.benchmark_cls(stories, custom_url=custom_url)  # pytype: disable=not-instantiable
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
    with mock.patch.object(self.benchmark_cls, "validate_url") as cm:
      runner.run()
    cm.assert_called_once()
    return runner

  def _verify_results(
      self,
      runner: Runner,
      expected_num_urls: Optional[int] = None) -> List[Dict[str, str]]:
    for browser in self.browsers:
      urls = self.filter_data_urls(browser.url_list)
      if expected_num_urls is not None:
        self.assertEqual(len(urls), expected_num_urls)
      self.assertIn(self.probe_cls.JS, browser.js_list)
      self.assertListEqual(browser.js_side_effects, [])

    with self.assertLogs(level='INFO') as cm:
      for probe in runner.probes:
        for run in runner.runs:
          probe.log_run_result(run)
    output = "\n".join(cm.output)
    self.assertIn("Speedometer results", output)

    with self.assertLogs(level='INFO') as cm:
      for probe in runner.probes:
        probe.log_browsers_result(runner.browser_group)
    output = "\n".join(cm.output)
    self.assertIn("Speedometer results", output)
    self.assertIn("102.22.33.44", output)
    self.assertIn("100.22.33.44", output)

    csv_files = list(runner.out_dir.glob("speedometer*.csv"))
    self.assertEqual(len(csv_files), 1)
    csv_file = self.out_dir / f"{self.probe_cls.NAME}.csv"
    rows: List[Dict[str, str]] = [{}]
    with csv_file.open(encoding="utf-8") as f:
      reader = csv.DictReader(f, delimiter="\t")
      rows = list(reader)
    self.assertListEqual(list(rows[0].keys()), ["label", "dev", "stable"])
    self.assertDictEqual(rows[1], {
        "label": "version",
        "dev": "102.22.33.44",
        "stable": "100.22.33.44",
    })
    return rows

  def test_run_throw(self):
    runner = self._test_run(throw=True)
    self._verify_results(runner)

  def test_run_default(self):
    runner = self._test_run()
    self._verify_results(runner)
    for browser in self.browsers:
      urls = self.filter_data_urls(browser.url_list)
      self.assertIn(f"{self.story_cls.URL}?iterationCount=10", urls)
      self.assertNotIn(f"{self.story_cls.URL_LOCAL}?iterationCount=10", urls)

  def test_run_custom_url(self):
    custom_url = "http://test.example.com/speedometer"
    runner = self._test_run(custom_url=custom_url)
    self._verify_results(runner)
    for browser in self.browsers:
      urls = self.filter_data_urls(browser.url_list)
      self.assertIn(f"{custom_url}?iterationCount=10", urls)
      self.assertNotIn(f"{self.story_cls.URL}?iterationCount=10", urls)
      self.assertNotIn(f"{self.story_cls.URL_LOCAL}?iterationCount=10", urls)

  def _verify_results_stories(self, rows, story_names):
    labels = [row['label'] for row in rows]
    self.assertNotIn(f"{self.benchmark_cls.NAME}_{'_'.join(story_names)}",
                     labels)
    for story_name in story_names:
      self.assertIn(story_name, labels)

  def _run_combined(self, story_names: Sequence[str]):
    runner = self._test_run(story_names=story_names, separate=False)
    rows = self._verify_results(runner, expected_num_urls=3)
    self._verify_results_stories(rows, story_names)

  def _run_separate(self, story_names: Sequence[str]):
    runner = self._test_run(story_names=story_names, separate=True)
    rows = self._verify_results(runner, expected_num_urls=6)
    self._verify_results_stories(rows, story_names)

  def test_run_combined(self):
    self._run_combined(["VanillaJS-TodoMVC", "Elm-TodoMVC"])

  def test_run_separate(self):
    self._run_separate(["VanillaJS-TodoMVC", "Elm-TodoMVC"])
