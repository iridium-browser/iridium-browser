# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import logging
import pathlib
from typing import TYPE_CHECKING, Any, Dict, Optional, Sequence, Type
from urllib.parse import urlparse
from crossbench import cli_helper

from crossbench.benchmarks.benchmark import StoryFilter, SubStoryBenchmark

from . import page_config
from .page import (PAGE_LIST, PAGE_LIST_SMALL, PAGES, CombinedPage, LivePage,
                   Page)
from .playback_controller import PlaybackController

if TYPE_CHECKING:
  from crossbench.stories.story import Story


class LoadingPageFilter(StoryFilter[Page]):
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
               playback: Optional[PlaybackController] = None) -> None:
    self._playback = playback or PlaybackController.once()
    super().__init__(story_cls, patterns, separate)

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
        name: Optional[str] = value
        if value.startswith("www."):
          url = f"https://{value}"
        else:
          url = value

        if use_hostname:
          parse_result = urlparse(url)
          if parse_result.scheme == "file":
            name = pathlib.Path(parse_result.path).name
          else:
            name = parse_result.hostname
        if not name:
          raise argparse.ArgumentTypeError(f"Invalid url: {url}")
        page = LivePage(name, url, playback=self._playback)
      else:
        # Use the last created page and set the duration on it
        assert page is not None, (
            f"Duration '{value}' has to follow a URL or page-name.")
        page.set_duration(cli_helper.Duration.parse(value))
        continue
      self.stories.append(page)

  def create_stories(self, separate: bool) -> Sequence[Page]:
    logging.info("SELECTED STORIES: %s", str(list(map(str, self.stories))))
    if not separate and len(self.stories) > 1:
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
  def add_cli_parser(
      cls, subparsers: argparse.ArgumentParser, aliases: Sequence[str] = ()
  ) -> cli_helper.CrossBenchArgumentParser:
    parser = super().add_cli_parser(subparsers, aliases)
    page_config_group = parser.add_mutually_exclusive_group()
    # TODO: Migrate to dest="stories" using LoadingPageFilter.parse
    # TODO: move --stories into mutually exclusive group as well
    page_config_group.add_argument(
        "--urls",
        "--url",
        dest="stories",
        help="List of urls and durations to load: url,seconds,...")
    page_config_group.add_argument(
        "--page-config",
        dest="stories",
        type=page_config.PageConfig.parse,
        help="Stories we want to perform in the benchmark run following a"
        "specified scenario. For a reference on how to build scenarios and"
        "possible actions check  pages.config.example.hjson")
    page_config_group.add_argument(
        "--devtools-recorder",
        dest="stories",
        type=page_config.DevToolsRecorderPageConfig.parse,
        help=("Run a single story from a serialized DevTools recorder session. "
              "See https://developer.chrome.com/docs/devtools/recorder/ "
              "for more details."))

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

  @classmethod
  def stories_from_cli_args(cls, args: argparse.Namespace) -> Sequence[Story]:
    if isinstance(args.stories, list):
      if args.separate or len(args.stories) == 1:
        return args.stories
      return (CombinedPage(args.stories, "Page Scenarios - Combined",
                           args.playback),)
    return super().stories_from_cli_args(args)

  def __init__(self, stories: Sequence[Page]) -> None:
    for story in stories:
      assert isinstance(story, Page)
    super().__init__(stories)
