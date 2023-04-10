# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import dataclasses
import datetime as dt
import enum
import logging
import os
import urllib.request
from typing import (TYPE_CHECKING, Any, Callable, Dict, Iterable, List,
                    Optional, Union)
from urllib.parse import urlparse

from crossbench import helper

if TYPE_CHECKING:
  from crossbench.probes.base import Probe
  from crossbench.runner import Runner


def merge_bool(name: str, left: Optional[bool],
               right: Optional[bool]) -> Optional[bool]:
  if left is None:
    return right
  if right is None:
    return left
  if left != right:
    raise ValueError(f"Conflicting merge values for {name}: "
                     f"{left} vs. {right}")
  return left


NumberT = Union[float, int]


def merge_number_max(name: str, left: Optional[NumberT],
                     right: Optional[NumberT]) -> Optional[NumberT]:
  del name
  if left is None:
    return right
  if right is None:
    return left
  return max(left, right)


def merge_number_min(name: str, left: Optional[NumberT],
                     right: Optional[NumberT]) -> Optional[NumberT]:
  del name
  if left is None:
    return right
  if right is None:
    return left
  return min(left, right)


def merge_str_list(name: str, left: Optional[List[str]],
                   right: Optional[List[str]]) -> Optional[List[str]]:
  del name
  if left is None:
    return right
  if right is None:
    return left
  return left + right


@dataclasses.dataclass(frozen=True)
class HostEnvironmentConfig:
  IGNORE = None

  disk_min_free_space_gib: Optional[float] = IGNORE
  power_use_battery: Optional[bool] = IGNORE
  screen_brightness_percent: Optional[int] = IGNORE
  cpu_max_usage_percent: Optional[float] = IGNORE
  cpu_min_relative_speed: Optional[float] = IGNORE
  system_allow_monitoring: Optional[bool] = IGNORE
  browser_allow_existing_process: Optional[bool] = IGNORE
  browser_is_headless: Optional[bool] = IGNORE
  require_probes: Optional[bool] = IGNORE
  system_forbidden_process_names: Optional[List[str]] = IGNORE
  screen_allow_autobrightness: Optional[bool] = IGNORE

  def merge(self, other: HostEnvironmentConfig) -> HostEnvironmentConfig:
    mergers: Dict[str, Callable[[str, Any, Any], Any]] = {
        "disk_min_free_space_gib": merge_number_max,
        "power_use_battery": merge_bool,
        "screen_brightness_percent": merge_number_max,
        "cpu_max_usage_percent": merge_number_min,
        "cpu_min_relative_speed": merge_number_max,
        "system_allow_monitoring": merge_bool,
        "browser_allow_existing_process": merge_bool,
        "browser_is_headless": merge_bool,
        "require_probes": merge_bool,
        "system_forbidden_process_names": merge_str_list,
        "screen_allow_autobrightness": merge_bool,
    }
    kwargs = {}
    for name, merger in mergers.items():
      self_value = getattr(self, name)
      other_value = getattr(other, name)
      kwargs[name] = merger(name, self_value, other_value)
    return HostEnvironmentConfig(**kwargs)


class ValidationMode(enum.Enum):
  THROW = "throw"
  PROMPT = "prompt"
  WARN = "warn"
  SKIP = "skip"


class ValidationError(Exception):
  pass


_config_default = HostEnvironmentConfig()
_config_strict = HostEnvironmentConfig(
    cpu_max_usage_percent=98,
    cpu_min_relative_speed=1,
    system_allow_monitoring=False,
    browser_allow_existing_process=False,
    require_probes=True,
)
_config_battery = _config_strict.merge(
    HostEnvironmentConfig(power_use_battery=True))
_config_power = _config_strict.merge(
    HostEnvironmentConfig(power_use_battery=False))
_config_catan = _config_strict.merge(
    HostEnvironmentConfig(
        screen_brightness_percent=65,
        system_forbidden_process_names=["terminal", "iterm2"],
        screen_allow_autobrightness=False))


class HostEnvironment:
  """
  HostEnvironment can check and enforce certain settings on a host
  where we run benchmarks.

  Modes:
    skip:     Do not perform any checks
    warn:     Only warn about mismatching host conditions
    enforce:  Tries to auto-enforce conditions and warns about others.
    prompt:   Interactive mode to skip over certain conditions
    fail:     Fast-fail on mismatch
  """

  CONFIGS = {
      "default": _config_default,
      "strict": _config_strict,
      "battery": _config_battery,
      "power": _config_power,
      "catan": _config_catan,
  }

  def __init__(self,
               runner: Runner,
               config: Optional[HostEnvironmentConfig] = None,
               validation_mode: ValidationMode = ValidationMode.THROW):
    self._wait_until = dt.datetime.now()
    self._config = config or HostEnvironmentConfig()
    self._runner = runner
    self._platform = runner.platform
    self._validation_mode = validation_mode

  @property
  def runner(self) -> Runner:
    return self._runner

  @property
  def config(self) -> HostEnvironmentConfig:
    return self._config

  def _add_min_delay(self, seconds: float) -> None:
    end_time = dt.datetime.now() + dt.timedelta(seconds=seconds)
    if end_time > self._wait_until:
      self._wait_until = end_time

  def _wait_min_time(self) -> None:
    delta = self._wait_until - dt.datetime.now()
    if delta > dt.timedelta(0):
      self._platform.sleep(delta)

  def handle_warning(self, message: str) -> None:
    """Process a warning, depending on the requested mode, this will
    - throw an error,
    - log a warning,
    - prompts for continue [Yn], or
    - skips (and just debug logs) a warning.
    If returned True (in the prompt mode) the env validation may continue.
    """
    if self._validation_mode == ValidationMode.SKIP:
      logging.debug("Skipped Runner/Host environment warning: %s", message)
      return
    if self._validation_mode == ValidationMode.WARN:
      logging.warning(message)
      return
    if self._validation_mode == ValidationMode.THROW:
      pass
    elif self._validation_mode == ValidationMode.PROMPT:
      result = input(f"{helper.TTYColor.RED}{message} Continue?"
                     f"{helper.TTYColor.RESET} [Yn]")
      # Accept <enter> as default input to continue.
      if result.lower() != "n":
        return
    else:
      raise ValueError(
          f"Invalid environment validation mode={self._validation_mode}")
    raise ValidationError(
        f"Runner/Host environment requests cannot be fulfilled: {message}")

  def validate_url(self, url: str) -> bool:
    try:
      result = urlparse(url)
      if not all([
          result.scheme in ["file", "http", "https"], result.netloc, result.path
      ]):
        return False
      if self._validation_mode != ValidationMode.PROMPT:
        return True
      with urllib.request.urlopen(url) as request:
        if request.getcode() == 200:
          return True
    except urllib.error.URLError:
      pass
    return False

  def _check_system_monitoring(self) -> None:
    # TODO(cbruni): refactor to use list_... and disable_system_monitoring api
    if self._platform.is_macos:
      self._check_crowdstrike()

  def _check_crowdstrike(self) -> None:
    """Crowdstrike security monitoring (for googlers go/crowdstrike-falcon) can
    have quite terrible overhead for each file-access. Disable it to reduce
    flakiness. """
    is_disabled = False
    force_disable = self._config.system_allow_monitoring is False
    try:
      # TODO(cbruni): refactor to use list_... and disable_system_monitoring api
      is_disabled = self._platform.check_system_monitoring(force_disable)
      if force_disable:
        # Add cool-down period, crowdstrike caused CPU usage spikes
        self._add_min_delay(5)
    except helper.SubprocessError as e:
      self.handle_warning(
          "Could not disable go/crowdstrike-falcon monitor which can cause"
          f" high background CPU usage: {e}")
      return
    if not is_disabled:
      self.handle_warning(
          "Crowdstrike monitoring is running, "
          "which can impact startup performance drastically.\n"
          "Use the following command to disable it manually:\n"
          "sudo /Applications/Falcon.app/Contents/Resources/falconctl unload\n")

  def _check_disk_space(self) -> None:
    limit = self._config.disk_min_free_space_gib
    if limit is HostEnvironmentConfig.IGNORE:
      return
    # Check the remaining disk space on the FS where we write the results.
    usage = self._platform.disk_usage(self._runner.out_dir)
    free_gib = round(usage.free / 1024 / 1024 / 1024, 2)
    if free_gib < limit:
      self.handle_warning(
          f"Only {free_gib}GiB disk space left, expected at least {limit}GiB.")

  def _check_power(self) -> None:
    use_battery = self._config.power_use_battery
    if use_battery is HostEnvironmentConfig.IGNORE:
      return
    battery_probes: List[Probe] = []
    # Certain probes may require battery power:
    for probe in self._runner.probes:
      if probe.BATTERY_ONLY:
        battery_probes.append(probe)
    if not use_battery and battery_probes:
      probes_str = ",".join(probe.name for probe in battery_probes)
      self.handle_warning("Requested battery_power=False, "
                          f"but probes={probes_str} require battery power.")
    sys_use_battery = self._platform.is_battery_powered
    if sys_use_battery != use_battery:
      self.handle_warning(
          f"Expected battery_power={use_battery}, "
          f"but the system reported battery_power={sys_use_battery}")

  def _check_cpu_usage(self) -> None:
    max_cpu_usage = self._config.cpu_max_usage_percent
    if max_cpu_usage is HostEnvironmentConfig.IGNORE:
      return
    cpu_usage_percent = round(100 * self._platform.cpu_usage(), 1)
    if cpu_usage_percent > max_cpu_usage:
      self.handle_warning(f"CPU usage={cpu_usage_percent}% is higher than "
                          f"requested max={max_cpu_usage}%.")

  def _check_cpu_temperature(self) -> None:
    min_relative_speed = self._config.cpu_min_relative_speed
    if min_relative_speed is HostEnvironmentConfig.IGNORE:
      return
    cpu_speed = self._platform.get_relative_cpu_speed()
    if cpu_speed < min_relative_speed:
      self.handle_warning("CPU thermal throttling is active. "
                          f"Relative speed is {cpu_speed}, "
                          f"but expected at least {min_relative_speed}.")

  def _check_forbidden_system_process(self) -> None:
    # Verify that no terminals are running.
    # They introduce too much overhead. (As measured with powermetrics)
    system_forbidden_process_names = self._config.system_forbidden_process_names
    if system_forbidden_process_names is HostEnvironmentConfig.IGNORE:
      return
    if process_found := self._platform.process_running(system_forbidden_process_names):
      raise Exception(f"Process:{process_found} found."
                      "Make sure not to have a terminal opened. Use SSH.")

  def _check_screen_autobrightness(self) -> None:
    auto_brightness = self._config.screen_allow_autobrightness
    if auto_brightness is HostEnvironmentConfig.IGNORE:
      return
    if self._platform.check_autobrightness():
      raise Exception("Auto-brightness was found to be ON. "
                      "Desactivate it in 'System Preferences/Displays'")

  def _check_cpu_power_mode(self) -> bool:
    # TODO Implement checks for performance mode
    return True

  def _check_running_binaries(self) -> None:
    if self._config.browser_allow_existing_process:
      return
    browser_binaries = helper.group_by(
        self._runner.browsers, key=lambda browser: str(browser.path))
    own_pid = os.getpid()
    for proc_info in self._platform.processes(["cmdline", "exe", "pid",
                                               "name"]):
      # Skip over this python script which might have the binary path as
      # part of the command line invocation.
      if proc_info["pid"] == own_pid:
        continue
      cmdline = " ".join(proc_info["cmdline"] or "")
      exe = proc_info["exe"]
      for binary, browsers in browser_binaries.items():
        # Add a white-space to get less false-positives
        if f"{binary} " not in cmdline and binary != exe:
          continue
        # Use the first in the group
        browser = browsers[0]
        logging.debug("Binary=%s", binary)
        logging.debug("PS status output:")
        logging.debug("proc(pid=%s, name=%s, cmd=%s)", proc_info["pid"],
                      proc_info["name"], cmdline)
        self.handle_warning(
            f"{browser.app_name} {browser.version} seems to be already running."
        )

  def _check_screen_brightness(self) -> None:
    brightness = self._config.screen_brightness_percent
    if brightness is HostEnvironmentConfig.IGNORE:
      return
    assert 0 <= brightness <= 100, f"Invalid brightness={brightness}"
    self._platform.set_main_display_brightness(brightness)
    current = self._platform.get_main_display_brightness()
    if current != brightness:
      self.handle_warning(f"Requested main display brightness={brightness}%, "
                          "but got {brightness}%")

  def _check_headless(self) -> None:
    requested_headless = self._config.browser_is_headless
    if requested_headless is HostEnvironmentConfig.IGNORE:
      return
    if self._platform.is_linux and not requested_headless:
      # Check that the system can run browsers with a UI.
      if not self._platform.has_display:
        self.handle_warning("Requested browser_is_headless=False, "
                            "but no DISPLAY is available to run with a UI.")
    # Check that browsers are running in the requested display mode:
    for browser in self._runner.browsers:
      if browser.is_headless != requested_headless:
        self.handle_warning(
            f"Requested browser_is_headless={requested_headless},"
            f"but browser {browser.unique_name} has conflicting "
            f"headless={browser.is_headless}.")

  def _check_probes(self) -> None:
    for probe in self._runner.probes:
      try:
        probe.pre_check(self)
      except Exception as e:
        raise ValidationError(
            f"Probe='{probe.NAME}' validation failed: {e}") from e
    require_probes = self._config.require_probes
    if require_probes is HostEnvironmentConfig.IGNORE:
      return
    if self._config.require_probes and not self._runner.probes:
      self.handle_warning("No probes specified.")

  def _check_results_dir(self) -> None:
    results_dir = self._runner.out_dir.parent
    if not results_dir.exists():
      return
    results = [path for path in results_dir.iterdir() if path.is_dir()]
    num_results = len(results)
    if num_results < 20:
      return
    message = (f"Found {num_results} existing crossbench results. "
               f"Consider cleaning stale results in '{results_dir}'")
    if num_results > 50:
      logging.error(message)
    else:
      logging.warning(message)

  def setup(self) -> None:
    self.validate()

  def validate(self) -> None:
    logging.info("-" * 80)
    if self._validation_mode == ValidationMode.SKIP:
      logging.info("VALIDATE ENVIRONMENT: SKIP")
      return
    message = "VALIDATE ENVIRONMENT"
    if self._validation_mode != ValidationMode.WARN:
      message += " (--env-validation=warn for soft warnings)"
    message += ": %s"
    logging.info(message, self._validation_mode.name)
    self._check_system_monitoring()
    self._check_power()
    self._check_disk_space()
    self._check_cpu_usage()
    self._check_cpu_temperature()
    self._check_cpu_power_mode()
    self._check_running_binaries()
    self._check_screen_brightness()
    self._check_headless()
    self._check_results_dir()
    self._check_probes()
    self._wait_min_time()
    self._check_forbidden_system_process()
    self._check_screen_autobrightness()

  def check_installed(self,
                      binaries: Iterable[str],
                      message: str = "Missing binaries: {}") -> None:
    assert not isinstance(binaries, str), "Expected iterable of strings."
    missing_binaries = list(
        binary for binary in binaries if not self._platform.which(binary))
    if missing_binaries:
      self.handle_warning(message.format(missing_binaries))

  def check_sh_success(self, *args,
                       message: str = "Could not execute: {}") -> None:
    assert args, "Missing sh arguments"
    try:
      assert self._platform.sh_stdout(*args, quiet=True)
    except helper.SubprocessError as e:
      self.handle_warning(message.format(e))
