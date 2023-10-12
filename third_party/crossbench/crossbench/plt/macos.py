# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import ctypes
import json
import logging
import pathlib
import plistlib
import traceback as tb
from subprocess import SubprocessError
from typing import Any, Dict, Optional, Tuple

import psutil

from .posix import PosixPlatform


class MacOSPlatform(PosixPlatform):
  SEARCH_PATHS = (
      pathlib.Path("."),
      pathlib.Path("/Applications"),
      pathlib.Path.home() / "Applications",
  )

  @property
  def is_macos(self) -> bool:
    return True

  @property
  def name(self) -> str:
    return "macos"

  @property
  def version(self) -> str:
    if not self._version:
      self._version = self.sh_stdout("sw_vers", "-productVersion").strip()
    return self._version

  @property
  def device(self) -> str:
    if not self._device:
      self._device = self.sh_stdout("sysctl",
                                    "hw.model").strip().split(maxsplit=1)[1]
    return self._device

  @property
  def cpu(self) -> str:
    if not self._cpu:
      brand = self.sh_stdout("sysctl", "-n", "machdep.cpu.brand_string").strip()
      cores = self.sh_stdout("sysctl", "-n", "machdep.cpu.core_count").strip()
      self._cpu = f"{brand} {cores} cores"
    return self._cpu

  @property
  def is_battery_powered(self) -> bool:
    if not self.is_remote:
      return super().is_battery_powered
    return "Battery Power" in self.sh_stdout("pmset", "-g", "batt")

  def _find_app_binary_path(self, app_path: pathlib.Path) -> pathlib.Path:
    assert app_path.suffix == ".app"
    bin_path = app_path / "Contents" / "MacOS" / app_path.stem
    if self.exists(bin_path):
      return bin_path
    assert not self.is_remote, "Unsupported operation on remote platform"
    binaries = [path for path in bin_path.parent.iterdir() if path.is_file()]
    if len(binaries) == 1:
      return binaries[0]
    # Fallback to read plist
    plist_path = app_path / "Contents" / "Info.plist"
    assert self.is_file(plist_path), (
        f"Could not find Info.plist in app bundle: {app_path}")
    with plist_path.open("rb") as f:
      plist = plistlib.load(f)
    bin_path = (
        app_path / "Contents" / "MacOS" /
        plist.get("CFBundleExecutable", app_path.stem))
    if self.is_file(bin_path):
      return bin_path
    raise ValueError(f"Invalid number of binaries candidates found: {binaries}")

  def search_binary(self, app_or_bin: pathlib.Path) -> Optional[pathlib.Path]:
    if not app_or_bin.parts:
      raise ValueError("Got empty path")
    if result_path := self.which(str(app_or_bin)):
      assert self.exists(result_path), f"{result_path} does not exist."
      return result_path
    is_app = app_or_bin.suffix == ".app"
    for search_path in self.SEARCH_PATHS:
      # Recreate Path object for easier pyfakefs testing
      result_path = pathlib.Path(search_path) / app_or_bin
      if not is_app:
        if self.is_file(result_path):
          return result_path
        continue
      if not self.is_dir(result_path):
        continue
      result_path = self._find_app_binary_path(result_path)
      if self.exists(result_path):
        return result_path
    return None

  def search_app(self, app_or_bin: pathlib.Path) -> Optional[pathlib.Path]:
    if not app_or_bin.parts:
      raise ValueError("Got empty path")
    assert not self.is_remote, "Unsupported operation on remote platform"
    if app_or_bin.suffix != ".app":
      raise ValueError("Expected app name with '.app' suffix, "
                       f"but got: '{app_or_bin.name}'")
    binary = self.search_binary(app_or_bin)
    if not binary:
      return None
    # input: /Applications/Safari.app/Contents/MacOS/Safari
    # output: /Applications/Safari.app
    app_path = binary.parents[2]
    assert app_path.suffix == ".app"
    assert app_path.is_dir()
    return app_path

  def app_version(self, app_or_bin: pathlib.Path) -> str:
    assert app_or_bin.exists(), f"Binary {app_or_bin} does not exist."

    app_path = None
    for current in (app_or_bin, *app_or_bin.parents):
      if current.suffix == ".app" and current.stem == app_or_bin.stem:
        app_path = current
        break
    if not app_path:
      # Most likely just a cli tool"
      return self.sh_stdout(app_or_bin, "--version").strip()
    version_string = self.sh_stdout("mdls", "-name", "kMDItemVersion",
                                    app_path).strip()
    logging.debug("version_string = %s %s", version_string, app_path)
    # Filter output: 'kMDItemVersion = "14.1"' => '"14.1"'
    _, version_string = version_string.split(" = ", maxsplit=1)
    if version_string != "(null)":
      # Strip quotes: '"14.1"' => '14.1'
      return version_string[1:-1]
    # Backup solution use the binary (not the .app bundle) with --version.
    maybe_bin_path: Optional[pathlib.Path] = app_or_bin
    if app_or_bin.suffix == ".app":
      maybe_bin_path = self.search_binary(app_or_bin)
    if not maybe_bin_path:
      raise ValueError(f"Could not extract app version: {app_or_bin}")
    try:
      return self.sh_stdout(maybe_bin_path, "--version").strip()
    except SubprocessError as e:
      raise ValueError(f"Could not extract app version: {app_or_bin}") from e

  def exec_apple_script(self, script: str, *args: str) -> str:
    if args:
      script = f"""on run argv
        {script.strip()}
      end run"""
    return self.sh_stdout("/usr/bin/osascript", "-e", script, *args)

  def foreground_process(self) -> Optional[Dict[str, Any]]:
    foreground_process_info = self.sh_stdout("lsappinfo", "front").strip()
    if not foreground_process_info:
      return None
    foreground_info = self.sh_stdout("lsappinfo", "info", "-only", "pid",
                                     foreground_process_info).strip()
    _, pid = foreground_info.split("=")
    if pid and pid.isdigit():
      return psutil.Process(int(pid)).as_dict()
    return None

  def get_relative_cpu_speed(self) -> float:
    try:
      lines = self.sh_stdout("pmset", "-g", "therm").split()
      for index, line in enumerate(lines):
        if line == "CPU_Speed_Limit":
          return int(lines[index + 2]) / 100.0
    except SubprocessError:
      pass
    logging.debug("Could not get relative CPU speed: %s", tb.format_exc())
    return 1

  def system_details(self) -> Dict[str, Any]:
    details = super().system_details()
    details.update({
        "system_profiler":
            self.sh_stdout("system_profiler", "SPHardwareDataType"),
        "sysctl_machdep_cpu":
            self.sh_stdout("sysctl", "machdep.cpu"),
        "sysctl_hw":
            self.sh_stdout("sysctl", "hw"),
    })
    return details

  def check_system_monitoring(self, disable: bool = False) -> bool:
    return self.check_crowdstrike(disable)

  def check_autobrightness(self) -> bool:
    output = self.sh_stdout("system_profiler", "SPDisplaysDataType",
                            "-json").strip()
    data = json.loads(output)
    if spdisplays_data := data.get("SPDisplaysDataType"):
      for data in spdisplays_data:
        if spdisplays_ndrvs := data.get("spdisplays_ndrvs"):
          for display in spdisplays_ndrvs:
            if auto_brightness := display.get("spdisplays_ambient_brightness"):
              return auto_brightness == "spdisplays_yes"
        raise ValueError(
            "Could not find 'spdisplays_ndrvs' from SPDisplaysDataType")
    raise ValueError("Could not get 'SPDisplaysDataType' form system profiler")

  def check_crowdstrike(self, disable: bool = False) -> bool:
    falconctl = pathlib.Path(
        "/Applications/Falcon.app/Contents/Resources/falconctl")
    if not falconctl.exists():
      logging.debug("You're fine, falconctl or %s are not installed.",
                    falconctl)
      return True
    if not disable:
      for process in self.processes(attrs=["exe"]):
        exe = process["exe"]
        if exe and exe.endswith("/com.crowdstrike.falcon.Agent"):
          return False
      return True
    try:
      logging.warning("Checking falcon sensor status:")
      status = self.sh_stdout("sudo", falconctl, "stats", "agent_info")
    except SubprocessError as e:
      logging.debug("Could not probe falconctl, assuming it's not running: %s",
                    e)
      return True
    if "operational: true" not in status:
      # Early return if not running, no need to disable the sensor.
      return True
    # Try disabling the process
    logging.warning("Disabling crowdstrike monitoring:")
    self.sh("sudo", falconctl, "unload")
    return True

  def _get_display_service(self) -> Tuple[ctypes.CDLL, Any]:
    core_graphics = ctypes.CDLL(
        "/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics")
    main_display = core_graphics.CGMainDisplayID()
    display_services = ctypes.CDLL(
        "/System/Library/PrivateFrameworks/DisplayServices.framework"
        "/DisplayServices")
    display_services.DisplayServicesSetBrightness.argtypes = [
        ctypes.c_int, ctypes.c_float
    ]
    display_services.DisplayServicesGetBrightness.argtypes = [
        ctypes.c_int, ctypes.POINTER(ctypes.c_float)
    ]
    return display_services, main_display

  def set_main_display_brightness(self, brightness_level: int) -> None:
    """Sets the main display brightness at the specified percentage by
    brightness_level.

    This function imitates the open-source "brightness" tool at
    https://github.com/nriley/brightness.
    Since the benchmark doesn't care about older MacOSen, multiple displays
    or other complications that tool has to consider, setting the brightness
    level boils down to calling this function for the main display.

    Args:
      brightness_level: Percentage at which we want to set screen brightness.

    Raises:
      AssertionError: An error occurred when we tried to set the brightness
    """
    display_services, main_display = self._get_display_service()
    ret = display_services.DisplayServicesSetBrightness(main_display,
                                                        brightness_level / 100)
    assert ret == 0

  def get_main_display_brightness(self) -> int:
    """Gets the current brightness level of the main display .

    This function imitates the open-source "brightness" tool at
    https://github.com/nriley/brightness.
    Since the benchmark doesn't care about older MacOSen, multiple displays
    or other complications that tool has to consider, setting the brightness
    level boils down to calling this function for the main display.

    Returns:
      An int of the current percentage value of the main screen brightness

    Raises:
      AssertionError: An error occurred when we tried to set the brightness
    """

    display_services, main_display = self._get_display_service()
    display_brightness = ctypes.c_float()
    ret = display_services.DisplayServicesGetBrightness(
        main_display, ctypes.byref(display_brightness))
    assert ret == 0
    return round(display_brightness.value * 100)
