# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import json
import logging
import multiprocessing
import pathlib
import signal
import subprocess
import time
from typing import TYPE_CHECKING, Dict, Iterable, List, Optional, cast

from crossbench import helper, plt
from crossbench.browsers.chromium.chromium import Chromium
from crossbench.probes.probe import Probe, ProbeConfigParser, ProbeScope, ResultLocation
from crossbench.probes.results import ProbeResult
from crossbench.probes.v8.log import V8LogProbe

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner.run import Run
  from crossbench.runner.groups import BrowsersRunGroup


class ProfilingProbe(Probe):
  """
  General-purpose sampling profiling probe.

  Implementation:
  - Uses linux-perf on linux platforms (per browser/renderer process)
  - Uses xctrace on MacOS (currently only system-wide)

  For linux-based Chromium browsers it also injects JS stack samples with names
  from V8. For Googlers it additionally can auto-upload symbolized profiles to
  pprof.
  """
  NAME = "profiling"
  RESULT_LOCATION = ResultLocation.BROWSER
  IS_GENERAL_PURPOSE = True

  V8_PERF_PROF_FLAG = ("--perf-prof",)
  V8_INTERPRETED_FRAMES_FLAG = "--interpreted-frames-native-stack"

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "js",
        type=bool,
        default=True,
        help="Chrome-only: expose JS function names to the native profiler")
    parser.add_argument(
        "browser_process",
        type=bool,
        default=False,
        help=("Chrome-only: also profile the browser process, "
              "(as opposed to only renderer processes)"))
    parser.add_argument(
        "spare_renderer_process",
        type=bool,
        default=False,
        help=("Chrome-only: Enable/Disable spare renderer processes via \n"
              "--enable-/--disable-features=SpareRendererForSitePerProcess.\n"
              "Spare renderers are disabled by default when profiling "
              "for fewer uninteresting processes."))
    parser.add_argument(
        "v8_interpreted_frames",
        type=bool,
        default=True,
        help=(
            f"Chrome-only: Sets the {cls.V8_INTERPRETED_FRAMES_FLAG} flag for "
            "V8, which exposes interpreted frames as native frames. "
            "Note that this comes at an additional performance and memory cost."
        ))
    parser.add_argument(
        "pprof",
        type=bool,
        default=True,
        help="linux-only: process collected samples with pprof.")
    return parser

  def __init__(self,
               js: bool = True,
               v8_interpreted_frames: bool = True,
               pprof: bool = True,
               browser_process: bool = False,
               spare_renderer_process: bool = False):
    super().__init__()
    self._sample_js: bool = js
    self._sample_browser_process: bool = browser_process
    self._spare_renderer_process: bool = spare_renderer_process
    self._run_pprof: bool = pprof
    self._expose_v8_interpreted_frames: bool = v8_interpreted_frames
    if v8_interpreted_frames:
      assert js, "Cannot expose V8 interpreted frames without js profiling."

  def is_compatible(self, browser: Browser) -> bool:
    if browser.platform.is_linux:
      return isinstance(browser, Chromium)
    if browser.platform.is_macos:
      return True
    return False

  @property
  def sample_js(self) -> bool:
    return self._sample_js

  @property
  def sample_browser_process(self) -> bool:
    return self._sample_browser_process

  @property
  def run_pprof(self) -> bool:
    return self._run_pprof

  def attach(self, browser: Browser) -> None:
    super().attach(browser)
    if browser.platform.is_linux:
      assert isinstance(browser, Chromium), (
          f"Expected Chromium-based browser, found {type(browser)}.")
    if isinstance(browser, Chromium):
      chromium = cast(Chromium, browser)
      if not self._spare_renderer_process:
        chromium.features.disable("SpareRendererForSitePerProcess")
      self._attach_linux(chromium)

  def pre_check(self, env: HostEnvironment) -> None:
    super().pre_check(env)
    for browser in self._browsers:
      self._pre_check_browser(browser, env)

  def _pre_check_browser(self, browser: Browser, env: HostEnvironment) -> None:
    browser_platform = browser.platform
    if self.run_pprof:
      self._run_pprof = browser_platform.which("gcert") is not None
      if not self.run_pprof:
        logging.warning(
            "Disabled automatic pprof uploading for non-googler machine.")
    if browser_platform.is_linux:
      env.check_installed(binaries=["pprof"])
      assert browser_platform.which("perf"), "Please install linux-perf"
    elif browser_platform.is_macos:
      assert browser_platform.which("xctrace"), (
          "Please install Xcode to use xctrace")
    if self.run_pprof:
      try:
        browser_platform.sh(browser_platform.which("gcertstatus"))
        return
      except plt.SubprocessError:
        env.handle_warning("Please run gcert for generating pprof results")
    # Only Linux-perf results can be merged
    if browser_platform.is_macos and env.runner.repetitions > 1:
      env.handle_warning(f"Probe={self.NAME} cannot merge data over multiple "
                         f"repetitions={env.runner.repetitions}.")

  def _attach_linux(self, browser: Chromium) -> None:
    if self._sample_js:
      browser.js_flags.update(self.V8_PERF_PROF_FLAG)
      if self._expose_v8_interpreted_frames:
        browser.js_flags.set(self.V8_INTERPRETED_FRAMES_FLAG)
    cmd = pathlib.Path(__file__).parent / "linux-perf-chrome-renderer-cmd.sh"
    assert not browser.platform.is_remote, (
        "Copying renderer command prefix to remote platform is "
        "not implemented yet")
    assert cmd.is_file(), f"Didn't find {cmd}"
    browser.flags["--renderer-cmd-prefix"] = str(cmd)
    # Disable sandbox to write profiling data
    browser.flags.set("--no-sandbox")

  def log_run_result(self, run: Run) -> None:
    self._log_results([run])

  def log_browsers_result(self, group: BrowsersRunGroup) -> None:
    self._log_results(group.runs)

  def _log_results(self, runs: Iterable[Run]) -> None:
    filtered_runs = list(run for run in runs if self in run.results)
    if not filtered_runs:
      return
    logging.info("-" * 80)
    logging.critical("Profiling results:")
    logging.info("  *.perf.data: 'perf report -i $FILE'")
    logging.info("- " * 40)
    for i, run in enumerate(filtered_runs):
      self._log_run_result_summary(run, i)

  def _log_run_result_summary(self, run: Run, i: int) -> None:
    if self not in run.results:
      return
    urls = run.results[self].url_list
    perf_files = run.results[self].file_list
    if not urls and not perf_files:
      return
    logging.info("Run %d: %s", i + 1, run.name)
    if urls:
      largest_perf_file = perf_files[0]
      logging.critical("    %s", urls[0])
    if perf_files:
      largest_perf_file = perf_files[0]
      logging.critical("    %s : %s", largest_perf_file,
                       helper.get_file_size(largest_perf_file))
      if len(perf_files) > 1:
        logging.info("    %s/*.perf.data*: %d more files",
                     largest_perf_file.parent, len(perf_files))

  def get_scope(self, run: Run) -> ProfilingScope:
    if run.platform.is_linux:
      return LinuxProfilingScope(self, run)
    if run.platform.is_macos:
      return MacOSProfilingScope(self, run)
    raise NotImplementedError("Invalid platform")


class ProfilingScope(ProbeScope[ProfilingProbe], metaclass=abc.ABCMeta):
  pass


class MacOSProfilingScope(ProfilingScope):
  _process: subprocess.Popen

  def get_default_result_path(self) -> pathlib.Path:
    return super().get_default_result_path().parent / "profile.trace"

  def start(self, run: Run) -> None:
    self._process = self.browser_platform.popen("xctrace", "record",
                                                "--template", "Time Profiler",
                                                "--all-processes", "--output",
                                                self.result_path)
    # xctrace takes some time to start up
    time.sleep(3)

  def stop(self, run: Run) -> None:
    # Needs to be SIGINT for xctrace, terminate won't work.
    self._process.send_signal(signal.SIGINT)

  def tear_down(self, run: Run) -> ProbeResult:
    while self._process.poll() is None:
      time.sleep(1)
    return self.browser_result(file=(self.result_path,))


class LinuxProfilingScope(ProfilingScope):
  PERF_DATA_PATTERN = "*.perf.data"
  TEMP_FILE_PATTERNS = (
      "*.perf.data.jitted",
      "jitted-*.so",
      "jit-*.dump",
  )

  def __init__(self, probe: ProfilingProbe, run: Run) -> None:
    super().__init__(probe, run)
    self._perf_process: Optional[subprocess.Popen] = None

  def start(self, run: Run) -> None:
    if not self.probe.sample_browser_process:
      return
    if run.browser.pid is None:
      logging.warning("Cannot sample browser process")
      return
    perf_data_file = run.out_dir / "browser.perf.data"
    # TODO: not fully working yet
    self._perf_process = self.browser_platform.popen(
        "perf", "record", "--call-graph=fp", "--freq=max", "--clockid=mono",
        f"--output={perf_data_file}", f"--pid={run.browser.pid}")

  def setup(self, run: Run) -> None:
    for probe in run.probes:
      assert not isinstance(probe, V8LogProbe), (
          "Cannot use profiler and v8.log probe in parallel yet")

  def stop(self, run: Run) -> None:
    if self._perf_process:
      self._perf_process.terminate()

  def tear_down(self, run: Run) -> ProbeResult:
    # Waiting for linux-perf to flush all perf data
    if self.probe.sample_browser_process:
      logging.debug("Waiting for browser process to stop")
      time.sleep(3)
    if self.probe.sample_browser_process:
      logging.info("Browser process did not stop after 3s. "
                   "You might get partial profiles")
    time.sleep(2)

    perf_files: List[pathlib.Path] = helper.sort_by_file_size(
        run.out_dir.glob(self.PERF_DATA_PATTERN))
    raw_perf_files = perf_files
    urls: List[str] = []
    try:
      if self.probe.sample_js:
        perf_files = self._inject_v8_symbols(run, perf_files)
      if self.probe.run_pprof:
        urls = self._export_to_pprof(run, perf_files)
    finally:
      self._clean_up_temp_files(run)
    if self.probe.run_pprof:
      logging.debug("Profiling results: %s", urls)
      return self.browser_result(url=urls, file=raw_perf_files)
    if self.browser_platform.which("pprof"):
      logging.info("Run pprof over all (or single) perf data files "
                   "for interactive analysis:")
      logging.info("   pprof --http=localhost:1984 %s",
                   " ".join(map(str, perf_files)))
    return self.browser_result(file=perf_files)

  def _inject_v8_symbols(self, run: Run,
                         perf_files: List[pathlib.Path]) -> List[pathlib.Path]:
    with run.actions(
        f"Probe {self.probe.name}: "
        f"Injecting V8 symbols into {len(perf_files)} profiles",
        verbose=True), helper.Spinner():
      # Filter out empty files
      perf_files = [file for file in perf_files if file.stat().st_size > 0]
      if self.browser_platform.is_remote:
        # Use loop, as we cannot easily serialize the remote platform.
        perf_jitted_files = [
            linux_perf_probe_inject_v8_symbols(file, self.browser_platform)
            for file in perf_files
        ]
      else:
        assert self.browser_platform == plt.PLATFORM
        with multiprocessing.Pool() as pool:
          perf_jitted_files = list(
              pool.imap(linux_perf_probe_inject_v8_symbols, perf_files))
      return [file for file in perf_jitted_files if file is not None]

  def _export_to_pprof(self, run: Run,
                       perf_files: List[pathlib.Path]) -> List[str]:
    assert self.probe.run_pprof
    run_details_json = json.dumps(run.get_browser_details_json())
    with run.actions(
        f"Probe {self.probe.name}: "
        f"exporting {len(perf_files)} profiles to pprof (slow)",
        verbose=True), helper.Spinner():
      self.browser_platform.sh(
          "gcertstatus >&/dev/null || "
          "(echo 'Authenticating with gcert:'; gcert)",
          shell=True)
      size = len(perf_files)
      items = zip(perf_files, [run_details_json] * size)
      urls: List[str] = []
      if self.browser_platform.is_remote:
        # Use loop, as we cannot easily serialize the remote platform.
        for perf_data_file, run_details in items:
          url = linux_perf_probe_pprof(perf_data_file, run_details,
                                       self.browser_platform)
          if url:
            urls.append(url)
      else:
        assert self.browser_platform == plt.PLATFORM
        with multiprocessing.Pool() as pool:
          urls = [
              url for url in pool.starmap(linux_perf_probe_pprof, items) if url
          ]
      try:
        if perf_files:
          # TODO: Add "combined" profile again
          pass
      except Exception as e:  # pylint: disable=broad-except
        logging.debug("Failed to run pprof: %s", e)
      return urls

  def _clean_up_temp_files(self, run: Run) -> None:
    if not self.probe.run_pprof:
      return
    for pattern in self.TEMP_FILE_PATTERNS:
      for file in run.out_dir.glob(pattern):
        file.unlink()


def prepare_linux_perf_env(platform: plt.Platform,
                           cwd: pathlib.Path) -> Dict[str, str]:
  env: Dict[str, str] = dict(platform.environ)
  env["JITDUMPDIR"] = str(cwd.absolute())
  return env


def linux_perf_probe_inject_v8_symbols(
    perf_data_file: pathlib.Path,
    platform: Optional[plt.Platform] = None) -> Optional[pathlib.Path]:
  assert perf_data_file.is_file()
  output_file = perf_data_file.with_suffix(".data.jitted")
  assert not output_file.exists()
  platform = platform or plt.PLATFORM
  env = prepare_linux_perf_env(platform, perf_data_file.parent)
  try:
    platform.sh(
        "perf",
        "inject",
        "--jit",
        f"--input={perf_data_file}",
        f"--output={output_file}",
        env=env)
  except plt.SubprocessError as e:
    KB = 1024
    if perf_data_file.stat().st_size > 200 * KB:
      logging.warning("Failed processing: %s\n%s", perf_data_file, e)
    else:
      # TODO: investigate why almost all small perf.data files fail
      logging.debug("Failed processing small profile (likely empty): %s\n%s",
                    perf_data_file, e)
  if not output_file.exists():
    return None
  return output_file


def linux_perf_probe_pprof(
    perf_data_file: pathlib.Path,
    run_details: str,
    platform: Optional[plt.Platform] = None) -> Optional[str]:
  size = helper.get_file_size(perf_data_file)
  platform = platform or plt.PLATFORM
  env = prepare_linux_perf_env(platform, perf_data_file.parent)
  url = ""
  try:
    url = platform.sh_stdout(
        "pprof",
        "-flame",
        f"-add_comment={run_details}",
        perf_data_file,
        env=env,
    ).strip()
  except plt.SubprocessError as e:
    # Occasionally small .jitted files fail, likely due perf inject silently
    # failing?
    raw_perf_data_file = perf_data_file.with_suffix("")
    if perf_data_file.suffix == ".jitted" and raw_perf_data_file.exists():
      logging.debug(
          "pprof best-effort: falling back to standard perf data "
          "without js symbols: %s \n"
          "Got failures for %s: %s", raw_perf_data_file, perf_data_file.name, e)
      try:
        perf_data_file = raw_perf_data_file
        url = platform.sh_stdout(
            "pprof",
            "-flame",
            f"-add_comment={run_details}",
            raw_perf_data_file,
        ).strip()
      except plt.SubprocessError as e2:
        logging.debug("pprof -flame failed: %s", e2)
    if not url:
      logging.warning("Failed processing: %s\n%s", perf_data_file, e)
      return None
  if perf_data_file.suffix == ".jitted":
    logging.info("PPROF (with js-symbols):")
  else:
    logging.info("PPROF (no js-symbols):")
  logging.info("  linux-perf:   %s %s", perf_data_file.name, size)
  logging.info("  pprof result: %s", url)
  return url
