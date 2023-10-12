# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import datetime as dt
import logging
import pathlib
import time
from threading import Timer
from typing import TYPE_CHECKING, Sequence, Tuple

from selenium import webdriver
from selenium.common.exceptions import (ElementNotInteractableException,
                                        TimeoutException)
from selenium.webdriver.common.by import By
from selenium.webdriver.support import expected_conditions
from selenium.webdriver.support.ui import WebDriverWait

from crossbench.benchmarks.benchmark import StoryFilter, SubStoryBenchmark
from crossbench.browsers.webdriver import WebDriverBrowser
from crossbench.stories.story import Story

if TYPE_CHECKING:
  from crossbench.runner.run import Run

STORY_LIST = [
  "YoutubeFullscreen",
  "ZoomMeeting",
  "Browsing",
]

class PowerBenchmarkStory(Story, metaclass=abc.ABCMeta):

  _driver: webdriver.Remote

  @classmethod
  def all_story_names(cls) -> Sequence[str]:
    return STORY_LIST

  def run(self, run: Run) -> None:
    self.get_driver(run)
    assert self._driver, "Could not find raw driver"

  def get_driver(self, run: Run) -> None:
    if isinstance(run.browser, WebDriverBrowser):
      self._driver = run.browser.driver
    else:
      raise TypeError("Power benchmark only supports WebDriverBrowser.")


class PowerBenchmarkStoryFilter(StoryFilter[PowerBenchmarkStory]):
  """
  Filter power benchmark stories by name.

  Syntax:
    "all", "default"    Include all stories.
    "name"              Include story with the given name.
  """
  story_names: Sequence[str]

  def process_all(self, patterns: Sequence[str]) -> None:
    if len(patterns) == 1 and patterns[0] in ("all", "default"):
      self.story_names = STORY_LIST
      return
    for story_name in patterns:
      assert story_name in STORY_LIST, f"Could not find {story_name} in STORY_LIST"
    self.story_names = patterns

  def create_stories(self, separate: bool) -> Sequence[PowerBenchmarkStory]:
    stories = []
    duration = dt.timedelta(15 * 60)
    for story_name in self.story_names:
      stories.append(globals()[story_name + "Story"](duration))
    return stories


class RepeatTimer(Timer):
  def run(self):
    while not self.finished.wait(self.interval):
      self.function(*self.args, **self.kwargs)


class BrowsingStory(PowerBenchmarkStory):

  def __init__(self, duration: dt.timedelta = dt.timedelta(15 * 60)):
    super().__init__("Browsing", duration)
    self._url_file = pathlib.Path(__file__).parent.absolute() / "browsing_urls.txt"
    self._urls = self.get_urls()
    self._idx = 0

  def get_urls(self) -> Sequence[str]:
    urls = []
    with self._url_file.open(encoding="utf-8") as f:
      for line in f:
        url = line.strip()
        if url:
            urls.append(url)
    return urls

  def browser_url(self) -> None:
    url = self._urls[self._idx]
    self._driver.get(url)
    self._idx += 1
    self._idx %= len(self._urls)

  def run(self, run: Run) -> None:
    self.get_driver(run)

    timer = RepeatTimer(15, self.browser_url)
    timer.start()
    time.sleep(self.duration.total_seconds())
    timer.cancel()


class ZoomMeetingStory(PowerBenchmarkStory):

  def __init__(self, duration: dt.timedelta = dt.timedelta(15 * 60)):
    super().__init__("ZoomMeeting", duration)

  def run(self, run: Run) -> None:
    super().run(run)

    duration_minutes = int(self.duration.total_seconds() // 60)
    for _ in range(duration_minutes):
      self._driver.get("https://zoom.us/test")

      # Click the "Join" button
      btn = WebDriverWait(self._driver, 10).until(
          expected_conditions.element_to_be_clickable((By.ID, "btnJoinTest")))
      btn.click()

      # Don't download the Zoom client, use Browser instead
      btn = WebDriverWait(self._driver, 15).until(
          expected_conditions.element_to_be_clickable(
              (By.XPATH, "//a[text()='Join from Your Browser']")))
      btn.click()

      # Input name
      name = WebDriverWait(self._driver, 10).until(
          expected_conditions.element_to_be_clickable((By.ID, "inputname")))
      name.send_keys("CBB Zoom Test")

      # Click the "Join" button
      btn = WebDriverWait(self._driver, 10).until(
          expected_conditions.element_to_be_clickable((By.ID, "joinBtn")))
      btn.click()

      # Wait for 10 seconds to make sure to finish joining the meeting
      time.sleep(10)

      # Click the "Join Audio by Computer" button if it shows up.
      # Audio is used by default if the button is not present.
      try:
        btn = WebDriverWait(self._driver, 10).until(
            expected_conditions.element_to_be_clickable(
                (By.XPATH, "//button[text()='Join Audio by Computer']")))
        btn.click()
      except (TimeoutException, ElementNotInteractableException):
        logging.info("Join audio by computer button is not present.")
        pass

      # Start a new test meeting every 1 minute to avoid the meeting to be
      # ended by the Zoom host.
      time.sleep(60)


class YoutubeFullscreenStory(PowerBenchmarkStory):

  def __init__(self, duration: dt.timedelta = dt.timedelta(15 * 60)):
    super().__init__("YoutubeFullscreen", duration)

  def click_button_by_xpath(self, xpath: str):
    """Find button by Xpath, click it when it's clickable."""
    btn = WebDriverWait(self._driver, 10).until(
        expected_conditions.element_to_be_clickable((By.XPATH, xpath)))
    btn.click()
    time.sleep(2)

  def hide_cookie_banner(self):
    buttons = self._driver.find_elements(
        By.XPATH,
        "//button[@aria-label='Accept the use of cookies and other data for the purposes described']"
    )
    if buttons:
      buttons[0].click()
    else:
      logging.info("No cookie banner found")
    time.sleep(1)

  def set_resolution_to_1080p(self):
    """Set the video quality to 1080p."""
    # Click the "Settings" button
    self.click_button_by_xpath(
        "//button[@data-tooltip-target-id='ytp-settings-button']")
    # Click the "Quality" button
    self.click_button_by_xpath("//div[text()='Quality']")
    # CLick the "1080p" button
    self.click_button_by_xpath("//span[contains(string(), '1080p')]")

  def mute_video(self):
    buttons = self._driver.find_elements(By.XPATH,
                                         "//button[@title='Mute (m)']")
    if not buttons:  # Already muted, since "Mute" button is not found
      return
    buttons[0].click()
    time.sleep(2)

  def is_playing(self) -> bool:
    """Check if the video is playing."""
    # If there is no "Play" button, then the video is playing.
    buttons = self._driver.find_elements(By.XPATH,
                                         "//button[@title='Play (k)']")
    return not buttons

  def playstop(self):
    """Play/Stop the video using shortcut key."""
    page = self._driver.find_element(By.TAG_NAME, "body")
    page.send_keys("K")
    time.sleep(2)

  def fullscreen(self):
    """Switch fullscreen on/off using shortcut key."""
    page = self._driver.find_element(By.TAG_NAME, "body")
    page.send_keys("F")
    time.sleep(2)

  def run(self, run: Run) -> None:
    super().run(run)
    with run.actions("loading page"):
      self._driver.get("https://www.youtube.com/watch?v=rV_ERKtNyNA?t=1")
      time.sleep(10)

    with run.actions("Preparing video"):
      self.hide_cookie_banner()
      # If the video is playing, stop it.
      if self.is_playing():
        self.playstop()

    with run.actions("Preparing video settings"):
      self.mute_video()
      self.set_resolution_to_1080p()

    with run.actions("Playing video"):
      self.playstop()
      self.fullscreen()
      # Sleep while playing video
      time.sleep(self.duration.total_seconds())


class PowerBenchmark(SubStoryBenchmark):
  """
  Benchmark runner for power benchmarks.
  """
  NAME = "powerbenchmark"
  DEFAULT_STORY_CLS = PowerBenchmarkStory
  STORY_FILTER_CLS = PowerBenchmarkStoryFilter

  def __init__(self, stories: Sequence[PowerBenchmarkStory]) -> None:
    for story in stories:
      assert isinstance(story, PowerBenchmarkStory)
    super().__init__(stories)

  @classmethod
  def aliases(cls) -> Tuple[str, ...]:
    return ("pb", "power")
