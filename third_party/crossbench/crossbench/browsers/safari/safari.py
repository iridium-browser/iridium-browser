# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import pathlib
from typing import TYPE_CHECKING, Optional

from crossbench import compat
from crossbench.browsers.browser import Browser

if TYPE_CHECKING:
  from crossbench import plt
  from crossbench.browsers.splash_screen import SplashScreen
  from crossbench.browsers.viewport import Viewport
  from crossbench.flags import Flags
  from crossbench.runner.runner import Runner


SAFARIDRIVER_PATH = pathlib.Path("/usr/bin/safaridriver")


def find_safaridriver(bin_path: pathlib.Path) -> pathlib.Path:
  assert bin_path.is_file(), f"Invalid binary path: {bin_path}"
  driver_path = bin_path.parent / "safaridriver"
  if driver_path.exists():
    return driver_path
  # The system-default Safari version doesn't come with the driver
  assert compat.is_relative_to(bin_path, Safari.default_path()), (
      f"Expected default Safari.app binary but got {bin_path}")
  return SAFARIDRIVER_PATH


class Safari(Browser):

  @classmethod
  def default_path(cls) -> pathlib.Path:
    return pathlib.Path("/Applications/Safari.app")

  @classmethod
  def technology_preview_path(cls) -> pathlib.Path:
    return pathlib.Path("/Applications/Safari Technology Preview.app")

  def __init__(
      self,
      label: str,
      path: pathlib.Path,
      flags: Optional[Flags.InitialDataType] = None,
      js_flags: Optional[Flags.InitialDataType] = None,
      cache_dir: Optional[pathlib.Path] = None,
      type: str = "safari",  # pylint: disable=redefined-builtin
      driver_path: Optional[pathlib.Path] = None,
      viewport: Optional[Viewport] = None,
      splash_screen: Optional[SplashScreen] = None,
      platform: Optional[plt.MacOSPlatform] = None):
    super().__init__(
        label,
        path,
        flags,
        js_flags=None,
        type=type,
        driver_path=driver_path,
        viewport=viewport,
        splash_screen=splash_screen,
        platform=platform)
    assert not js_flags, "Safari doesn't support custom js_flags"
    assert self.platform.is_macos, "Safari only works on MacOS"
    assert self.path
    self.bundle_name = self.path.stem.replace(" ", "")
    assert cache_dir is None, "Cannot set custom cache dir for Safari"
    self.cache_dir = pathlib.Path(
        f"~/Library/Containers/com.apple.{self.bundle_name}/Data/Library/Caches"
    ).expanduser()

  def clear_cache(self, runner: Runner) -> None:
    self._clear_cache()

  def _clear_cache(self) -> None:
    logging.info("CLEAR CACHE: %s", self)
    self.platform.exec_apple_script(f"""
      tell application "{self.app_path}" to activate
      tell application "System Events"
          keystroke "e" using {{command down, option down}}
      end tell""")

  def _extract_version(self) -> str:
    # Use the shipped safaridriver to get the more detailed version
    driver_version = self.platform.app_version(find_safaridriver(self.path))
    # Input: "Included with Safari 16.6 (18615.3.6.11.1)"
    # Output: " (18615.3.6.11.1)"
    driver_version = " (" + driver_version.split(" (", maxsplit=1)[1]
    assert self.path
    app_path = self.path.parents[2]
    return self.platform.app_version(app_path) + driver_version
