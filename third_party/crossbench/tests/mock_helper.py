# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import pathlib
from typing import TYPE_CHECKING, Type
from unittest import mock

import psutil
from pyfakefs import fake_filesystem_unittest

from crossbench import helper
import crossbench
from crossbench.benchmarks.base import SubStoryBenchmark
from crossbench.cli import CrossBenchCLI
from crossbench.flags import Flags
from crossbench.stories import Story

if TYPE_CHECKING:
  from crossbench.runner import Runner

from . import mock_browser

GIB = 1014**3

ActivePlatformClass: Type[helper.Platform] = type(helper.platform)


class MockPlatform(ActivePlatformClass):

  def __init__(self, is_battery_powered=False):
    self._is_battery_powered = is_battery_powered

  @property
  def is_battery_powered(self):
    return self._is_battery_powered

  def is_thermal_throttled(self) -> bool:
    return False

  def disk_usage(self, path: pathlib.Path):
    del path
    # pylint: disable=protected-access
    return psutil._common.sdiskusage(
        total=GIB * 100, used=20 * GIB, free=80 * GIB, percent=20)

  def cpu_usage(self):
    return 0.1

  def system_details(self):
    return {"CPU": "20-core 3.1 GHz"}

  def sleep(self, duration):
    del duration

  def processes(self, attrs=()):
    del attrs
    return []

  def process_children(self, parent_pid: int, recursive=False):
    del parent_pid, recursive
    return []

  def foreground_process(self):
    return None


mock_platform = MockPlatform()  # pytype: disable=not-instantiable


class MockStory(Story):
  pass


class MockBenchmark(SubStoryBenchmark):
  DEFAULT_STORY_CLS = MockStory


class MockCLI(CrossBenchCLI):
  runner: Runner

  def _get_runner(self, args, benchmark, env_config, env_validation_mode,
                  timing):
    if not args.out_dir:
      # Use stable mock out dir
      args.out_dir = pathlib.Path("/results")
      assert not args.out_dir.exists()
    runner_kwargs = self.RUNNER_CLS.kwargs_from_cli(args)
    self.runner = self.RUNNER_CLS(
        benchmark=benchmark,
        env_config=env_config,
        env_validation_mode=env_validation_mode,
        timing=timing,
        **runner_kwargs,
        # Use custom platform
        platform=mock_platform)
    return self.runner


class BaseCrossbenchTestCase(
    fake_filesystem_unittest.TestCase, metaclass=abc.ABCMeta):

  def setUp(self):
    super().setUp()
    self.setUpPyfakefs(modules_to_reload=[crossbench, mock_browser])
    for mock_browser_cls in mock_browser.ALL:
      mock_browser_cls.setup_fs(self.fs)
      self.assertTrue(mock_browser_cls.APP_PATH.exists())
    self.platform = mock_platform
    self.out_dir = pathlib.Path("/tmp/results/test")
    self.out_dir.parent.mkdir(parents=True)
    self.browsers = [
        mock_browser.MockChromeDev("dev", platform=self.platform),
        mock_browser.MockChromeStable("stable", platform=self.platform)
    ]
    self.sleep_patcher = mock.patch('time.sleep', return_value=None)
    self.sleep_patcher.start()

  def tearDown(self) -> None:
    self.sleep_patcher.stop()
    super().tearDown()
