# Copyright 2023 The Chromium Authors
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
from crossbench.browsers.viewport import Viewport
from crossbench.browsers.webdriver import WebdriverBrowser

from .safari import Safari

if TYPE_CHECKING:
  from crossbench.flags import Flags
  from crossbench.runner import Run, Runner


class SafariWebDriver(WebdriverBrowser, Safari):

  def __init__(self,
               label: str,
               path: pathlib.Path,
               flags: Flags.InitialDataType = None,
               cache_dir: Optional[pathlib.Path] = None,
               viewport: Viewport = Viewport.DEFAULT,
               platform: Optional[helper.MacOSPlatform] = None):
    super().__init__(label, path, flags, cache_dir, viewport, platform)
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
    args = self._get_browser_flags(run)
    for arg in args:
      options.add_argument(arg)
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
    assert self._driver_path
    for parent in self._driver_path.parents:
      if parent == self.path.parent:
        return
    version = self.platform.sh_stdout(self._driver_path, "--version")
    assert str(self.major_version) in version, (
        f"safaridriver={self._driver_path} version='{version}' "
        f" doesn't match safari version={self.major_version}")

  def quit(self, runner: Runner) -> None:
    super().quit(runner)
    # Safari needs some additional push to quit properly
    self.platform.exec_apple_script(f"""
        tell application "{self.app_name}"
          quit
        end tell""")
