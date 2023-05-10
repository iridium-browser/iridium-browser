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
import argparse

from crossbench.browsers import chrome
from crossbench.cbb import cbb_adapter


class CbbTest(unittest.TestCase, metaclass=ABCMeta):

  __test__ = False

  @abstractproperty
  def benchmark_name(self) -> str:
    """Each child class benchmark test will need to define it's corresponding benchmark_name"""

  def setUp(self):
    parser = argparse.ArgumentParser()
    parser.add_argument('--browserpath', default=None)
    parser.add_argument('--driverpath', default=None)
    args = parser.parse_args()
    self.output_dir = str(
        pathlib.Path(tempfile.gettempdir()).joinpath(self.benchmark_name))

    self._browser_path = pathlib.Path(
        args.browserpath) if args.browserpath else chrome.Chrome.default_path()
    assert self._browser_path.exists()
    if args.driverpath:
      self._driver_path = pathlib.Path(args.driverpath)
      assert self._driver_path.exists()
    else:
      self._driver_path = None

  def tearDown(self):
    shutil.rmtree(self.output_dir, True)

  def get_benchmark(self):
    """Returns a benchmark instance for the corresponding benchmark_name"""
    story_class = cbb_adapter.get_pressbenchmark_story_cls(self.benchmark_name)
    if not story_class:
      return None

    stories = story_class.default_story_names()[:1]
    workload = story_class(substories=stories)

    benchmark_cls = cbb_adapter.get_pressbenchmark_cls(self.benchmark_name)
    benchmark = benchmark_cls(stories=[workload])
    return benchmark

  def test_benchmark_chrome(self):
    """Tests that we can execute the specified benchmark and obtain result data post execution.
    This test uses Chrome browser to run the benchmarks.
    """
    benchmark = self.get_benchmark()
    browser = chrome.ChromeWebDriver(
        "Chrome", self._browser_path, driver_path=self._driver_path)

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
