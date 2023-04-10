# Copyright 2022 The Chromium Authors
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
from typing import TYPE_CHECKING, Any, Dict, Final, List, Optional, Tuple, Type

from selenium.webdriver.chromium.options import ChromiumOptions
from selenium.webdriver.chromium.service import ChromiumService
from selenium.webdriver.chromium.webdriver import ChromiumDriver

from crossbench import exception, helper
from crossbench.browsers.base import (BROWSERS_CACHE, Browser,
                                      convert_flags_to_label)
from crossbench.browsers.webdriver import WebdriverMixin
from crossbench.flags import ChromeFeatures, ChromeFlags, Flags, JSFlags

if TYPE_CHECKING:
  from crossbench.runner import Run, Runner


class Chromium(Browser):
  DEFAULT_FLAGS = [
      "--no-default-browser-check",
      "--disable-component-update",
      "--disable-sync",
      "--no-experiments",
      "--enable-benchmarking",
      "--disable-extensions",
      "--no-first-run",
      # limit the effects of putting the browser in the background:
      "--disable-background-timer-throttling",
      "--disable-renderer-backgrounding",
  ]

  @classmethod
  def default_path(cls) -> pathlib.Path:
    return helper.search_app_or_executable(
        "Chromium",
        macos=["Chromium.app"],
        linux=["google-chromium", "chromium"],
        win=["Google/Chromium/Application/chromium.exe"])

  @classmethod
  def default_flags(cls,
                    initial_data: Flags.InitialDataType = None) -> ChromeFlags:
    return ChromeFlags(initial_data)

  def __init__(
      self,
      label: str,
      path: pathlib.Path,
      js_flags: Flags.InitialDataType = None,
      flags: Flags.InitialDataType = None,
      cache_dir: Optional[pathlib.Path] = None,
      type: str = "chromium",  # pylint: disable=redefined-builtin
      platform: Optional[helper.Platform] = None):
    super().__init__(label, path, type=type, platform=platform)
    assert not isinstance(js_flags, str), (
        f"js_flags should be a list, but got: {repr(js_flags)}")
    assert not isinstance(
        flags, str), (f"flags should be a list, but got: {repr(flags)}")
    self._flags: ChromeFlags = self.default_flags(self.DEFAULT_FLAGS)
    self._flags.update(flags)
    self.js_flags.update(js_flags)
    if cache_dir is None:
      cache_dir = self._flags.get("--user-data-dir")
    if cache_dir is None:
      # pylint: disable=bad-option-value, consider-using-with
      self.cache_dir = pathlib.Path(
          tempfile.TemporaryDirectory(prefix=type).name)
      self.clear_cache_dir = True
    else:
      self.cache_dir = cache_dir
      self.clear_cache_dir = False
    self._stdout_log_file = None

  def _extract_version(self) -> str:
    assert self.path
    version_string = self.platform.app_version(self.path)
    # Sample output: "Chromium 90.0.4430.212 dev" => "90.0.4430.212"
    return re.findall(r"[\d\.]+", version_string)[0]

  @property
  def is_headless(self) -> bool:
    return "--headless" in self._flags

  @property
  def chrome_log_file(self) -> pathlib.Path:
    assert self.log_file
    return self.log_file.with_suffix(f".{self.type}.log")

  @property
  def js_flags(self) -> JSFlags:
    return self._flags.js_flags

  @property
  def features(self) -> ChromeFeatures:
    return self._flags.features

  def exec_apple_script(self, script: str):
    assert self.platform.is_macos
    return self.platform.exec_apple_script(script)

  def details_json(self) -> Dict[str, Any]:
    details = super().details_json()
    if self.log_file:
      details["log"][self.type] = str(self.chrome_log_file)
      details["log"]["stdout"] = str(self.stdout_log_file)
    details["js_flags"] = tuple(self.js_flags.get_list())
    return details

  def _get_browser_flags(self, run: Run) -> Tuple[str, ...]:
    js_flags_copy = self.js_flags.copy()
    js_flags_copy.update(run.extra_js_flags)

    flags_copy = self.flags.copy()
    flags_copy.update(run.extra_flags)
    if "--start-fullscreen" in flags_copy:
      self._start_fullscreen = True
    else:
      flags_copy["--window-size"] = f"{self.width},{self.height}"
    if len(js_flags_copy):
      flags_copy["--js-flags"] = str(js_flags_copy)
    if user_data_dir := self.flags.get("--user-data-dir"):
      assert user_data_dir == self.cache_dir, (
          f"--user-data-dir path: {user_data_dir} was passed"
          f"but does not match cache-dir: {self.cache_dir}")
    if self.cache_dir:
      flags_copy["--user-data-dir"] = str(self.cache_dir)
    if self.log_file:
      flags_copy.set("--enable-logging")
      flags_copy["--log-file"] = str(self.chrome_log_file)

    return tuple(flags_copy.get_list())

  def get_label_from_flags(self) -> str:
    return convert_flags_to_label(*self.flags, *self.js_flags)

  def start(self, run: Run) -> None:
    # TODO: fix applescript version
    raise NotImplementedError(
        "Running the browser with AppleScript is currently broken.")

  def _start_broken(self, run: Run) -> None:
    runner = run.runner
    assert self.platform.is_macos, (
        f"Sorry, f{self.__class__} is only supported on MacOS for now")
    assert not self._is_running
    assert self._stdout_log_file is None
    if self.log_file:
      self._stdout_log_file = self.stdout_log_file.open("w", encoding="utf-8")
    # self._pid = runner.popen(
    #     self.path, *self._get_browser_flags(run), stdout=self._stdout_log_file)
    runner.wait(0.5)
    self.exec_apple_script(f"""
tell application "{self.app_name}"
    activate
    set the bounds of the first window to {{50,50,1050,1050}}
end tell
    """)
    self._is_running = True

  def quit(self, runner: Runner) -> None:
    super().quit(runner)
    if self._stdout_log_file:
      self._stdout_log_file.close()
      self._stdout_log_file = None

  def show_url(self, runner: Runner, url: str) -> None:
    self.exec_apple_script(f"""
tell application "{self.app_name}"
    activate
    set URL of active tab of front window to '{url}'
end tell
    """)


class ChromiumWebDriver(WebdriverMixin, Chromium, metaclass=abc.ABCMeta):

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
      platform: Optional[helper.Platform] = None):
    super().__init__(label, path, js_flags, flags, cache_dir, type, platform)
    self._driver_path = driver_path

  def _find_driver(self) -> pathlib.Path:
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
      raise Exception(f"Driver '{self.driver_path}' does not exist. "
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
    driver_version = None
    listing_url = None
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
    if driver_version is not None:
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
    else:
      # Try downloading the canary version
      # Lookup the branch name
      url = f"{self.OMAHA_PROXY_URL}?version={self.browser.version}"
      with helper.urlopen(url) as response:
        version_info = json.loads(response.read().decode("utf-8"))
        assert version_info["chromium_version"] == self.browser.version
        chromium_base_position = int(version_info["chromium_base_position"])
      # Use prefixes to limit listing results and increase chances of finding
      # a matching version
      arch_suffix = "Linux"
      if self.platform.is_macos:
        arch_suffix = "Mac"
        if self.platform.is_arm64:
          arch_suffix = "Mac_Arm"
      elif self.platform.is_win:
        arch_suffix = "Win"
      base_prefix = str(chromium_base_position)[:4]
      listing_url = (
          self.CHROMIUM_LISTING_URL +
          f"?prefix={arch_suffix}/{base_prefix}&maxResults=10000")
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
        if len(parts) != 3:
          continue
        _, base, _ = parts
        versions.append((int(base), version["mediaLink"]))
      versions.sort()

      url = None
      for i in range(len(versions)):
        base, url = versions[i]
        if base > chromium_base_position:
          base, url = versions[i - 1]
          break

      assert url is not None, (
          "Please manually compile/download chromedriver for "
          f"{self.browser.type} {self.browser.version}")

    logging.info("CHROMEDRIVER Downloading for version %s: %s", major_version,
                 listing_url or url)
    with tempfile.TemporaryDirectory() as tmp_dir:
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
