# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse

from crossbench.browsers.chrome.webdriver import ChromeWebDriver
from tests import run_helper
from tests.crossbench import mock_browser
from tests.crossbench.mock_helper import BaseCrossbenchTestCase


class ChromeWebDriverForTesting(ChromeWebDriver):

  def _extract_version(self) -> str:
    return mock_browser.MockChromeStable.VERSION


class ChromeWebdriverTestCase(BaseCrossbenchTestCase):

  def test_conflicting_finch_flags(self) -> None:
    with self.assertRaises(argparse.ArgumentTypeError) as cm:
      ChromeWebDriverForTesting(
          label="browser-label",
          path=mock_browser.MockChromeStable.APP_PATH,
          js_flags=[],
          flags=["--disable-field-trial-config", "--enable-field-trial-config"],
          platform=self.platform)
    msg = str(cm.exception)
    self.assertIn("--enable-field-trial-config", msg)
    self.assertIn("--disable-field-trial-config", msg)

  def test_auto_disabling_field_trials(self):
    browser = ChromeWebDriverForTesting(
        label="browser-label",
        path=mock_browser.MockChromeStable.APP_PATH,
        platform=self.platform)
    self.assertIn("--disable-field-trial-config", browser.flags)

    browser_field_trial = ChromeWebDriverForTesting(
        label="browser-label",
        path=mock_browser.MockChromeStable.APP_PATH,
        flags=["--force-fieldtrials"],
        platform=self.platform)
    self.assertIn("--force-fieldtrials", browser_field_trial.flags)
    self.assertNotIn("--disable-field-trial-config", browser_field_trial.flags)


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
