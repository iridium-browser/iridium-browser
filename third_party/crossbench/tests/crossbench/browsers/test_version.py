# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import unittest
from typing import Optional, cast

from crossbench.browsers.chromium.version import ChromiumVersion
from crossbench.browsers.firefox.version import FirefoxVersion
from crossbench.browsers.safari.version import SafariVersion
from crossbench.browsers.version import BrowserVersion, BrowserVersionChannel


class _BrowserVersionTestCase(unittest.TestCase, metaclass=abc.ABCMeta):
  LTS_VERSION_STR: Optional[str] = ""
  STABLE_VERSION_STR: str = ""
  BETA_VERSION_STR: Optional[str] = ""
  ALPHA_VERSION_STR: Optional[str] = ""
  PRE_ALPHA_VERSION_STR: Optional[str] = ""

  @abc.abstractmethod
  def parse(self, value: str) -> BrowserVersion:
    pass

  def _parse_helper(self, value: str) -> BrowserVersion:
    self.assertTrue(value)
    version: BrowserVersion = self.parse(value)
    self.assertGreater(version.major, 0)
    self.assertGreaterEqual(version.minor, 0)
    return version

  def test_parse_lts(self) -> BrowserVersion:
    if self.LTS_VERSION_STR is None:
      self.skipTest("lts version not supported")
    version: BrowserVersion = self._parse_helper(self.LTS_VERSION_STR)
    self.assertEqual(version.channel, BrowserVersionChannel.LTS)
    self.assertTrue(version.is_lts)
    self.assertFalse(version.is_stable)
    self.assertFalse(version.is_beta)
    self.assertFalse(version.is_alpha)
    self.assertFalse(version.is_pre_alpha)
    return version

  def test_parse_stable(self) -> BrowserVersion:
    version: BrowserVersion = self._parse_helper(self.STABLE_VERSION_STR)
    self.assertEqual(version.channel, BrowserVersionChannel.STABLE)
    self.assertFalse(version.is_lts)
    self.assertTrue(version.is_stable)
    self.assertFalse(version.is_beta)
    self.assertFalse(version.is_alpha)
    self.assertFalse(version.is_pre_alpha)
    return version

  def test_parse_beta(self) -> BrowserVersion:
    if not self.BETA_VERSION_STR:
      self.skipTest("beta version not supported.")
    version: BrowserVersion = self._parse_helper(self.BETA_VERSION_STR)
    self.assertEqual(version.channel, BrowserVersionChannel.BETA)
    self.assertFalse(version.is_lts)
    self.assertFalse(version.is_stable)
    self.assertTrue(version.is_beta)
    self.assertFalse(version.is_alpha)
    self.assertFalse(version.is_pre_alpha)
    return version

  def test_parse_alpha(self) -> BrowserVersion:
    if self.ALPHA_VERSION_STR is None:
      self.skipTest("alpha version not supported")
    version: BrowserVersion = self._parse_helper(self.ALPHA_VERSION_STR)
    self.assertEqual(version.channel, BrowserVersionChannel.ALPHA)
    self.assertFalse(version.is_lts)
    self.assertFalse(version.is_stable)
    self.assertFalse(version.is_beta)
    self.assertTrue(version.is_alpha)
    self.assertFalse(version.is_pre_alpha)
    return version

  def test_parse_pre_alpha(self) -> BrowserVersion:
    if self.PRE_ALPHA_VERSION_STR is None:
      self.skipTest("nightly version not supported")
    version: BrowserVersion = self._parse_helper(self.PRE_ALPHA_VERSION_STR)
    self.assertEqual(version.channel, BrowserVersionChannel.PRE_ALPHA)
    self.assertFalse(version.is_lts)
    self.assertFalse(version.is_stable)
    self.assertFalse(version.is_beta)
    self.assertFalse(version.is_alpha)
    self.assertTrue(version.is_pre_alpha)
    return version

  def test_equal_stable(self):
    version_a = self.parse(self.STABLE_VERSION_STR)
    version_b = self.parse(self.STABLE_VERSION_STR)
    self.assertEqual(version_a, version_a)
    self.assertEqual(version_a, version_b)
    self.assertEqual(version_b, version_a)

  def test_no_equal_stable_beta(self):
    if not self.BETA_VERSION_STR:
      self.skipTest("beta version not supported.")
    version_stable = self.parse(self.STABLE_VERSION_STR)
    version_beta = self.parse(self.BETA_VERSION_STR)
    self.assertNotEqual(version_stable, version_beta)
    self.assertNotEqual(version_beta, version_stable)

  def test_stable_lt_beta(self):
    if not self.BETA_VERSION_STR:
      self.skipTest("beta version not supported.")
    version_stable = self.parse(self.STABLE_VERSION_STR)
    version_beta = self.parse(self.BETA_VERSION_STR)
    # pylint: disable=comparison-with-itself
    self.assertFalse(version_stable > version_stable)
    self.assertFalse(version_stable < version_stable)
    self.assertLess(version_stable, version_beta)
    self.assertGreater(version_beta, version_stable)

  def test_invalid(self):
    with self.assertRaises(ValueError):
      self.parse("")
    with self.assertRaises(ValueError):
      self.parse("no numbers here")


class ChromiumVersionTestCase(_BrowserVersionTestCase):
  LTS_VERSION_STR = None
  STABLE_VERSION_STR = "Google Chromium 115.0.5790.114"
  BETA_VERSION_STR = None
  ALPHA_VERSION_STR = None
  PRE_ALPHA_VERSION_STR = None

  def parse(self, value: str) -> BrowserVersion:
    return ChromiumVersion(value)


class ChromeBrowserVersionTestCase(_BrowserVersionTestCase):
  LTS_VERSION_STR = None
  STABLE_VERSION_STR = "Google Chrome 115.0.5790.114"
  BETA_VERSION_STR = "Google Chrome 116.0.5845.50 beta"
  ALPHA_VERSION_STR = "Google Chrome 117.0.5911.2 dev"
  PRE_ALPHA_VERSION_STR = "Google Chrome 117.0.5921.0 canary"

  def parse(self, value: str) -> BrowserVersion:
    return ChromiumVersion(value)

  def test_parse_invalid(self):
    with self.assertRaises(ValueError):
      self.parse("Google Chrome 115.0.5790.114.0.0.")
    with self.assertRaises(ValueError):
      self.parse("Google Chrome 115.0.5790..114")
    with self.assertRaises(ValueError):
      self.parse("Google Chrome 115.a.5790.114")

  def test_str(self):
    self.assertEqual(
        str(self.parse(self.STABLE_VERSION_STR)), "115.0.5790.114 stable")
    self.assertEqual(
        str(self.parse(self.BETA_VERSION_STR)), "116.0.5845.50 beta")
    self.assertEqual(
        str(self.parse(self.ALPHA_VERSION_STR)), "117.0.5911.2 dev")
    self.assertEqual(
        str(self.parse(self.PRE_ALPHA_VERSION_STR)), "117.0.5921.0 canary")

  def test_parse_stable(self) -> BrowserVersion:
    version = super().test_parse_stable()
    self.assertEqual(version.major, 115)
    self.assertEqual(version.minor, 0)
    self.assertEqual(version.minor, 0)
    self.assertEqual(version.channel_name, "stable")
    chrome_version = cast(ChromiumVersion, version)
    self.assertEqual(chrome_version.build, 5790)
    self.assertEqual(chrome_version.patch, 114)
    self.assertFalse(chrome_version.is_dev)
    self.assertFalse(chrome_version.is_canary)
    return version

  def test_parse_beta(self) -> BrowserVersion:
    version = super().test_parse_beta()
    self.assertEqual(version.major, 116)
    self.assertEqual(version.minor, 0)
    self.assertEqual(version.channel_name, "beta")
    chrome_version = cast(ChromiumVersion, version)
    self.assertEqual(chrome_version.build, 5845)
    self.assertEqual(chrome_version.patch, 50)
    self.assertFalse(chrome_version.is_dev)
    self.assertFalse(chrome_version.is_canary)
    return version

  def test_parse_alpha(self) -> BrowserVersion:
    version = super().test_parse_alpha()
    self.assertEqual(version.major, 117)
    self.assertEqual(version.minor, 0)
    self.assertEqual(version.channel_name, "dev")
    chrome_version = cast(ChromiumVersion, version)
    self.assertEqual(chrome_version.build, 5911)
    self.assertEqual(chrome_version.patch, 2)
    self.assertTrue(chrome_version.is_dev)
    self.assertFalse(chrome_version.is_canary)
    return version

  def test_parse_pre_alpha(self) -> BrowserVersion:
    version = super().test_parse_pre_alpha()
    self.assertEqual(version.major, 117)
    self.assertEqual(version.minor, 0)
    self.assertEqual(version.channel_name, "canary")
    chrome_version = cast(ChromiumVersion, version)
    self.assertEqual(chrome_version.build, 5921)
    self.assertEqual(chrome_version.patch, 0)
    self.assertFalse(chrome_version.is_dev)
    self.assertTrue(chrome_version.is_canary)
    return version


class FirefoxVersionTestCase(_BrowserVersionTestCase):
  LTS_VERSION_STR = "Mozilla Firefox 114.0.1esr"
  STABLE_VERSION_STR = "Mozilla Firefox 115.0.3"
  # IRL Firefox version numbers do not distinct beta from stable. so we
  # remap Firefox Dev => beta.
  BETA_VERSION_STR = "Mozilla Firefox 116.0b4"
  ALPHA_VERSION_STR = "Mozilla Firefox 117.0a1"
  PRE_ALPHA_VERSION_STR = None

  def parse(self, value: str) -> BrowserVersion:
    return FirefoxVersion(value)

  def test_parse_invalid(self):
    with self.assertRaises(ValueError):
      self.parse("Mozilla Firefox 116.0b4esr")
    with self.assertRaises(ValueError):
      self.parse("Mozilla Firefox 116.0X4")
    with self.assertRaises(ValueError):
      self.parse("Mozilla Firefox 116.0a4b5")
    with self.assertRaises(ValueError):
      self.parse("Mozilla Firefox 116.10.0a")
    with self.assertRaises(ValueError):
      self.parse("Mozilla Firefox 116..0a")

  def test_parse_lts(self) -> BrowserVersion:
    version = super().test_parse_lts()
    self.assertEqual(version.major, 114)
    self.assertEqual(version.minor, 0)
    self.assertEqual(version.channel_name, "esr")
    return version

  def test_parse_stable(self) -> BrowserVersion:
    version = super().test_parse_stable()
    self.assertEqual(version.major, 115)
    self.assertEqual(version.minor, 0)
    self.assertEqual(version.channel_name, "stable")
    return version

  def test_parse_beta(self) -> BrowserVersion:
    version = super().test_parse_beta()
    self.assertEqual(version.major, 116)
    self.assertEqual(version.minor, 0)
    self.assertEqual(version.channel_name, "dev")
    return version

  def test_parse_alpha(self) -> BrowserVersion:
    version = super().test_parse_alpha()
    self.assertEqual(version.major, 117)
    self.assertEqual(version.minor, 0)
    self.assertEqual(version.channel_name, "nightly")
    return version

  def test_str(self):
    self.assertEqual(str(self.parse(self.LTS_VERSION_STR)), "114.0.1 esr")
    self.assertEqual(str(self.parse(self.STABLE_VERSION_STR)), "115.0.3 stable")
    self.assertEqual(str(self.parse(self.BETA_VERSION_STR)), "116.0b4 dev")
    self.assertEqual(str(self.parse(self.ALPHA_VERSION_STR)), "117.0a1 nightly")


class SafariBrowserVersionTestCase(_BrowserVersionTestCase):
  LTS_VERSION_STR = None
  # Additionally use the `safaridriver --version``
  STABLE_VERSION_STR = "16.6 Included with Safari 16.6 (18615.3.12.11.2)"
  BETA_VERSION_STR = ("17.0 Included with Safari Technology Preview "
                      "(Release 175, 18617.1.1.2)")
  ALPHA_VERSION_STR = None
  PRE_ALPHA_VERSION_STR = None

  def parse(self, value: str) -> BrowserVersion:
    return SafariVersion(value)

  def test_parse_invalid(self):
    with self.assertRaises(ValueError):
      self.parse("(Release 175, 18617.1.1.2)")
    with self.assertRaises(ValueError):
      self.parse("16.7 (Release 175, 18617.1.1.2)")
    with self.assertRaises(ValueError):
      self.parse("16.7 XXX (Release, 18617.1.1.2)")
    with self.assertRaises(ValueError):
      self.parse("16.6 XXX (18615.3...12.11.2)")
    with self.assertRaises(ValueError):
      self.parse("16.6 XXX (18615.3)")

  def test_parse_stable(self) -> BrowserVersion:
    version = super().test_parse_stable()
    self.assertEqual(version.major, 16)
    self.assertEqual(version.minor, 6)
    safari_version = cast(SafariVersion, version)
    self.assertFalse(safari_version.is_tech_preview)
    self.assertEqual(safari_version.release, 0)
    self.assertEqual(version.channel_name, "stable")
    return version

  def test_parse_beta(self) -> BrowserVersion:
    version = super().test_parse_beta()
    self.assertEqual(version.major, 17)
    self.assertEqual(version.minor, 0)
    safari_version = cast(SafariVersion, version)
    self.assertTrue(safari_version.is_tech_preview)
    self.assertEqual(safari_version.release, 175)
    self.assertEqual(version.channel_name, "technology preview")
    return version

  def test_str(self):
    self.assertEqual(
        str(self.parse(self.STABLE_VERSION_STR)),
        "16.6 (18615.3.12.11.2) stable")
    self.assertEqual(
        str(self.parse(self.BETA_VERSION_STR)),
        "17.0 (Release 175, 18617.1.1.2) technology preview")


# Hide the abstract base test class from all test runner
del _BrowserVersionTestCase
