# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import pathlib
import threading
import time
from typing import TYPE_CHECKING

from crossbench import helper
from crossbench.probes import base
from crossbench.probes.results import ProbeResult

if TYPE_CHECKING:
  from crossbench.browsers.base import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner import Run


class SystemStatsProbe(base.Probe):
  """
  General-purpose probe to periodically collect system-wide CPU and memory
  stats on unix systems.
  """
  NAME = "system.stats"
  CMD = ("ps", "-a", "-e", "-o", "pcpu,pmem,args", "-r")

  _interval: float

  def __init__(self, *args, interval: float = 1, **kwargs):
    super().__init__(*args, **kwargs)
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

  @classmethod
  def poll(cls, interval: float, path: pathlib.Path,
           event: threading.Event) -> None:
    while not event.is_set():
      # TODO(cbruni): support remote platform
      data = helper.platform.sh_stdout(*cls.CMD)
      out_file = path / f"{time.time()}.txt"
      with out_file.open("w", encoding="utf-8") as f:
        f.write(data)
      time.sleep(interval)

  class Scope(base.Probe.Scope):
    _event: threading.Event
    _poller: threading.Thread

    def setup(self, run: Run) -> None:
      self.results_file.mkdir()

    def start(self, run: Run) -> None:
      self._event = threading.Event()
      assert self.browser_platform == helper.platform, (
          "Remote platforms are not supported yet")
      self._poller = threading.Thread(
          target=SystemStatsProbe.poll,
          args=(self.probe.interval, self.results_file, self._event))
      self._poller.start()

    def stop(self, run: Run) -> None:
      self._event.set()

    def tear_down(self, run: Run) -> ProbeResult:
      return ProbeResult(file=(self.results_file,))
