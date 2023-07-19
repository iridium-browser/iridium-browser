# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import json
import logging
import pathlib
import re
import shlex
import stat
import tempfile
import urllib.error
import zipfile
from typing import TYPE_CHECKING, Final, List, Optional, Tuple, Type

from selenium.webdriver.chromium.options import ChromiumOptions
from selenium.webdriver.chromium.service import ChromiumService
from selenium.webdriver.chromium.webdriver import ChromiumDriver

from crossbench import exception, helper
from crossbench.browsers.browser import BROWSERS_CACHE
from crossbench.browsers.viewport import Viewport
from crossbench.browsers.webdriver import WebdriverBrowser
from crossbench.flags import Flags

from .chromium import Chromium

if TYPE_CHECKING:
  from crossbench.runner import Run


class ChromiumWebDriver(WebdriverBrowser, Chromium, metaclass=abc.ABCMeta):

  WEB_DRIVER_OPTIONS: Type[ChromiumOptions] = ChromiumOptions
  WEB_DRIVER_SERVICE: Type[ChromiumService] = ChromiumService

  def __init__(
      self,
      label: str,
      path: pathlib.Path,
      js_flags: Flags.InitialDataType = None,
      flags: Flags.InitialDataType = None,
      cache_dir: Optional[pathlib.Path] = None,
      type: str = "chromium",  # pylint: disable=redefined-builtin
      driver_path: Optional[pathlib.Path] = None,
      viewport: Viewport = Viewport.DEFAULT,
      platform: Optional[helper.Platform] = None):
    super().__init__(label, path, js_flags, flags, cache_dir, type, viewport,
                     platform)
    self._driver_path = driver_path

  def _find_driver(self) -> pathlib.Path:
    if self._driver_path:
      return self._driver_path
    finder = ChromeDriverFinder(self)
    assert self.app_path
    if self.major_version == 0 or (self.app_path.parent / "args.gn").exists():
      return finder.find_local_build()
    return finder.download()

  def _start_driver(self, run: Run,
                    driver_path: pathlib.Path) -> ChromiumDriver:
    assert not self._is_running
    assert self.log_file
    options = self.WEB_DRIVER_OPTIONS()
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
    # pytype: disable=wrong-keyword-args
    service = self.WEB_DRIVER_SERVICE(
        executable_path=str(driver_path),
        log_path=str(self.driver_log_file),
        # TODO: support clean logging of chrome stdout / stderr
        service_args=["--verbose"])
    service.log_file = self.stdout_log_file.open("w", encoding="utf-8")
    driver = self._create_driver(options, service)
    # pytype: enable=wrong-keyword-args
    # Prevent debugging overhead.
    driver.execute_cdp_cmd("Runtime.setMaxCallStackSizeToCapture", {"size": 0})
    return driver

  @abc.abstractmethod
  def _create_driver(self, options, service) -> ChromiumDriver:
    pass

  def _check_driver_version(self) -> None:
    # TODO
    # version = self.platform.sh_stdout(self._driver_path, "--version")
    pass


class ChromeDriverFinder:
  URL: Final[str] = "http://chromedriver.storage.googleapis.com"
  OMAHA_PROXY_URL: Final[str] = "https://omahaproxy.appspot.com/deps.json"
  CHROMIUM_LISTING_URL: Final[str] = (
      "https://www.googleapis.com/storage/v1/b/chromium-browser-snapshots/o/")

  driver_path: pathlib.Path

  def __init__(self, browser: ChromiumWebDriver):
    self.browser = browser
    self.platform = browser.platform
    assert self.browser.is_local, (
        "Cannot download chromedriver for remote browser yet")
    extension = ""
    if self.platform.is_win:
      extension = ".exe"
    self.driver_path = (
        BROWSERS_CACHE /
        f"chromedriver-{self.browser.major_version}{extension}")

  def find_local_build(self) -> pathlib.Path:
    assert self.browser.app_path
    # assume it's a local build
    self.driver_path = self.browser.app_path.parent / "chromedriver"
    if not self.driver_path.exists():
      raise ValueError(f"Driver '{self.driver_path}' does not exist. "
                       "Please build 'chromedriver' manually for local builds.")
    return self.driver_path

  def download(self) -> pathlib.Path:
    if not self.driver_path.exists():
      with exception.annotate(
          f"Downloading chromedriver for {self.browser.version}"):
        self._download()

    return self.driver_path

  def _download(self) -> None:
    major_version = self.browser.major_version
    logging.info("CHROMEDRIVER Downloading from %s for %s v%s", self.URL,
                 self.browser.type, major_version)
    listing_url, url = self._find_stable_url(major_version)
    if not url:
      url = self._find_canary_url()

    assert url is not None, (
        "Please manually compile/download chromedriver for "
        f"{self.browser.type} {self.browser.version}")

    logging.info("CHROMEDRIVER Downloading for version %s: %s", major_version,
                 listing_url or url)
    with tempfile.TemporaryDirectory() as tmp_dir:
      if ".zip" not in url:
        maybe_driver = pathlib.Path(tmp_dir) / "chromedriver"
        self.platform.download_to(url, maybe_driver)
      else:
        zip_file = pathlib.Path(tmp_dir) / "download.zip"
        self.platform.download_to(url, zip_file)
        with zipfile.ZipFile(zip_file, "r") as zip_ref:
          zip_ref.extractall(zip_file.parent)
        zip_file.unlink()
        maybe_driver = None
        candidates: List[pathlib.Path] = [
            path for path in zip_file.parent.glob("**/*")
            if path.is_file() and "chromedriver" in path.name
        ]
        # Find exact match first:
        maybe_drivers: List[pathlib.Path] = [
            path for path in candidates if path.stem == "chromedriver"
        ]
        # Backup less strict matching:
        maybe_drivers += candidates
        if len(maybe_drivers) > 0:
          maybe_driver = maybe_drivers[0]
      assert maybe_driver and maybe_driver.is_file(), (
          f"Extracted driver at {maybe_driver} does not exist.")
      BROWSERS_CACHE.mkdir(parents=True, exist_ok=True)
      maybe_driver.rename(self.driver_path)
      self.driver_path.chmod(self.driver_path.stat().st_mode | stat.S_IEXEC)

  def _find_stable_url(
      self, major_version: int) -> Tuple[Optional[str], Optional[str]]:
    driver_version: Optional[str] = None
    listing_url: Optional[str] = None
    if major_version <= 69:
      with helper.urlopen(f"{self.URL}/2.46/notes.txt") as response:
        lines = response.read().decode("utf-8").split("\n")
        for i, line in enumerate(lines):
          if not line.startswith("---"):
            continue
          [min_version, max_version] = map(int,
                                           re.findall(r"\d+", lines[i + 1]))
          if min_version <= major_version <= max_version:
            match = re.search(r"\d\.\d+", line)
            assert match, "Could not parse version number"
            driver_version = match.group(0)
            break
    else:
      url = f"{self.URL}/LATEST_RELEASE_{major_version}"
      try:
        with helper.urlopen(url) as response:
          driver_version = response.read().decode("utf-8")
        listing_url = f"{self.URL}/index.html?path={driver_version}/"
      except urllib.error.HTTPError as e:
        if e.code != 404:
          raise
    if not driver_version:
      return listing_url, None

    if self.platform.is_linux:
      arch_suffix = "linux64"
    elif self.platform.is_macos:
      arch_suffix = "mac64"
      if self.platform.is_arm64:
        # The uploaded chromedriver archives changed the naming scheme after
        # chrome version 106.0.5249.21 for Arm64 (previously m1):
        #   before: chromedriver_mac64_m1.zip
        #   after:  chromedriver_mac_arm64.zip
        last_old_naming_version = (106, 0, 5249, 21)
        version_tuple = tuple(map(int, driver_version.split(".")))
        if version_tuple <= last_old_naming_version:
          arch_suffix = "mac64_m1"
        else:
          arch_suffix = "mac_arm64"
    elif self.platform.is_win:
      arch_suffix = "win32"
    else:
      raise NotImplementedError("Unsupported chromedriver platform")
    url = (f"{self.URL}/{driver_version}/" f"chromedriver_{arch_suffix}.zip")
    return listing_url, url

  def _find_canary_url(self) -> Optional[str]:
    logging.debug("Try downloading the chromedriver canary version")
    # Lookup the branch name
    url = f"{self.OMAHA_PROXY_URL}?version={self.browser.version}"
    with helper.urlopen(url) as response:
      version_info = json.loads(response.read().decode("utf-8"))
      assert version_info["chromium_version"] == self.browser.version
      chromium_base_position = int(version_info["chromium_base_position"])
    # Use prefixes to limit listing results and increase chances of finding
    # a matching version
    platform_name = "Linux"
    cpu_suffix = ""
    if self.platform.is_macos:
      platform_name = "Mac"
      if self.platform.is_arm64:
        cpu_suffix = "_Arm"
    else:
      if self.platform.is_win:
        platform_name = "Win"
      if self.platform.is_x64:
        cpu_suffix = "_x64"
      else:
        raise NotImplementedError("Unsupported chromedriver platform")

    base_prefix = str(chromium_base_position)[:4]
    listing_url = (
        self.CHROMIUM_LISTING_URL +
        f"?prefix={platform_name}{cpu_suffix}/{base_prefix}&maxResults=10000")
    with helper.urlopen(listing_url) as response:
      listing = json.loads(response.read().decode("utf-8"))

    versions = []
    for version in listing["items"]:
      if "name" not in version:
        continue
      if "mediaLink" not in version:
        continue
      name = version["name"]
      if "chromedriver" not in name:
        continue
      parts = name.split("/")
      if "chromedriver" not in parts[-1] or len(parts) < 3:
        continue
      base = parts[1]
      try:
        int(base)
      except ValueError:
        # Ignore ig base is not an int
        continue
      versions.append((int(base), version["mediaLink"]))
    versions.sort()

    for i in range(len(versions)):
      base, url = versions[i]
      if base > chromium_base_position:
        base, url = versions[i - 1]
        return url
    return None
