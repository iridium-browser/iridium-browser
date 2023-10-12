# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import os
import pathlib
import shutil
import stat
import tempfile
from typing import TYPE_CHECKING, Optional

from selenium import webdriver
from selenium.webdriver.edge.options import Options as EdgeOptions
from selenium.webdriver.edge.service import Service as EdgeService

import crossbench
import crossbench.exception
import crossbench.flags
from crossbench.browsers.browser_helper import BROWSERS_CACHE
from crossbench.browsers.chromium.webdriver import ChromiumWebDriver
from crossbench.browsers.splash_screen import SplashScreen
from crossbench.browsers.viewport import Viewport

if TYPE_CHECKING:
  from selenium.webdriver.chromium.webdriver import ChromiumDriver

  import crossbench.runner
  from crossbench import plt

FlagsInitialDataType = crossbench.flags.Flags.InitialDataType


class EdgeWebDriver(ChromiumWebDriver):

  WEB_DRIVER_OPTIONS = EdgeOptions
  WEB_DRIVER_SERVICE = EdgeService

  def __init__(
      self,
      label: str,
      path: pathlib.Path,
      flags: FlagsInitialDataType = None,
      js_flags: FlagsInitialDataType = None,
      cache_dir: Optional[pathlib.Path] = None,
      type: str = "edge",  # pylint: disable=redefined-builtin
      driver_path: Optional[pathlib.Path] = None,
      viewport: Optional[Viewport] = None,
      splash_screen: Optional[SplashScreen] = None,
      platform: Optional[plt.Platform] = None):
    super().__init__(
        label,
        path,
        flags,
        js_flags,
        cache_dir,
        type=type,
        driver_path=driver_path,
        viewport=viewport,
        splash_screen=splash_screen,
        platform=platform)

  def _find_driver(self) -> pathlib.Path:
    finder = EdgeWebDriverDownloader(self)
    return finder.download()

  def _create_driver(self, options, service) -> ChromiumDriver:
    return webdriver.Edge(  # pytype: disable=wrong-keyword-args
        options=options, service=service)


class EdgeWebDriverDownloader:
  BASE_URL = "https://msedgedriver.azureedge.net"

  def __init__(self, browser: EdgeWebDriver) -> None:
    self.browser = browser
    self.platform = browser.platform
    assert self.browser.is_local, (
        "Cannot download chromedriver for remote browser yet")
    self.extension = ""
    if self.platform.is_win:
      self.extension = ".exe"
    self.driver_path = (
        BROWSERS_CACHE /
        f"edgedriver-{self.browser.major_version}{self.extension}")

  def download(self) -> pathlib.Path:
    if not self.driver_path.exists():
      with crossbench.exception.annotate(
          f"Downloading edgedriver for {self.browser.version}"):
        self._download()
    return self.driver_path

  def _download(self) -> None:
    arch = self._arch_identifier()
    archive_name = f"edgedriver_{arch}.zip"
    url = self.BASE_URL + f"/{self.browser.version}/{archive_name}"
    logging.info("EDGEDRIVER downloading %s: %s", self.browser.version, url)
    with tempfile.TemporaryDirectory() as tmp_dir:
      archive_file = pathlib.Path(tmp_dir) / archive_name
      self.platform.download_to(url, archive_file)
      unpack_dir = pathlib.Path(tmp_dir) / "extracted"
      shutil.unpack_archive(archive_file, unpack_dir)
      driver = unpack_dir / f"msedgedriver{self.extension}"
      assert driver.is_file(), (f"Extracted driver at {driver} does not exist.")
      BROWSERS_CACHE.mkdir(parents=True, exist_ok=True)
      shutil.move(os.fspath(driver), self.driver_path)
      self.driver_path.chmod(self.driver_path.stat().st_mode | stat.S_IEXEC)

  def _arch_identifier(self) -> str:
    if self.platform.is_linux:
      assert self.platform.is_x64, "edgedriver is only available on linux x64"
      return "linux64"
    if self.platform.is_macos:
      if self.platform.is_arm64:
        return "mac64_m1"
      if self.platform.is_x64:
        return "mac64"
    elif self.platform.is_win:
      if self.platform.is_x64:
        return "win64"
      if self.platform.is_ia32:
        return "win32"
    raise ValueError(f"Unsupported edgedriver platform {self.platform}")
