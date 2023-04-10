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

import os
import pathlib
from typing import List, Optional

import crossbench.benchmarks.all
import crossbench.browsers.base
from crossbench.browsers import webdriver as cb_webdriver
import crossbench.env
import crossbench.runner
from selenium import webdriver

press_benchmarks = [
    crossbench.benchmarks.speedometer.Speedometer20Benchmark,
    crossbench.benchmarks.speedometer.Speedometer21Benchmark,
    crossbench.benchmarks.motionmark.MotionMark12Benchmark,
    crossbench.benchmarks.jetstream.JetStream20Benchmark,
    crossbench.benchmarks.jetstream.JetStream21Benchmark,
]

press_benchmarks_dict = {cls.NAME: cls for cls in press_benchmarks}


def get_pressbenchmark_cls(benchmark_name: str):
  """Returns the class of the specified pressbenchmark.

  Args:
    benchmark_name: Name of the benchmark.

  Returns:
    An instance of the benchmark implementation
  """
  return press_benchmarks_dict.get(benchmark_name)


def get_pressbenchmark_story_cls(benchmark_name: str):
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

  browser = cb_webdriver.RemoteWebDriver('default', driver)
  browser.version = driver.capabilities['browserVersion']
  return browser


def get_probe_result_file(benchmark_name: str,
                          browser: crossbench.browsers.base.Browser,
                          output_dir: str,
                          probe_name: Optional[str] = None):
  """Returns the path to the probe result file.

  Args:
    benchmark_name: Name of the benchmark.
    browser: Browser instance.
    output_dir: Path to the directory where the output of the benchmark execution was written.
    probe_name: Optional name of the probe for the result file. If not specified, the first
                probe from the default benchmark story will be used."""

  if probe_name == None:
    if benchmark_name in press_benchmarks_dict:
      benchmark_cls = press_benchmarks_dict[benchmark_name]
      probe_cls = benchmark_cls.DEFAULT_STORY_CLS.PROBES[0]
      probe_name = probe_cls.NAME
    else:
      return None

  return os.path.join(output_dir, browser.unique_name, f'{probe_name}.json')


def run_benchmark(output_folder: str,
                  browser_list: List[crossbench.browsers.base.Browser],
                  benchmark):
  """Runs the benchmark using crossbench runner.

  Args:
    output_folder: Path to the directory where the output of the benchmark execution will be written.
    browser_list: List of browsers to run the benchmark on.
    benchmark: The Benchmark instance to run.
  """
  runner = crossbench.runner.Runner(
      out_dir=pathlib.Path(output_folder),
      browsers=browser_list,
      benchmark=benchmark,
      env_validation_mode=crossbench.env.ValidationMode.SKIP,
      throw=True)

  runner.run()
