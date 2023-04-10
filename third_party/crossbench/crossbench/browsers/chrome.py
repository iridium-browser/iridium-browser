# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import logging
import pathlib
import plistlib
import re
import shutil
import tempfile
from typing import TYPE_CHECKING, Final, List, Optional, Tuple

from selenium import webdriver
from selenium.webdriver.chrome.options import Options as ChromeOptions
from selenium.webdriver.chrome.service import Service as ChromeService

from crossbench import helper
from crossbench.browsers import BROWSERS_CACHE
from crossbench.browsers.chromium import Chromium, ChromiumWebDriver

if TYPE_CHECKING:
  from selenium.webdriver.chromium.webdriver import ChromiumDriver

  from crossbench.flags import Flags


class Chrome(Chromium):

  @classmethod
  def default_path(cls) -> pathlib.Path:
    return cls.stable_path()

  @classmethod
  def stable_path(cls) -> pathlib.Path:
    return helper.search_app_or_executable(
        "Chrome Stable",
        macos=["Google Chrome.app"],
        linux=["google-chrome", "chrome"],
        win=["Google/Chrome/Application/chrome.exe"])

  @classmethod
  def beta_path(cls) -> pathlib.Path:
    return helper.search_app_or_executable(
        "Chrome Beta",
        macos=["Google Chrome Beta.app"],
        linux=["google-chrome-beta"],
        win=["Google/Chrome Beta/Application/chrome.exe"])

  @classmethod
  def dev_path(cls) -> pathlib.Path:
    return helper.search_app_or_executable(
        "Chrome Dev",
        macos=["Google Chrome Dev.app"],
        linux=["google-chrome-unstable"],
        win=["Google/Chrome Dev/Application/chrome.exe"])

  @classmethod
  def canary_path(cls) -> pathlib.Path:
    return helper.search_app_or_executable(
        "Chrome Canary",
        macos=["Google Chrome Canary.app"],
        win=["Google/Chrome SxS/Application/chrome.exe"])

  def __init__(self,
               label: str,
               path: pathlib.Path,
               js_flags: Flags.InitialDataType = None,
               flags: Flags.InitialDataType = None,
               cache_dir: Optional[pathlib.Path] = None,
               platform: Optional[helper.Platform] = None):
    super().__init__(
        label,
        path,
        js_flags,
        flags,
        cache_dir,
        type="chrome",
        platform=platform)


class ChromeWebDriver(ChromiumWebDriver):

  WEB_DRIVER_OPTIONS = ChromeOptions
  WEB_DRIVER_SERVICE = ChromeService

  def __init__(self,
               label: str,
               path: pathlib.Path,
               js_flags: Flags.InitialDataType = None,
               flags: Flags.InitialDataType = None,
               cache_dir: Optional[pathlib.Path] = None,
               driver_path: Optional[pathlib.Path] = None,
               platform: Optional[helper.Platform] = None):
    super().__init__(
        label,
        path,
        js_flags,
        flags,
        cache_dir,
        type="chrome",
        driver_path=driver_path,
        platform=platform)

  def _create_driver(self, options, service) -> ChromiumDriver:
    return webdriver.Chrome(  # pytype: disable=wrong-keyword-args
        options=options, service=service)


class ChromeDownloader(abc.ABC):

  VERSION_RE: Final = re.compile(r"^chrome-((m[0-9]+)|([0-9]+(\.[0-9]+){3}))$",
                                 re.I)
  ANY_MARKER: Final = 9999
  URL: Final = "gs://chrome-signed/desktop-5c0tCh/"

  @classmethod
  def load(cls, version_identifier: str) -> pathlib.Path:
    loader: ChromeDownloader
    if helper.platform.is_macos:
      loader = ChromeDownloaderMacOS(version_identifier)
    elif helper.platform.is_linux:
      loader = ChromeDownloaderLinux(version_identifier)
    else:
      raise ValueError(
          f"Unsupported platform to download chrome: {helper.platform.machine}")
    assert loader.path.exists(), "Could not download browser"
    return loader.path

  def __init__(self, version_identifier: str, platform_name: str):
    assert platform_name
    self.platform_name = platform_name
    self.version_identifier = ""
    self.platform = helper.platform
    self._pre_check()
    self.requested_version = (0, 0, 0, 0)
    self.requested_version_str = "0.0.0.0"
    self.requested_exact_version = False
    version_identifier = version_identifier.lower()
    self._parse_version(version_identifier)
    self.path = self._get_path()
    logging.info("-" * 80)
    if self.path.exists():
      cached_version = self._validate_cached()
      logging.info("CACHED BROWSER: %s %s", cached_version, self.path)
    else:
      logging.info("DOWNLOADING CHROME %s", version_identifier.upper())
      self._download()

  def _pre_check(self) -> None:
    assert not self.platform.is_remote, (
        "Browser download only supported on local machines")
    if not self.platform.which("gsutil"):
      raise ValueError(
          f"Cannot download chrome version {self.version_identifier}: "
          "please install gsutil.\n"
          "- https://cloud.google.com/storage/docs/gsutil_install\n"
          "- Run 'gcloud auth login' to get access to the archives")

  def _parse_version(self, version_identifier: str) -> None:
    match = self.VERSION_RE.match(version_identifier)
    assert match, (f"Invalid chrome version identifier: {version_identifier}")
    self.version_identifier = version_identifier = match[1]
    if version_identifier[0] == "m":
      self.requested_version = (int(version_identifier[1:]), self.ANY_MARKER,
                                self.ANY_MARKER, self.ANY_MARKER)
      self.requested_version_str = f"M{self.requested_version[0]}"
      self.requested_exact_version = False
    else:
      self.requested_version = tuple(map(int,
                                         version_identifier.split(".")))[:4]
      self.requested_version_str = ".".join(map(str, self.requested_version))
      self.requested_exact_version = True
    assert len(self.requested_version) == 4

  @abc.abstractmethod
  def _get_path(self) -> pathlib.Path:
    pass

  def _validate_cached(self) -> str:
    # "Google Chrome 107.0.5304.121" => "107.0.5304.121"
    app_version = self.platform.app_version(self.path)
    version_match = re.search(r"[\d\.]+", app_version)
    assert version_match, (
        f"Got invalid version string from chrome binary: {app_version}")
    cached_version_str = version_match.group(0)
    cached_version = tuple(map(int, cached_version_str.split(".")))
    assert len(cached_version) == 4, f"Got invalid version: {app_version}"
    if not self._version_matches(cached_version):
      raise ValueError(
          f"Previously downloaded browser at {self.path} might have been auto-updated.\n"
          "Please delete the old version and re-install/-download it.\n"
          f"Expected: {self.requested_version} Got: {cached_version}")
    return cached_version_str

  def _download(self) -> None:
    archive_url = self._find_archive_url()
    if not archive_url:
      raise ValueError(
          f"Could not find matching version for {self.requested_version}")
    self._download_and_extract(archive_url)

  def _find_archive_url(self) -> Optional[str]:
    milestone: int = self.requested_version[0]
    # Quick probe for complete versions
    if self.requested_exact_version:
      test_url = f"{self.URL}{self.version_identifier}/{self.platform_name}/"
      logging.info("LIST VERSION for M%s (fast): %s", milestone, test_url)
      maybe_archive_url = self._filter_candidates([test_url])
      if maybe_archive_url:
        return maybe_archive_url
    list_url = f"{self.URL}{milestone}.*/{self.platform_name}/"
    logging.info("LIST ALL VERSIONS for M%s (slow): %s", milestone, list_url)
    try:
      listing: List[str] = self.platform.sh_stdout(
          "gsutil", "ls", "-d", list_url).strip().splitlines()
    except helper.SubprocessError as e:
      if "One or more URLs matched no objects" in str(e):
        raise ValueError(
            f"Could not find version {self.requested_version_str} "
            f"for {self.platform.name} {self.platform.machine} ") from e
      raise
    logging.info("FILTERING %d CANDIDATES", len(listing))
    return self._filter_candidates(listing)

  def _filter_candidates(self, listing: List[str]) -> Optional[str]:
    version_items = []
    for url in listing:
      version_str, _ = url[len(self.URL):].split("/", maxsplit=1)
      version = tuple(map(int, version_str.split(".")))
      version_items.append((version, url))
    version_items.sort(reverse=True)
    # Iterate from new to old version and and the first one that is older or
    # equal than the requested version.
    for version, url in version_items:
      if not self._version_matches(version):
        logging.debug("Skipping download candidate: %s %s", version, url)
        continue
      version_str = ".".join(map(str, version))
      archive_url = self._archive_url(url, version_str)
      try:
        result = self.platform.sh_stdout("gsutil", "ls", archive_url)
      except helper.SubprocessError:
        continue
      if result:
        return archive_url
    return None

  @abc.abstractmethod
  def _archive_url(self, folder_url: str, version_str: str) -> str:
    pass

  def _version_matches(self, version: Tuple[int, int, int, int]) -> bool:
    # Iterate over the version parts. Use 9999 as placeholder to accept
    # an arbitrary version part.
    #
    # Requested: 100.0.4000.500
    # version:   100.0.4000.501 => False
    #
    # Requested: 100.0.4000.ANY_MARKER
    # version:   100.0.4000.501 => True
    # version:   100.0.4001.501 => False
    # version:   101.0.4000.501 => False
    #
    # We assume that the user iterates over a sorted list from new to old
    # versions for a matching milestone.
    for got, expected in zip(version, self.requested_version):
      if expected == self.ANY_MARKER:
        continue
      if got != expected:
        return False
    return True

  def _download_and_extract(self, archive_url: str) -> None:
    with tempfile.TemporaryDirectory(prefix="crossbench_download") as tmp_path:
      tmp_dir = pathlib.Path(tmp_path)
      archive_path = self._download_archive(archive_url, tmp_dir)
      self._extract_archive(archive_path)

  def _download_archive(
      self,
      archive_url: str,
      tmp_dir: pathlib.Path,
  ) -> pathlib.Path:
    logging.info("DOWNLOADING %s", archive_url)
    self.platform.sh("gsutil", "cp", archive_url, tmp_dir)
    archive_candidates = list(tmp_dir.glob("*"))
    assert len(archive_candidates) == 1, (
        f"Download tmp dir contains more than one file: {tmp_dir}")
    archive_path = archive_candidates[0]
    return archive_path

  @abc.abstractmethod
  def _extract_archive(self, archive_path: pathlib.Path) -> None:
    pass


class ChromeDownloaderLinux(ChromeDownloader):

  def __init__(self, version_identifier: str):
    assert helper.platform.is_linux
    if helper.platform.is_x64:
      platform_name = "linux64"
    else:
      raise ValueError("Unsupported linux architecture for downloading chrome: "
                       f"got={helper.platform.machine} supported=x64")
    super().__init__(version_identifier, platform_name)

  def _get_path(self) -> pathlib.Path:
    return (BROWSERS_CACHE / self.requested_version_str /
            "opt/google/chrome-unstable/chrome")

  def _archive_url(self, folder_url: str, version_str: str) -> str:
    return f"{folder_url}google-chrome-unstable-{version_str}-1.x86_64.rpm"

  def _extract_archive(self, archive_path: pathlib.Path) -> None:
    assert helper.platform.which("rpm2cpio"), (
        "Need rpm2cpio to extract downloaded .rpm chrome archive")
    assert helper.platform.which("cpio"), (
        "Need cpio to extract downloaded .rpm chrome archive")
    cpio_file = archive_path.with_suffix(".cpio")
    assert not cpio_file.exists()
    archive_path.parent.mkdir(parents=True, exist_ok=True)
    with cpio_file.open("w") as f:
      self.platform.sh("rpm2cpio", archive_path, stdout=f)
    assert cpio_file.is_file(), f"Could not extract archive: {archive_path}"
    out_dir = BROWSERS_CACHE / self.requested_version_str
    assert not out_dir.exists()
    with cpio_file.open() as f:
      self.platform.sh(
          "cpio",
          "--extract",
          f"--directory={out_dir}",
          "--make-directories",
          stdin=f)
    assert self.path.is_file(), f"Could not extract chrome binary: {self.path}"
    cpio_file.unlink()
    archive_path.unlink()


class ChromeDownloaderMacOS(ChromeDownloader):

  def __init__(self, version_identifier: str):
    assert helper.platform.is_macos
    super().__init__(version_identifier, platform_name="mac-universal")

  def _download(self) -> None:
    if self.platform.is_arm64 and self.requested_version < (87, 0, 0, 0):
      raise ValueError(
          "Chrome Arm64 Apple Silicon is only available starting with M87, "
          f"but requested {self.requested_version_str}")
    super()._download()

  def _archive_url(self, folder_url, version_str) -> str:
    # Use ChromeCanary since it's built for all version (unlike stable/beta).
    return f"{folder_url}GoogleChromeCanary-{version_str}.dmg"

  def _get_path(self) -> pathlib.Path:
    return BROWSERS_CACHE / f"Google Chrome {self.requested_version_str}.app"

  def _extract_archive(self, archive_path: pathlib.Path) -> None:
    result = self.platform.sh_stdout("hdiutil", "attach", "-plist",
                                     archive_path).strip()
    data = plistlib.loads(str.encode(result))
    dmg_path: Optional[pathlib.Path] = None
    for item in data["system-entities"]:
      mount_point = item.get("mount-point", None)
      if mount_point:
        dmg_path = pathlib.Path(mount_point)
        if dmg_path.exists():
          break
    if not dmg_path:
      raise ValueError("Could not mount downloaded disk image")
    apps = list(dmg_path.glob("*.app"))
    assert len(apps) == 1, "Mounted disk image contains more than 1 app"
    app = apps[0]
    try:
      logging.info("COPYING BROWSER src=%s dst=%s", app, self.path)
      shutil.copytree(app, self.path, dirs_exist_ok=False)
    finally:
      self.platform.sh("hdiutil", "detach", dmg_path)
      archive_path.unlink()
