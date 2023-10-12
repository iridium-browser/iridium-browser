# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import abc
import copy
import csv
from typing import Optional, Type
from unittest import mock

from crossbench.benchmarks.jetstream.jetstream_2 import (JetStream2Benchmark,
                                                         JetStream2Probe,
                                                         JetStream2Story)
from crossbench.env import (HostEnvironment, HostEnvironmentConfig,
                            ValidationMode)
from crossbench.runner.runner import Runner
from tests.crossbench.benchmarks import helper


class JetStream2BaseTestCase(
    helper.PressBaseBenchmarkTestCase, metaclass=abc.ABCMeta):

  @property
  @abc.abstractmethod
  def benchmark_cls(self) -> Type[JetStream2Benchmark]:
    pass

  @property
  @abc.abstractmethod
  def story_cls(self) -> Type[JetStream2Story]:
    pass

  @property
  @abc.abstractmethod
  def probe_cls(self) -> Type[JetStream2Probe]:
    pass

  def test_run_throw(self):
    self._test_run(throw=True)

  def test_run_default(self):
    self._test_run()
    for browser in self.browsers:
      urls = self.filter_data_urls(browser.url_list)
      self.assertIn(self.story_cls.URL, urls)
      self.assertNotIn(self.story_cls.URL_LOCAL, urls)

  def test_run_custom_url(self):
    custom_url = "http://test.example.com/jetstream"
    self._test_run(custom_url)
    for browser in self.browsers:
      urls = self.filter_data_urls(browser.url_list)
      self.assertIn(custom_url, urls)
      self.assertNotIn(self.story_cls.URL, urls)
      self.assertNotIn(self.story_cls.URL_LOCAL, urls)

  def _test_run(self, custom_url: Optional[str] = None, throw: bool = False):
    repetitions = 3
    stories = self.story_cls.from_names(["WSL"], url=custom_url)
    example_story_data = {
        "firstIteration": 1,
        "average": 0.1,
        "worst4": 1.1,
        "score": 1
    }
    # The order should match Runner.get_runs
    for _ in range(repetitions):
      for _ in stories:
        jetstream_probe_results = {
            story.name: example_story_data for story in stories
        }
        js_side_effects = [
            True,  # Page is ready
            None,  # filter benchmarks
            True,  # UI is updated and ready,
            None,  # Start running benchmark
            True,  # Wait until done
            jetstream_probe_results,
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
    with mock.patch.object(
        HostEnvironment, "validate_url", return_value=True) as cm:
      runner.run()
    cm.assert_called_once()
    for browser in self.browsers:
      urls = self.filter_data_urls(browser.url_list)
      self.assertEqual(len(urls), repetitions)
      self.assertIn(self.probe_cls.JS, browser.js_list)

    csv_file = self.out_dir / f"{self.probe_cls.NAME}.csv"
    with csv_file.open(encoding="utf-8") as f:
      csv_data = list(csv.DictReader(f, delimiter="\t"))
    self.assertListEqual(list(csv_data[0].keys()), ["label", "dev", "stable"])
    self.assertDictEqual(csv_data[1], {
        "label": "version",
        "dev": "102.22.33.44",
        "stable": "100.22.33.44",
    })

    with self.assertLogs(level='INFO') as cm:
      for probe in runner.probes:
        for run in runner.runs:
          probe.log_run_result(run)
    output = "\n".join(cm.output)
    self.assertIn("JetStream results", output)

    with self.assertLogs(level='INFO') as cm:
      for probe in runner.probes:
        probe.log_browsers_result(runner.browser_group)
    output = "\n".join(cm.output)
    self.assertIn("JetStream results", output)
    self.assertIn("102.22.33.44", output)
    self.assertIn("100.22.33.44", output)
