# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import abc
import argparse
import csv
from dataclasses import dataclass
from typing import Optional, Sequence, Type
from unittest import mock

from crossbench.benchmarks.speedometer import (Speedometer2Benchmark,
                                               Speedometer2Probe,
                                               Speedometer2Story)
from crossbench.env import (HostEnvironment, HostEnvironmentConfig,
                            ValidationMode)
from crossbench.runner import Runner
from tests.benchmarks import helper


class Speedometer2BaseTestCase(
    helper.PressBaseBenchmarkTestCase, metaclass=abc.ABCMeta):

  @property
  @abc.abstractmethod
  def benchmark_cls(self) -> Type[Speedometer2Benchmark]:
    pass

  @property
  @abc.abstractmethod
  def story_cls(self) -> Type[Speedometer2Story]:
    pass

  @property
  @abc.abstractmethod
  def probe_cls(self) -> Type[Speedometer2Probe]:
    pass

  @property
  @abc.abstractmethod
  def name(self) -> str:
    pass

  def test_iterations(self):
    with self.assertRaises(AssertionError):
      self.benchmark_cls(iterations=-1)
    benchmark = self.benchmark_cls(iterations=123)
    for story in benchmark.stories:
      assert isinstance(story, self.story_cls)
      self.assertEqual(story.iterations, 123)

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
    with self.assertRaises(AssertionError):
      args.iterations = "-10"
      self.benchmark_cls.from_cli_args(args)
    args.iterations = "1234"
    benchmark = self.benchmark_cls.from_cli_args(args)
    for story in benchmark.stories:
      assert isinstance(story, self.story_cls)
      self.assertEqual(story.iterations, 1234)

  def test_story_filtering_cli_args_all_separate(self):
    stories = self.story_cls.default(separate=True)
    args = mock.Mock()
    args.stories = "all"
    args.separate = True
    stories_all = self.benchmark_cls.stories_from_cli_args(args)
    self.assertListEqual(
        [story.name for story in stories],
        [story.name for story in stories_all],
    )

  def test_story_filtering_cli_args_all(self):
    stories = self.story_cls.default(separate=False)
    args = mock.Mock()
    args.stories = "all"
    args.custom_benchmark_url = self.story_cls.URL_LOCAL
    args.separate = False
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

    args.custom_benchmark_url = None
    args.separate = False
    stories_all = self.benchmark_cls.stories_from_cli_args(args)
    self.assertEqual(len(stories_all), 1)
    story = stories_all[0]
    assert isinstance(story, self.story_cls)
    self.assertEqual(story.name, self.name)
    self.assertEqual(story.url, self.story_cls.URL)

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
    repetitions = 3
    iterations = 2
    stories = self.story_cls.from_names(["VanillaJS-TodoMVC"], url=custom_url)
    example_story_data = {
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
    }
    speedometer_probe_results = [{
        "tests": {story.name: example_story_data for story in stories},
        "total": 1000,
        "mean": 2000,
        "geomean": 3000,
        "score": 10
    } for i in range(iterations)]

    for browser in self.browsers:
      browser.js_side_effect = [
          True,  # Page is ready
          None,  # filter benchmarks
          None,  # Start running benchmark
          True,  # Wait until done
          speedometer_probe_results,
      ]
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
    for browser in self.browsers:
      urls = self.filter_data_urls(browser.url_list)
      self.assertEqual(len(urls), repetitions)
      self.assertIn(Speedometer2Probe.JS, browser.js_list)

    with (self.out_dir /
          f"{self.probe_cls.NAME}.csv").open(encoding="utf-8") as f:
      csv_data = list(csv.DictReader(f, delimiter="\t"))
    self.assertListEqual(list(csv_data[0].keys()), ["label", "dev", "stable"])
    self.assertDictEqual(csv_data[1], {
        'label': 'version',
        'dev': '102.22.33.44',
        'stable': '100.22.33.44'
    })

  def _run_story_names(self, story_names: Sequence[str], separate: bool,
                       expected_num_urls: int):
    repetitions = 3
    iterations = 2
    stories = self.story_cls.from_names(story_names, separate=separate)
    example_story_data = {
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
    }
    speedometer_probe_results = [{
        "tests": {story.name: example_story_data for story in stories},
        "total": 1000,
        "mean": 2000,
        "geomean": 3000,
        "score": 10
    } for i in range(iterations)]

    for browser in self.browsers:
      browser.js_side_effect = [
          True,  # Page is ready
          None,  # filter benchmarks
          None,  # Start running benchmark
          True,  # Wait until done
          speedometer_probe_results,
      ]
    benchmark = self.benchmark_cls(stories)
    self.assertTrue(len(benchmark.describe()) > 0)
    runner = Runner(
        self.out_dir,
        self.browsers,
        benchmark,
        env_config=HostEnvironmentConfig(),
        env_validation_mode=ValidationMode.SKIP,
        platform=self.platform,
        repetitions=repetitions)
    with mock.patch.object(self.benchmark_cls, "validate_url") as cm:
      runner.run()
    cm.assert_called_once()

    for browser in self.browsers:
      urls = self.filter_data_urls(browser.url_list)
      self.assertEqual(len(urls), expected_num_urls)
      self.assertIn(self.probe_cls.JS, browser.js_list)

    with (self.out_dir /
          f"{self.probe_cls.NAME}.csv").open(encoding="utf-8") as f:
      csv_data = list(csv.DictReader(f, delimiter="\t"))
    self.assertListEqual(list(csv_data[0].keys()), ["label", "dev", "stable"])
    self.assertDictEqual(csv_data[1], {
        'label': 'version',
        'dev': '102.22.33.44',
        'stable': '100.22.33.44'
    })
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

  def test_run_combined(self):
    self._run_story_names(["VanillaJS-TodoMVC", "Elm-TodoMVC"],
                          separate=False,
                          expected_num_urls=3)

  def test_run_separate(self):
    self._run_story_names(["VanillaJS-TodoMVC", "Elm-TodoMVC"],
                          separate=True,
                          expected_num_urls=6)
