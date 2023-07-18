# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from __future__ import annotations

import abc
import argparse
import datetime as dt
import logging
import math
import re
import time
from enum import Enum
from typing import TYPE_CHECKING
from urllib.parse import urlparse

import hjson

from crossbench.benchmarks.benchmark import StoryFilter, SubStoryBenchmark
from crossbench.cli_helper import parse_file_path
from crossbench.exception import ExceptionAnnotator
from crossbench.stories import Story

if TYPE_CHECKING:
  from typing import (Any, Dict, Iterator, List, Optional, Sequence, TextIO,
                      Tuple, Type, Union)

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


class PlaybackController:

  @classmethod
  def parse(cls, value: str) -> PlaybackController:
    if not value or value == "once":
      return cls.once()
    if value in ("inf", "infinity", "forever"):
      return cls.forever()
    if value[-1].isnumeric():
      raise argparse.ArgumentTypeError(
          f"Missing unit suffix: '{value}'\n"
          "Use 'x' for repetitions or time unit 's', 'm', 'h'")
    if value[-1] == "x":
      try:
        loops = int(value[:-1])
      except ValueError as e:
        raise argparse.ArgumentTypeError(
            f"Repeat-count must be a valid int, {e}")
      if loops <= 0:
        raise argparse.ArgumentTypeError(
            f"Repeat-count must be positive: {value}")
      try:
        return cls.repeat(loops)
      except ValueError:
        pass
    duration: Optional[float] = _Duration.parse(value)
    if not duration:
      raise argparse.ArgumentTypeError(f"Invalid cycle argument: {value}")
    return cls.timeout(duration)

  @classmethod
  def once(cls) -> RepeatPlaybackController:
    return RepeatPlaybackController(1)

  @classmethod
  def repeat(cls, count: int) -> RepeatPlaybackController:
    return RepeatPlaybackController(count)

  @classmethod
  def forever(cls) -> PlaybackController:
    return PlaybackController()

  @classmethod
  def timeout(cls, duration: float) -> TimeoutPlaybackController:
    return TimeoutPlaybackController(duration)

  def __iter__(self) -> Iterator[None]:
    while True:
      yield None


class TimeoutPlaybackController(PlaybackController):

  def __init__(self, duration: float) -> None:
    # TODO: support --time-unit
    self._duration = duration

  @property
  def duration(self) -> float:
    return self._duration

  def __iter__(self) -> Iterator[None]:
    end = dt.datetime.now() + dt.timedelta(seconds=self._duration)
    while dt.datetime.now() <= end:
      yield None


class RepeatPlaybackController(PlaybackController):

  def __init__(self, count: int) -> None:
    assert count > 0, f"Invalid page playback count: {count}"
    self._count = count

  def __iter__(self) -> Iterator[None]:
    for _ in range(self._count):
      yield None

  @property
  def count(self) -> int:
    return self._count


class Page(Story, metaclass=abc.ABCMeta):

  url: Optional[str]

  @classmethod
  def all_story_names(cls) -> Tuple[str, ...]:
    return tuple(page.name for page in PAGE_LIST)

  def __init__(self,
               name: str,
               duration: float,
               playback: Optional[PlaybackController] = None):
    self._playback = playback or PlaybackController.once()
    super().__init__(name, duration)

  def set_parent(self, parent: Page):
    # TODO: support nested playback controllers.
    self._playback = PlaybackController.once()
    del parent


class LivePage(Page):
  url: str

  def __init__(self,
               name: str,
               url: str,
               duration: float = 15,
               playback: Optional[PlaybackController] = None) -> None:
    super().__init__(name, duration, playback)
    assert url, "Invalid page url"
    self.url: str = url

  def details_json(self) -> Dict[str, Any]:
    result = super().details_json()
    result["url"] = str(self.url)
    return result

  def run(self, run: Run) -> None:
    for _ in self._playback:
      run.browser.show_url(run.runner, self.url)
      run.runner.wait(self.duration + 1)

  def __str__(self) -> str:
    return f"Page(name={self.name}, url={self.url})"


class CombinedPage(Page):

  def __init__(self,
               pages: Sequence[Page],
               name: str = "combined",
               playback: Optional[PlaybackController] = None):
    assert len(pages), "No sub-pages provided for CombinedPage"
    assert len(pages) > 1, "Combined Page needs more than one page"
    self._pages = pages
    for page in self._pages:
      page.set_parent(self)
    duration = sum(page.duration for page in pages)
    super().__init__(name, duration, playback)
    self.url = None

  def details_json(self) -> Dict[str, Any]:
    result = super().details_json()
    result["pages"] = list(page.details_json() for page in self._pages)
    return result

  def run(self, run: Run) -> None:
    for _ in self._playback:
      for page in self._pages:
        page.run(run)

  def __str__(self) -> str:
    combined_name = ",".join(page.name for page in self._pages)
    return f"CombinedPage({combined_name})"


class InteractivePage(Page):

  def __init__(self,
               actions: List[Action],
               name: str,
               playback: Optional[PlaybackController] = None):
    self._name = name
    assert isinstance(actions, list)
    self._actions = actions
    assert self._actions, "Must have at least 1 valid action"
    duration = self._get_duration()
    super().__init__(name, duration, playback)

  @property
  def actions(self) -> List[Action]:
    return self._actions

  def run(self, run: Run) -> None:
    for _ in self._playback:
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
    kwargs["playback"] = args.playback
    return kwargs

  def __init__(self,
               story_cls: Type[Page],
               patterns: Sequence[str],
               separate: bool = True,
               playback: Optional[PlaybackController] = None):
    self.separate = separate
    self._playback = playback or PlaybackController.once()
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
        template = PAGES[value]
        # Create copy so we can modify the playback value.
        page = LivePage(
            template.name,
            template.url,
            template.duration,
            playback=self._playback)
      elif "://" in value or value.startswith("www."):
        name = value
        if value.startswith("www."):
          url = f"https://{value}"
        else:
          url = value
        if use_hostname:
          name = urlparse(url).hostname
        if not name:
          raise argparse.ArgumentTypeError(f"Invalid url: {url}")
        page = LivePage(name, url, playback=self._playback)
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
      self.stories = (CombinedPage(self.stories, combined_name,
                                   self._playback),)
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
                           "Page Scenarios - Combined", args.playback),)
    return super().stories_from_cli_args(args)

  @classmethod
  def add_cli_parser(
      cls, subparsers: argparse.ArgumentParser, aliases: Sequence[str] = ()
  ) -> argparse.ArgumentParser:
    parser = super().add_cli_parser(subparsers, aliases)
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--urls",
        "--url",
        dest="stories",
        help="List of urls and durations to load: url,seconds,...")
    group.add_argument(
        "--page-config",
        type=parse_file_path,
        help="Stories we want to perform in the benchmark run following a"
        "specified scenario. For a reference on how to build scenarios and"
        "possible actions check  pages.config.example.hjson")

    playback_group = parser.add_mutually_exclusive_group()
    playback_group.add_argument(
        "--playback",
        "--cycle",
        default=PlaybackController.once(),
        type=PlaybackController.parse,
        help="Set limit on looping through/repeating the selected stories. "
        "Default is once."
        "Valid values are: 'once', 'forever', number, time. "
        "Cycle 10 times: '--playback=10x'. "
        "Repeat for 1.5 hours: '--playback=1.5h'.")
    playback_group.add_argument(
        "--forever",
        dest="playback",
        const=PlaybackController(),
        action="store_const",
        help="Equivalent to --playback=infinity")
    return parser

  def __init__(self, stories: Sequence[Page], duration: Optional[float] = None):
    for story in stories:
      assert isinstance(story, Page)
      if duration is not None:
        assert duration > 0, f"Invalid page duration={duration}s"
        story.duration = duration
    super().__init__(stories)


class PageConfig:

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
    playback: PlaybackController = PlaybackController.parse(
        pages.get("playback", "1x"))
    for scenario_name, actions in pages.items():
      with self._exceptions.info(f"Parsing scenario ...['{scenario_name}']"):
        actions = self._parse_actions(actions, scenario_name)
        self.stories.append(InteractivePage(actions, scenario_name, playback))

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
          action_duration = _Duration.parse(action_duration)
        value = step.get("url") or step.get("value")
        actions_list.append(
            self._create_action(action_type, value, action_duration))
    assert get_action_found, ("Not a valid entry for scenario: "
                              f"{scenario_name}. No 'get' action found.")
    assert actions_list, (f"Not valid entry for scenario {scenario_name} "
                          "does not contain any valid actions")
    return actions_list

  def _create_action(self, action_type, value, duration) -> Action:
    if action_type not in _ACTION_FACTORY:
      raise ValueError(f"Unknown action name: '{action_type}'")
    return _ACTION_FACTORY[action_type](action_type, value, duration)


class _Duration:
  _DURATION_RE = re.compile(r"(?P<value>(\d+(\.\d+)?)) ?(?P<unit>[^0-9\.]+)?")

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

  @classmethod
  def parse(cls, time_value: Union[float, int, str]) -> Optional[float]:
    """
    This function will parse the measurement and the value from string value.
    Keep in mind the return is in seconds.

    For example:
    5s => 5
    5m => 5*60 = 300

    """
    if isinstance(time_value, (int, float)):
      if time_value < 0:
        raise argparse.ArgumentTypeError(
            f"Duration must be positive, but got: {time_value}")
      return float(time_value)

    if not time_value:
      return None

    match = cls._DURATION_RE.fullmatch(time_value)
    if match is None:
      return None

    value = match.group('value')
    if not value:
      raise argparse.ArgumentTypeError(
          "Error: Duration value not found."
          "Make sure to include a valid duration value")
    time_unit = match.group('unit')
    try:
      time_value = float(value)
    except ValueError as e:
      raise argparse.ArgumentTypeError(f"Duration must be a valid number, {e}")
    if time_value < 0 or math.isnan(time_value) or math.isinf(time_value):
      raise argparse.ArgumentTypeError(
          f"Duration must be positive, but got: {time_value}")
    if not time_unit:
      # If no time unit provided we assume it is in seconds.
      return time_value
    return time_value * cls.get_multiplier(time_unit)


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
    assert duration >= 0 and not math.isinf(duration), (
        f"Invalid duration: {duration}")

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
