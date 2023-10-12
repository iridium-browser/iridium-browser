# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import pathlib
import shutil
import unittest
from typing import Union

from crossbench import compat
from crossbench.browsers.firefox.downloader import FirefoxDownloader
from crossbench.browsers.firefox.webdriver import (FirefoxWebDriver,
                                                   FirefoxDriverFinder)
from crossbench import plt
from tests import run_helper


@unittest.skipIf(not plt.PLATFORM.is_macos, "Only supported on macOS")
class FirefoxDownloaderTestCase():

  def _load_and_check_version(self,
                              output_dir: pathlib.Path,
                              archive_dir: pathlib.Path,
                              version_or_archive: Union[str, pathlib.Path],
                              version_str: str,
                              expect_archive: bool = True) -> pathlib.Path:
    app_path: pathlib.Path = FirefoxDownloader.load(version_or_archive,
                                                    plt.PLATFORM, output_dir)
    assert compat.is_relative_to(app_path, output_dir)
    assert archive_dir.exists()
    assert app_path.exists()
    if plt.PLATFORM.is_macos:
      assert set(output_dir.iterdir()) == {app_path, archive_dir}
    assert version_str in plt.PLATFORM.app_version(app_path)
    archives = list(archive_dir.iterdir())
    if expect_archive:
      assert len(archives) == 1
    else:
      assert not archives
    assert app_path.exists()
    browser = FirefoxWebDriver("test-browser", app_path, platform=plt.PLATFORM)
    # TODO: fix using dedicated Version object
    base_version_str = version_str.split("b")[0]
    assert base_version_str in browser.version
    self._load_and_check_webdriver(output_dir, browser)
    return app_path

  def _load_and_check_webdriver(self, output_dir,
                                browser: FirefoxWebDriver) -> None:
    driver_dir = output_dir / "chromedriver-binaries"
    driver_dir.mkdir()
    finder = FirefoxDriverFinder(browser, cache_dir=driver_dir)
    assert not list(driver_dir.iterdir())
    driver_path: pathlib.Path = finder.download()
    assert list(driver_dir.iterdir()) == [driver_path]
    assert driver_path.is_file()
    # Downloading again should use the cache-version
    driver_path: pathlib.Path = finder.download()
    assert list(driver_dir.iterdir()) == [driver_path]
    assert driver_path.is_file()
    # Restore output dir state.
    driver_path.unlink()
    driver_dir.rmdir()

  def test_download_specific_version(self, output_dir, archive_dir) -> None:
    assert not list(output_dir.iterdir())
    version_str = "106.0.4"
    self._load_and_check_version(output_dir, archive_dir,
                                 f"firefox-{version_str}", version_str)

    # Re-downloading should work as well and hit the extracted app.
    app_path = self._load_and_check_version(output_dir, archive_dir,
                                            f"firefox-{version_str}",
                                            version_str)

    # Delete the extracted app and reload, should reuse the cached archive.
    if plt.PLATFORM.is_macos:
      shutil.rmtree(app_path)
    else:
      shutil.rmtree(output_dir.output_dir / version_str)
    assert not app_path.exists()
    app_path = self._load_and_check_version(output_dir, archive_dir,
                                            f"firefox-{version_str}",
                                            version_str)
    # Delete app and install from archive.
    if plt.PLATFORM.is_macos:
      shutil.rmtree(app_path)
    else:
      shutil.rmtree(output_dir.output_dir / version_str)
    assert not app_path.exists()
    archives = list(archive_dir.iterdir())
    assert len(archives) == 1
    archive = archives[0]
    app_path = self._load_and_check_version(output_dir, archive_dir, archive,
                                            version_str)
    assert list(archive_dir.iterdir()) == [archive]

  def test_download_specific_beta_version(self, output_dir,
                                          archive_dir) -> None:
    assert not list(output_dir.iterdir())
    version_str = "115.0b4"
    self._load_and_check_version(output_dir, archive_dir,
                                 f"firefox-{version_str}", version_str)

    # Re-downloading should work as well and hit the extracted app.
    app_path = self._load_and_check_version(output_dir, archive_dir,
                                            f"firefox-{version_str}",
                                            version_str)

    # Delete the extracted app and reload, should reuse the cached archive.
    if plt.PLATFORM.is_macos:
      shutil.rmtree(app_path)
    else:
      shutil.rmtree(output_dir.output_dir / version_str)
    assert not app_path.exists()
    app_path = self._load_and_check_version(output_dir, archive_dir,
                                            f"firefox-{version_str}",
                                            version_str)

    # Delete app and install from archive.
    if plt.PLATFORM.is_macos:
      shutil.rmtree(app_path)
    else:
      shutil.rmtree(output_dir.output_dir / version_str)
    assert not app_path.exists()
    archives = list(archive_dir.iterdir())
    assert len(archives) == 1
    archive = archives[0]
    app_path = self._load_and_check_version(output_dir, archive_dir, archive,
                                            version_str)
    assert list(archive_dir.iterdir()) == [archive]


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
