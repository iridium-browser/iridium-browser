# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
import logging
import sys
from typing import TYPE_CHECKING, Any, Optional, Sequence, Union

from crossbench import helper

if TYPE_CHECKING:
  from .runner import Runner
  from .run import Run
  from .timing import Timing
  from crossbench.browsers.browser import Browser
  from crossbench import plt


class Actions(helper.TimeScope):

  def __init__(self,
               message: str,
               run: Run,
               runner: Optional[Runner] = None,
               browser: Optional[Browser] = None,
               verbose: bool = False,
               measure: bool = True,
               timeout: dt.timedelta = dt.timedelta()):
    assert message, "Actions need a name"
    super().__init__(message)
    self._exception_annotation = run.exceptions.info(f"Action: {message}")
    self._run: Run = run
    self._browser: Browser = browser or run.browser
    self._runner: Runner = runner or run.runner
    self._is_active: bool = False
    self._verbose: bool = verbose
    self._measure = measure
    if timeout:
      self._max_end_datetime = min(dt.datetime.now() + timeout,
                                   run.max_end_datetime())
    else:
      self._max_end_datetime = run.max_end_datetime()

  @property
  def timing(self) -> Timing:
    return self._runner.timing

  @property
  def run(self) -> Run:
    return self._run

  @property
  def platform(self) -> plt.Platform:
    return self._run.platform

  def __enter__(self) -> Actions:
    self._exception_annotation.__enter__()
    super().__enter__()
    self._is_active = True
    logging.debug("Action begin: %s", self._message)
    if self._verbose:
      logging.info(self._message)
    else:
      # Print message that doesn't overlap with helper.Spinner
      sys.stdout.write(f"   {self._message}\r")
    return self

  def __exit__(self, exc_type, exc_value, exc_traceback) -> None:
    self._is_active = False
    self._exception_annotation.__exit__(exc_type, exc_value, exc_traceback)
    super().__exit__(exc_type, exc_value, exc_traceback)
    logging.debug("Action end: %s", self._message)
    if self._measure:
      self.run.durations[f"actions-duration {self.message}"] = self.duration

  def _assert_is_active(self) -> None:
    assert self._is_active, "Actions have to be used in a with scope"

  def js(self,
         js_code: str,
         timeout: Union[float, int] = 10,
         arguments: Sequence[object] = (),
         **kwargs) -> Any:
    self._assert_is_active()
    assert js_code, "js_code must be a valid JS script"
    if kwargs:
      js_code = js_code.format(**kwargs)
    delta = self.timing.timeout_timedelta(timeout)
    return self._browser.js(
        self._runner,  # pytype: disable=wrong-arg-types
        js_code,
        delta,
        arguments=arguments)

  def wait_js_condition(self, js_code: str, min_wait: Union[dt.timedelta,
                                                            float],
                        timeout: Union[dt.timedelta, float]) -> None:
    wait_range = helper.WaitRange(
        self.timing.timedelta(min_wait), self.timing.timeout_timedelta(timeout))
    assert "return" in js_code, (
        f"Missing return statement in js-wait code: {js_code}")
    for _, time_left in helper.wait_with_backoff(wait_range):
      time_units = self.timing.units(time_left)
      result = self.js(js_code, timeout=time_units, absolute_time=True)
      if result:
        return
      assert result is False, (
          f"js_code did not return a bool, but got: {result}\n"
          f"js-code: {js_code}")

  def show_url(self, url: str) -> None:
    self._assert_is_active()
    self._browser.show_url(
        self._runner,  # pytype: disable=wrong-arg-types
        url)

  def wait(
      self, seconds: Union[dt.timedelta,
                           float] = dt.timedelta(seconds=1)) -> None:
    self._assert_is_active()
    self.platform.sleep(seconds)
