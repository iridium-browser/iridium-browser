# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import contextlib
import io
import pathlib
from typing import List, Tuple
from unittest import mock

import pytest

import crossbench.browsers.all as browsers
from crossbench.cli import CrossBenchCLI
from crossbench import plt
from tests import run_helper


class SysExitException(Exception):

  def __init__(self):
    super().__init__("sys.exit")


@pytest.fixture(autouse=True)
def cli_test_context(browser_path, driver_path):
  # Mock out chrome's stable path to be able to run on the CQ with the
  # --test-browser-path option.
  with mock.patch(
      "crossbench.browsers.all.Chrome.stable_path", return_value=browser_path):
    if not driver_path:
      yield
    else:
      # The CQ uses the latest canary, which might not have a easily publicly
      # accessible chromedriver available.
      with mock.patch(
          "crossbench.browsers.chromium.webdriver.ChromeDriverFinder.download",
          return_value=driver_path):
        yield


def _run_cli(*args: str) -> Tuple[CrossBenchCLI, io.StringIO]:
  cli = CrossBenchCLI()
  with contextlib.redirect_stdout(io.StringIO()) as stdout:
    with mock.patch("sys.exit", side_effect=SysExitException):
      cli.run(args)
  return cli, stdout


def _get_browser_dirs(results_dir: pathlib.Path) -> List[pathlib.Path]:
  assert results_dir.is_dir()
  browser_dirs = [path for path in results_dir.iterdir() if path.is_dir()]
  return browser_dirs


def _get_v8_log_files(results_dir: pathlib.Path) -> List[pathlib.Path]:
  return list(results_dir.glob("**/*-v8.log"))


@pytest.mark.skipif(
    not plt.PLATFORM.has_display, reason="end2end test cannot run headless")
def test_speedometer_2_0(output_dir, cache_dir, root_dir) -> None:
  # - Speedometer 2.0
  # - Speedometer --iterations flag
  # - Tracing probe with inline args
  # - --browser-config
  with pytest.raises(SysExitException):
    _run_cli("speedometer_2.0", "--help")
  _run_cli("describe", "benchmark", "speedometer_2.0")
  browser_config = root_dir / "config" / "browser.config.example.hjson"
  assert browser_config.is_file()
  results_dir = output_dir / "results"
  assert not results_dir.exists()
  _run_cli("sp_2.0", f"--browser-config={browser_config}", "--iterations=2",
           "--env-validation=skip", f"--out-dir={results_dir}",
           f"--cache-dir={cache_dir}", "--probe=tracing:{preset:'minimal'}")


@pytest.mark.skipif(
    not plt.PLATFORM.has_display, reason="end2end test cannot run headless")
def test_speedometer_2_1(output_dir, cache_dir) -> None:
  # - Speedometer 2.1
  # - Story filtering with regexp
  # - V8 probes
  # - minimal splashscreen
  # - inline probe arguments
  with pytest.raises(SysExitException):
    _run_cli("speedometer_2.1", "--help")
  _run_cli("describe", "benchmark", "speedometer_2.1")
  results_dir = output_dir / "results"
  assert not results_dir.exists()
  _run_cli("sp2.1", "--browser=chrome-stable", "--splashscreen=minimal",
           "--iterations=2", "--env-validation=skip",
           f"--out-dir={results_dir}", f"--cache-dir={cache_dir}",
           "--stories=.*Vanilla.*",
           "--probe=v8.log:{log_all:false,js_flags:['--log-maps']}",
           "--probe=v8.turbolizer")

  browser_dirs = _get_browser_dirs(results_dir)
  assert len(browser_dirs) == 1
  v8_log_files = _get_v8_log_files(results_dir)
  assert len(v8_log_files) > 1


def test_speedometer_2_1_custom_chrome_download(output_dir, cache_dir) -> None:
  # - Custom chrome version downloads
  # - headless
  if not plt.PLATFORM.which("gsutil"):
    pytest.skip("Missing required 'gsutil', skipping test.")
  results_dir = output_dir / "results"
  # TODO: speed up --browser=chrome-M111 and add it.
  assert len(list(cache_dir.iterdir())) == 0
  _run_cli("sp2.1", f"--cache-dir={cache_dir}", "--browser=chrome-M113",
           "--browser=chrome-111.0.5563.110", "--headless", "--iterations=1",
           "--env-validation=skip", f"--out-dir={results_dir}",
           f"--cache-dir={cache_dir}", "--stories=.*Vanilla.*")

  browser_dirs = _get_browser_dirs(results_dir)
  assert len(browser_dirs) == 2
  v8_log_files = _get_v8_log_files(results_dir)
  assert not v8_log_files


@pytest.mark.skipif(
    not plt.PLATFORM.has_display, reason="end2end test cannot run headless")
def test_speedometer_2_1_chrome_safari(output_dir, cache_dir,
                                       driver_path) -> None:
  # - Speedometer 3
  # - Merging stories over multiple iterations and browsers
  # - Testing safari
  # - --verbose flag
  # - no splashscreen
  # This fails on the CQ bot, so make sure we skip it there:
  if driver_path:
    pytest.skip("Skipping test on CQ.")
  if not plt.PLATFORM.is_macos and (
      not browsers.Safari.default_path().exists()):
    pytest.skip("Test requires Safari, skipping on non macOS devices.")
  results_dir = output_dir / "results"
  assert not results_dir.exists()
  _run_cli("sp2.1", "--browser=chrome", "--browser=safari",
           "--splashscreen=none", "--iterations=1", "--repeat=2",
           "--env-validation=skip", "--verbose", f"--out-dir={results_dir}",
           f"--cache-dir={cache_dir}", "--stories=.*React.*")

  browser_dirs = _get_browser_dirs(results_dir)
  assert len(browser_dirs) == 2
  v8_log_files = _get_v8_log_files(results_dir)
  assert v8_log_files == []


@pytest.mark.skipif(
    not plt.PLATFORM.has_display, reason="end2end test cannot run headless")
def test_jetstream_2_0(output_dir, cache_dir) -> None:
  # - jetstream 2.0
  # - merge / run separate stories
  # - custom multiple --js-flags
  # - custom viewport
  # - quiet flag
  with pytest.raises(SysExitException):
    _run_cli("jetstream_2.0", "--help")
  _run_cli("describe", "--json", "benchmark", "jetstream_2.0")
  results_dir = output_dir / "results"
  assert not results_dir.exists()
  _run_cli("jetstream_2.0", "--browser=chrome-stable", "--separate",
           "--repeat=2", "--env-validation=skip", f"--out-dir={results_dir}",
           f"--cache-dir={cache_dir}", "--viewport=maximised",
           "--stories=.*date-format.*", "--quiet",
           "--js-flags=--log,--log-opt,--log-deopt", "--", "--no-sandbox")

  v8_log_files = _get_v8_log_files(results_dir)
  assert len(v8_log_files) > 1
  browser_dirs = _get_browser_dirs(results_dir)
  assert len(browser_dirs) == 1


@pytest.mark.skipif(
    not plt.PLATFORM.has_display, reason="end2end test cannot run headless")
def test_jetstream_2_1(output_dir, cache_dir, root_dir) -> None:
  # - jetstream 2.1
  # - custom --time-unit
  # - explicit single story
  # - custom splashscreen
  # - custom viewport
  # - --probe-config
  with pytest.raises(SysExitException):
    _run_cli("jetstream_2.1", "--help")
  _run_cli("describe", "benchmark", "jetstream_2.1")
  probe_config = root_dir / "config" / "probe.config.example.hjson"
  assert probe_config.is_file()
  results_dir = output_dir / "results"
  assert not results_dir.exists()
  chrome_version = "--browser=chrome"
  _run_cli("jetstream_2.1", chrome_version, "--env-validation=skip",
           "--splashscreen=http://google.com", f"--out-dir={results_dir}",
           f"--cache-dir={cache_dir}", "--viewport=900x800", "--stories=Box2D",
           "--time-unit=0.9", f"--probe-config={probe_config}")

  browser_dirs = _get_browser_dirs(results_dir)
  assert len(browser_dirs) == 1
  v8_log_files = _get_v8_log_files(results_dir)
  assert len(v8_log_files) > 1


@pytest.mark.skipif(
    not plt.PLATFORM.has_display, reason="end2end test cannot run headless")
def test_loading(output_dir, cache_dir) -> None:
  # - loading using named pages with timeouts
  # - custom cooldown time
  # - custom viewport
  # - performance.mark probe
  with pytest.raises(SysExitException):
    _run_cli("loading", "--help")
  _run_cli("describe", "benchmark", "loading")
  results_dir = output_dir / "results"
  assert not results_dir.exists()
  _run_cli("loading", "--browser=chr", "--env-validation=skip",
           f"--out-dir={results_dir}", f"--cache-dir={cache_dir}",
           "--viewport=headless", "--stories=cnn,facebook",
           "--cool-down-time=2.5", "--probe=performance.entries")

  browser_dirs = _get_browser_dirs(results_dir)
  assert len(browser_dirs) == 1


@pytest.mark.skipif(
    not plt.PLATFORM.has_display, reason="end2end test cannot run headless")
def test_loading_page_config(output_dir, cache_dir, root_dir) -> None:
  # - loading with config file
  page_config = root_dir / "config" / "page.config.example.hjson"
  assert page_config.is_file()
  results_dir = output_dir / "results"
  assert not results_dir.exists()
  _run_cli("loading", "--env-validation=skip", f"--out-dir={results_dir}",
           f"--cache-dir={cache_dir}", f"--page-config={page_config}",
           "--probe=performance.entries")


@pytest.mark.skipif(
    not plt.PLATFORM.has_display, reason="end2end test cannot run headless")
def test_loading_playback_urls(output_dir, cache_dir) -> None:
  # - loading using url
  # - combined pages and --playback controller
  results_dir = output_dir / "results"

  assert not results_dir.exists()
  _run_cli("loading", "--env-validation=skip", f"--out-dir={results_dir}",
           f"--cache-dir={cache_dir}", "--playback=5.3s",
           "--viewport=fullscreen",
           "--stories=http://google.com,0.5,http://bing.com,0.4",
           "--probe=performance.entries")


@pytest.mark.skipif(
    not plt.PLATFORM.has_display, reason="end2end test cannot run headless")
def test_loading_playback(output_dir, cache_dir) -> None:
  # - loading using named pages with timeouts
  # - separate pages and --playback controller
  # - viewport-size via chrome flag
  results_dir = output_dir / "results"
  assert not results_dir.exists()
  _run_cli("loading", "--browser=chr", "--env-validation=skip",
           f"--out-dir={results_dir}", f"--cache-dir={cache_dir}",
           "--playback=5.3s", "--separate", "--stories=twitter,2,facebook,0.4",
           "--probe=performance.entries", "--", "--window-size=900,500",
           "--window-position=150,150")


@pytest.mark.skipif(
    not plt.PLATFORM.has_display, reason="end2end test cannot run headless")
def test_loading_playback_firefox(output_dir, cache_dir) -> None:
  # - loading using named pages with timeouts
  # - --playback controller
  # - Firefox
  try:
    if not browsers.Firefox.default_path().exists():
      pytest.skip("Test requires Firefox.")
  except Exception:
    pytest.skip("Test requires Firefox.")
  results_dir = output_dir / "results"
  assert not results_dir.exists()
  _run_cli("loading", "--browser=chr", "--browser=ff", "--env-validation=skip",
           f"--out-dir={results_dir}", f"--cache-dir={cache_dir}",
           "--playback=2x", "--stories=twitter,1,facebook,0.4",
           "--probe=performance.entries")

  browser_dirs = _get_browser_dirs(results_dir)
  assert len(browser_dirs) == 2


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
