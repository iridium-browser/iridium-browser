# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import argparse
import datetime as dt
import pathlib
from typing import (TYPE_CHECKING, Any, Dict, Generic, Iterable, Optional, Set,
                    Type, TypeVar)

from crossbench import helper
from crossbench.config import ConfigParser
from crossbench import plt
from crossbench.probes.results import (BrowserProbeResult, EmptyProbeResult,
                                       ProbeResult)

if TYPE_CHECKING:
  from selenium.webdriver.common.options import BaseOptions

  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench import plt
  from crossbench.runner.groups import (BrowsersRunGroup, RepetitionsRunGroup,
                                        StoriesRunGroup)
  from crossbench.runner.run import Run
  from crossbench.runner.runner import Runner

ProbeT = TypeVar("ProbeT", bound="Probe")


class ProbeConfigParser(ConfigParser[ProbeT]):

  def __init__(self, probe_cls: Type[ProbeT]) -> None:
    super().__init__("Probe", probe_cls)
    self._probe_cls = probe_cls


class ResultLocation(helper.EnumWithHelp):
  LOCAL = ("local",
           "Probe always produces results on the runner's local platform.")
  BROWSER = ("browser",
             ("Probe produces results on the browser's platform. "
              "This can be either remote (for instance android browser) "
              "or local (default system browser)."))


class ProbeMissingDataError(ValueError):
  pass


class Probe(abc.ABC):
  """
  Abstract Probe class.

  Probes are responsible for extracting performance numbers from websites
  / stories.

  Probe interface:
  - scope(): Return a custom ProbeScope (see below)
  - is_compatible(): Use for checking whether a Probe can be used with a
    given browser
  - pre_check(): Customize to display warnings before using Probes with
    incompatible settings.
  The Probe object can the customize how to merge probe (performance) date at
  multiple levels:
  - multiple repetitions of the same story
  - merged repetitions from multiple stories (same browser)
  - Probe data from all Runs

  Probes use a ProbeScope that is active during a story-Run.
  The ProbeScope class defines a customizable interface
  - setup(): Used for high-overhead Probe initialization
  - start(): Low-overhead start-to-measure signal
  - stop():  Low-overhead stop-to-measure signal
  - tear_down(): Used for high-overhead Probe cleanup

  """

  @property
  @abc.abstractmethod
  def NAME(self) -> str:
    pass

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    return ProbeConfigParser(cls)

  @classmethod
  def from_config(cls: Type[ProbeT], config_data: Dict) -> ProbeT:
    config_parser = cls.config_parser()
    kwargs: Dict[str, Any] = config_parser.kwargs_from_config(config_data)
    if config_data:
      raise argparse.ArgumentTypeError(
          f"Config for Probe={cls.NAME} contains unused properties: "
          f"{', '.join(config_data.keys())}")
    return cls(**kwargs)

  @classmethod
  def help_text(cls) -> str:
    return str(cls.config_parser())

  # Set to False if the Probe cannot be used with arbitrary Stories or Pages
  IS_GENERAL_PURPOSE: bool = True
  PRODUCES_DATA: bool = True
  # Set the default probe result location, used to figure out whether result
  # files need to be transferred from a remote machine.
  RESULT_LOCATION: ResultLocation = ResultLocation.LOCAL
  # Set to True if the probe only works on battery power with single runs
  BATTERY_ONLY: bool = False

  _browsers: Set[Browser]

  def __init__(self) -> None:
    assert self.name is not None, "A Probe must define a name"
    self._browsers = set()

  def __str__(self) -> str:
    return type(self).__name__

  @property
  def runner_platform(self) -> plt.Platform:
    return plt.PLATFORM

  @property
  def name(self) -> str:
    return self.NAME

  @property
  def result_path_name(self) -> str:
    return self.name

  def is_compatible(self, browser: Browser) -> bool:
    """
    Returns a boolean to indicate whether this Probe can be used with the given
    Browser. Override to make browser-specific Probes.
    """
    del browser
    return True

  @property
  def is_attached(self) -> bool:
    return len(self._browsers) > 0

  def attach(self, browser: Browser) -> None:
    assert self.is_compatible(browser), (
        f"Probe {self.name} is not compatible with browser {browser.type}")
    assert browser not in self._browsers, (
        f"Probe={self.name} is attached multiple times to the same browser")
    self._browsers.add(browser)

  def pre_check(self, env: HostEnvironment) -> None:
    """
    Part of the Checklist, make sure everything is set up correctly for a probe
    to run.
    """
    del env
    # Ensure that the proper super methods for setting up a probe were
    # called.
    assert self.is_attached, (
        f"Probe {self.name} is not properly attached to a browser")
    for browser in self._browsers:
      assert self.is_compatible(browser)

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    """
    Can be used to merge probe data from multiple repetitions of the same story.
    Return None, a result file Path (or a list of Paths)
    """
    del group
    return EmptyProbeResult()

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    """
    Can be used to merge probe data from multiple stories for the same browser.
    Return None, a result file Path (or a list of Paths)
    """
    del group
    return EmptyProbeResult()

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    """
    Can be used to merge all probe data (from multiple stories and browsers.)
    Return None, a result file Path (or a list of Paths)
    """
    del group
    return EmptyProbeResult()

  @abc.abstractmethod
  def get_scope(self: ProbeT, run: Run) -> ProbeScope[ProbeT]:
    pass

  def log_run_result(self, run: Run) -> None:
    """
    Override to print a short summary of the collected results after a run
    completes.
    """
    del run

  def log_browsers_result(self, group: BrowsersRunGroup) -> None:
    """
    Override to print a short summary of all the collected results.
    """
    del group


class ProbeScope(abc.ABC, Generic[ProbeT]):
  """
  A scope during which a probe is actively collecting data.
  Override in Probe subclasses to implement actual performance data
  collection.
  - The data should be written to self.result_path.
  - A file / list / dict of result file Paths should be returned by the
    override tear_down() method
  """

  def __init__(self, probe: ProbeT, run: Run) -> None:
    self._probe: ProbeT = probe
    self._run: Run = run
    self._default_result_path: pathlib.Path = self.get_default_result_path()
    self._is_active: bool = False
    self._is_success: bool = False
    self._start_time: Optional[dt.datetime] = None
    self._stop_time: Optional[dt.datetime] = None

  def get_default_result_path(self) -> pathlib.Path:
    return self._run.get_default_probe_result_path(self._probe)

  def set_start_time(self, start_datetime: dt.datetime) -> None:
    assert self._start_time is None
    self._start_time = start_datetime

  def __enter__(self) -> ProbeScope[ProbeT]:
    assert not self._is_active
    assert not self._is_success
    with self._run.exception_handler(f"Probe {self.name} start"):
      self._is_active = True
      self.start(self._run)
    return self

  def __exit__(self, exc_type, exc_value, traceback) -> None:
    assert self._is_active
    with self._run.exception_handler(f"Probe {self.name} stop"):
      self.stop(self._run)
      self._is_success = True
      assert self._stop_time is None
    self._stop_time = dt.datetime.now()

  @property
  def probe(self) -> ProbeT:
    return self._probe

  @property
  def run(self) -> Run:
    return self._run

  @property
  def browser(self) -> Browser:
    return self._run.browser

  @property
  def runner(self) -> Runner:
    return self._run.runner

  @property
  def browser_platform(self) -> plt.Platform:
    return self.browser.platform

  @property
  def runner_platform(self) -> plt.Platform:
    return self.runner.platform

  def browser_result(
      self,
      url: Optional[Iterable[str]] = None,
      file: Optional[Iterable[pathlib.Path]] = None,
      json: Optional[Iterable[pathlib.Path]] = None,
      csv: Optional[Iterable[pathlib.Path]] = None) -> BrowserProbeResult:
    """Helper to create BrowserProbeResult that might be stored on a remote
    browser/device and need to be copied over to the local machine."""
    return BrowserProbeResult(self.run, url, file, json, csv)

  @property
  def start_time(self) -> dt.datetime:
    """
    Returns a unified start time that is the same for all ProbeScopes
    within a run. This can be used to account for startup delays caused by other
    Probes.
    """
    assert self._start_time
    return self._start_time

  @property
  def duration(self) -> dt.timedelta:
    assert self._start_time and self._stop_time
    return self._stop_time - self._start_time

  @property
  def is_success(self) -> bool:
    return self._is_success

  @property
  def result_path(self) -> pathlib.Path:
    return self._default_result_path

  @property
  def name(self) -> str:
    return self.probe.name

  @property
  def browser_pid(self) -> int:
    maybe_pid = self.run.browser.pid
    assert maybe_pid, "Browser is not runner or does not provide a pid."
    return maybe_pid

  def setup(self, run: Run) -> None:
    """
    Called before starting the browser, typically used to set run-specific
    browser flags.
    """
    del run

  def setup_selenium_options(self, options: BaseOptions) -> None:
    """
    Custom hook to change selenium options before starting the browser.
    """
    del options

  @abc.abstractmethod
  def start(self, run: Run) -> None:
    """
    Called immediately before starting the given Run, after the browser started.
    This method should have as little overhead as possible. If possible,
    delegate heavy computation to the "SetUp" method.
    """

  def start_story_run(self, run: Run) -> None:
    """
    Called before running a Story's core workload (Story.run)
    and after running Story.setup.
    """

  def stop_story_run(self, run: Run) -> None:
    """
    Called after running a Story's core workload (Story.run) and before running
    Story.tear_down.
    """

  @abc.abstractmethod
  def stop(self, run: Run) -> None:
    """
    Called immediately after finishing the given Run with the browser still
    running.
    This method should have as little overhead as possible. If possible,
    delegate heavy computation to the "tear_down" method.
    """
    return None

  @abc.abstractmethod
  def tear_down(self, run: Run) -> ProbeResult:
    """
    Called after stopping all probes and shutting down the browser.
    Returns
    - None if no data was collected
    - If Data was collected:
      - Either a path (or list of paths) to results file
      - Directly a primitive json-serializable object containing the data
    """
    return EmptyProbeResult()
