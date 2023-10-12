# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import pathlib
import re
import urllib.parse
from typing import Dict, Final, Optional, Tuple, Type, Union

from crossbench.browsers.downloader import DMGArchiveHelper, Downloader
from crossbench import plt

_PLATFORM_NAME_LOOKUP: Final[Dict[Tuple[str, str], str]] = {
    ("win", "ia32"): "win32",
    ("win", "x64"): "win64",
    ("win", "arm64"): "win-aarch64",
    ("linux", "x64"): "linux-x86-64",
    ("linux", "ia32"): "linux-i686",
    ("macos", "x64"): "mac",
    ("macos", "arm64"): "mac"
}


class FirefoxDownloader(Downloader):
  # TODO: support nightly versions as well
  VERSION_RE: Final = re.compile(
      r"(firefox-)?(?P<version>([0-9]+(([.ab])[0-9]+){1,3}))", re.I)
  STORAGE_URL: Final = "https://ftp.mozilla.org/pub/firefox/releases/"

  @classmethod
  def _get_loader_cls(cls, platform: plt.Platform) -> Type[FirefoxDownloader]:
    if platform.is_macos:
      return FirefoxDownloaderMacOS
    if platform.is_linux:
      return FirefoxDownloaderLinux
    if platform.is_win:
      return FirefoxDownloaderWin
    raise ValueError("Downloading Firefox is not supported "
                     f"{platform.name} {platform.machine}")

  def __init__(self,
               version_identifier: Union[str, pathlib.Path],
               browser_type: str,
               platform_name: str,
               platform: plt.Platform,
               cache_dir: Optional[pathlib.Path] = None):
    assert not browser_type
    assert not platform_name
    firefox_platform_name = _PLATFORM_NAME_LOOKUP.get(platform.key)
    if not firefox_platform_name:
      raise ValueError(
          "Unsupported macOS architecture for downloading Firefox: "
          f"got={platform.machine}")
    super().__init__(version_identifier, "firefox", firefox_platform_name,
                     platform, cache_dir)

  def _parse_version(
      self, version_identifier: str) -> Tuple[str, Tuple[int, ...], str, bool]:
    match = self.VERSION_RE.search(version_identifier)
    assert match, f"Invalid Firefox version identifier: {version_identifier}"
    version_identifier = version_identifier = match["version"]
    requested_version = tuple(map(int, re.split(r"[.ab]",
                                                version_identifier)))[:4]
    requested_version_str = ".".join(map(str, requested_version))
    requested_exact_version = True
    assert len(requested_version) == 3
    return (version_identifier, requested_version, requested_version_str,
            requested_exact_version)

  MIN_MAC_ARM64_VERSION = (84, 0, 0, 0)

  def _version_check(self) -> None:
    major_version: int = self._requested_version[0]
    if (self._platform.is_macos and self._platform.is_arm64 and
        major_version < self.MIN_MAC_ARM64_VERSION[0]):
      raise ValueError(
          "Native Mac arm64/m1 Firefox version is available with v84, "
          f"but requested {major_version}.")

  def _find_archive_url(self) -> Optional[str]:
    # Quick probe for complete versions
    if self._requested_exact_version:
      return self._find_exact_archive_url(self._version_identifier)
    raise NotImplementedError("Only full-release versions supported.")

  def _find_exact_archive_url(self, version: str) -> Optional[str]:
    folder_url = f"{self.STORAGE_URL}{version}/mac/en-GB"
    return self._archive_url(folder_url, version)

  @abc.abstractmethod
  def _archive_url(self, folder_url: str, version_str: str) -> str:
    pass

  def _download_archive(self, archive_url: str, tmp_dir: pathlib.Path) -> None:
    self._platform.download_to(archive_url,
                               tmp_dir / f"archive.{self.ARCHIVE_SUFFIX}")
    archive_candidates = list(tmp_dir.glob("*"))
    assert len(archive_candidates) == 1, (
        f"Download tmp dir contains more than one file: {tmp_dir}")
    candidate = archive_candidates[0]
    assert not self._archive_path.exists(), (
        f"Archive was already downloaded: {self._archive_path}")
    candidate.replace(self._archive_path)

  @abc.abstractmethod
  def _extract_archive(self, archive_path: pathlib.Path) -> None:
    pass


class FirefoxDownloaderLinux(FirefoxDownloader):
  ARCHIVE_SUFFIX: str = ".tar.bz2"

  @classmethod
  def is_valid(cls, path_or_identifier: Union[str, pathlib.Path],
               platform: plt.Platform) -> bool:
    if cls.VERSION_RE.fullmatch(str(path_or_identifier)):
      return True
    path = pathlib.Path(path_or_identifier)
    return path.exists() and path.suffix == cls.ARCHIVE_SUFFIX

  def _default_extracted_path(self) -> pathlib.Path:
    return self._out_dir / self._requested_version_str

  def _default_app_path(self) -> pathlib.Path:
    return self._default_extracted_path() / "firefox-bin"

  def _archive_url(self, folder_url: str, version_str: str) -> str:
    return f"{folder_url}/firefox-{version_str}.tar.bz2"

  def _extract_archive(self, archive_path: pathlib.Path) -> None:
    raise NotImplementedError("Missing linux supoprt")


class FirefoxDownloaderMacOS(FirefoxDownloader):
  ARCHIVE_SUFFIX: str = ".dmg"

  @classmethod
  def is_valid(cls, path_or_identifier: Union[str, pathlib.Path],
               platform: plt.Platform) -> bool:
    if cls.VERSION_RE.fullmatch(str(path_or_identifier)):
      return True
    path = pathlib.Path(path_or_identifier)
    return path.exists() and path.suffix == cls.ARCHIVE_SUFFIX

  def _download_archive(self, archive_url: str, tmp_dir: pathlib.Path) -> None:
    assert self._platform.is_macos
    if self._platform.is_arm64 and (self._requested_version <
                                    self.MIN_MAC_ARM64_VERSION):
      raise ValueError(
          "Firefox Arm64 Apple Silicon is only available starting with "
          f"{self.MIN_MAC_ARM64_VERSION[0]}, "
          f"but requested {self._requested_version_str} is too old.")
    super()._download_archive(archive_url, tmp_dir)

  def _archive_url(self, folder_url: str, version_str: str) -> str:
    return f"{folder_url}/" + urllib.parse.quote(f"Firefox {version_str}.dmg")

  def _default_extracted_path(self) -> pathlib.Path:
    return self._default_app_path()

  def _default_app_path(self) -> pathlib.Path:
    return self._out_dir / f"Firefox {self._requested_version_str}.app"

  def _extract_archive(self, archive_path: pathlib.Path) -> None:
    extracted_path = self._default_extracted_path()
    DMGArchiveHelper.extract(self._platform, archive_path, extracted_path)
    assert extracted_path.exists()


class FirefoxDownloaderWin(FirefoxDownloader):

  @classmethod
  def is_valid(cls, path_or_identifier: Union[str, pathlib.Path],
               platform: plt.Platform) -> bool:
    return False

  def _extract_archive(self, archive_path: pathlib.Path) -> None:
    raise NotImplementedError("Missing windows supoprt")
