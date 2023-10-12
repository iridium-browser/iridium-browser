# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import contextlib
import datetime as dt
import enum
import logging
import pathlib
from typing import (TYPE_CHECKING, Any, Iterable, Iterator, List, Optional,
                    Tuple)

from crossbench import exception, helper
from crossbench.flags import Flags, JSFlags
from crossbench.probes import internal as internal_probe
from crossbench.probes.probe import ResultLocation
from crossbench.probes.results import (EmptyProbeResult, ProbeResult,
                                       ProbeResultDict)

from .actions import Actions
from .timing import Timing

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench import plt
  from crossbench.probes.probe import Probe, ProbeScope
  from crossbench.stories.story import Story
  from crossbench.types import JsonDict

  from .groups import BrowserSessionRunGroup
  from .runner import Runner


@enum.unique
class RunState(enum.Enum):
  INITIAL = enum.auto()
  SETUP = enum.auto()
  RUN = enum.auto()
  DONE = enum.auto()


class Run:

  def __init__(self,
               runner: Runner,
               browser_session: BrowserSessionRunGroup,
               story: Story,
               repetition: int,
               index: int,
               root_dir: pathlib.Path,
               name: Optional[str] = None,
               temperature: Optional[int] = None,
               timeout: dt.timedelta = dt.timedelta(),
               throw: bool = False):
    self._state = RunState.INITIAL
    self._browser_session = browser_session
    browser_session.append(self)
    self._runner = runner
    self._browser = browser_session.browser
    self._story = story
    assert repetition >= 0
    self._repetition = repetition
    assert index >= 0
    self._index = index
    self._name = name
    self._out_dir = self.get_out_dir(root_dir).absolute()
    self._probe_results = ProbeResultDict(self._out_dir)
    self._probe_scopes: List[ProbeScope] = []
    self._extra_js_flags = JSFlags()
    self._extra_flags = Flags()
    self._durations = helper.Durations()
    self._start_datetime = dt.datetime.utcfromtimestamp(0)
    self._temperature = temperature
    self._timeout = timeout
    self._exceptions = exception.Annotator(throw)
    self._browser_tmp_dir: Optional[pathlib.Path] = None

  def __str__(self) -> str:
    return f"Run({self.name}, {self._state}, {self.browser})"

  def get_out_dir(self, root_dir: pathlib.Path) -> pathlib.Path:
    return root_dir / self.browser.unique_name / self.story.name / str(
        self._repetition)

  @property
  def group_dir(self) -> pathlib.Path:
    return self.out_dir.parent

  def actions(self,
              name: str,
              verbose: bool = False,
              measure: bool = True) -> Actions:
    return Actions(name, self, verbose=verbose, measure=measure)

  @property
  def info_stack(self) -> exception.TInfoStack:
    return (
        f"Run({self.name})",
        (f"browser={self.browser.type} label={self.browser.label} "
         "binary={self.browser.path}"),
        f"story={self.story}",
        f"repetition={self.repetition}",
    )

  def details_json(self) -> JsonDict:
    return {
        "name": self.name,
        "repetition": self.repetition,
        "temperature": self.temperature,
        "story": str(self.story),
        "probes": [probe.name for probe in self.probes],
        "duration": self.story.duration.total_seconds(),
        "startDateTime": str(self.start_datetime),
        "timeout": self.timeout.total_seconds(),
    }

  @property
  def temperature(self) -> Optional[int]:
    return self._temperature

  @property
  def timing(self) -> Timing:
    return self.runner.timing

  @property
  def durations(self) -> helper.Durations:
    return self._durations

  @property
  def start_datetime(self) -> dt.datetime:
    return self._start_datetime

  def max_end_datetime(self) -> dt.datetime:
    if not self._timeout:
      return dt.datetime.max
    return self._start_datetime + self._timeout

  @property
  def timeout(self) -> dt.timedelta:
    return self._timeout

  @property
  def repetition(self) -> int:
    return self._repetition

  @property
  def index(self) -> int:
    return self._index

  @property
  def runner(self) -> Runner:
    return self._runner

  @property
  def browser_session(self) -> BrowserSessionRunGroup:
    return self._browser_session

  @property
  def browser(self) -> Browser:
    return self._browser

  @property
  def platform(self) -> plt.Platform:
    return self.browser_platform

  @property
  def environment(self) -> HostEnvironment:
    # TODO: replace with custom BrowserEnvironment
    return self.runner.env

  @property
  def browser_platform(self) -> plt.Platform:
    return self._browser.platform

  @property
  def runner_platform(self) -> plt.Platform:
    return self.runner.platform

  @property
  def is_remote(self) -> bool:
    return self.browser_platform.is_remote

  @property
  def out_dir(self) -> pathlib.Path:
    """A local directory where all result files are gathered.
    Results from browsers on remote platforms are transferred to this dir
    as well."""
    return self._out_dir

  @property
  def browser_tmp_dir(self) -> pathlib.Path:
    """Returns a path to a tmp dir on the browser platform."""
    if not self._browser_tmp_dir:
      prefix = "cb_run_results"
      self._browser_tmp_dir = self.browser_platform.mkdtemp(prefix)
    return self._browser_tmp_dir

  @property
  def results(self) -> ProbeResultDict:
    return self._probe_results

  @property
  def story(self) -> Story:
    return self._story

  @property
  def name(self) -> Optional[str]:
    return self._name

  @property
  def extra_js_flags(self) -> JSFlags:
    return self._extra_js_flags

  @property
  def extra_flags(self) -> Flags:
    return self._extra_flags

  @property
  def probes(self) -> Iterable[Probe]:
    return self._runner.probes

  @property
  def exceptions(self) -> exception.Annotator:
    return self._exceptions

  @property
  def is_success(self) -> bool:
    return self._exceptions.is_success

  @property
  def probe_scopes(self) -> Iterator[ProbeScope]:
    return iter(self._probe_scopes)

  @contextlib.contextmanager
  def measure(
      self, label: str
  ) -> Iterator[Tuple[exception.ExceptionAnnotationScope,
                      helper.DurationMeasureContext]]:
    # Return a combined context manager that adds an named exception info
    # and measures the time during the with-scope.
    with self._exceptions.info(label) as stack, self._durations.measure(
        label) as timer:
      yield (stack, timer)

  def exception_info(self,
                     *stack_entries: str) -> exception.ExceptionAnnotationScope:
    return self._exceptions.info(*stack_entries)

  def exception_handler(
      self,
      *stack_entries: str,
      exceptions: exception.TExceptionTypes = (Exception,)
  ) -> exception.ExceptionAnnotationScope:
    return self._exceptions.capture(*stack_entries, exceptions=exceptions)

  def get_browser_details_json(self) -> JsonDict:
    details_json = self.browser.details_json()
    assert isinstance(details_json["js_flags"], (list, tuple))
    details_json["js_flags"] += tuple(self.extra_js_flags.get_list())
    assert isinstance(details_json["flags"], (list, tuple))
    details_json["flags"] += tuple(self.extra_flags.get_list())
    return details_json

  def get_default_probe_result_path(self, probe: Probe) -> pathlib.Path:
    """Return a local or remote/browser-based result path depending on the
    Probe default RESULT_LOCATION."""
    if probe.RESULT_LOCATION == ResultLocation.BROWSER:
      return self.get_browser_probe_result_path(probe)
    if probe.RESULT_LOCATION == ResultLocation.LOCAL:
      return self.get_local_probe_result_path(probe)
    raise ValueError(f"Invalid probe.RESULT_LOCATION {probe.RESULT_LOCATION} "
                     f"for probe {probe}")

  def get_local_probe_result_path(self, probe: Probe) -> pathlib.Path:
    file = self._out_dir / probe.result_path_name
    assert not file.exists(), f"Probe results file exists already. file={file}"
    return file

  def get_browser_probe_result_path(self, probe: Probe) -> pathlib.Path:
    """Returns a temporary path on the remote browser or the same as
    get_local_probe_result_path() on a local browser."""
    if not self.is_remote:
      return self.get_local_probe_result_path(probe)
    path = self.browser_tmp_dir / probe.result_path_name
    self.browser_platform.mkdir(path.parent)
    logging.debug("Creating remote result dir=%s on platform=%s", path.parent,
                  self.browser_platform)
    return path

  def run(self, is_dry_run: bool = False) -> None:
    self._advance_state(RunState.INITIAL, RunState.SETUP)
    self._start_datetime = dt.datetime.now()
    self._out_dir.mkdir(parents=True, exist_ok=True)
    with helper.ChangeCWD(self._out_dir), self.exception_info(*self.info_stack):
      assert not self._probe_scopes
      try:
        self._probe_scopes = self._setup_probes(is_dry_run)
        self._setup_browser(is_dry_run)
      except Exception as e:  # pylint: disable=broad-except
        self._handle_setup_error(e)
        return
      try:
        self._run(is_dry_run)
      except Exception as e:  # pylint: disable=broad-except
        self._exceptions.append(e)
      finally:
        if not is_dry_run:
          self.tear_down()

  def _setup_probes(self, is_dry_run: bool) -> List[ProbeScope[Any]]:
    assert self._state == RunState.SETUP
    logging.debug("SETUP")
    logging.info("PROBES: %s", ", ".join(probe.NAME for probe in self.probes))
    logging.info("STORY: %s", self.story)
    logging.info("STORY DURATION: expected=%s timeout=%s",
                 self.timing.timedelta(self.story.duration),
                 self.timing.timeout_timedelta(self.story.duration))
    logging.info("RUN DIR: %s", self._out_dir)
    logging.debug("CWD %s", self._out_dir)

    if is_dry_run:
      return []

    with self.measure("runner-cooldown"):
      self._runner.wait(self._runner.timing.cool_down_time, absolute_time=True)
      self._runner.cool_down()

    probe_run_scopes: List[ProbeScope] = []
    with self.measure("probes-creation"):
      probe_set = set()
      for probe in self.probes:
        assert probe not in probe_set, (
            f"Got duplicate probe name={probe.name}")
        probe_set.add(probe)
        if probe.PRODUCES_DATA:
          self._probe_results[probe] = EmptyProbeResult()
        assert probe.is_attached, (
            f"Probe {probe.name} is not properly attached to a browser")
        probe_run_scopes.append(probe.get_scope(self))

    with self.measure("probes-setup"):
      for probe_scope in probe_run_scopes:
        with self.measure(f"probes-setup {probe_scope.name}"):
          probe_scope.setup(self)  # pytype: disable=wrong-arg-types
    return probe_run_scopes

  def _setup_browser(self, is_dry_run: bool) -> None:
    assert self._state == RunState.SETUP
    if is_dry_run:
      logging.info("BROWSER: %s", self.browser.path)
      return

    browser_log_file = self._out_dir / "browser.log"
    assert not browser_log_file.exists(), (
        f"Default browser log file {browser_log_file} already exists.")
    self._browser.set_log_file(browser_log_file)

    with self.measure("browser-setup"):
      try:
        # pytype somehow gets the package path wrong here, disabling for now.
        self._browser.setup(self)  # pytype: disable=wrong-arg-types
      except Exception as e:
        logging.debug("Browser setup failed: %s", e)
        # Clean up half-setup browser instances
        self._browser.force_quit()
        raise

  def _handle_setup_error(self, setup_exception: BaseException) -> None:
    self._advance_state(RunState.SETUP, RunState.DONE)
    self._exceptions.append(setup_exception)
    assert self._state == RunState.DONE
    assert not self._exceptions.is_success
    # Special handling for crucial runner probes
    internal_probe_scopes = [
        scope for scope in self._probe_scopes
        if isinstance(scope.probe, internal_probe.InternalProbe)
    ]
    self._tear_down_probe_scopes(internal_probe_scopes)

  def _run(self, is_dry_run: bool) -> None:
    self._advance_state(RunState.SETUP, RunState.RUN)
    assert self._probe_scopes
    probe_start_time = dt.datetime.now()
    probe_scope_manager = contextlib.ExitStack()

    for probe_scope in self._probe_scopes:
      probe_scope.set_start_time(probe_start_time)
      probe_scope_manager.enter_context(probe_scope)

    with probe_scope_manager:
      self._durations["probes-start"] = dt.datetime.now() - probe_start_time
      logging.info("RUNNING STORY")
      assert self._state == RunState.RUN, "Invalid state"
      try:
        with self.measure("run"), helper.Spinner():
          if not is_dry_run:
            self._run_story_setup()
            self._story.run(self)
            self._run_story_tear_down()
      except TimeoutError as e:
        # Handle TimeoutError earlier since they might be caused by
        # throttled down non-foreground browser.
        self._exceptions.append(e)
      self.environment.check_browser_focused(self.browser)

  def _run_story_setup(self) -> None:
    with self.measure("story-setup"):
      self._story.setup(self)
    with self.measure("probes-start_story_run"):
      for probe_scope in self._probe_scopes:
        with self.exception_handler(
            f"Probe {probe_scope.name} start_story_run"):
          probe_scope.start_story_run(self)

  def _run_story_tear_down(self) -> None:
    with self.measure("probes-stop_story_run"):
      for probe_scope in self._probe_scopes:
        with self.exception_handler(f"Probe {probe_scope.name} stop_story_run"):
          probe_scope.stop_story_run(self)
    with self.measure("story-tear-down"):
      self._story.tear_down(self)

  def _advance_state(self, expected: RunState, next_state: RunState) -> None:
    assert self._state == expected, (
        f"Invalid state got={self._state} expected={expected}")
    self._state = next_state

  def tear_down(self, is_shutdown: bool = False) -> None:
    self._advance_state(RunState.RUN, RunState.DONE)
    with self.measure("browser-tear_down"):
      if self._browser.is_running is False:
        logging.warning("Browser is no longer running (crashed or closed).")
      else:
        if is_shutdown:
          try:
            self._browser.quit(self._runner)  # pytype: disable=wrong-arg-types
          except Exception as e:  # pylint: disable=broad-except
            logging.warning("Error quitting browser: %s", e)
            return
        with self._exceptions.capture("Quit browser"):
          self._browser.quit(self._runner)  # pytype: disable=wrong-arg-types
    with self.measure("probes-tear_down"):
      self._tear_down_probe_scopes(self._probe_scopes)
      self._probe_scopes = []
    self._rm_browser_tmp_dir()

  def _tear_down_probe_scopes(self, probe_scopes: List[ProbeScope]) -> None:
    assert self._state == RunState.DONE
    assert probe_scopes, "Expected non-empty probe_scopes list."
    logging.debug("PROBE SCOPE TEARDOWN")
    for probe_scope in reversed(probe_scopes):
      with self.exceptions.capture(f"Probe {probe_scope.name} teardown"):
        assert probe_scope.run == self
        probe_results: ProbeResult = probe_scope.tear_down(self)  # pytype: disable=wrong-arg-types
        probe = probe_scope.probe
        if probe_results.is_empty:
          logging.warning("Probe did not extract any data. probe=%s run=%s",
                          probe, self)
        self._probe_results[probe] = probe_results

  def _rm_browser_tmp_dir(self) -> None:
    if not self._browser_tmp_dir:
      return
    self.browser_platform.rm(self._browser_tmp_dir, dir=True)

  def log_results(self) -> None:
    for probe in self.probes:
      probe.log_run_result(self)
