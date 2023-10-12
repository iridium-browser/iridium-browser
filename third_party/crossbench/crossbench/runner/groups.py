# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import contextlib
import logging
import pathlib
import threading
from typing import TYPE_CHECKING, Dict, Iterable, Iterator, List, Optional

from crossbench import compat, exception, helper
from crossbench.probes.results import ProbeResult, ProbeResultDict

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.probes.probe import Probe
  from crossbench.stories.story import Story
  from crossbench.types import JsonDict

  from .run import Run
  from .runner import Runner


class RunGroup(abc.ABC):

  def __init__(self, throw: bool = False) -> None:
    self._exceptions = exception.Annotator(throw)
    self._path: Optional[pathlib.Path] = None
    self._merged_probe_results: Optional[ProbeResultDict] = None

  def _set_path(self, path: pathlib.Path) -> None:
    assert self._path is None
    self._path = path
    self._merged_probe_results = ProbeResultDict(path)

  @property
  def results(self) -> ProbeResultDict:
    assert self._merged_probe_results
    return self._merged_probe_results

  @property
  def path(self) -> pathlib.Path:
    assert self._path
    return self._path

  @property
  def exceptions(self) -> exception.Annotator:
    return self._exceptions

  @property
  def is_success(self) -> bool:
    return self._exceptions.is_success

  @property
  @abc.abstractmethod
  def info_stack(self) -> exception.TInfoStack:
    pass

  @property
  @abc.abstractmethod
  def info(self) -> JsonDict:
    pass

  @property
  @abc.abstractmethod
  def runs(self) -> Iterable[Run]:
    pass

  @property
  def failed_runs(self) -> Iterable[Run]:
    for run in self.runs:
      if not run.is_success:
        yield run

  def get_local_probe_result_path(self,
                                  probe: Probe,
                                  exists_ok: bool = False) -> pathlib.Path:
    new_file = self.path / probe.result_path_name
    if not exists_ok:
      assert not new_file.exists(), (
          f"Merged file {new_file} for {self.__class__} exists already.")
    return new_file

  def merge(self, runner: Runner) -> None:
    assert self._merged_probe_results
    with self._exceptions.info(*self.info_stack):
      for probe in reversed(tuple(runner.probes)):
        with self._exceptions.capture(f"Probe {probe.name} merge results"):
          results = self._merge_probe_results(probe)
          if results is None:
            continue
          self._merged_probe_results[probe] = results

  @abc.abstractmethod
  def _merge_probe_results(self, probe: Probe) -> ProbeResult:
    pass


class RepetitionsRunGroup(RunGroup):
  """
  A group of Run objects that are different repetitions for the same Story with
  and the same browser.
  """

  @classmethod
  def groups(cls,
             runs: Iterable[Run],
             throw: bool = False) -> List[RepetitionsRunGroup]:
    return list(
        helper.group_by(
            runs,
            key=lambda run: (run.story, run.browser),
            group=lambda _: cls(throw),
            sort_key=None).values())

  def __init__(self, throw: bool = False):
    super().__init__(throw)
    self._runs: List[Run] = []
    self._story: Optional[Story] = None
    self._browser: Optional[Browser] = None

  def append(self, run: Run) -> None:
    if self._path is None:
      self._set_path(run.group_dir)
      self._story = run.story
      self._browser = run.browser
    assert self._story == run.story
    assert self._path == run.group_dir
    assert self._browser == run.browser
    self._runs.append(run)

  @property
  def runs(self) -> Iterable[Run]:
    return iter(self._runs)

  @property
  def story(self) -> Story:
    assert self._story
    return self._story

  @property
  def browser(self) -> Browser:
    assert self._browser
    return self._browser

  @property
  def info_stack(self) -> exception.TInfoStack:
    return ("Merging results from multiple repetitions",
            f"browser={self.browser.unique_name}", f"story={self.story}")

  @property
  def info(self) -> JsonDict:
    return {
        "story": str(self.story),
        "runs": len(tuple(self.runs)),
        "failed runs": len(tuple(self.failed_runs))
    }

  def _merge_probe_results(self, probe: Probe) -> ProbeResult:
    return probe.merge_repetitions(self)  # pytype: disable=wrong-arg-types


class StoriesRunGroup(RunGroup):
  """
  A group of StoryRepetitionsRunGroups for the same browser.
  """

  def __init__(self, throw: bool = False) -> None:
    super().__init__(throw)
    self._repetitions_groups: List[RepetitionsRunGroup] = []
    self._browser: Browser = None

  @classmethod
  def groups(cls,
             run_groups: Iterable[RepetitionsRunGroup],
             throw: bool = False) -> List[StoriesRunGroup]:
    return list(
        helper.group_by(
            run_groups,
            key=lambda run_group: run_group.browser,
            group=lambda _: cls(throw),
            sort_key=None).values())

  def append(self, group: RepetitionsRunGroup) -> None:
    if self._path is None:
      self._set_path(group.path.parent)
      self._browser = group.browser
    assert self._path == group.path.parent
    assert self._browser == group.browser
    self._repetitions_groups.append(group)

  @property
  def repetitions_groups(self) -> List[RepetitionsRunGroup]:
    return self._repetitions_groups

  @property
  def runs(self) -> Iterable[Run]:
    for group in self._repetitions_groups:
      yield from group.runs

  @property
  def browser(self) -> Browser:
    return self._browser

  @property
  def info_stack(self) -> exception.TInfoStack:
    return (
        "Merging results from multiple stories",
        f"browser={self.browser.unique_name}",
    )

  @property
  def info(self) -> JsonDict:
    return {
        "label": self.browser.label,
        "browser": self.browser.app_name.title(),
        "version": self.browser.version,
        "os": self.browser.platform.full_version,
        "device": self.browser.platform.device,
        "cpu": self.browser.platform.cpu,
        "binary": str(self.browser.path),
        "flags": str(self.browser.flags),
        "runs": len(tuple(self.runs)),
        "failed runs": len(tuple(self.failed_runs))
    }

  @property
  def stories(self) -> Iterable[Story]:
    return (group.story for group in self._repetitions_groups)

  def _merge_probe_results(self, probe: Probe) -> ProbeResult:
    # TODO: enable pytype again
    return probe.merge_stories(self)  # pytype: disable=wrong-arg-types


class BrowsersRunGroup(RunGroup):
  _story_groups: Iterable[StoriesRunGroup]

  def __init__(self, story_groups, throw: bool) -> None:
    super().__init__(throw)
    self._story_groups = story_groups
    self._set_path(story_groups[0].path.parent)

  @property
  def story_groups(self) -> Iterable[StoriesRunGroup]:
    return self._story_groups

  @property
  def browsers(self) -> Iterable[Browser]:
    for story_group in self._story_groups:
      yield story_group.browser

  @property
  def repetitions_groups(self) -> Iterable[RepetitionsRunGroup]:
    for story_group in self._story_groups:
      yield from story_group.repetitions_groups

  @property
  def runs(self) -> Iterable[Run]:
    for group in self._story_groups:
      yield from group.runs

  @property
  def info_stack(self) -> exception.TInfoStack:
    return ("Merging results from multiple browsers",)

  @property
  def info(self) -> JsonDict:
    return {
        "runs": len(tuple(self.runs)),
        "failed runs": len(tuple(self.failed_runs))
    }

  def _merge_probe_results(self, probe: Probe) -> ProbeResult:
    return probe.merge_browsers(self)  # pytype: disable=wrong-arg-types


class RunThreadGroup(threading.Thread):

  def __init__(self, runs: List[Run]) -> None:
    super().__init__()
    assert len(runs), "Got unexpected empty runs list"
    self._runner: Runner = runs[0].runner
    self._runs = runs
    self.is_dry_run: bool = False
    self._verify_contains_all_browser_session_runs()

  def _verify_contains_all_browser_session_runs(self) -> None:
    runs_set = set(self._runs)
    for run in self._runs:
      for session_run in run.browser_session.runs:
        assert session_run in runs_set, (
            f"BrowserSession {run.browser_session} is not allowed to have "
            f"{session_run} in another RunThreadGroup.")

  def run(self) -> None:
    total_run_count = len(tuple(self._runner.runs))
    for run in self._runs:
      logging.info("=" * 80)
      logging.info("RUN %s/%s", run.index + 1, total_run_count)
      logging.info("=" * 80)
      run.run(self.is_dry_run)
      if run.is_success:
        run.log_results()
      else:
        self._runner.exceptions.extend(run.exceptions)


class BrowserSessionRunGroup:
  """
  Groups Run objects together that are run within the same browser session.
  At the beginning of a new session the caches are cleared and the
  browser is (re-)started.
  """

  class State(compat.StrEnum):
    READY = "ready"
    STARTING = "starting"
    RUNNING = "running"
    STOPPING = "stopping"
    DONE = "done"

  def __init__(self, browser: Browser) -> None:
    self._browser = browser
    self._runs: List[Run] = []
    self._state = self.State.READY

  def append(self, run: Run) -> None:
    assert self._state == self.State.READY
    assert run.browser_session == self
    self._runs.append(run)

  @property
  def browser(self) -> Browser:
    return self._browser

  @property
  def runs(self) -> Iterable[Run]:
    return iter(self._runs)

  @contextlib.contextmanager
  def open(self) -> Iterator[BrowserSessionRunGroup]:
    self._setup()
    try:
      yield self
    finally:
      self._teardown()

  def _setup(self) -> None:
    assert self._state == self.State.READY
    self._state = self.State.STARTING
    self._start_browser()
    self._state = self.State.RUNNING

  def _start_browser(self) -> None:
    assert self._state == self.State.STARTING
    # TODO: implement

  def _teardown(self) -> None:
    assert self._state == self.State.RUNNING
    self._state = self.State.STOPPING
    try:
      self._stop_browser()
    finally:
      assert self._state == self.State.STOPPING
      self._state = self.State.DONE

  def _stop_browser(self) -> None:
    assert self._state == self.State.STOPPING
    # TODO: implement
