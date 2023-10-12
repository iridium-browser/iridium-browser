# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import os
import pathlib
from typing import Any, Dict, Optional

from .posix import PosixPlatform


class LinuxPlatform(PosixPlatform):
  SEARCH_PATHS = (
      pathlib.Path("."),
      pathlib.Path("/usr/local/sbin"),
      pathlib.Path("/usr/local/bin"),
      pathlib.Path("/usr/sbin"),
      pathlib.Path("/usr/bin"),
      pathlib.Path("/sbin"),
      pathlib.Path("/bin"),
      pathlib.Path("/opt/google"),
  )

  @property
  def is_linux(self) -> bool:
    return True

  @property
  def name(self) -> str:
    return "linux"

  def check_system_monitoring(self, disable: bool = False) -> bool:
    return True

  @property
  def device(self) -> str:
    if not self._device:
      vendor = self.cat("/sys/devices/virtual/dmi/id/sys_vendor").strip()
      product = self.cat("/sys/devices/virtual/dmi/id/product_name").strip()
      self._device = f"{vendor} {product}"
    return self._device

  @property
  def cpu(self) -> str:
    if self._cpu:
      return self._cpu

    for line in self.cat("/proc/cpuinfo").splitlines():
      if line.startswith("model name"):
        _, self._cpu = line.split(":", maxsplit=2)
        break
    try:
      _, max_core = self.cat("/sys/devices/system/cpu/possible").strip().split(
          "-", maxsplit=1)
      cores = int(max_core) + 1
      self._cpu = f"{self._cpu} {cores} cores"
    except Exception as e:
      logging.debug("Failed to get detailed CPU stats: %s", e)
    return self._cpu

  @property
  def has_display(self) -> bool:
    return "DISPLAY" in os.environ

  @property
  def is_battery_powered(self) -> bool:
    if not self.is_remote:
      return super().is_battery_powered
    if self.which("on_ac_power"):
      return self.sh("on_ac_power", check=False).returncode == 1
    return False

  def system_details(self) -> Dict[str, Any]:
    details = super().system_details()
    for info_bin in ("lscpu", "inxi"):
      if self.which(info_bin):
        details[info_bin] = self.sh_stdout(info_bin)
    return details

  def search_binary(self, app_or_bin: pathlib.Path) -> Optional[pathlib.Path]:
    if not app_or_bin.parts:
      raise ValueError("Got empty path")
    if result_path := self.which(str(app_or_bin)):
      assert self.exists(result_path), f"{result_path} does not exist."
      return result_path
    for path in self.SEARCH_PATHS:
      # Recreate Path object for easier pyfakefs testing
      result_path = pathlib.Path(path) / app_or_bin
      if self.exists(result_path):
        return result_path
    return None
