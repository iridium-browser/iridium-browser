# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import unittest

from crossbench import plt
from crossbench.plt.posix import PosixPlatform
from tests.crossbench.mock_helper import MockPlatform


class BasePlatformTestCase(unittest.TestCase, metaclass=abc.ABCMeta):
  __test__ = False
  platform: plt.Platform

  def setUp(self) -> None:
    super().setUp()
    self.mock_platform = MockPlatform()  # pytype: disable=not-instantiable

  def tearDown(self):
    self.assertFalse(self.mock_platform.expected_sh_cmds)
    super().tearDown()

  def expect_sh(self, *args, result=""):
    self.mock_platform.expect_sh(*args, result=result)

  def test_is_android(self):
    self.assertFalse(self.platform.is_android)

  def test_is_macos(self):
    self.assertFalse(self.platform.is_macos)

  def test_is_linux(self):
    self.assertFalse(self.platform.is_linux)

  def test_is_win(self):
    self.assertFalse(self.platform.is_win)

  def test_is_posix(self):
    self.assertFalse(self.platform.is_posix)


class PosixPlatformTestCase(BasePlatformTestCase):
  platform: PosixPlatform

  def tearDown(self) -> None:
    assert isinstance(self.platform, PosixPlatform)
    super().tearDown()

  def test_is_posix(self):
    self.assertTrue(self.platform.is_posix)
