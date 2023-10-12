# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
import time
import logging
import pathlib
import threading
from typing import TYPE_CHECKING, Sequence

from crossbench.probes import probe
from crossbench.probes.results import LocalProbeResult, ProbeResult

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench import plt
  from crossbench.runner.run import Run


class SystemStatsProbe(probe.Probe):
  """
  General-purpose probe to periodically collect system-wide CPU and memory
  stats on unix systems.
  """
  NAME = "system.stats"
  CMD = ("ps", "-a", "-e", "-o", "pcpu,pmem,args", "-r")

  _interval: float

  def __init__(self, interval: float = 0.1) -> None:
    super().__init__()
    self._interval = interval

  @property
  def interval(self) -> float:
    return self._interval

  def is_compatible(self, browser: Browser) -> bool:
    return not browser.platform.is_remote and (browser.platform.is_linux or
                                               browser.platform.is_macos)

  def pre_check(self, env: HostEnvironment) -> None:
    super().pre_check(env)
    if env.runner.repetitions != 1:
      env.handle_warning(f"Probe={self.NAME} cannot merge data over multiple "
                         f"repetitions={env.runner.repetitions}.")


  def get_scope(self, run: Run) -> SystemStatsProbeScope:
    return SystemStatsProbeScope(self, run)


class SystemStatsProbeScope(probe.ProbeScope[SystemStatsProbe]):
  _poller: CMDPoller

  def setup(self, run: Run) -> None:
    self.result_path.mkdir()

  def start(self, run: Run) -> None:
    self._poller = CMDPoller(self.browser_platform, self.probe.CMD,
                             self.probe.interval, self.result_path)
    self._poller.start()

  def stop(self, run: Run) -> None:
    self._poller.stop()

  def tear_down(self, run: Run) -> ProbeResult:
    return LocalProbeResult(file=(self.result_path,))


class CMDPoller(threading.Thread):

  def __init__(self, platform: plt.Platform, cmd: Sequence[str],
               interval: float, path: pathlib.Path):
    super().__init__()
    self._platform = platform
    self._cmd = cmd
    self._path = path
    if interval < 0.1:
      raise ValueError("Poller interval should be more than 0.1s for accuracy, "
                       f"but got {interval}s")
    self._interval = interval
    self._event = threading.Event()

  def stop(self) -> None:
    self._event.set()
    self.join()

  def run(self) -> None:
    start_time = time.monotonic_ns()
    while not self._event.is_set():
      poll_start = dt.datetime.now()

      data = self._platform.sh_stdout(*self._cmd)
      datetime_str = poll_start.strftime("%Y-%m-%d_%H%M%S_%f")
      out_file = self._path / f"{datetime_str}.txt"
      with out_file.open("w", encoding="utf-8") as f:
        f.write(data)

      poll_end = dt.datetime.now()
      diff = (poll_end - poll_start).total_seconds()
      if diff > self._interval:
        logging.warning("Poller command took longer than expected %fs: %s",
                        self._interval, self._cmd)

      # Calculate wait_time against fixed start time to avoid drifting.
      total_time = ((time.monotonic_ns() - start_time) / 10.0**9)
      wait_time = self._interval - (total_time % self._interval)
      self._event.wait(wait_time)
