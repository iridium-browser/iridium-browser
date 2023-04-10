# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import datetime as dt
import pathlib
from typing import (TYPE_CHECKING, Any, Dict, Generic, Optional, Set, Type,
                    TypeVar)

import crossbench
from crossbench import helper
from crossbench.config import ConfigParser
from crossbench.probes.results import ProbeResult

#TODO: fix imports
cb = crossbench

if TYPE_CHECKING:
  from crossbench.browsers.base import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner import (BrowsersRunGroup, RepetitionsRunGroup, Run,
                                 Runner, StoriesRunGroup)

ProbeT = TypeVar("ProbeT", bound="Probe")


class ProbeConfigParser(ConfigParser):

  def __init__(self, probe_cls: Type[Probe]):
    super().__init__("Probe", probe_cls)
    self._probe_cls = probe_cls


class Probe(abc.ABC):
  """
  Abstract Probe class.

  Probes are responsible for extracting performance numbers from websites
  / stories.

  Probe interface:
  - scope(): Return a custom Probe.Scope (see below)
  - is_compatible(): Use for checking whether a Probe can be used with a
    given browser
  - pre_check(): Customize to display warnings before using Probes with
    incompatible settings.
  The Probe object can the customize how to merge probe (performance) date at
  multiple levels:
  - multiple repetitions of the same story
  - merged repetitions from multiple stories (same browser)
  - Probe data from all Runs

  Probes use a Probe.Scope that is active during a story-Run.
  The Probe.Scope class defines a customizable interface
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
  def from_config(cls, config_data: Dict, throw: bool = False) -> Probe:
    config_parser = cls.config_parser()
    kwargs: Dict[str, Any] = config_parser.kwargs_from_config(
        config_data, throw=throw)
    if config_data:
      raise ValueError(
          f"Config for Probe={cls.NAME} contains unused properties: "
          f"{', '.join(config_data.keys())}")
    return cls(**kwargs)

  @classmethod
  def help_text(cls) -> str:
    return str(cls.config_parser())


  # Set to False if the Probe cannot be used with arbitrary Stories or Pages
  IS_GENERAL_PURPOSE: bool = True
  PRODUCES_DATA: bool = True
  # Set to True if the probe only works on battery power
  BATTERY_ONLY: bool = False

  _browsers: Set[Browser]
  _browser_platform: helper.Platform

  def __init__(self):
    assert self.name is not None, "A Probe must define a name"
    self._browsers = set()

  @property
  def browser_platform(self) -> helper.Platform:
    return self._browser_platform

  @property
  def runner_platform(self) -> helper.Platform:
    # TODO(cbruni): support remote platforms
    return helper.platform

  @property
  def name(self) -> str:
    return self.NAME

  @property
  def results_file_name(self) -> str:
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
    if not self._browsers:
      self._browser_platform = browser.platform
    else:
      assert self._browser_platform == browser.platform, (
          "All browsers must run on the same platform"
          f"existing={self._browser_platform }, new={browser.platform}")
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
    return ProbeResult()

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    """
    Can be used to merge probe data from multiple stories for the same browser.
    Return None, a result file Path (or a list of Paths)
    """
    del group
    return ProbeResult()

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    """
    Can be used to merge all probe data (from multiple stories and browsers.)
    Return None, a result file Path (or a list of Paths)
    """
    del group
    return ProbeResult()

  def get_scope(self: ProbeT, run: Run) -> Probe.Scope[ProbeT]:
    assert self.is_attached, (
        f"Probe {self.name} is not properly attached to a browser")
    return self.Scope(self, run)  # pylint: disable=abstract-class-instantiated

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

  class Scope(Generic[ProbeT], metaclass=abc.ABCMeta):
    """
    A scope during which a probe is actively collecting data.
    Override in Probe subclasses to implement actual performance data
    collection.
    - The data should be written to self.results_file.
    - A file / list / dict of result file Paths should be returned by the
      override tear_down() method
    """

    def __init__(self, probe: ProbeT, run: Run):
      self._probe = probe
      self._run = run
      self._default_results_file = run.get_probe_results_file(probe)
      self._is_active = False
      self._is_success = False
      self._start_time: Optional[dt.datetime] = None
      self._stop_time: Optional[dt.datetime] = None

    def set_start_time(self, start_datetime: dt.datetime) -> None:
      assert self._start_time is None
      self._start_time = start_datetime

    def __enter__(self):
      assert not self._is_active
      assert not self._is_success
      with self._run.exception_handler(f"Probe {self.name} start"):
        self._is_active = True
        self.start(self._run)
      return self

    def __exit__(self, exc_type, exc_value, traceback):
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
    def browser_platform(self) -> helper.Platform:
      return self.browser.platform

    @property
    def runner_platform(self) -> helper.Platform:
      return self.runner.platform

    @property
    def start_time(self) -> dt.datetime:
      """
      Returns a unified start time that is the same for all Probe.Scopes
      within a run. This can be to account for startup delays caused by other
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
    def results_file(self) -> pathlib.Path:
      return self._default_results_file

    @property
    def name(self) -> str:
      return self.probe.name

    @property
    def browser_pid(self) -> str:
      maybe_pid = self.run.browser.pid
      assert maybe_pid, "Browser is not runner or does not provide a pid."
      return maybe_pid

    def setup(self, run: Run) -> None:
      """
      Called before starting the browser, typically used to set run-specific
      browser flags.
      """
      del run

    @abc.abstractmethod
    def start(self, run: Run) -> None:
      """
      Called immediately before starting the given Run.
      This method should have as little overhead as possible. If possible,
      delegate heavy computation to the "SetUp" method.
      """

    @abc.abstractmethod
    def stop(self, run: Run) -> None:
      """
      Called immediately after finishing the given Run.
      This method should have as little overhead as possible. If possible,
      delegate heavy computation to the "collect" method.
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
      return ProbeResult()
