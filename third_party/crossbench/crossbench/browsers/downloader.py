# Copyright 2023 The Chromium Authors
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
from typing import Final, Optional, Tuple, Type, Union

from crossbench.browsers.browser_helper import BROWSERS_CACHE
from crossbench import plt


class Downloader(abc.ABC):
  ARCHIVE_SUFFIX: str = ""
  ANY_MARKER: Final = 9999
  VERSION_RE: re.Pattern = re.compile(".*")

  @classmethod
  @abc.abstractmethod
  def _get_loader_cls(cls, platform: plt.Platform) -> Type[Downloader]:
    pass

  @classmethod
  def is_valid(cls, path_or_identifier: Union[str, pathlib.Path],
               platform: plt.Platform) -> bool:
    return cls._get_loader_cls(platform).is_valid(path_or_identifier, platform)

  @classmethod
  def load(cls,
           archive_path_or_version_identifier: Union[str, pathlib.Path],
           platform: plt.Platform,
           cache_dir: Optional[pathlib.Path] = None) -> pathlib.Path:
    loader_cls: Type[Downloader] = cls._get_loader_cls(platform)
    loader: Downloader = loader_cls(archive_path_or_version_identifier, "", "",
                                    platform, cache_dir)
    return loader.app_path

  def __init__(self,
               archive_path_or_version_identifier: Union[str, pathlib.Path],
               browser_type: str,
               platform_name: str,
               platform: plt.Platform,
               cache_dir: Optional[pathlib.Path] = None):
    assert browser_type, "Missing browser_type"
    self._browser_type = browser_type
    self._platform = platform
    self._platform_name = platform_name
    assert platform_name, "Missing platform_name"
    self._archive_path: pathlib.Path = pathlib.Path()
    self._out_dir = cache_dir or BROWSERS_CACHE
    self._archive_dir = self._out_dir / "archive"
    self._archive_dir.mkdir(parents=True, exist_ok=True)
    self._app_path = self._out_dir / self._browser_type
    # TODO replace version* variable with version object
    self._version_identifier: str = ""
    self._requested_version: Tuple[int, ...] = (0, 0, 0, 0)
    self._requested_version_str: str = "0.0.0.0"
    self._requested_exact_version = False
    if self.VERSION_RE.fullmatch(str(archive_path_or_version_identifier)):
      self._version_identifier = str(archive_path_or_version_identifier)
      self._pre_check()
      self._load_from_version()
    else:
      self._archive_path = pathlib.Path(archive_path_or_version_identifier)
      self._pre_check()
      if not archive_path_or_version_identifier or (
          not self._archive_path.exists()):
        raise ValueError(
            f"{self._browser_type} archive does not exist: {self._archive_path}"
        )
      self._load_from_archive()
    assert self._app_path.exists(), (
        f"Could not extract {self._browser_type}  binary: {self._app_path}")
    logging.debug("Extracted app: %s", self._app_path)

  @property
  def app_path(self) -> pathlib.Path:
    assert self._app_path.exists(), "Could not download browser"
    return self._app_path

  def _pre_check(self) -> None:
    assert not self._platform.is_remote, (
        "Browser download only supported on local machines")

  def _load_from_version(self) -> None:
    (self._version_identifier, self._requested_version,
     self._requested_version_str,
     self._requested_exact_version) = self._parse_version(
         self._version_identifier)
    self._app_path = self._default_app_path()
    logging.info("-" * 80)
    if self._app_path.exists():
      cached_version = self._validate_cached()
      logging.info("CACHED BROWSER: %s %s", cached_version, self._app_path)
      return
    self._version_check()
    self._archive_path = self._archive_dir / (
        f"{self._requested_version_str}{self.ARCHIVE_SUFFIX}")
    if not self._archive_path.exists():
      logging.info("DOWNLOADING %s %s", self._browser_type,
                   self._version_identifier.upper())
      archive_url = self._find_archive_url()
      if not archive_url:
        raise ValueError(
            f"Could not find matching version for {self._requested_version}")
      logging.info("DOWNLOADING %s", archive_url)
      with tempfile.TemporaryDirectory(suffix="cb_download") as tmp_dir_name:
        tmp_dir = pathlib.Path(tmp_dir_name)
        self._download_archive(archive_url, tmp_dir)
    else:
      logging.info("CACHED DOWNLOAD: %s", self._archive_path)
    self._extract_archive(self._archive_path)
    if not self._requested_exact_version:
      self._archive_path.unlink()

  @abc.abstractmethod
  def _version_check(self) -> None:
    pass

  def _load_from_archive(self) -> None:
    assert not self._requested_exact_version
    assert not self._version_identifier
    assert self._archive_path.exists()
    logging.info("EXTRACTING ARCHIVE: %s", self._archive_path)
    self._requested_version_str = "temp"
    self._app_path = self._default_app_path()
    temp_extracted_path = self._default_extracted_path()
    if temp_extracted_path.exists():
      assert not self._platform.is_remote, "Remote device support missing"
      logging.info("Deleting previously extracted browser: %s",
                   temp_extracted_path)
      shutil.rmtree(temp_extracted_path)
    self._extract_archive(self._archive_path)
    logging.info("Parsing browser version: %s", self._app_path)
    assert self._app_path.exists(), (
        f"Extraction failed, app does not exist: {self._app_path}")
    full_version_string = self._platform.app_version(self._app_path)
    (self._version_identifier, self._requested_version,
     self._requested_version_str,
     self._requested_exact_version) = self._parse_version(full_version_string)
    assert self._requested_exact_version
    assert self._version_identifier
    versioned_path = self._default_extracted_path()
    self._app_path = self._default_app_path()
    if self._app_path.exists():
      cached_version = self._validate_cached()
      logging.info("Deleting temporary browser: %s", self._app_path)
      shutil.rmtree(temp_extracted_path)
      logging.info("CACHED BROWSER: %s %s", cached_version, self._app_path)
    else:
      assert not versioned_path.exists()
      temp_extracted_path.rename(versioned_path)

  @abc.abstractmethod
  def _parse_version(
      self, version_identifier: str) -> Tuple[str, Tuple[int, ...], str, bool]:
    pass

  @abc.abstractmethod
  def _default_extracted_path(self) -> pathlib.Path:
    pass

  @abc.abstractmethod
  def _default_app_path(self) -> pathlib.Path:
    pass

  def _validate_cached(self) -> str:
    # "XXX YYY 107.0.5304.121" => "107.0.5304.121"
    app_version = self._platform.app_version(self._app_path)
    version_match = re.search(r"([^0-9]+)(?P<version>[\d\.ab]+)", app_version)
    assert version_match, (
        f"Got invalid version string from {self._browser_type} binary: {app_version}"
    )
    cached_version_str = version_match["version"]
    # TODO: fix using dedicated Version object
    cached_version = tuple(map(int, re.split(r"[\.ab]", cached_version_str)))
    assert 3 <= len(cached_version) <= 4, f"Got invalid version: {app_version}"
    if not self._version_matches(cached_version):
      raise ValueError(
          f"Previously downloaded browser at {self._app_path} "
          "might have been auto-updated.\n"
          "Please delete the old version and re-install/-download it.\n"
          f"Expected: {self._requested_version} Got: {cached_version}")
    return cached_version_str

  @abc.abstractmethod
  def _find_archive_url(self) -> Optional[str]:
    pass

  @abc.abstractmethod
  def _archive_url(self, folder_url: str, version_str: str) -> str:
    pass

  def _version_matches(self, version: Tuple[int, ...]) -> bool:
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
    for got, expected in zip(version, self._requested_version):
      if expected == self.ANY_MARKER:
        continue
      if got != expected:
        return False
    return True

  @abc.abstractmethod
  def _download_archive(self, archive_url: str, tmp_dir: pathlib.Path) -> None:
    pass

  @abc.abstractmethod
  def _extract_archive(self, archive_path: pathlib.Path) -> None:
    pass


class ArchiveHelper(abc.ABC):

  @classmethod
  @abc.abstractmethod
  def extract(cls, platform: plt.Platform, archive_path: pathlib.Path,
              dest_path: pathlib.Path) -> pathlib.Path:
    pass


class RPMArchiveHelper():

  @classmethod
  def extract(cls, platform: plt.Platform, archive_path: pathlib.Path,
              dest_path: pathlib.Path) -> pathlib.Path:
    assert platform.which("rpm2cpio"), (
        "Need rpm2cpio to extract downloaded .rpm archive")
    assert platform.which("cpio"), (
        "Need cpio to extract downloaded .rpm archive")
    cpio_file = archive_path.with_suffix(".cpio")
    assert not cpio_file.exists()
    archive_path.parent.mkdir(parents=True, exist_ok=True)
    with cpio_file.open("w") as f:
      platform.sh("rpm2cpio", archive_path, stdout=f)
    assert cpio_file.is_file(), f"Could not extract archive: {archive_path}"
    assert not dest_path.exists()
    with cpio_file.open() as f:
      platform.sh(
          "cpio",
          "--extract",
          f"--directory={dest_path}",
          "--make-directories",
          stdin=f)
    cpio_file.unlink()
    if not dest_path.exists():
      raise ValueError(f"Could not extract archive to {dest_path}")
    return dest_path


class DMGArchiveHelper:

  @classmethod
  def extract(cls, platform: plt.Platform, archive_path: pathlib.Path,
              dest_path: pathlib.Path) -> pathlib.Path:
    assert platform.is_macos, "DMG are only supported on macOS."
    assert not platform.is_remote, "Remote platform not supported yet"
    result = platform.sh_stdout("hdiutil", "attach", "-plist",
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
      logging.info("COPYING BROWSER src=%s dst=%s", app, dest_path)
      shutil.copytree(app, dest_path, symlinks=True, dirs_exist_ok=False)
    finally:
      platform.sh("hdiutil", "detach", dmg_path)
    if not dest_path.exists():
      raise ValueError(f"Could not extract archive to {dest_path}")
    return dest_path
