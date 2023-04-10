# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import json
import logging
import multiprocessing
import pathlib
import signal
import subprocess
import time
from typing import TYPE_CHECKING, Iterable, List, Optional

from crossbench import helper
from crossbench.probes.base import Probe, ProbeConfigParser
from crossbench.probes.results import ProbeResult
from crossbench.probes.v8.log import V8LogProbe
from crossbench.browsers.chromium import Chromium

if TYPE_CHECKING:
  from crossbench.browsers.base import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner import Run, Runner, BrowsersRunGroup


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

  JS_FLAGS_PERF = (
      "--perf-prof",
      "--no-write-protect-code-memory",
  )
  _INTERPRETED_FRAMES_FLAG = "--interpreted-frames-native-stack"
  IS_GENERAL_PURPOSE = True

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
        "v8_interpreted_frames",
        type=bool,
        default=True,
        help=(
            f"Chrome-only: Sets the {cls._INTERPRETED_FRAMES_FLAG} flag for "
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
               browser_process: bool = False):
    super().__init__()
    self._sample_js = js
    self._sample_browser_process = browser_process
    self._run_pprof = pprof
    self._expose_v8_interpreted_frames = v8_interpreted_frames
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
    if self.browser_platform.is_linux:
      assert isinstance(browser, Chromium), (
          f"Expected Chromium-based browser, found {type(browser)}.")
      self._attach_linux(browser)

  def pre_check(self, env: HostEnvironment) -> None:
    super().pre_check(env)
    if self.browser_platform.is_linux:
      env.check_installed(binaries=["pprof"])
      assert self.browser_platform.which("perf"), "Please install linux-perf"
    elif self.browser_platform.is_macos:
      assert self.browser_platform.which("xctrace"), (
          "Please install Xcode to use xctrace")
    if self._run_pprof:
      try:
        self.browser_platform.sh(self.browser_platform.which("gcertstatus"))
        return
      except helper.SubprocessError:
        env.handle_warning("Please run gcert for generating pprof results")
    # Only Linux-perf results can be merged
    if self.browser_platform.is_macos and env.runner.repetitions > 1:
      env.handle_warning(f"Probe={self.NAME} cannot merge data over multiple "
                         f"repetitions={env.runner.repetitions}.")

  def _attach_linux(self, browser: Chromium) -> None:
    if self._sample_js:
      browser.js_flags.update(self.JS_FLAGS_PERF)
      if self._expose_v8_interpreted_frames:
        browser.js_flags.set(self._INTERPRETED_FRAMES_FLAG)
    cmd = pathlib.Path(__file__).parent / "linux-perf-chrome-renderer-cmd.sh"
    assert not self.browser_platform.is_remote, (
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

  def _log_results(self, runs: Iterable[Run]):
    filtered_runs = list(run for run in runs if self in run.results)
    if not filtered_runs:
      return
    logging.info("-" * 80)
    logging.info("Profiling results:")
    logging.info("  *.perf.data: 'perf report -i $FILE'")
    logging.info("- " * 40)
    for i, run in enumerate(filtered_runs):
      self._log_run_result_summary(run, i)

  def _log_run_result_summary(self, run: Run, i: int) -> None:
    if self not in run.results:
      return
    cwd = pathlib.Path.cwd()
    urls = run.results[self].url_list
    perf_files = run.results[self].file_list
    if not urls and not perf_files:
      return
    logging.info("Run %d: %s", i + 1, run.name)
    if urls:
      largest_perf_file = perf_files[0]
      logging.info("    %s", urls[0])
    if perf_files:
      largest_perf_file = perf_files[0]
      logging.info("    %s : %s", largest_perf_file.relative_to(cwd),
                   helper.get_file_size(largest_perf_file))
      if len(perf_files) > 1:
        logging.info("    %s/*.perf.data*: %d more files",
                     largest_perf_file.parent.relative_to(cwd), len(perf_files))

  def get_scope(self, run: Run) -> Probe.Scope:
    if self.browser_platform.is_linux:
      return self.LinuxProfilingScope(self, run)
    if self.browser_platform.is_macos:
      return self.MacOSProfilingScope(self, run)
    raise Exception("Invalid platform")

  class MacOSProfilingScope(Probe.Scope):
    _process: subprocess.Popen

    def __init__(self, *args, **kwargs):
      super().__init__(*args, **kwargs)
      self._default_results_file = self.results_file.parent / "profile.trace"

    def start(self, run: Run) -> None:
      self._process = self.browser_platform.popen("xctrace", "record",
                                                  "--template", "Time Profiler",
                                                  "--all-processes", "--output",
                                                  self.results_file)
      # xctrace takes some time to start up
      time.sleep(3)

    def stop(self, run: Run) -> None:
      # Needs to be SIGINT for xctrace, terminate won't work.
      self._process.send_signal(signal.SIGINT)

    def tear_down(self, run: Run) -> ProbeResult:
      while self._process.poll() is None:
        time.sleep(1)
      return ProbeResult(file=(self.results_file,))

  class LinuxProfilingScope(Probe.Scope):
    PERF_DATA_PATTERN = "*.perf.data"
    TEMP_FILE_PATTERNS = (
        "*.perf.data.jitted",
        "jitted-*.so",
        "jit-*.dump",
    )

    def __init__(self, probe: ProfilingProbe, run: Run):
      super().__init__(probe, run)
      self._perf_process = None

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

      perf_files = helper.sort_by_file_size(
          run.out_dir.glob(self.PERF_DATA_PATTERN))
      try:
        if not self.probe.run_pprof or not self.browser_platform.which("gcert"):
          return ProbeResult(file=perf_files)
        pprof_inputs = perf_files
        if self.probe.sample_js:
          pprof_inputs = self._inject_v8_symbols(run, perf_files)
        urls = self._export_to_pprof(run, pprof_inputs)
      finally:
        self._clean_up_temp_files(run)
      logging.debug("Profiling results: %s", urls)
      return ProbeResult(url=urls, file=perf_files)

    def _inject_v8_symbols(self, run: Run, perf_files: List[pathlib.Path]
                          ) -> List[pathlib.Path]:
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
          assert self.browser_platform == helper.platform
          with multiprocessing.Pool() as pool:
            perf_jitted_files = list(
                pool.imap(linux_perf_probe_inject_v8_symbols, perf_files))
        return [file for file in perf_jitted_files if file is not None]

    def _export_to_pprof(self, run: Run,
                         perf_files: List[pathlib.Path]) -> List[str]:
      run_details_json = json.dumps(run.get_browser_details_json())
      with run.actions(
          f"Probe {self.probe.name}: "
          f"exporting {len(perf_files)} profiles to pprof (slow)",
          verbose=True), helper.Spinner():
        self.browser_platform.sh(
            "gcertstatus >&/dev/null || "
            "(echo 'Authenticating with gcert:'; gcert)",
            shell=True)
        items = zip(perf_files, [run_details_json] * len(perf_files))
        urls = []
        if self.browser_platform.is_remote:
          # Use loop, as we cannot easily serialize the remote platform.
          for perf_data_file, run_details in items:
            url = linux_perf_probe_pprof(perf_data_file, run_details,
                                         self.browser_platform)
            if url:
              urls.append(url)
        else:
          assert self.browser_platform == helper.platform
          with multiprocessing.Pool() as pool:
            urls = [
                url for url in pool.starmap(linux_perf_probe_pprof, items)
                if url
            ]
        try:
          if perf_files:
            # TODO: Add "combined" profile again
            pass
        except Exception as e:  # pylint: disable=broad-except
          logging.debug("Failed to run pprof: %s", e)
        return urls

    def _clean_up_temp_files(self, run: Run) -> None:
      for pattern in self.TEMP_FILE_PATTERNS:
        for file in run.out_dir.glob(pattern):
          file.unlink()


def linux_perf_probe_inject_v8_symbols(
    perf_data_file: pathlib.Path,
    platform: Optional[helper.Platform] = None) -> Optional[pathlib.Path]:
  assert perf_data_file.is_file()
  output_file = perf_data_file.with_suffix(".data.jitted")
  assert not output_file.exists()
  platform = platform or helper.platform
  try:
    platform.sh("perf", "inject", "--jit", f"--input={perf_data_file}",
                f"--output={output_file}")
  except helper.SubprocessError as e:
    KB = 1024
    if perf_data_file.stat().st_size > 200 * KB:
      logging.warning("Failed processing: %s\n%s", perf_data_file, e)
    else:
      # TODO: investigate why almost all small perf.data files fail
      logging.debug("Failed processing small profile (likely empty): %s\n%s",
                    perf_data_file, e)
  if not output_file.exists:
    return None
  return output_file


def linux_perf_probe_pprof(perf_data_file: pathlib.Path,
                           run_details: str,
                           platform: Optional[helper.Platform] = None
                          ) -> Optional[str]:
  size = helper.get_file_size(perf_data_file)
  platform = platform or helper.platform
  url = ""
  try:
    url = platform.sh_stdout(
        "pprof",
        "-flame",
        f"-add_comment={run_details}",
        perf_data_file,
    ).strip()
  except helper.SubprocessError as e:
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
      except helper.SubprocessError:
        pass
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
