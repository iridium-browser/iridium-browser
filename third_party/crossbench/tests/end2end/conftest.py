# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import pathlib
import tempfile
from typing import Optional

import pytest

from crossbench import cli_helper
from crossbench.browsers import all as browsers

# pytest.fixtures rely on params having the same name as the fixture function
# pylint: disable=redefined-outer-name


def pytest_addoption(parser):
  parser.addoption(
      "--test-browser-path",
      "--browserpath",
      default=None,
      type=cli_helper.parse_path)
  parser.addoption(
      "--test-driver-path",
      "--driverpath",
      default=None,
      type=cli_helper.parse_path)


@pytest.fixture(scope="session", autouse=True)
def driver_path(request) -> Optional[pathlib.Path]:
  maybe_driver_path: Optional[pathlib.Path] = request.config.getoption(
      "--test-driver-path")
  if maybe_driver_path:
    logging.info("driver path: %s", maybe_driver_path)
    assert maybe_driver_path.exists()
  return maybe_driver_path


@pytest.fixture(scope="session", autouse=True)
def browser_path(request) -> pathlib.Path:
  maybe_browser_path: Optional[pathlib.Path] = request.config.getoption(
      "--test-browser-path")
  if maybe_browser_path:
    logging.info("browser path: %s", maybe_browser_path)
    assert maybe_browser_path.exists()
    return maybe_browser_path
  logging.info("Trying default browser path for local runs.")
  return browsers.Chrome.stable_path()


@pytest.fixture
def output_dir():
  with tempfile.TemporaryDirectory() as tmpdirname:
    yield pathlib.Path(tmpdirname)


@pytest.fixture(scope="session")
def root_dir() -> pathlib.Path:
  return pathlib.Path(__file__).parents[2]


@pytest.fixture
def cache_dir(output_dir) -> pathlib.Path:
  path = output_dir / "cache"
  assert not path.exists()
  path.mkdir()
  return path


@pytest.fixture
def archive_dir(output_dir) -> pathlib.Path:
  path = output_dir / "archive"
  assert not path.exists()
  return path
