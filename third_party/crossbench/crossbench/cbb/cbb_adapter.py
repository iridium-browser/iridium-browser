# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
An adapter class for CBB to interact with the crossbench runner.
The goal is to abstract out the crossbench runner interface details and
provide an integration point for CBB.

Any breaking changes in the function definitions here need to be coordinated
with corresponding changes in CBB in google3
"""

import pathlib
from typing import List, Optional, Type, Union

from selenium import webdriver

import crossbench.benchmarks.all as benchmarks
import crossbench.browsers.browser
import crossbench.browsers.webdriver as cb_webdriver
import crossbench.env
import crossbench.runner.runner
from crossbench.benchmarks.benchmark import PressBenchmark
from crossbench.stories.press_benchmark import PressBenchmarkStory

press_benchmarks = [
    benchmarks.Speedometer20Benchmark,
    benchmarks.Speedometer21Benchmark,
    benchmarks.Speedometer30Benchmark,
    benchmarks.MotionMark12Benchmark,
    benchmarks.JetStream20Benchmark,
    benchmarks.JetStream21Benchmark,
]

press_benchmarks_dict = {cls.NAME: cls for cls in press_benchmarks}


def get_pressbenchmark_cls(
    benchmark_name: str) -> Optional[Type[PressBenchmark]]:
  """Returns the class of the specified pressbenchmark.

  Args:
    benchmark_name: Name of the benchmark.

  Returns:
    An instance of the benchmark implementation
  """
  return press_benchmarks_dict.get(benchmark_name)


def get_pressbenchmark_story_cls(
    benchmark_name: str) -> Optional[Type[PressBenchmarkStory]]:
  """Returns the class of the specified pressbenchmark story.

  Args:
    benchmark_name: Name of the benchmark.

  Returns:
    An instance of the benchmark default story
  """

  benchmark_cls = get_pressbenchmark_cls(benchmark_name)
  if benchmark_cls is not None:
    return benchmark_cls.DEFAULT_STORY_CLS

  return None


def create_remote_webdriver(driver: webdriver.Remote
                           ) -> cb_webdriver.RemoteWebDriver:
  """Creates a remote webdriver instance for the driver.

  Args:
    driver: Remote web driver instance.
  """

  browser = cb_webdriver.RemoteWebDriver("default", driver)
  browser.version = driver.capabilities["browserVersion"]
  return browser


def get_probe_result_file(benchmark_name: str,
                          browser: crossbench.browsers.browser.Browser,
                          output_dir: Union[str, pathlib.Path],
                          probe_name: Optional[str] = None) -> Optional[str]:
  """Returns the path to the probe result file.

  Args:
    benchmark_name: Name of the benchmark.
    browser: Browser instance.
    output_dir: Path to the directory where the output of the benchmark
                execution was written.
    probe_name: Optional name of the probe for the result file. If not
                specified, the first probe from the default benchmark story
                will be used."""
  output_dir_path = pathlib.Path(output_dir)
  if probe_name is None:
    if benchmark_name not in press_benchmarks_dict:
      return None
    benchmark_cls = press_benchmarks_dict[benchmark_name]
    probe_cls = benchmark_cls.DEFAULT_STORY_CLS.PROBES[0]
    probe_name = probe_cls.NAME

  result_file = output_dir_path / browser.unique_name / f"{probe_name}.json"
  return str(result_file)


def run_benchmark(output_folder: Union[str, pathlib.Path],
                  browser_list: List[crossbench.browsers.browser.Browser],
                  benchmark: PressBenchmark) -> None:
  """Runs the benchmark using crossbench runner.

  Args:
    output_folder: Path to the directory where the output of the benchmark
                  execution will be written.
    browser_list: List of browsers to run the benchmark on.
    benchmark: The Benchmark instance to run.
  """
  runner = crossbench.runner.runner.Runner(
      out_dir=pathlib.Path(output_folder),
      browsers=browser_list,
      benchmark=benchmark,
      env_validation_mode=crossbench.env.ValidationMode.SKIP)

  runner.run()
