# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import pathlib
from typing import TYPE_CHECKING, Optional

from selenium import webdriver
from selenium.webdriver.common.desired_capabilities import DesiredCapabilities
from selenium.webdriver.safari.options import Options as SafariOptions

from crossbench import helper
from crossbench.browsers.base import Browser
from crossbench.browsers.webdriver import WebdriverMixin

if TYPE_CHECKING:
  from crossbench.flags import Flags
  from crossbench.runner import Run, Runner


class Safari(Browser):

  @classmethod
  def default_path(cls) -> pathlib.Path:
    return pathlib.Path("/Applications/Safari.app")

  @classmethod
  def technology_preview_path(cls) -> pathlib.Path:
    return pathlib.Path("/Applications/Safari Technology Preview.app")

  def __init__(self,
               label: str,
               path: pathlib.Path,
               flags: Flags.InitialDataType = None,
               cache_dir: Optional[pathlib.Path] = None,
               platform: Optional[helper.MacOSPlatform] = None):
    super().__init__(label, path, flags, type="safari", platform=platform)
    assert self.platform.is_macos, "Safari only works on MacOS"
    assert self.path
    self.bundle_name = self.path.stem.replace(" ", "")
    assert cache_dir is None, "Cannot set custom cache dir for Safari"
    self.cache_dir = pathlib.Path(
        f"~/Library/Containers/com.apple.{self.bundle_name}/Data/Library/Caches"
    ).expanduser()
    if flags and "--start-fullscreen" in str(flags):
      self._start_fullscreen = True

  def _extract_version(self) -> str:
    assert self.path
    app_path = self.path.parents[2]
    return self.platform.app_version(app_path)

  def start(self, run: Run) -> None:
    assert self.platform.is_macos
    assert not self._is_running
    self.platform.exec_apple_script(f"""
tell application "{self.app_name}"
  activate
end tell
    """)
    self.platform.sleep(1)
    self.platform.exec_apple_script(f"""
tell application "{self.app_name}"
  tell application "System Events"
      to click menu item "New Private Window"
      of menu "File" of menu bar 1
      of process '{self.bundle_name}'
      if {self._start_fullscreen} then
        keystroke "f" using {{command down, control down}}
      end if
  set URL of current tab of front window to ''
  if {not self._start_fullscreen} then
    set the bounds of the first window
        to {{{self.x},{self.y},{self.width},{self.height}}}
  end if
  tell application "System Events"
      to keystroke "e" using {{command down, option down}}
  tell application "System Events"
      to click menu item 1 of menu 2 of menu bar 1
      of process '{self.bundle_name}'
  tell application "System Events"
      to set position of window 1
      of process '{self.bundle_name}' to {400, 400}
end tell
    """)
    self.platform.sleep(2)
    self._is_running = True

  def show_url(self, runner: Runner, url: str) -> None:
    self.platform.exec_apple_script(f"""
tell application "{self.app_name}"
    activate
    set URL of current tab of front window to '{url}'
end tell
    """)


class SafariWebDriver(WebdriverMixin, Safari):

  def __init__(self,
               label: str,
               path: pathlib.Path,
               flags: Flags.InitialDataType = None,
               cache_dir: Optional[pathlib.Path] = None,
               platform: Optional[helper.MacOSPlatform] = None):
    super().__init__(label, path, flags, cache_dir, platform)
    assert self.platform.is_macos

  def _find_driver(self) -> pathlib.Path:
    assert self.path
    driver_path = self.path.parent / "safaridriver"
    if not driver_path.exists():
      # The system-default Safari version doesn't come with the driver
      driver_path = pathlib.Path("/usr/bin/safaridriver")
    return driver_path

  def _start_driver(self, run: Run,
                    driver_path: pathlib.Path) -> webdriver.Safari:
    assert not self._is_running
    logging.info("STARTING BROWSER: browser: %s driver: %s", self.path,
                 driver_path)
    options = SafariOptions()
    options.binary_location = str(self.path)
    capabilities = DesiredCapabilities.SAFARI.copy()
    capabilities["safari.cleanSession"] = "true"
    # Don't wait for document-ready.
    capabilities["pageLoadStrategy"] = "eager"
    # Enable browser logging
    capabilities["safari:diagnose"] = "true"
    if "Technology Preview" in self.app_name:
      capabilities["browserName"] = "Safari Technology Preview"
    driver = webdriver.Safari(  # pytype: disable=wrong-keyword-args
        executable_path=str(driver_path),
        desired_capabilities=capabilities,
        options=options)
    assert driver.session_id, "Could not start webdriver"
    logs = (
        pathlib.Path("~/Library/Logs/com.apple.WebDriver/").expanduser() /
        driver.session_id)
    self.log_file = list(logs.glob("safaridriver*"))[0]
    assert self.log_file.is_file()
    return driver

  def _check_driver_version(self) -> None:
    # The bundled driver is always ok
    for parent in self._driver_path.parents:
      if parent == self.path.parent:
        return
    version = self.platform.sh_stdout(self._driver_path, "--version")
    assert str(self.major_version) in version, (
        f"safaridriver={self._driver_path} version='{version}' "
        f" doesn't match safari version={self.major_version}")

  def clear_cache(self, runner: Runner) -> None:
    pass

  def quit(self, runner: Runner) -> None:
    super().quit(runner)
    # Safari needs some additional push to quit properly
    self.platform.exec_apple_script(f"""
        tell application "{self.app_name}"
          quit
        end tell""")
