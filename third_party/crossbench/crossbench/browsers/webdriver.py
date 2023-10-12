# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import logging
import pathlib
import time
import traceback
from typing import TYPE_CHECKING, Any, List, Optional, Sequence, cast

import selenium.common.exceptions
from selenium import webdriver

from crossbench.types import JsonDict

from .browser import Browser

if TYPE_CHECKING:
  import datetime as dt

  from crossbench.browsers.splash_screen import SplashScreen
  from crossbench.browsers.viewport import Viewport
  from crossbench.flags import Flags
  from crossbench import plt
  from crossbench.runner.run import Run
  from crossbench.runner.runner import Runner

  from selenium.webdriver.common.timeouts import Timeouts


class DriverException(RuntimeError):
  """Wrapper for more readable error messages than the default
  WebDriver exceptions."""

  def __init__(self, msg: str, browser: Optional[Browser] = None) -> None:
    self._browser = browser
    self._msg = msg
    super().__init__(msg)

  def __str__(self) -> str:
    browser_prefix = ""
    if self._browser:
      browser_prefix = f"browser={self._browser}: "
    return f"{browser_prefix}{self._msg}"


class WebDriverBrowser(Browser, metaclass=abc.ABCMeta):
  _driver: webdriver.Remote
  _driver_path: Optional[pathlib.Path]
  _driver_pid: int
  log_file: Optional[pathlib.Path]

  def __init__(
      self,
      label: str,
      path: Optional[pathlib.Path] = None,
      flags: Optional[Flags.InitialDataType] = None,
      js_flags: Optional[Flags.InitialDataType] = None,
      cache_dir: Optional[pathlib.Path] = None,
      type: str = "webdriver",  # pylint: disable=redefined-builtin
      driver_path: Optional[pathlib.Path] = None,
      viewport: Optional[Viewport] = None,
      splash_screen: Optional[SplashScreen] = None,
      platform: Optional[plt.Platform] = None):
    super().__init__(label, path, flags, js_flags, cache_dir, type, None,
                     viewport, splash_screen, platform)
    self._driver_path = driver_path

  @property
  def driver(self) -> webdriver.Remote:
    return self._driver

  @property
  def driver_log_file(self) -> pathlib.Path:
    log_file = self.log_file
    assert log_file
    return log_file.with_suffix(".driver.log")

  def setup_binary(self, runner: Runner) -> None:
    self._driver_path = self._find_driver().absolute()
    assert self._driver_path.exists(), (
        f"Webdriver path '{self._driver_path}' does not exist")

  @abc.abstractmethod
  def _find_driver(self) -> pathlib.Path:
    pass

  @abc.abstractmethod
  def _check_driver_version(self) -> None:
    pass

  def start(self, run: Run) -> None:
    self._check_driver_version()
    assert self._driver_path
    try:
      self._driver = self._start_driver(run, self._driver_path)
    except selenium.common.exceptions.SessionNotCreatedException as e:
      msg = e.msg or "Could not create Webdriver session."
      raise DriverException(msg, self) from e
    self._find_driver_pid()
    self._set_driver_timeouts(run)
    self._is_running = True
    self._setup_window()
    self._check_driver_version()

  def _find_driver_pid(self) -> None:
    service = getattr(self._driver, "service", None)
    if not service:
      return
    self._driver_pid = service.process.pid
    candidates: List[int] = []
    for child in self.platform.process_children(self._driver_pid):
      if str(child["exe"]) == str(self.path):
        candidates.append(child["pid"])
    if len(candidates) == 1:
      self._pid = candidates[0]
    else:
      logging.debug(
          "Could not find unique browser process for webdriver: %s, got %s",
          self, candidates)

  def _set_driver_timeouts(self, run: Run) -> None:
    """Adjust the global webdriver timeouts if the runner has custom timeout
    unit values.
    If timing.has_no_timeout each value is set to SAFE_MAX_TIMEOUT_TIMEDELTA."""
    timing = run.timing
    if not timing.timeout_unit:
      return
    if timing.has_no_timeout:
      logging.info("Disabling webdriver timeouts")
    else:
      factor = timing.timeout_unit.total_seconds()
      logging.info("Increasing webdriver timeouts by %fx", factor)
    timeouts: Timeouts = self.driver.timeouts
    if implicit_wait := getattr(timeouts, "implicit_wait", None):
      timeouts.implicit_wait = timing.timeout_timedelta(
          implicit_wait).total_seconds()
    if script := getattr(timeouts, "script", None):
      timeouts.script = timing.timeout_timedelta(script).total_seconds()
    if page_load := getattr(timeouts, "page_load", None):
      timeouts.page_load = timing.timeout_timedelta(page_load).total_seconds()
    self.driver.timeouts = timeouts

  def _setup_window(self) -> None:
    # Force main window to foreground.
    self._driver.switch_to.window(self._driver.current_window_handle)
    if self.viewport.is_headless:
      return
    if self.viewport.is_fullscreen:
      self._driver.fullscreen_window()
    elif self.viewport.is_maximized:
      self._driver.maximize_window()
    else:
      self._driver.set_window_position(self.viewport.x, self.viewport.y)
      self._driver.set_window_size(self.viewport.width, self.viewport.height)

  @abc.abstractmethod
  def _start_driver(self, run: Run,
                    driver_path: pathlib.Path) -> webdriver.Remote:
    pass

  def details_json(self) -> JsonDict:
    details: JsonDict = super().details_json()
    log = cast(JsonDict, details["log"])
    log["driver"] = str(self.driver_log_file)
    return details

  def show_url(self, runner: Runner, url: str) -> None:
    logging.debug("WebDriverBrowser.show_url(%s)", url)
    assert self._driver.window_handles, "Browser has no more opened windows."
    self._driver.switch_to.window(self._driver.window_handles[0])
    try:
      self._driver.get(url)
    except selenium.common.exceptions.WebDriverException as e:
      if e.msg and "net::ERR_CONNECTION_REFUSED" in e.msg:
        # pylint: disable=raise-missing-from
        raise DriverException(
            f"Browser failed to load URL={url}. The URL is likely unreachable.",
            self)
      raise

  def js(self,
         runner: Runner,
         script: str,
         timeout: Optional[dt.timedelta] = None,
         arguments: Sequence[object] = ()) -> Any:
    logging.debug("WebDriverBrowser.js() timeout=%s, script: %s", timeout,
                  script)
    assert self._is_running
    try:
      if timeout is not None:
        assert timeout.total_seconds() > 0, (
            f"timeout must be a positive number, got: {timeout}")
        self._driver.set_script_timeout(timeout.total_seconds())
      return self._driver.execute_script(script, *arguments)
    except selenium.common.exceptions.WebDriverException as e:
      # pylint: disable=raise-missing-from
      raise ValueError(f"Could not execute JS: {e.msg}")

  def quit(self, runner: Runner) -> None:
    assert self._is_running
    self.force_quit()

  def force_quit(self) -> None:
    if getattr(self, "_driver", None) is None:
      return
    logging.debug("WebDriverBrowser.force_quit()")
    try:
      try:
        # Close the current window.
        self._driver.close()
        time.sleep(0.1)
      except selenium.common.exceptions.NoSuchWindowException:
        # No window is good.
        pass
      try:
        self._driver.quit()
      except selenium.common.exceptions.InvalidSessionIdException:
        return
      # Sometimes a second quit is needed, ignore any warnings there
      try:
        self._driver.quit()
      except Exception as e:  # pylint: disable=broad-except
        logging.debug("Driver raised exception on quit: %s\n%s", e,
                      traceback.format_exc())
      return
    except Exception as e:  # pylint: disable=broad-except
      logging.debug("Could not quit browser: %s\n%s", e, traceback.format_exc())
    finally:
      self._is_running = False
    return


class RemoteWebDriver(WebDriverBrowser, Browser):
  """Represent a remote WebDriver that has already been started"""

  def __init__(self, label: str, driver: webdriver.Remote) -> None:
    super().__init__(label=label, path=None, type="remote")
    self._driver = driver
    self.version: str = driver.capabilities["browserVersion"]
    self.major_version: int = int(self.version.split(".")[0])

  def _check_driver_version(self) -> None:
    raise NotImplementedError()

  def _extract_version(self) -> str:
    raise NotImplementedError()

  def _find_driver(self) -> pathlib.Path:
    raise NotImplementedError()

  def _start_driver(self, run: Run,
                    driver_path: pathlib.Path) -> webdriver.Remote:
    raise NotImplementedError()

  def setup_binary(self, runner: Runner) -> None:
    pass

  def start(self, run: Run) -> None:
    # Driver has already been started. We just need to mark it as running.
    self._is_running = True
    if self.viewport.is_fullscreen:
      self._driver.fullscreen_window()
    elif self.viewport.is_maximized:
      self._driver.maximize_window()
    else:
      self._driver.set_window_position(self.viewport.x, self.viewport.y)
      self._driver.set_window_size(self.viewport.width, self.viewport.height)

  def quit(self, runner: Runner) -> None:
    # External code that started the driver is responsible for shutting it down.
    self._is_running = False
