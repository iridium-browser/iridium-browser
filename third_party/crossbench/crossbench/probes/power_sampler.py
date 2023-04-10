# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import csv
import logging
import pathlib
import subprocess
from typing import TYPE_CHECKING, Optional, Sequence, Tuple

from crossbench.probes.base import Probe, ProbeConfigParser

if TYPE_CHECKING:
  from crossbench.browsers.base import Browser
  from crossbench.env import HostEnvironment
  from crossbench.probes.results import ProbeResult
  from crossbench.runner import Run


class PowerSamplerProbe(Probe):
  """
  Probe for chrome's power_sampler helper binary to collect MacOS specific
  battery and system usage metrics.
  Note that the battery monitor only gets a value infrequently (> 30s), thus
  this probe mostly makes sense for long-running benchmarks.
  """

  NAME = "powersampler"
  BATTERY_ONLY = True
  SAMPLERS = ("smc", "user_idle_level", "main_display")

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument("bin_path", type=pathlib.Path)
    parser.add_argument("sampling_interval", type=int, default=10)
    parser.add_argument(
        "samplers",
        type=str,
        choices=cls.SAMPLERS,
        default=cls.SAMPLERS,
        is_list=True)
    parser.add_argument(
        "wait_for_battery",
        type=bool,
        default=True,
        help="Wait for the first non-100% battery measurement before "
        "running the benchmark to ensure accurate readings.")
    return parser

  def __init__(self,
               bin_path: pathlib.Path,
               sampling_interval: int = 10,
               samplers: Sequence[str] = SAMPLERS,
               wait_for_battery: bool = True):
    super().__init__()
    self._bin_path = bin_path
    assert self._bin_path.exists(), ("Could not find power_sampler binary at "
                                     f"'{self._bin_path}'")
    self._sampling_interval = sampling_interval
    assert sampling_interval > 0, (
        f"Invalid sampling_interval={sampling_interval}")
    assert "battery" not in samplers
    self._samplers = tuple(samplers)
    self._wait_for_battery = wait_for_battery

  @property
  def bin_path(self) -> pathlib.Path:
    return self._bin_path

  @property
  def sampling_interval(self) -> int:
    return self._sampling_interval

  @property
  def samplers(self) -> Tuple[str, ...]:
    return self._samplers

  @property
  def wait_for_battery(self) -> bool:
    return self._wait_for_battery

  def pre_check(self, env: HostEnvironment) -> None:
    super().pre_check(env)
    if not self.browser_platform.is_battery_powered:
      env.handle_warning("Power Sampler only works on battery power!")
    # TODO() warn when external monitors are connected
    # TODO() warn about open terminals

  def is_compatible(self, browser: Browser) -> bool:
    # For now only supported on MacOs
    return browser.platform.is_macos

  class Scope(Probe.Scope):

    def __init__(self, probe: PowerSamplerProbe, run: Run):
      super().__init__(probe, run)
      self._bin_path = probe.bin_path
      self._active_user_process: Optional[subprocess.Popen] = None
      self._battery_process: Optional[subprocess.Popen] = None
      self._power_process: Optional[subprocess.Popen] = None
      self._battery_output = self.results_file.with_suffix(".battery.json")
      self._power_output = self.results_file.with_suffix(".power.json")

    def setup(self, run: Run) -> None:
      self._active_user_process = self.browser_platform.popen(
          self._bin_path,
          "--no-samplers",
          "--simulate-user-active",
          stdout=subprocess.DEVNULL)
      assert self._active_user_process is not None, (
          "Could not start active user background sa")
      if self.probe.wait_for_battery:
        self._wait_for_battery_not_full(run)

    def start(self, run: Run) -> None:
      assert self._active_user_process is not None
      self._battery_process = self.browser_platform.popen(
          self._bin_path,
          "--sample-on-notification",
          "--samplers=battery",
          f"--json-output-file={self._battery_output}",
          stdout=subprocess.DEVNULL)
      assert self._battery_process is not None, (
          "Could not start battery sampler")
      self._power_process = self.browser_platform.popen(
          self._bin_path,
          f"--sample-interval={self.probe.sampling_interval}",
          f"--samplers={','.join(self.probe.samplers)}",
          f"--json-output-file={self._power_output}",
          f"--resource-coalition-pid={self.browser_pid}",
          stdout=subprocess.DEVNULL)
      assert self._power_process is not None, "Could not start power sampler"

    def stop(self, run: Run) -> None:
      if self._power_process:
        self._power_process.terminate()
      if self._battery_process:
        self._battery_process.terminate()

    def tear_down(self, run: Run) -> ProbeResult:
      if self._power_process:
        self._power_process.kill()
      if self._battery_process:
        self._battery_process.kill()
      if self._active_user_process:
        self._active_user_process.terminate()
      return ProbeResult(file=(self._power_output, self._battery_output))

    def _wait_for_battery_not_full(self, run: Run) -> None:
      """
      Empirical evidence has shown that right after a full battery charge, the
      current capacity stays equal to the maximum capacity for several minutes,
      despite the fact that power is definitely consumed. To ensure that power
      consumption estimates from battery level are meaningful, wait until the
      battery is no longer reporting being fully charged before crossbench.
      """
      del run
      logging.info("POWER SAMPLER: Waiting for non-100% battery or "
                   "initial sample to synchronize")
      while True:
        assert self.browser_platform.is_battery_powered, (
            "Cannot wait for draining if power is connected.")

        power_sampler_output = self.browser_platform.sh_stdout(
            self._bin_path, "--sample-on-notification", "--samplers=battery",
            "--sample-count=1")

        for row in csv.DictReader(power_sampler_output.splitlines()):
          max_capacity = float(row["battery_max_capacity(Ah)"])
          current_capacity = float(row["battery_current_capacity(Ah)"])
          percent = 100 * current_capacity / max_capacity
          logging.debug("POWER SAMPLER: Battery level is %.2f%%", percent)
          if max_capacity != current_capacity:
            return
