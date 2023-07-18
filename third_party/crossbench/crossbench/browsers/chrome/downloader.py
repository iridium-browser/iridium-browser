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
from typing import Final, List, Optional, Tuple, Type, Union

from crossbench import helper
from crossbench.browsers.browser import BROWSERS_CACHE


class ChromeDownloader(abc.ABC):
  ARCHIVE_SUFFIX: str = ""
  VERSION_RE: Final = re.compile(
      r"(chrome-)?(?P<version>(m[0-9]+)|([0-9]+(\.[0-9]+){3}))", re.I)
  ANY_MARKER: Final = 9999
  URL: Final = "gs://chrome-signed/desktop-5c0tCh/"

  @classmethod
  def _get_loader_cls(cls, platform: helper.Platform) -> Type[ChromeDownloader]:
    if platform.is_macos:
      return ChromeDownloaderMacOS
    if platform.is_linux:
      return ChromeDownloaderLinux
    raise ValueError("Downloading chrome is only support on linux and macOS, "
                     f"but not on {platform.machine}")

  @classmethod
  def is_valid(cls, path_or_identifier: Union[str, pathlib.Path],
               platform: helper.Platform) -> bool:
    return cls._get_loader_cls(platform).is_valid(path_or_identifier, platform)

  @classmethod
  def load(cls,
           archive_path_or_version_identifier: Union[str, pathlib.Path],
           platform: helper.Platform,
           cache_dir: Optional[pathlib.Path] = None) -> pathlib.Path:
    loader_cls: Type[ChromeDownloader] = cls._get_loader_cls(platform)
    loader: ChromeDownloader = loader_cls(archive_path_or_version_identifier,
                                          "", platform, cache_dir)
    return loader.app_path

  def __init__(self,
               archive_path_or_version_identifier: Union[str, pathlib.Path],
               platform_name: str,
               platform: helper.Platform,
               cache_dir: Optional[pathlib.Path] = None):
    self._platform = platform
    self._platform_name = platform_name
    self._archive_path: pathlib.Path = pathlib.Path()
    self._out_dir = cache_dir or BROWSERS_CACHE
    self._archive_dir = self._out_dir / "archive"
    self._archive_dir.mkdir(parents=True, exist_ok=True)
    self._app_path = self._out_dir / "chrome"
    self._version_identifier: str = ""
    self._requested_version = (0, 0, 0, 0)
    self._requested_version_str = "0.0.0.0"
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
        raise ValueError(f"Chrome archive does not exist: {self._archive_path}")
      self._load_from_archive()

  @property
  def app_path(self) -> pathlib.Path:
    assert self._app_path.exists(), "Could not download browser"
    return self._app_path

  def _pre_check(self) -> None:
    assert not self._platform.is_remote, (
        "Browser download only supported on local machines")
    if self._version_identifier and not self._platform.which("gsutil"):
      raise ValueError(
          f"Cannot download chrome version {self._version_identifier}: "
          "please install gsutil.\n"
          "- https://cloud.google.com/storage/docs/gsutil_install\n"
          "- Run 'gcloud auth login' to get access to the archives "
          "(googlers only).")

  def _load_from_version(self) -> None:
    self._parse_version(self._version_identifier)
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
      logging.info("DOWNLOADING CHROME %s", self._version_identifier.upper())
      archive_url = self._find_archive_url()
      if not archive_url:
        raise ValueError(
            f"Could not find matching version for {self._requested_version}")
      self._download_archive(archive_url)
    else:
      logging.info("CACHED DOWNLOAD: %s", self._archive_path)
    self._extract_archive(self._archive_path)
    if not self._requested_exact_version:
      self._archive_path.unlink()

  def _version_check(self) -> None:
    major_version: int = self._requested_version[0]
    if self._platform.is_macos and self._platform.is_arm64 and (major_version <
                                                                87):
      raise ValueError(
          "Native Mac arm64/m1 chrome version is available with M87, "
          f"but requested M{major_version}.")

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
    assert self._app_path.exists, (
        f"Extraction failed, app does not exist: {self._app_path}")
    full_version_string = self._platform.app_version(self._app_path)
    self._parse_version(full_version_string)
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

  def _parse_version(self, version_identifier: str) -> None:
    match = self.VERSION_RE.search(version_identifier)
    assert match, f"Invalid chrome version identifier: {version_identifier}"
    self._version_identifier = version_identifier = match["version"]
    if version_identifier[0].upper() == "M":
      self._requested_version = (int(version_identifier[1:]), self.ANY_MARKER,
                                 self.ANY_MARKER, self.ANY_MARKER)
      self._requested_version_str = f"M{self._requested_version[0]}"
      self._requested_exact_version = False
    else:
      self._requested_version = tuple(map(int,
                                          version_identifier.split(".")))[:4]
      self._requested_version_str = ".".join(map(str, self._requested_version))
      self._requested_exact_version = True
    assert len(self._requested_version) == 4

  @abc.abstractmethod
  def _default_extracted_path(self) -> pathlib.Path:
    pass

  @abc.abstractmethod
  def _default_app_path(self) -> pathlib.Path:
    pass

  def _validate_cached(self) -> str:
    # "Google Chrome 107.0.5304.121" => "107.0.5304.121"
    app_version = self._platform.app_version(self._app_path)
    version_match = re.search(r"[\d\.]+", app_version)
    assert version_match, (
        f"Got invalid version string from chrome binary: {app_version}")
    cached_version_str = version_match.group(0)
    cached_version = tuple(map(int, cached_version_str.split(".")))
    assert len(cached_version) == 4, f"Got invalid version: {app_version}"
    if not self._version_matches(cached_version):
      raise ValueError(
          f"Previously downloaded browser at {self._app_path} might have been auto-updated.\n"
          "Please delete the old version and re-install/-download it.\n"
          f"Expected: {self._requested_version} Got: {cached_version}")
    return cached_version_str

  def _find_archive_url(self) -> Optional[str]:
    milestone: int = self._requested_version[0]
    # Quick probe for complete versions
    if self._requested_exact_version:
      test_url = f"{self.URL}{self._version_identifier}/{self._platform_name}/"
      logging.info("LIST VERSION for M%s (fast): %s", milestone, test_url)
      maybe_archive_url = self._filter_candidates([test_url])
      if maybe_archive_url:
        return maybe_archive_url
    list_url = f"{self.URL}{milestone}.*/{self._platform_name}/"
    logging.info("LIST ALL VERSIONS for M%s (slow): %s", milestone, list_url)
    try:
      listing: List[str] = self._platform.sh_stdout(
          "gsutil", "ls", "-d", list_url).strip().splitlines()
    except helper.SubprocessError as e:
      if "One or more URLs matched no objects" in str(e):
        raise ValueError(
            f"Could not find version {self._requested_version_str} "
            f"for {self._platform.name} {self._platform.machine} ") from e
      if "AccessDeniedException" in str(e):
        raise ValueError(f"Could not access {list_url}.\n"
                         "Please run `gcert` and `gcloud auth login` "
                         "(googlers only).") from e
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
        result = self._platform.sh_stdout("gsutil", "ls", archive_url)
      except helper.SubprocessError:
        continue
      if result:
        return archive_url
    return None

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

  def _download_archive(self, archive_url: str) -> None:
    logging.info("DOWNLOADING %s", archive_url)
    with tempfile.TemporaryDirectory(suffix="cb_download") as tmp_dir_name:
      tmp_dir = pathlib.Path(tmp_dir_name)
      self._platform.sh("gsutil", "cp", archive_url, tmp_dir)
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


class ChromeDownloaderLinux(ChromeDownloader):
  ARCHIVE_SUFFIX: str = ".rpm"

  @classmethod
  def is_valid(cls, path_or_identifier: Union[str, pathlib.Path],
               platform: helper.Platform) -> bool:
    if cls.VERSION_RE.fullmatch(str(path_or_identifier)):
      return True
    path = pathlib.Path(path_or_identifier)
    return path.exists() and path.suffix == cls.ARCHIVE_SUFFIX

  def __init__(self,
               version_identifier: Union[str, pathlib.Path],
               platform_name: str,
               platform: helper.Platform,
               cache_dir: Optional[pathlib.Path] = None):
    assert platform.is_linux
    if platform.is_x64:
      platform_name = "linux64"
    else:
      raise ValueError("Unsupported linux architecture for downloading chrome: "
                       f"got={platform.machine} supported=x64")
    super().__init__(version_identifier, platform_name, platform, cache_dir)

  def _default_extracted_path(self) -> pathlib.Path:
    return self._out_dir / self._requested_version_str

  def _default_app_path(self) -> pathlib.Path:
    return self._default_extracted_path() / "opt/google/chrome-unstable/chrome"

  def _archive_url(self, folder_url: str, version_str: str) -> str:
    return f"{folder_url}google-chrome-unstable-{version_str}-1.x86_64.rpm"

  def _extract_archive(self, archive_path: pathlib.Path) -> None:
    assert self._platform.which("rpm2cpio"), (
        "Need rpm2cpio to extract downloaded .rpm chrome archive")
    assert self._platform.which("cpio"), (
        "Need cpio to extract downloaded .rpm chrome archive")
    cpio_file = archive_path.with_suffix(".cpio")
    assert not cpio_file.exists()
    archive_path.parent.mkdir(parents=True, exist_ok=True)
    with cpio_file.open("w") as f:
      self._platform.sh("rpm2cpio", archive_path, stdout=f)
    assert cpio_file.is_file(), f"Could not extract archive: {archive_path}"
    extracted_path = self._default_extracted_path()
    assert not extracted_path.exists()
    with cpio_file.open() as f:
      self._platform.sh(
          "cpio",
          "--extract",
          f"--directory={extracted_path}",
          "--make-directories",
          stdin=f)
    assert self._app_path.is_file(), (
        f"Could not extract chrome binary: {self._app_path}")
    cpio_file.unlink()


class ChromeDownloaderMacOS(ChromeDownloader):
  ARCHIVE_SUFFIX: str = ".dmg"
  MIN_MAC_ARM64_VERSION = (87, 0, 0, 0)

  @classmethod
  def is_valid(cls, path_or_identifier: Union[str, pathlib.Path],
               platform: helper.Platform) -> bool:
    if cls.VERSION_RE.fullmatch(str(path_or_identifier)):
      return True
    path = pathlib.Path(path_or_identifier)
    return path.exists() and path.suffix == cls.ARCHIVE_SUFFIX

  def __init__(self,
               version_identifier: Union[str, pathlib.Path],
               platform_name: str,
               platform: helper.Platform,
               cache_dir: Optional[pathlib.Path] = None):
    assert platform.is_macos, f"{type(self)} can only be used on macOS"
    platform_name = "mac-universal"
    super().__init__(version_identifier, platform_name, platform, cache_dir)

  def _download_archive(self, archive_url: str) -> None:
    assert self._platform.is_macos
    if self._platform.is_arm64 and (self._requested_version <
                                    self.MIN_MAC_ARM64_VERSION):
      raise ValueError(
          "Chrome Arm64 Apple Silicon is only available starting with M87, "
          f"but requested {self._requested_version_str} is too old.")
    super()._download_archive(archive_url)

  def _archive_url(self, folder_url: str, version_str: str) -> str:
    # Use ChromeCanary since it's built for all version (unlike stable/beta).
    return f"{folder_url}GoogleChromeCanary-{version_str}.dmg"

  def _default_extracted_path(self) -> pathlib.Path:
    return self._default_app_path()

  def _default_app_path(self) -> pathlib.Path:
    return self._out_dir / f"Google Chrome {self._requested_version_str}.app"

  def _extract_archive(self, archive_path: pathlib.Path) -> None:
    result = self._platform.sh_stdout("hdiutil", "attach", "-plist",
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
      extracted_path = self._default_extracted_path()
      logging.info("COPYING BROWSER src=%s dst=%s", app, extracted_path)
      shutil.copytree(app, extracted_path, symlinks=True, dirs_exist_ok=False)
    finally:
      self._platform.sh("hdiutil", "detach", dmg_path)
