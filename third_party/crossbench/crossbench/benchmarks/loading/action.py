# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import argparse
import datetime as dt
import logging
import json
import time
from typing import TYPE_CHECKING, Any, Dict, Tuple, Type

from crossbench import cli_helper, compat

if TYPE_CHECKING:
  from crossbench.runner.run import Run
  from crossbench.stories.story import Story
  from crossbench.types import JsonDict


class ParsingEnum(compat.StrEnum):

  @classmethod
  def parse(cls, value: Any) -> ParsingEnum:
    value_str: str = cli_helper.parse_non_empty_str(value, cls.__name__).upper()
    if enum_instance := getattr(cls, value_str, None):
      return enum_instance
    choices = ", ".join(e.name for e in cls)  # pytype: disable=missing-parameter
    raise argparse.ArgumentTypeError(f"Unknown {cls.__name__}: '{value}', "
                                     f"choices are {choices}")


class ScrollDirection(ParsingEnum):
  UP = "up"
  DOWN = "down"


class ButtonClick(ParsingEnum):
  LEFT = "left"
  RIGHT = "right"
  MIDDLE = "middle"


class ActionType(ParsingEnum):
  GET = "get"
  WAIT = "wait"
  SCROLL = "scroll"
  CLICK = "click"


ACTION_TIMEOUT = dt.timedelta(seconds=20)


class Action(abc.ABC):
  TYPE: ActionType = ActionType.GET

  @classmethod
  def kwargs_from_dict(cls, value: JsonDict) -> Dict[str, Any]:
    kwargs = {}
    if timeout := value.pop("timeout", None):
      kwargs["timeout"] = cli_helper.Duration.parse_non_zero(timeout)
    return kwargs

  @classmethod
  def pop_required_input(cls, value: JsonDict, key: str) -> Any:
    if key not in value:
      raise argparse.ArgumentTypeError(
          f"{cls.__name__}: Missing '{key}' property in {json.dumps(value)}")
    value = value.pop(key)
    if value is None:
      raise argparse.ArgumentTypeError(
          f"{cls.__name__}: {key} should not be None")
    return value

  def __init__(self, timeout: dt.timedelta = ACTION_TIMEOUT):
    self._timeout: dt.timedelta = timeout
    self.validate()

  @property
  def duration(self) -> dt.timedelta:
    return dt.timedelta(milliseconds=10)

  @property
  def timeout(self) -> dt.timedelta:
    return self._timeout

  @property
  def has_timeout(self) -> bool:
    return self._timeout != dt.timedelta.max

  @abc.abstractmethod
  def run(self, run: Run) -> None:
    pass

  def validate(self) -> None:
    if self._timeout.total_seconds() < 0:
      raise ValueError(
          f"{self}.timeout should be positive, but got {self.timeout}")

  def to_json(self) -> JsonDict:
    return {"type": self.TYPE, "timeout": self.timeout.total_seconds()}


class ReadyState(ParsingEnum):
  """See https://developer.mozilla.org/en-US/docs/Web/API/Document/readyState"""
  # Non-blocking:
  ANY = "any"
  # Blocking (on dom event):
  LOADING = "loading"
  INTERACTIVE = "interactive"
  COMPLETE = "complete"


class GetAction(Action):
  TYPE: ActionType = ActionType.GET

  @classmethod
  def kwargs_from_dict(cls, value: JsonDict) -> Dict[str, Any]:
    kwargs = super().kwargs_from_dict(value)
    kwargs["url"] = cli_helper.parse_url_str(
        cls.pop_required_input(value, "url"))
    if duration := value.pop("duration", None):
      kwargs["duration"] = cli_helper.Duration.parse_zero(duration)
    if ready_state := value.pop("ready-state", None):
      kwargs["ready_state"] = ReadyState.parse(ready_state)
    return kwargs

  def __init__(self,
               url: str,
               duration: dt.timedelta = dt.timedelta(),
               timeout: dt.timedelta = ACTION_TIMEOUT,
               ready_state: ReadyState = ReadyState.ANY):
    self._url: str = url
    self._duration = duration
    self._ready_state = ready_state
    super().__init__(timeout)

  @property
  def url(self) -> str:
    return self._url

  @property
  def ready_state(self) -> ReadyState:
    return self._ready_state

  @property
  def duration(self) -> dt.timedelta:
    return self._duration

  def run(self, run: Run) -> None:
    start_time = time.time()
    expected_end_time = start_time + self.duration.total_seconds()

    with run.actions("GetAction", measure=False) as action:
      action.show_url(self.url)
      if self._ready_state != ReadyState.ANY:
        action.wait_js_condition(
            f"return document.readyState === '{self._ready_state}'", 0.5, 15)
        return
      # Wait for the given duration from the start of the action.
      wait_time_seconds = expected_end_time - time.time()
      if wait_time_seconds > 0:
        action.wait(wait_time_seconds)
      elif self.duration:
        run_duration = dt.timedelta(seconds=time.time() - start_time)
        logging.info("%s took longer (%s) than expected action duration (%s).",
                     self, run_duration, self.duration)

  def validate(self) -> None:
    super().validate()
    if not self.url:
      raise ValueError(f"{self}.url is missing")
    if self._ready_state == ReadyState.ANY:
      return
    if self.duration != dt.timedelta():
      raise ValueError(
          f"Expected empty duration with ReadyState {self._ready_state} "
          f"but got: {self.duration}")

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["url"] = self.url
    details["duration"] = self.duration.total_seconds()
    details["ready_state"] = str(self.ready_state)
    return details


class DurationAction(Action):
  TYPE: ActionType = ActionType.WAIT

  @classmethod
  def kwargs_from_dict(cls, value: JsonDict) -> Dict[str, Any]:
    kwargs = super().kwargs_from_dict(value)
    kwargs["duration"] = cli_helper.Duration.parse_non_zero(
        cls.pop_required_input(value, "duration"))
    return kwargs

  def __init__(self,
               duration: dt.timedelta,
               timeout: dt.timedelta = ACTION_TIMEOUT) -> None:
    self._duration: dt.timedelta = duration
    super().__init__(timeout)

  @property
  def duration(self) -> dt.timedelta:
    return self._duration

  def validate(self) -> None:
    super().validate()
    if self.duration.total_seconds() <= 0:
      raise ValueError(
          f"{self}.duration should be positive, but got {self.duration}")

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["duration"] = self.duration.total_seconds()
    return details


class WaitAction(DurationAction):
  TYPE: ActionType = ActionType.WAIT

  def run(self, run: Run) -> None:
    run.runner.wait(self.duration)


class ScrollAction(DurationAction):
  TYPE: ActionType = ActionType.SCROLL

  @classmethod
  def kwargs_from_dict(cls, value: JsonDict) -> Dict[str, Any]:
    kwargs = super().kwargs_from_dict(value)
    if direction := value.pop("direction", None):
      kwargs["direction"] = ScrollDirection.parse(direction)
    return kwargs

  def __init__(self,
               direction: ScrollDirection = ScrollDirection.DOWN,
               duration: dt.timedelta = dt.timedelta(seconds=1),
               timeout: dt.timedelta = ACTION_TIMEOUT) -> None:
    self._direction: ScrollDirection = direction
    super().__init__(duration, timeout)

  @property
  def direction(self) -> ScrollDirection:
    return self._direction

  def run(self, run: Run) -> None:
    time_end = time.time() + self.duration.total_seconds()
    direction = 1 if self.direction == ScrollDirection.UP else -1

    start = 0
    end = direction

    while time.time() < time_end:
      # TODO: REMOVE COMMENT CODE ONCE pyautogui ALLOWED ON GOOGLE3
      # if events_source == 'js'
      run.browser.js(run.runner, f"window.scrollTo({start}, {end});")
      start = end
      end += 100
      # else :
      #   pyautogui.scroll(direction)

  def validate(self) -> None:
    super().validate()
    if not self.direction:
      raise ValueError(f"{self}.direction is not provided")

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["direction"] = str(self.direction)
    return details


class ClickAction(Action):
  TYPE: ActionType = ActionType.CLICK

  @classmethod
  def kwargs_from_dict(cls, value: JsonDict) -> Dict[str, Any]:
    kwargs = super().kwargs_from_dict(value)
    kwargs["selector"] = cls.pop_required_input(value, "selector")
    if scroll_into_view := value.pop("scroll_into_view", None):
      kwargs["scroll_into_view"] = cli_helper.parse_bool(scroll_into_view)
    return kwargs

  def __init__(self,
               selector: str,
               scroll_into_view: bool = False,
               timeout: dt.timedelta = ACTION_TIMEOUT):
    # TODO: convert to custom selector object.
    self._selector = selector
    self._scroll_into_view: bool = scroll_into_view
    super().__init__(timeout)

  @property
  def scroll_into_view(self) -> bool:
    return self._scroll_into_view

  @property
  def selector(self) -> str:
    return self._selector

  def run(self, run: Run) -> None:
    # TODO: support more selector types.
    prefix = "xpath/"
    if self.selector.startswith(prefix):
      xpath: str = self.selector[len(prefix):]
      run.browser.js(
          run.runner,
          """
       let element = document.evaluate(arguments[0], document).iterateNext();
       if (arguments[1]) element.scrollIntoView()
       element.click()
       """,
          arguments=[xpath, self._scroll_into_view])
    else:
      raise NotImplementedError(f"Unsupported selector: {self.selector}")

  def validate(self) -> None:
    super().validate()
    if not self.selector:
      raise ValueError(f"{self}.selector is missing.")

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["selector"] = self.selector
    details["scroll_into_view"] = self.scroll_into_view
    return details


ACTIONS_TUPLE: Tuple[Type[Action], ...] = (
    ClickAction,
    GetAction,
    ScrollAction,
    WaitAction,
)

ACTIONS: Dict[ActionType, Type] = {
    action_cls.TYPE: action_cls for action_cls in ACTIONS_TUPLE
}

assert len(ACTIONS_TUPLE) == len(ACTIONS), "Non unique Action.TYPE present"
