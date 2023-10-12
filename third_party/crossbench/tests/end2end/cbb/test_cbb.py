# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import pathlib

import pytest

from crossbench.benchmarks import all as benchmarks
from crossbench.benchmarks.benchmark import PressBenchmark
from crossbench.browsers.chrome import webdriver as chrome_webdriver
from crossbench.cbb import cbb_adapter
from tests import run_helper

# pytest.fixtures rely on params having the same name as the fixture function
# pylint: disable=redefined-outer-name


def get_benchmark(benchmark_cls) -> PressBenchmark:
  """Returns a benchmark instance for the corresponding benchmark_name"""
  story_class = cbb_adapter.get_pressbenchmark_story_cls(benchmark_cls.NAME)
  assert story_class
  stories = story_class.default_story_names()[:1]
  workload = story_class(  # pytype: disable=not-instantiable
      substories=stories)
  benchmark_cls_lookup = cbb_adapter.get_pressbenchmark_cls(benchmark_cls.NAME)
  assert benchmark_cls_lookup, (
      f"Could not find benchmark class for '{benchmark_cls.NAME}'")
  assert benchmark_cls_lookup == benchmark_cls
  benchmark = benchmark_cls_lookup(stories=[workload])  # pytype: disable=not-instantiable
  return benchmark


@pytest.fixture
def webdriver(driver_path, browser_path):
  return chrome_webdriver.ChromeWebDriver(
      "Chrome", browser_path, driver_path=driver_path)


def run_benchmark(output_dir, webdriver, benchmark_cls) -> None:
  """Tests that we can execute the specified benchmark and obtain result data post execution.
  This test uses Chrome browser to run the benchmarks.
  """
  benchmark = get_benchmark(benchmark_cls)
  assert benchmark
  results_dir = output_dir / "result"

  maybe_results_file = cbb_adapter.get_probe_result_file(
      benchmark_cls.NAME, webdriver, results_dir)
  assert maybe_results_file
  results_file = pathlib.Path(maybe_results_file)
  assert not results_file.exists()

  cbb_adapter.run_benchmark(
      output_folder=results_dir, browser_list=[webdriver], benchmark=benchmark)

  assert results_file.exists()
  with results_file.open(encoding="utf-8") as f:
    benchmark_data = json.load(f)
  assert benchmark_data


def test_speedometer_20(output_dir, webdriver):
  run_benchmark(output_dir, webdriver, benchmarks.Speedometer20Benchmark)


def test_speedometer_21(output_dir, webdriver):
  run_benchmark(output_dir, webdriver, benchmarks.Speedometer21Benchmark)


def test_motionmark_12(output_dir, webdriver):
  run_benchmark(output_dir, webdriver, benchmarks.MotionMark12Benchmark)


def test_jetstream_20(output_dir, webdriver):
  run_benchmark(output_dir, webdriver, benchmarks.JetStream20Benchmark)


def test_jetstream_21(output_dir, webdriver):
  run_benchmark(output_dir, webdriver, benchmarks.JetStream21Benchmark)


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
