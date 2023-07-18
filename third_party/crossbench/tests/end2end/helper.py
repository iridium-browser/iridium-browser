# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Base class for the end-to-end unit tests
"""

import argparse
import logging
import pathlib
import shutil
import tempfile
import unittest
from abc import ABCMeta
from typing import Optional

import crossbench.browsers.all as browsers
from crossbench import cli_helper, helper


class End2EndTestCase(unittest.TestCase, metaclass=ABCMeta):
  """
  End to end crossbench tests that tests on live browsers and pages.
  These tests should not be run as part of the presubmit checks.
  """

  __test__ = False

  output_dir: pathlib.Path
  browser_path: pathlib.Path
  driver_path: Optional[pathlib.Path]

  def setUp(self) -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--test-browser-path",
        "--browserpath",
        default=None,
        type=cli_helper.parse_path)
    parser.add_argument(
        "--test-driver-path",
        "--driverpath",
        default=None,
        type=cli_helper.parse_path)
    # Use parse_known_args to allow for other custom arguments.
    args, _ = parser.parse_known_args()
    self.platform = helper.platform
    self.output_dir = pathlib.Path(tempfile.mkdtemp(suffix=type(self).__name__))
    self.browser_path = args.test_browser_path
    if self.browser_path:
      logging.info("browser path: %s", self.browser_path)
      assert self.browser_path.exists()
    else:
      logging.info("Trying default browser path for local runs.")
      self.browser_path = browsers.Chrome.stable_path()
    self.driver_path: Optional[pathlib.Path] = args.test_driver_path
    if self.driver_path:
      logging.info("driver path: %s", self.driver_path)
      assert self.driver_path.exists()

  def tearDown(self) -> None:
    shutil.rmtree(self.output_dir, True)
