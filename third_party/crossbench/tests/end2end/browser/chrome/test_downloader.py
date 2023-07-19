# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import shutil
import pathlib
from typing import Union
import pytest
import sys
from crossbench.browsers.chrome.downloader import ChromeDownloader
from tests.end2end.helper import End2EndTestCase


class ChromeDownloaderTestCase(End2EndTestCase):
  __test__ = True

  def setUp(self) -> None:
    super().setUp()
    if not self.platform.which("gsutil"):
      self.skipTest("Missing required 'gsutil', skipping test.")
    self.archive_dir = self.output_dir / "archive"
    self.assertFalse(self.archive_dir.exists())

  def load_and_check_version(self, version_or_archive: Union[str, pathlib.Path],
                             version_str: str) -> pathlib.Path:
    app_path = ChromeDownloader.load(version_or_archive, self.platform,
                                     self.output_dir)
    self.assertSetEqual(
        set(self.output_dir.iterdir()), {app_path, self.archive_dir})
    self.assertIn(version_str, self.platform.app_version(app_path))
    archives = list(self.archive_dir.iterdir())
    self.assertEqual(len(archives), 1)
    return app_path

  def test_download_major_version(self) -> None:
    self.assertListEqual(list(self.output_dir.iterdir()), [])
    self.load_and_check_version("chrome-M111", "111")

    # Re-downloading should reuse the extracted app.
    app_path = self.load_and_check_version("chrome-M111", "111")

    # Delete the extracted app and reload, should reuse the cached archive.
    shutil.rmtree(app_path)
    self.assertFalse(app_path.exists())
    self.load_and_check_version("chrome-M111", "111")

  def test_download_specific_version(self) -> None:
    self.assertListEqual(list(self.output_dir.iterdir()), [])
    version_str = "111.0.5563.110"
    self.load_and_check_version(f"chrome-{version_str}", version_str)

    # Re-downloading should work as well and hit the extracted app.
    app_path = self.load_and_check_version(f"chrome-{version_str}", version_str)

    # Delete the extracted app and reload, should reuse the cached archive.
    shutil.rmtree(app_path)
    self.assertFalse(app_path.exists())
    app_path = self.load_and_check_version(f"chrome-{version_str}", version_str)

    # Delete app and install from archive.
    shutil.rmtree(app_path)
    self.assertFalse(app_path.exists())
    archives = list(self.archive_dir.iterdir())
    self.assertEqual(len(archives), 1)
    archive = archives[0]
    app_path = self.load_and_check_version(archive, version_str)
    self.assertListEqual(list(self.archive_dir.iterdir()), [archive])


if __name__ == "__main__":
  sys.exit(pytest.main([__file__]))
