# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import io
import pathlib
import sys
from typing import List, Tuple
from unittest import mock

import pytest

import crossbench.browsers.all as browsers
from crossbench.cli import CrossBenchCLI
from tests.end2end.helper import End2EndTestCase


class SysExitException(Exception):

  def __init__(self):
    super().__init__("sys.exit")


class CLIEnd2EndTestCase(End2EndTestCase):
  """A combination of all benchmarks with various probes and flags.
  These tests should cover what cannot be easily done with simple isolated
  unit tests."""

  __test__ = True

  def setUp(self) -> None:
    super().setUp()
    self.root_dir = pathlib.Path(__file__).parents[2]
    self.cache_dir = self.output_dir / "cache"
    self.cache_dir.mkdir()
    # Mock out chrome's stable path to be able to run on the CQ with the
    # --test-browser-path option.
    stable_path_patcher = mock.patch(
        "crossbench.browsers.all.Chrome.stable_path",
        return_value=self.browser_path)
    self.addCleanup(stable_path_patcher.stop)
    stable_path_patcher.start()
    # The CQ uses the latest canary, which might not have a easily publicly
    # accessible chromedriver available.
    if self.driver_path:
      driver_patcher = mock.patch(
          "crossbench.browsers.chromium.chromium_webdriver.ChromeDriverFinder.download",
          return_value=self.driver_path)
      self.addCleanup(driver_patcher.stop)
      driver_patcher.start()

  def run_cli(self, *args: str) -> Tuple[CrossBenchCLI, io.StringIO]:
    cli = CrossBenchCLI()
    with contextlib.redirect_stdout(io.StringIO()) as stdout:
      with mock.patch("sys.exit", side_effect=SysExitException):
        cli.run(args)
    return cli, stdout

  def get_browser_dirs(self, results_dir: pathlib.Path) -> List[pathlib.Path]:
    self.assertTrue(results_dir.is_dir())
    browser_dirs = [path for path in results_dir.iterdir() if path.is_dir()]
    return browser_dirs

  def get_v8_log_files(self, results_dir: pathlib.Path) -> List[pathlib.Path]:
    return list(results_dir.glob("**/*-v8.log"))

  def test_speedometer_2_0(self) -> None:
    # - Speedometer 2.0
    # - Speedometer --iterations flag
    # - Tracing probe with inline args
    # - --browser-config
    with self.assertRaises(SysExitException):
      self.run_cli("speedometer_2.0", "--help")
    self.run_cli("describe", "benchmark", "speedometer_2.0")
    browser_config = self.root_dir / "config" / "browser.config.example.hjson"
    self.assertTrue(browser_config.is_file())
    results_dir = self.output_dir / "results"
    self.assertFalse(results_dir.exists())
    self.run_cli("sp20", f"--browser-config={browser_config}", "--iterations=2",
                 "--env-validation=skip", f"--out-dir={results_dir}",
                 f"--cache-dir={self.cache_dir}",
                 "--probe=tracing:{preset:'minimal'}")

    browser_dirs = self.get_browser_dirs(results_dir)
    self.assertTrue(len(browser_dirs) > 1)

  def test_speedometer_2_1(self) -> None:
    # - Speedometer 2.1
    # - Story filtering with regexp
    # - V8 probes
    # - inline probe arguments
    with self.assertRaises(SysExitException):
      self.run_cli("speedometer_2.1", "--help")
    self.run_cli("describe", "benchmark", "speedometer_2.1")
    results_dir = self.output_dir / "results"
    self.assertFalse(results_dir.exists())
    self.run_cli("sp21", "--browser=chrome-stable", "--iterations=2",
                 "--env-validation=skip", f"--out-dir={results_dir}",
                 f"--cache-dir={self.cache_dir}", "--stories=.*Vanilla.*",
                 "--probe=v8.log:{js_flags:['--log-maps']}",
                 "--probe=v8.turbolizer")

    browser_dirs = self.get_browser_dirs(results_dir)
    self.assertEqual(len(browser_dirs), 1)
    v8_log_files = self.get_v8_log_files(results_dir)
    self.assertTrue(len(v8_log_files) > 1)

  def test_speedometer_2_1_custom_chrome_download(self) -> None:
    # - Custom chrome version downloads
    # - headless
    if not self.platform.which("gsutil"):
      self.skipTest("Missing required 'gsutil', skipping test.")
    results_dir = self.output_dir / "results"
    # TODO: speed up --browser=chrome-M111 and add it.
    self.assertEqual(len(list(self.cache_dir.iterdir())), 0)
    self.run_cli("sp21", f"--cache-dir={self.cache_dir}",
                 "--browser=chrome-stable", "--browser=chrome-111.0.5563.110",
                 "--headless", "--iterations=1", "--env-validation=skip",
                 f"--out-dir={results_dir}", f"--cache-dir={self.cache_dir}",
                 "--stories=.*Vanilla.*")

    browser_dirs = self.get_browser_dirs(results_dir)
    self.assertEqual(len(browser_dirs), 2)
    v8_log_files = self.get_v8_log_files(results_dir)
    self.assertListEqual(v8_log_files, [])

  def test_speedometer_2_1_chrome_safari(self) -> None:
    # - Speedometer 3
    # - Merging stories over multiple iterations and browsers
    # - Testing safari
    # - --verbose flag
    # This fails on the CQ bot, so make sure we skip it there:
    if self.driver_path:
      self.skipTest("Skipping test on CQ.")
    if not self.platform.is_macos and (
        not browsers.Safari.default_path().exists()):
      self.skipTest("Test requires Safari, skipping on non macOS devices.")
    results_dir = self.output_dir / "results"
    self.assertFalse(results_dir.exists())
    self.run_cli("sp21", "--browser=chrome", "--browser=safari",
                 "--iterations=1", "--repeat=2", "--env-validation=skip",
                 "--verbose", f"--out-dir={results_dir}",
                 f"--cache-dir={self.cache_dir}", "--stories=.*React.*")

    browser_dirs = self.get_browser_dirs(results_dir)
    self.assertEqual(len(browser_dirs), 2)
    v8_log_files = self.get_v8_log_files(results_dir)
    self.assertListEqual(v8_log_files, [])

  def test_jetstream_2_0(self) -> None:
    # - jetstream 2.0
    # - merge / run separate stories
    # - custom multiple --js-flags
    # - custom viewport
    # - quiet flag
    with self.assertRaises(SysExitException):
      self.run_cli("jetstream_2.0", "--help")
    self.run_cli("describe", "--json", "benchmark", "jetstream_2.0")
    results_dir = self.output_dir / "results"
    self.assertFalse(results_dir.exists())
    self.run_cli("jetstream_2.0", "--browser=chrome-stable", "--separate",
                 "--repeat=2", "--env-validation=skip",
                 f"--out-dir={results_dir}", f"--cache-dir={self.cache_dir}",
                 "--viewport=maximised", "--stories=.*date-format.*", "--quiet",
                 "--js-flags=--log,--log-opt,--log-deopt", "--", "--no-sandbox")

    v8_log_files = self.get_v8_log_files(results_dir)
    self.assertTrue(len(v8_log_files) > 1)
    browser_dirs = self.get_browser_dirs(results_dir)
    self.assertEqual(len(browser_dirs), 1)

  def test_jetstream_2_1(self) -> None:
    # - jetstream 2.1
    # - custom --time-unit
    # - explicit single story
    # - custom viewport
    # - --probe-config
    with self.assertRaises(SysExitException):
      self.run_cli("jetstream_2.1", "--help")
    self.run_cli("describe", "benchmark", "jetstream_2.1")
    probe_config = self.root_dir / "config" / "probe.config.example.hjson"
    self.assertTrue(probe_config.is_file())
    results_dir = self.output_dir / "results"
    self.assertFalse(results_dir.exists())
    self.run_cli("jetstream_2.1", "--browser=chr", "--env-validation=skip",
                 f"--out-dir={results_dir}", f"--cache-dir={self.cache_dir}",
                 "--viewport=900x800", "--stories=Box2D", "--time-unit=0.9",
                 f"--probe-config={probe_config}")

    browser_dirs = self.get_browser_dirs(results_dir)
    self.assertEqual(len(browser_dirs), 1)
    v8_log_files = self.get_v8_log_files(results_dir)
    self.assertTrue(len(v8_log_files) > 1)

  def test_loading(self) -> None:
    # - loading using named pages with timeouts
    # - custom cooldown time
    # - custom viewport
    # - performance.mark probe
    with self.assertRaises(SysExitException):
      self.run_cli("loading", "--help")
    self.run_cli("describe", "benchmark", "loading")
    results_dir = self.output_dir / "results"
    self.assertFalse(results_dir.exists())
    self.run_cli("loading", "--browser=chr", "--env-validation=skip",
                 f"--out-dir={results_dir}", f"--cache-dir={self.cache_dir}",
                 "--viewport=headless", "--stories=cnn,facebook",
                 "--cool-down-time=2.5", "--probe=performance.entries")

    browser_dirs = self.get_browser_dirs(results_dir)
    self.assertEqual(len(browser_dirs), 1)

  def test_loading_page_config(self) -> None:
    # - loading with config file
    page_config = self.root_dir / "config" / "page.config.example.hjson"
    self.assertTrue(page_config.is_file())
    results_dir = self.output_dir / "results"
    self.assertFalse(results_dir.exists())
    self.run_cli("loading", "--env-validation=skip", f"--out-dir={results_dir}",
                 f"--cache-dir={self.cache_dir}",
                 f"--page-config={page_config}", "--probe=performance.entries")

  def test_loading_playback_urls(self) -> None:
    # - loading using url
    # - combined pages and --playback controller
    results_dir = self.output_dir / "results"
    self.assertFalse(results_dir.exists())
    self.run_cli("loading", "--env-validation=skip", f"--out-dir={results_dir}",
                 f"--cache-dir={self.cache_dir}", "--playback=5.3s",
                 "--viewport=fullscreen",
                 "--stories=http://google.com,0.5,http://bing.com,0.4",
                 "--probe=performance.entries")

  def test_loading_playback(self) -> None:
    # - loading using named pages with timeouts
    # - separate pages and --playback controller
    # - viewport-size via chrome flag
    results_dir = self.output_dir / "results"
    self.assertFalse(results_dir.exists())
    self.run_cli("loading", "--browser=chr", "--env-validation=skip",
                 f"--out-dir={results_dir}", f"--cache-dir={self.cache_dir}",
                 "--playback=5.3s", "--separate",
                 "--stories=twitter,2,facebook,0.4",
                 "--probe=performance.entries", "--", "--window-size=900,500",
                 "--window-position=150,150")

  def test_loading_playback_firefox(self) -> None:
    # - loading using named pages with timeouts
    # - --playback controller
    # - Firefox
    try:
      if not browsers.Firefox.default_path().exists():
        self.skipTest("Test requires Firefox.")
    except Exception:
      self.skipTest("Test requires Firefox.")
    results_dir = self.output_dir / "results"
    self.assertFalse(results_dir.exists())
    self.run_cli("loading", "--browser=chr", "--browser=ff",
                 "--env-validation=skip", f"--out-dir={results_dir}",
                 f"--cache-dir={self.cache_dir}", "--playback=2x",
                 "--stories=twitter,1,facebook,0.4",
                 "--probe=performance.entries")

    browser_dirs = self.get_browser_dirs(results_dir)
    self.assertEqual(len(browser_dirs), 2)


if __name__ == "__main__":
  sys.exit(pytest.main([__file__]))
