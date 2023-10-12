# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import json
import logging
import pathlib
import re
from typing import Dict, Final, List, Optional, Tuple, Type, Union

from crossbench import helper
from crossbench.browsers.downloader import (DMGArchiveHelper, Downloader,
                                            RPMArchiveHelper)
from crossbench import plt


class ChromeDownloader(Downloader):
  VERSION_RE: Final = re.compile(
      r"(chrome-)?(?P<version>(m[0-9]{2,})|([0-9]+(\.[0-9]+){3}))", re.I)
  STORAGE_URL: Final = "gs://chrome-signed/desktop-5c0tCh/"
  VERSION_URL = (
      "https://versionhistory.googleapis.com/v1/"
      "chrome/platforms/{platform}/channels/{channel}/versions?filter={filter}")

  @classmethod
  def _get_loader_cls(cls, platform: plt.Platform) -> Type[ChromeDownloader]:
    if platform.is_macos:
      return ChromeDownloaderMacOS
    if platform.is_linux:
      return ChromeDownloaderLinux
    if platform.is_win:
      return ChromeDownloaderWin
    raise ValueError("Downloading chrome is only supported on linux and macOS, "
                     f"but not on {platform.name} {platform.machine}")

  def _pre_check(self) -> None:
    super()._pre_check()
    if self._version_identifier and not self._platform.which("gsutil"):
      raise ValueError(
          f"Cannot download chrome version {self._version_identifier}: "
          "please install gsutil.\n"
          "- https://cloud.google.com/storage/docs/gsutil_install\n"
          "- Run 'gcloud auth login' to get access to the archives "
          "(googlers only).")

  def _version_check(self) -> None:
    major_version: int = self._requested_version[0]
    if self._platform.is_macos and self._platform.is_arm64 and (major_version <
                                                                87):
      raise ValueError(
          "Native Mac arm64/m1 Chrome version is available with M87, "
          f"but requested M{major_version}.")

  def _parse_version(
      self, version_identifier: str) -> Tuple[str, Tuple[int, ...], str, bool]:
    match = self.VERSION_RE.search(version_identifier)
    assert match, f"Invalid chrome version identifier: {version_identifier}"
    version_identifier = match["version"]
    if version_identifier[0].upper() == "M":
      requested_version = (int(version_identifier[1:]), self.ANY_MARKER,
                           self.ANY_MARKER, self.ANY_MARKER)
      requested_version_str = f"M{requested_version[0]}"
      requested_exact_version = False
    else:
      requested_version = tuple(map(int, version_identifier.split(".")))[:4]
      requested_version_str = ".".join(map(str, requested_version))
      requested_exact_version = True
    assert len(requested_version) == 4
    return (version_identifier, requested_version, requested_version_str,
            requested_exact_version)

  VERSION_URL_PLATFORM_LOOKUP: Dict[Tuple[str, str], Tuple[str, str]] = {
      ("win", "ia32"): ("win", "canary"),
      ("win", "x64"): ("win64", "canary"),
      # Linux doesn't have canary versions.
      ("linux", "x64"): ("linux", "dev"),
      ("macos", "x64"): ("mac", "canary"),
      ("macos", "arm64"): ("mac_arm64", "canary"),
      ("android", "arm64"): ("android", "canary"),
  }

  def _find_archive_url(self) -> Optional[str]:
    # Quick probe for complete versions
    if self._requested_exact_version:
      return self._find_exact_archive_url(self._version_identifier)
    return self._find_milestone_archive_url()

  def _find_milestone_archive_url(self) -> Optional[str]:
    milestone: int = self._requested_version[0]
    platform, channel = self.VERSION_URL_PLATFORM_LOOKUP.get(
        self._platform.key, (None, None))
    if not platform:
      raise ValueError(f"Unsupported platform {self._platform}")
    url = self.VERSION_URL.format(
        platform=platform,
        channel=channel,
        filter=f"version>={milestone},version<{milestone+1}&")
    logging.info("LIST ALL VERSIONS for M%s (slow): %s", milestone, url)
    try:
      with helper.urlopen(url) as response:
        raw_infos = json.loads(response.read().decode("utf-8"))["versions"]
        version_urls: List[str] = [
            f"{self.STORAGE_URL}{info['version']}/{self._platform_name}/"
            for info in raw_infos
        ]
    except Exception as e:
      raise ValueError(
          f"Could not find version {self._requested_version_str} "
          f"for {self._platform.name} {self._platform.machine} ") from e
    logging.info("FILTERING %d CANDIDATES", len(version_urls))
    return self._filter_candidate_urls(version_urls)

  def _find_exact_archive_url(self, version: str) -> Optional[str]:
    test_url = f"{self.STORAGE_URL}{version}/{self._platform_name}/"
    logging.info("LIST VERSION for M%s (fast): %s", version, test_url)
    return self._filter_candidate_urls([test_url])

  def _filter_candidate_urls(self, versions_urls: List[str]) -> Optional[str]:
    version_items = []
    for url in versions_urls:
      version_str, _ = url[len(self.STORAGE_URL):].split("/", maxsplit=1)
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
        result = self._platform.sh_stdout("gsutil", "ls", archive_url)
      except plt.SubprocessError as e:
        logging.debug("gsutil failed: %s", e)
        continue
      if result:
        return archive_url
    return None

  @abc.abstractmethod
  def _archive_url(self, folder_url: str, version_str: str) -> str:
    pass

  def _download_archive(self, archive_url: str, tmp_dir: pathlib.Path) -> None:
    self._platform.sh("gsutil", "cp", archive_url, tmp_dir)
    archive_candidates = list(tmp_dir.glob("*"))
    assert len(archive_candidates) == 1, (
        f"Download tmp dir contains more than one file: {tmp_dir}")
    candidate = archive_candidates[0]
    assert not self._archive_path.exists(), (
        f"Archive was already downloaded: {self._archive_path}")
    candidate.replace(self._archive_path)


class ChromeDownloaderLinux(ChromeDownloader):
  ARCHIVE_SUFFIX: str = ".rpm"

  @classmethod
  def is_valid(cls, path_or_identifier: Union[str, pathlib.Path],
               platform: plt.Platform) -> bool:
    if cls.VERSION_RE.fullmatch(str(path_or_identifier)):
      return True
    path = pathlib.Path(path_or_identifier)
    return path.exists() and path.suffix == cls.ARCHIVE_SUFFIX

  def __init__(self,
               version_identifier: Union[str, pathlib.Path],
               browser_type: str,
               platform_name: str,
               platform: plt.Platform,
               cache_dir: Optional[pathlib.Path] = None):
    assert not browser_type
    assert platform.is_linux
    if platform.is_x64:
      platform_name = "linux64"
    else:
      raise ValueError("Unsupported linux architecture for downloading chrome: "
                       f"got={platform.machine} supported=x64")
    super().__init__(version_identifier, "chrome", platform_name, platform,
                     cache_dir)

  def _default_extracted_path(self) -> pathlib.Path:
    return self._out_dir / self._requested_version_str

  def _default_app_path(self) -> pathlib.Path:
    return self._default_extracted_path() / "opt/google/chrome-unstable/chrome"

  def _archive_url(self, folder_url: str, version_str: str) -> str:
    return f"{folder_url}google-chrome-unstable-{version_str}-1.x86_64.rpm"

  def _extract_archive(self, archive_path: pathlib.Path) -> None:
    extracted_path = self._default_extracted_path()
    RPMArchiveHelper.extract(self._platform, archive_path, extracted_path)
    assert extracted_path.exists()


class ChromeDownloaderMacOS(ChromeDownloader):
  ARCHIVE_SUFFIX: str = ".dmg"
  MIN_MAC_ARM64_VERSION = (87, 0, 0, 0)

  @classmethod
  def is_valid(cls, path_or_identifier: Union[str, pathlib.Path],
               platform: plt.Platform) -> bool:
    if cls.VERSION_RE.fullmatch(str(path_or_identifier)):
      return True
    path = pathlib.Path(path_or_identifier)
    return path.exists() and path.suffix == cls.ARCHIVE_SUFFIX

  def __init__(self,
               version_identifier: Union[str, pathlib.Path],
               browser_type: str,
               platform_name: str,
               platform: plt.Platform,
               cache_dir: Optional[pathlib.Path] = None):
    assert not browser_type
    assert platform.is_macos, f"{type(self)} can only be used on macOS"
    platform_name = "mac-universal"
    super().__init__(version_identifier, "chrome", platform_name, platform,
                     cache_dir)

  def _download_archive(self, archive_url: str, tmp_dir: pathlib.Path) -> None:
    assert self._platform.is_macos
    if self._platform.is_arm64 and (self._requested_version <
                                    self.MIN_MAC_ARM64_VERSION):
      raise ValueError(
          "Chrome Arm64 Apple Silicon is only available starting with M87, "
          f"but requested {self._requested_version_str} is too old.")
    super()._download_archive(archive_url, tmp_dir)

  def _archive_url(self, folder_url: str, version_str: str) -> str:
    # Use ChromeCanary since it's built for all version (unlike stable/beta).
    return f"{folder_url}GoogleChromeCanary-{version_str}.dmg"

  def _default_extracted_path(self) -> pathlib.Path:
    return self._default_app_path()

  def _default_app_path(self) -> pathlib.Path:
    return self._out_dir / f"Google Chrome {self._requested_version_str}.app"

  def _extract_archive(self, archive_path: pathlib.Path) -> None:
    extracted_path = self._default_extracted_path()
    DMGArchiveHelper.extract(self._platform, archive_path, extracted_path)
    assert extracted_path.exists()


class ChromeDownloaderWin(ChromeDownloader):
  # TODO: fully implement

  @classmethod
  def is_valid(cls, path_or_identifier: Union[str, pathlib.Path],
               platform: plt.Platform) -> bool:
    return False

  def _archive_url(self, folder_url: str, version_str: str) -> str:
    raise NotImplementedError("Downloading on Windows not yet supported")
