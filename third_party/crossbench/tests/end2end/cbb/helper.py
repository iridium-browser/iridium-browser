# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Base class for the CBB Adapter unit tests
"""

import abc
import json
import pathlib
from typing import Optional

from crossbench.benchmarks.benchmark import PressBenchmark
from crossbench.browsers import chrome
from crossbench.cbb import cbb_adapter
from tests.end2end import helper as end2end


class CBBTestCase(end2end.End2EndTestCase, metaclass=abc.ABCMeta):

  __test__ = False

  @abc.abstractproperty
  def benchmark_name(self) -> str:
    """Each child class benchmark test will need to define it's corresponding benchmark_name"""

  def get_benchmark(self) -> Optional[PressBenchmark]:
    """Returns a benchmark instance for the corresponding benchmark_name"""
    story_class = cbb_adapter.get_pressbenchmark_story_cls(self.benchmark_name)
    if not story_class:
      return None
    stories = story_class.default_story_names()[:1]
    workload = story_class(  # pytype: disable=not-instantiable
        substories=stories)

    benchmark_cls = cbb_adapter.get_pressbenchmark_cls(  # pytype: disable=not-instantiable
        self.benchmark_name)
    assert benchmark_cls, (
        f"Could not find benchmark class for '{self.benchmark_name}'")
    benchmark = benchmark_cls(stories=[workload])
    return benchmark

  def test_benchmark_chrome(self) -> None:
    """Tests that we can execute the specified benchmark and obtain result data post execution.
    This test uses Chrome browser to run the benchmarks.
    """
    benchmark = self.get_benchmark()
    assert benchmark
    browser = chrome.ChromeWebDriver(
        "Chrome", self.browser_path, driver_path=self.driver_path)
    results_dir = self.output_dir / "result"

    maybe_results_file = cbb_adapter.get_probe_result_file(
        self.benchmark_name, browser, results_dir)
    self.assertIsNotNone(maybe_results_file)
    assert maybe_results_file
    results_file = pathlib.Path(maybe_results_file)
    self.assertFalse(results_file.exists())

    cbb_adapter.run_benchmark(
        output_folder=results_dir, browser_list=[browser], benchmark=benchmark)

    self.assertTrue(results_file.exists())
    with results_file.open(encoding="utf-8") as f:
      benchmark_data = json.load(f)

    self.assertIsNotNone(benchmark_data)
