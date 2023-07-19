# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import pathlib
import sys
from unittest import mock
import pytest
import abc

from crossbench.browsers.chrome.downloader import ChromeDownloader
from tests.crossbench.mock_helper import BaseCrossbenchTestCase


class AbstractChromeDownloaderTestCase(
    BaseCrossbenchTestCase, metaclass=abc.ABCMeta):
  __test__ = False

  def setUp(self) -> None:
    super().setUp()
    self.platform = mock.Mock(is_remote=False, is_linux=False, is_macos=False)
    self.platform.which = lambda x: True

  def test_wrong_versions(self) -> None:
    with self.assertRaises(ValueError):
      ChromeDownloader.load("", self.platform)
    with self.assertRaises(ValueError):
      ChromeDownloader.load("M", self.platform)
    with self.assertRaises(ValueError):
      ChromeDownloader.load("M-100", self.platform)
    with self.assertRaises(ValueError):
      ChromeDownloader.load("M100.1.2.3.4.5", self.platform)
    with self.assertRaises(ValueError):
      ChromeDownloader.load("100.1.2.3.4.5", self.platform)

  def test_empty_path(self) -> None:
    with self.assertRaises(ValueError):
      ChromeDownloader.load(pathlib.Path("custom"), self.platform)

  def test_load_valid_no_googler(self) -> None:
    self.platform.which = lambda x: False
    with self.assertRaises(ValueError):
      ChromeDownloader.load("chrome-111.0.5563.110", self.platform)

  def test_is_valid_strings(self) -> None:
    self.assertFalse(ChromeDownloader.is_valid("", self.platform))
    self.assertFalse(ChromeDownloader.is_valid("MM100", self.platform))
    self.assertTrue(ChromeDownloader.is_valid("M100", self.platform))
    self.assertTrue(ChromeDownloader.is_valid("chrome-m100", self.platform))
    self.assertFalse(
        ChromeDownloader.is_valid("M100.1.2.123.9999", self.platform))
    self.assertFalse(
        ChromeDownloader.is_valid("M111.0.5563.110", self.platform))
    self.assertTrue(ChromeDownloader.is_valid("111.0.5563.110", self.platform))

  def test_is_valid_path(self) -> None:
    self.assertFalse(
        ChromeDownloader.is_valid(pathlib.Path("custom"), self.platform))
    path = pathlib.Path("download/archive.foo")
    self.fs.create_file(path)
    self.assertFalse(ChromeDownloader.is_valid(path, self.platform))


class BasicChromeDownloaderTestCaseLinux(AbstractChromeDownloaderTestCase):
  __test__ = True

  def setUp(self) -> None:
    super().setUp()
    self.platform.is_linux = True

  def test_is_valid_archive(self) -> None:
    path = pathlib.Path("download/archive.rpm")
    self.fs.create_file(path)
    self.assertTrue(ChromeDownloader.is_valid(path, self.platform))


class BasicChromeDownloaderTestCaseMacOS(AbstractChromeDownloaderTestCase):
  __test__ = True

  def setUp(self) -> None:
    super().setUp()
    self.platform.is_macos = True

  def test_is_valid_archive(self) -> None:
    path = pathlib.Path("download/archive.dmg")
    self.fs.create_file(path)
    self.assertTrue(ChromeDownloader.is_valid(path, self.platform))


if __name__ == "__main__":
  sys.exit(pytest.main([__file__]))
