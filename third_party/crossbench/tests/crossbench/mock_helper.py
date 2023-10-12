# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import logging
import pathlib
import platform
from typing import (TYPE_CHECKING, Any, Dict, List, Mapping, Optional, Sequence,
                    Tuple, Type, Union)
from unittest import mock

import psutil
from pyfakefs import fake_filesystem_unittest

import crossbench
from crossbench import plt
from crossbench.benchmarks.benchmark import SubStoryBenchmark
from crossbench.cli import CrossBenchCLI
from crossbench.plt.base import MachineArch
from crossbench.stories.story import Story

if TYPE_CHECKING:
  from crossbench.runner.runner import Runner

from . import mock_browser

GIB = 1014**3

ActivePlatformClass: Type[plt.Platform] = type(plt.PLATFORM)

ShellArgsT = Tuple[Union[str, pathlib.Path]]


class MockPlatform(ActivePlatformClass):

  def __init__(self, is_battery_powered=False):
    self._is_battery_powered = is_battery_powered
    # Cache some helper properties that might fail under pyfakefs.
    self._key = plt.PLATFORM.key
    self._machine: MachineArch = plt.PLATFORM.machine
    self.sh_cmds: List[ShellArgsT] = []
    self.expected_sh_cmds: Optional[List[ShellArgsT]] = None
    self.sh_results: List[str] = []

  def expect_sh(self,
                *args: Union[str, pathlib.Path],
                result: str = "") -> None:
    if self.expected_sh_cmds is None:
      self.expected_sh_cmds = []
    self.expected_sh_cmds.insert(0, args)
    self.sh_results.insert(0, result)

  @property
  def key(self) -> str:
    return f"mock-{self._key}"

  @property
  def machine(self) -> MachineArch:
    return self._machine

  @property
  def version(self) -> str:
    return "1.2.3.4.5"

  @property
  def device(self) -> str:
    return "TestBook Pro"

  @property
  def cpu(self) -> str:
    return "Mega CPU @ 3.00GHz"

  @property
  def is_battery_powered(self) -> bool:
    return self._is_battery_powered

  def is_thermal_throttled(self) -> bool:
    return False

  def disk_usage(self, path: pathlib.Path):
    del path
    # pylint: disable=protected-access
    return psutil._common.sdiskusage(
        total=GIB * 100, used=20 * GIB, free=80 * GIB, percent=20)

  def cpu_usage(self) -> float:
    return 0.1

  def cpu_details(self) -> Dict[str, Any]:
    return {"physical cores": 2, "logical cores": 4, "info": self.cpu}

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

  def sh_stdout(self,
                *args: Union[str, pathlib.Path],
                shell: bool = False,
                quiet: bool = False,
                encoding: str = "utf-8",
                env: Optional[Mapping[str, str]] = None,
                check: bool = True) -> str:
    del shell, quiet, encoding, env, check
    if self.expected_sh_cmds is not None:
      assert self.expected_sh_cmds, f"Missing expected sh_cmds, but got: {args}"
      expected = self.expected_sh_cmds.pop()
      assert expected == args, f"Expected sh_cmd: {expected}, got: {args}"
    self.sh_cmds.append(args)
    if not self.sh_results:
      raise ValueError("MockPlatform has no more sh outputs.")
    return self.sh_results.pop()

  def sh(self,
         *args: Union[str, pathlib.Path],
         shell: bool = False,
         capture_output: bool = False,
         stdout=None,
         stderr=None,
         stdin=None,
         env: Optional[Mapping[str, str]] = None,
         quiet: bool = False,
         check: bool = False):
    raise NotImplementedError("MockPlatform does not support generic sh().")



class MockStory(Story):
  pass


class MockBenchmark(SubStoryBenchmark):
  DEFAULT_STORY_CLS = MockStory


class MockCLI(CrossBenchCLI):
  runner: Runner
  platform: MockPlatform

  def __init__(self, *args, **kwargs) -> None:
    self.platform = kwargs.pop("platform")
    super().__init__(*args, **kwargs)

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
        platform=self.platform)
    return self.runner


class CrossbenchFakeFsTestCase(
    fake_filesystem_unittest.TestCase, metaclass=abc.ABCMeta):

  def setUp(self) -> None:
    super().setUp()
    self.setUpPyfakefs(modules_to_reload=[crossbench, mock_browser])
    # gettext is used extensively in argparse
    gettext_patcher = mock.patch(
        "gettext.dgettext", side_effect=lambda domain, message: message)
    gettext_patcher.start()
    self.addCleanup(gettext_patcher.stop)
    sleep_patcher = mock.patch('time.sleep', return_value=None)
    self.addCleanup(sleep_patcher.stop)


class BaseCrossbenchTestCase(CrossbenchFakeFsTestCase, metaclass=abc.ABCMeta):

  def filter_data_urls(self, urls: Sequence[str]) -> List[str]:
    return [url for url in urls if not url.startswith("data:")]

  def setUp(self) -> None:
    # Instantiate MockPlatform before setting up fake_filesystem so we can
    # still interact with the original, real plt.Platform object for extracting
    # basic system information.
    self.platform = MockPlatform()  # pytype: disable=not-instantiable
    super().setUp()
    self._default_log_level = logging.getLogger().getEffectiveLevel()
    logging.getLogger().setLevel(logging.CRITICAL)
    for mock_browser_cls in mock_browser.ALL:
      mock_browser_cls.setup_fs(self.fs)
      self.assertTrue(mock_browser_cls.APP_PATH.exists())
    self.out_dir = pathlib.Path("/tmp/results/test")
    self.out_dir.parent.mkdir(parents=True)
    self.browsers = [
        mock_browser.MockChromeDev("dev", platform=self.platform),
        mock_browser.MockChromeStable("stable", platform=self.platform)
    ]
    mock_platform_patcher = mock.patch.object(plt, "PLATFORM", self.platform)
    mock_platform_patcher.start()
    self.addCleanup(mock_platform_patcher.stop)
    for browser in self.browsers:
      self.assertListEqual(browser.js_side_effects, [])

  def tearDown(self) -> None:
    logging.getLogger().setLevel(self._default_log_level)
    self.assertListEqual(self.platform.sh_results, [])
    super().tearDown()
