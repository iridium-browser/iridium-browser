# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Base class for the CBB Adapter unit tests
"""

from abc import ABCMeta, abstractproperty, abstractmethod
import unittest
import pathlib
import shutil
import tempfile
import json

from crossbench.browsers import chrome

from crossbench.cbb import cbb_adapter


class CbbTest(unittest.TestCase, metaclass=ABCMeta):

  __test__ = False

  @abstractproperty
  def benchmark_name(self) -> str:
    """Each child class benchmark test will need to define it's corresponding benchmark_name"""

  def setUp(self):
    self.output_dir = str(
        pathlib.Path(tempfile.gettempdir()).joinpath(self.benchmark_name))

  def tearDown(self):
    shutil.rmtree(self.output_dir)

  def get_benchmark(self):
    """Returns a benchmark instance for the corresponding benchmark_name"""
    story_class = cbb_adapter.get_pressbenchmark_story_cls(self.benchmark_name)
    if not story_class:
      return None

    stories = story_class.default_story_names()
    workload = story_class(substories=stories)

    benchmark_cls = cbb_adapter.get_pressbenchmark_cls(self.benchmark_name)
    benchmark = benchmark_cls(stories=[workload])
    return benchmark

  def test_benchmark_chrome(self):
    """Tests that we can execute the specified benchmark and obtain result data post execution.
    This test uses Chrome browser to run the benchmarks.
    """
    benchmark = self.get_benchmark()
    chrome_path = chrome.Chrome.default_path()
    browser = chrome.ChromeWebDriver("Chrome", chrome_path)

    benchmark_output = cbb_adapter.get_probe_result_file(
        self.benchmark_name, browser, self.output_dir)
    cbb_adapter.run_benchmark(
        output_folder=self.output_dir,
        browser_list=[browser],
        benchmark=benchmark)

    self.assertIsNotNone(benchmark_output)
    self.assertTrue(pathlib.Path(benchmark_output).exists())

    with open(benchmark_output) as f:
      benchmark_data = json.load(f)

    self.assertIsNotNone(benchmark_data)
