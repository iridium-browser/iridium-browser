# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from __future__ import annotations

import abc
import logging
import re
import time
from enum import Enum
from typing import (TYPE_CHECKING, Any, Dict, List, Optional, Sequence, TextIO,
                    Tuple, Type, Union)
from urllib.parse import urlparse

import hjson

from crossbench.benchmarks.base import StoryFilter, SubStoryBenchmark
from crossbench.cli_helper import existing_file_type
from crossbench.exception import ExceptionAnnotator
from crossbench.stories import Story

if TYPE_CHECKING:
  import argparse

  from crossbench.runner import Run


class Scroll(str, Enum):
  UP = "up"
  DOWN = "down"


class ButtonClick(str, Enum):
  LEFT = "left"
  RIGHT = "right"
  MIDDLE = "middle"


class ActionType(str, Enum):
  # CLICK = 'click'
  GET = "get"
  WAIT = "wait"
  SCROLL = "scroll"


class Page(Story, metaclass=abc.ABCMeta):

  url: Optional[str]

  @classmethod
  def all_story_names(cls) -> Tuple[str, ...]:
    return tuple(page.name for page in PAGE_LIST)

class LivePage(Page):

  def __init__(self, name: str, url: str, duration: float = 15):
    super().__init__(name, duration)
    assert url, "Invalid page url"
    self.url = url

  def details_json(self) -> Dict[str, Any]:
    result = super().details_json()
    result["url"] = str(self.url)
    return result

  def run(self, run: Run) -> None:
    run.browser.show_url(run.runner, self.url)
    run.runner.wait(self.duration + 1)

  def __str__(self) -> str:
    return f"Page(name={self.name}, url={self.url})"


class CombinedPage(Page):

  def __init__(self, pages: Sequence[Page], name: str = "combined"):
    assert len(pages), "No sub-pages provided for CombinedPage"
    assert len(pages) > 1, "Combined Page needs more than one page"
    self._pages = pages
    duration = sum(page.duration for page in pages)
    super().__init__(name, duration)
    self.url = None

  def details_json(self) -> Dict[str, Any]:
    result = super().details_json()
    result["pages"] = list(page.details_json() for page in self._pages)
    return result

  def run(self, run: Run) -> None:
    for page in self._pages:
      page.run(run)

  def __str__(self) -> str:
    combined_name = ",".join(page.name for page in self._pages)
    return f"CombinedPage({combined_name})"


class InteractivePage(Page):

  def __init__(self, actions: List[Action], name: str):
    self._name = name
    assert isinstance(actions, list)
    self._actions = actions
    assert self._actions, "Must have at least 1 valid action"
    duration = self._get_duration()
    super().__init__(name, duration)

  @property
  def actions(self) -> List[Action]:
    return self._actions

  def run(self, run: Run) -> None:
    for action in self._actions:
      action.run(run, self)

  def details_json(self) -> Dict[str, Any]:
    result = super().details_json()
    result["actions"] = list(action.details_json() for action in self._actions)
    return result

  def _get_duration(self) -> float:
    duration: float = 0
    for action in self._actions:
      if action.duration is not None:
        duration += action.duration
    return duration


PAGE_LIST = (
    LivePage("amazon", "https://www.amazon.de/s?k=heizkissen", 5),
    LivePage("bing", "https://www.bing.com/images/search?q=not+a+squirrel", 5),
    LivePage("caf", "http://www.caf.fr", 6),
    LivePage("cnn", "https://cnn.com/", 7),
    LivePage("ecma262", "https://tc39.es/ecma262/#sec-numbers-and-dates", 10),
    LivePage("expedia", "https://www.expedia.com/", 7),
    LivePage("facebook", "https://facebook.com/shakira", 8),
    LivePage("maps", "https://goo.gl/maps/TEZde4y4Hc6r2oNN8", 10),
    LivePage("microsoft", "https://microsoft.com/", 6),
    LivePage("provincial", "http://www.provincial.com", 6),
    LivePage("sueddeutsche", "https://www.sueddeutsche.de/wirtschaft", 8),
    LivePage("timesofindia", "https://timesofindia.indiatimes.com/", 8),
    LivePage("twitter", "https://twitter.com/wernertwertzog?lang=en", 6),
)
PAGES = {page.name: page for page in PAGE_LIST}
PAGE_LIST_SMALL = (PAGES["facebook"], PAGES["maps"], PAGES["timesofindia"],
                   PAGES["cnn"])


class LoadingPageFilter(StoryFilter):
  """
  Filter / create loading stories

  Syntax:
    "name"            Include LivePage with the given name from predefined list.
    "name", 10        Include predefined page with given 10s timeout.
    "http://..."      Include custom page at the given URL with a default
                      timeout of 15 seconds.
    "http://...", 12  Include custom page at the given URL with a 12s timeout

  These patterns can be combined:
    ["http://foo.com", 5, "http://bar.co.jp", "amazon"]
  """
  _DURATION_RE = re.compile(r"((\d*[.])?\d+)s?")

  stories: Sequence[Page]

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    kwargs = super().kwargs_from_cli(args)
    kwargs["separate"] = args.separate
    return kwargs

  def __init__(self,
               story_cls: Type[Page],
               patterns: Sequence[str],
               separate: bool = True):
    self.separate = separate
    super().__init__(story_cls, patterns)

  def process_all(self, patterns: Sequence[str]) -> None:
    name_or_url_list = patterns
    if len(name_or_url_list) == 1:
      if name_or_url_list[0] == "all":
        self.stories = PAGE_LIST
        return
      if name_or_url_list[0] == "default":
        self.stories = PAGE_LIST_SMALL
        return
    self._resolve_name_or_urls(name_or_url_list)
    # Check if we have unique domain names for better short names
    urls = list(urlparse(page.url) for page in self.stories)
    hostnames = set(url.hostname for url in urls)
    if len(hostnames) == len(urls):
      # Regenerate with short names
      self._resolve_name_or_urls(name_or_url_list, use_hostname=True)

  def _resolve_name_or_urls(self,
                            name_or_url_list: Sequence[str],
                            use_hostname: bool = False) -> None:
    page = None
    self.stories = []
    for value in name_or_url_list:
      if value in PAGES:
        page = PAGES[value]
      elif "://" in value or value.startswith("www."):
        name = value
        if value.startswith("www."):
          url = f"https://{value}"
        else:
          url = value
        if use_hostname:
          name = urlparse(url).hostname
        page = LivePage(name, url)
      else:
        # Use the last created page and set the duration on it
        assert page is not None, (
            f"Duration '{value}' has to follow a URL or page-name.")
        match = self._DURATION_RE.match(value)
        assert match, f"Duration '{value}' is not a number."
        duration = float(match.group(1))
        assert duration != 0, ("Duration should be positive. "
                               f"Got duration=0 page={page.name}")
        page.duration = duration
        continue
      self.stories.append(page)

  def create_stories(self) -> Sequence[Page]:
    logging.info("SELECTED STORIES: %s", str(list(map(str, self.stories))))
    if not self.separate and len(self.stories) > 1:
      combined_name = "_".join(page.name for page in self.stories)
      self.stories = (CombinedPage(self.stories, combined_name),)
    return self.stories


class PageLoadBenchmark(SubStoryBenchmark):
  """
  Benchmark runner for loading pages.

  Use --urls/--stories to either choose from an existing set of pages, or direct
  URLs. After each page you can also specify a custom wait/load duration in
  seconds. Multiple URLs/page names can be provided as a comma-separated list.

  Use --separate to load each page individually.

  Example:
    --urls=amazon
    --urls=http://cnn.com,10s
    --urls=http://twitter.com,5s,http://cnn.com,10s
  """
  NAME = "loading"
  DEFAULT_STORY_CLS = Page
  STORY_FILTER_CLS = LoadingPageFilter

  @classmethod
  def stories_from_cli_args(cls, args: argparse.Namespace) -> Sequence[Story]:
    if args.page_config:
      args.page_config = PageConfig.from_cli_args(args)
      if args.separate:
        return args.page_config.stories
      return (CombinedPage(args.page_config.stories,
                           "Page Scenarios - Combined"),)
    return super().stories_from_cli_args(args)

  @classmethod
  def add_cli_parser(cls, subparsers, aliases: Sequence[str] = ()):
    parser = super().add_cli_parser(subparsers, aliases)
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--urls",
        "--url",
        dest="stories",
        help="List of urls and durations to load: url,seconds,...")
    group.add_argument(
        "--page-config",
        type=existing_file_type,
        help="Stories we want to perform in the benchmark run following a"
        "specified scenario. For a reference on how to build scenarios and"
        "possible actions check  pages.config.example.hjson")
    return parser

  def __init__(self, stories: Sequence[Page], duration: Optional[float] = None):
    for story in stories:
      assert isinstance(story, Page)
      if duration is not None:
        assert duration > 0, f"Invalid page duration={duration}s"
        story.duration = duration
    super().__init__(stories)


class PageConfig:
  _DURATION_RE = re.compile(r"(?P<value>(\d+(\.\d+)?)) ?(?P<unit>[^0-9\.]+)?")

  @classmethod
  def from_cli_args(cls, args: argparse.Namespace) -> PageConfig:
    assert args.page_config
    with args.page_config.open(encoding="utf-8") as f:
      config = PageConfig()
      config.load(f)
      return config

  def __init__(self, raw_config_data: Optional[Dict] = None):
    self._exceptions = ExceptionAnnotator(throw=True)
    self.stories: List[InteractivePage] = []
    if raw_config_data:
      self.load_dict(raw_config_data)

  def load(self, f: TextIO, throw: bool = False) -> None:
    assert not self.stories
    self._exceptions.throw = throw
    with self._exceptions.capture(f"Loading Pages config file: {f.name}"):
      with self._exceptions.info(f"Parsing {hjson.__name__}"):
        config = hjson.load(f)
        self.load_dict(config, throw=throw)
    self._exceptions.assert_success()

  def load_dict(self, raw_config_data: Dict, throw: bool = False) -> None:
    assert not self.stories
    self._exceptions.throw = throw
    with self._exceptions.capture("Parsing scenarios / pages"):
      if "pages" not in raw_config_data:
        raise ValueError("Config does not provide a 'pages' dict.")
      if not raw_config_data["pages"]:
        raise ValueError("Config contains empty 'pages' dict.")
      with self._exceptions.info("Parsing config 'pages'"):
        self._parse_pages(raw_config_data["pages"])
    self._exceptions.assert_success()

  def _parse_pages(self, pages: Dict[str, Any]) -> None:
    """
    Behaviour to be aware

    There's no default Actions. In other words, if there's no Actions
    for a scenario this specific scenario will be ignored since there's
    nothing to do.

    If one would want to simply navigate to a site it is important to at
    least include: {action: "GET", value/url: google.com} in the specific
    scenario.

    As an example look at: page.config.example.hjson
    """
    for scenario_name, actions in pages.items():
      with self._exceptions.info(f"Parsing scenario ...['{scenario_name}']"):
        actions = self._parse_actions(actions, scenario_name)
        self.stories.append(InteractivePage(actions, scenario_name))

  def _parse_actions(self, actions: List[Dict[str, Any]],
                     scenario_name: str) -> List[Action]:
    if not actions:
      raise ValueError(f"Scenario '{scenario_name}' has no action")
    if not isinstance(actions, list):
      raise ValueError(f"Expected list, got={type(actions)}, '{actions}'")
    actions_list: List[Action] = []
    get_action_found = False
    for i, step in enumerate(actions):
      with self._exceptions.info(f"Parsing action ...['{scenario_name}'][{i}]"):
        if step.get("action") == ActionType.GET:
          get_action_found = True
        if "action" not in step:
          raise ValueError("No 'action' property found.")
        action_type = step["action"]
        if not action_type:
          raise ValueError("Empty 'action' property")
        if action_duration := step.get("duration", 0.0):
          action_duration = self._parse_duration(action_duration)
        value = step.get("url") or step.get("value")
        actions_list.append(
            self._create_action(action_type, value, action_duration))
    assert get_action_found, ("Not a valid entry for scenario: "
                              f"{scenario_name}. No 'get' action found.")
    assert actions_list, (f"Not valid entry for scenario {scenario_name} "
                          "does not contain any valid actions")
    return actions_list

  def _parse_duration(self,
                      time_str: Union[float, int, str]) -> Optional[float]:
    """
    This function will parse the measurement and the value from string value.
    Keep in mind the return is in seconds.

    For example:
    5s => 5
    5m => 5*60 = 300

    """
    if isinstance(time_str, (int, float)):
      return float(time_str)

    if not time_str:
      return None

    match = self._DURATION_RE.fullmatch(time_str)
    if match is None:
      return None

    value = match.group('value')
    if not value:
      raise Exception("Error: Duration value not found."
                      "Make sure to include a valid duration value")
    time_unit = match.group('unit')
    if not time_unit:
      # If no time unit provided we assume it is in seconds.
      return float(value)
    return float(value) * _TimeSuffix.get_multiplier(time_unit)

  def _create_action(self, action_type, value, duration) -> Action:
    if action_type not in _ACTION_FACTORY:
      raise ValueError(f"Unknown action name: '{action_type}'")
    return _ACTION_FACTORY[action_type](action_type, value, duration)


class _TimeSuffix:
  _MILLISECONDS_MULTIPLIER = 0.001
  _SECONDS_MULTIPLIER = 1
  _MINUTES_MULTIPLIER = 60
  _HOURS_MULTIPLIER = 3600

  @classmethod
  def get_multiplier(cls, suffix: str) -> float:
    if suffix in {"ms", "millis", "milliseconds"}:
      return cls._MILLISECONDS_MULTIPLIER
    if suffix in {"s", "sec", "secs", "second", "seconds"}:
      return cls._SECONDS_MULTIPLIER
    if suffix in {"m", "min", "mins", "minute", "minutes"}:
      return cls._MINUTES_MULTIPLIER
    if suffix in {"h", "hrs", "hour", "hours"}:
      return cls._HOURS_MULTIPLIER
    raise ValueError(f"Error: {suffix} is not support for duration. "
                     "Make sure to use a supported time unit/suffix")


class Action(abc.ABC):
  timeout: float
  _story: Story

  _EXCEPTION_BASE_STR = "Not valid action for scenario: "

  def __init__(self,
               action_type: ActionType,
               value: Optional[str] = None,
               duration: float = 0.0):
    self.action_type = action_type
    self.value = value
    assert isinstance(duration, float)
    self.duration = duration

  @abc.abstractmethod
  def run(self, run: Run, story: Story) -> None:
    pass

  @abc.abstractmethod
  def _validate_action(self) -> None:
    pass

  @abc.abstractmethod
  def details_json(self) -> Dict[str, Any]:
    pass


class GetAction(Action):

  def run(self, run: Run, story: Story) -> None:
    self._story = story
    self._validate_action()
    run.browser.show_url(run.runner, self.value)

  def _validate_action(self) -> None:
    if not self.value:
      raise Exception(self._EXCEPTION_BASE_STR +
                      f"{self._story._name}. Argument 'value' is not provided")

  def details_json(self) -> Dict[str, Any]:
    return {"action": self.action_type, "value": self.value}


class WaitAction(Action):

  def run(self, run: Run, story: Story) -> None:
    self._story = story
    self._validate_action()
    run.runner.wait(self.duration)

  def _validate_action(self) -> None:
    if not self.duration:
      raise Exception(
          self._EXCEPTION_BASE_STR +
          f"{self._story._name}. Argument 'duration' is not provided")

  def details_json(self) -> Dict[str, Any]:
    return {"action": self.action_type, "duration": self.duration}


class ScrollAction(Action):

  def run(self, run: Run, story: Story) -> None:
    self._story = story
    self._validate_action()
    time_end = self.duration + time.time()
    direction = 1 if self.value == Scroll.UP else -1

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

  def _validate_action(self) -> None:
    if not self.duration or not self.value:
      raise Exception(
          self._EXCEPTION_BASE_STR +
          f"{self._story._name}. Argument 'duration' is not provided")

  def details_json(self) -> Dict[str, Any]:
    return {
        "action": self.action_type,
        "value": self.value,
        "duration": self.duration
    }


# class ClickAction(Action):
# TODO: implement click even coming form the OS here
#   # # method to click at coordinates x,y on the screen.
#  Important this uses the screen size and not the browser window.
#   # def _click(x_coordinate: int,
#   #           y_coordinate: int,
#   #           button: ButtonClick = ButtonClick.LEFT,
#   #           clicks: int = 1,
#   #           interval: float=0):
#   #   # TODO:Implement the click action
#   #   return

_ACTION_FACTORY = {
    ActionType.GET: GetAction,
    ActionType.WAIT: WaitAction,
    ActionType.SCROLL: ScrollAction,
}
