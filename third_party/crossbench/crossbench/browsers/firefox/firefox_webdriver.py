# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import json
import logging
import pathlib
import shlex
import shutil
import stat
import tempfile
from typing import TYPE_CHECKING, Dict, Optional, Tuple

from selenium import webdriver
from selenium.webdriver.firefox.options import Options as FirefoxOptions
from selenium.webdriver.firefox.service import Service as FirefoxService

from crossbench import exception, helper
from crossbench.browsers.browser import BROWSERS_CACHE
from crossbench.browsers.viewport import Viewport
from crossbench.browsers.webdriver import WebdriverBrowser

from .firefox import Firefox

if TYPE_CHECKING:
  from crossbench.flags import Flags
  from crossbench.runner import Run


class FirefoxWebDriver(WebdriverBrowser, Firefox):

  def __init__(self,
               label: str,
               path: pathlib.Path,
               flags: Flags.InitialDataType = None,
               cache_dir: Optional[pathlib.Path] = None,
               driver_path: Optional[pathlib.Path] = None,
               viewport: Viewport = Viewport.DEFAULT,
               platform: Optional[helper.Platform] = None):
    super().__init__(label, path, flags, cache_dir, viewport, platform)
    self._driver_path = driver_path

  def _find_driver(self) -> pathlib.Path:
    finder = FirefoxDriverFinder(self)
    return finder.download()

  def _start_driver(self, run: Run,
                    driver_path: pathlib.Path) -> webdriver.Firefox:
    assert not self._is_running
    assert self.log_file
    options = FirefoxOptions()
    options.set_capability("browserVersion", str(self.major_version))
    # Don't wait for document-ready.
    options.set_capability("pageLoadStrategy", "eager")
    args = self._get_browser_flags(run)
    for arg in args:
      options.add_argument(arg)
    options.binary_location = str(self.path)
    logging.info("STARTING BROWSER: %s", self.path)
    logging.info("STARTING BROWSER: driver: %s", driver_path)
    logging.info("STARTING BROWSER: args: %s", shlex.join(args))
    service = FirefoxService(
        executable_path=str(driver_path),
        log_path=str(self.driver_log_file),
        service_args=[])
    service.log_file = self.stdout_log_file.open("w", encoding="utf-8")
    driver = webdriver.Firefox(  # pytype: disable=wrong-keyword-args
        options=options, service=service)
    return driver

  def _check_driver_version(self) -> None:
    # TODO
    # version = self.platform.sh_stdout(self._driver_path, "--version")
    pass


class FirefoxDriverFinder:
  RELEASES_URL = "https://api.github.com/repos/mozilla/geckodriver/releases"

  def __init__(self, browser: FirefoxWebDriver):
    self.browser = browser
    self.platform = browser.platform
    self.extension = ""
    if self.platform.is_win:
      self.extension = ".exe"
    self.driver_path = (
        BROWSERS_CACHE /
        f"geckodriver-{self.browser.major_version}{self.extension}")

  def download(self) -> pathlib.Path:
    if not self.driver_path.exists():
      with exception.annotate(
          f"Downloading geckodriver for {self.browser.version}"):
        self._download()
    return self.driver_path

  def _download(self) -> None:
    url, archive_type = self._find_driver_download_url()
    with tempfile.TemporaryDirectory() as tmp_dir:
      tar_file = pathlib.Path(tmp_dir) / f"download.{archive_type}"
      self.platform.download_to(url, tar_file)
      unpack_dir = pathlib.Path(tmp_dir) / "extracted"
      shutil.unpack_archive(tar_file, unpack_dir)
      driver = unpack_dir / f"geckodriver{self.extension}"
      assert driver.is_file(), (f"Extracted driver at {driver} does not exist.")
      BROWSERS_CACHE.mkdir(parents=True, exist_ok=True)
      driver.rename(self.driver_path)
      self.driver_path.chmod(self.driver_path.stat().st_mode | stat.S_IEXEC)

  def _find_driver_download_url(self) -> Tuple[str, str]:
    driver_version = self._get_driver_version()
    all_releases = self._load_releases()
    matching_release = {}
    for version, version_release in all_releases.items():
      if version <= driver_version:
        matching_release = version_release
        break
    if not matching_release:
      raise ValueError("No matching geckodriver version found")
    arch = self._arch_identifier()
    version = matching_release["tag_name"]
    archive_type = "tar.gz"
    if self.platform.is_win:
      archive_type = "zip"
    driver_asset_name = f"geckodriver-{version}-{arch}.{archive_type}"
    url = ""
    for asset in matching_release["assets"]:
      if asset["name"] == driver_asset_name:
        url = asset["browser_download_url"]
        break
    if not url:
      raise ValueError(
          f"Could not find geckodriver {version} for platform {arch}")
    logging.info("GECKODRIVER downloading %s: %s", version, url)
    return url, archive_type

  def _get_driver_version(self) -> Tuple[int, int, int]:
    version = self.browser.major_version
    # See https://firefox-source-docs.mozilla.org/testing/geckodriver/Support.html
    if version < 52:
      raise ValueError(f"Firefox {version} is too old for geckodriver.")
    if version < 53:
      return (0, 18, 0)
    if version < 57:
      return (0, 20, 1)
    if version < 60:
      return (0, 25, 0)
    if version < 78:
      return (0, 30, 0)
    if version < 91:
      return (0, 31, 0)
    return (9999, 9999, 9999)

  def _load_releases(self) -> Dict[Tuple[int, ...], Dict]:
    with helper.urlopen(self.RELEASES_URL) as response:
      releases = json.loads(response.read().decode("utf-8"))
    assert isinstance(releases, list)
    versions = {}
    for release in releases:
      # "v0.10.2" => "0.10.2"
      version = release["tag_name"][1:]
      # "0.10.2" => (0, 10, 2)
      version = tuple(int(i) for i in version.split("."))
      assert version not in versions
      versions[version] = release
    return dict(sorted(versions.items(), reverse=True))

  def _arch_identifier(self) -> str:
    if self.platform.is_linux:
      arch = "linux"
    elif self.platform.is_macos:
      arch = "macos"
    elif self.platform.is_win:
      arch = "win"
    else:
      raise ValueError(f"Unsupported geckodriver platform {self.platform}")
    if not self.platform.is_macos:
      if self.platform.is_x64:
        arch += "64"
      elif self.platform.is_ia32:
        arch += "32"
    if self.platform.is_arm64:
      arch += "-aarch64"
    return arch