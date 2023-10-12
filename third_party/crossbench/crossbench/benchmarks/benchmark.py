# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import argparse
import logging
import re
from typing import (TYPE_CHECKING, Any, Dict, Generic, List, Optional, Sequence,
                    Tuple, Type, TypeVar, cast)

from crossbench import cli_helper, helper
from crossbench.stories.press_benchmark import PressBenchmarkStory
from crossbench.stories.story import Story

if TYPE_CHECKING:
  from crossbench.runner.runner import Runner


class Benchmark(abc.ABC):
  NAME: str = ""
  DEFAULT_STORY_CLS: Type[Story] = Story

  @classmethod
  def cli_help(cls) -> str:
    assert cls.__doc__, (f"Benchmark class {cls} must provide a doc string.")
    # Return the first non-empty line
    return cls.__doc__.strip().splitlines()[0]

  @classmethod
  def cli_description(cls) -> str:
    assert cls.__doc__
    return cls.__doc__.strip()

  @classmethod
  def cli_epilog(cls) -> str:
    return ""

  @classmethod
  def aliases(cls) -> Tuple[str, ...]:
    return tuple()

  @classmethod
  def add_cli_parser(
      cls, subparsers, aliases: Sequence[str] = ()
  ) -> cli_helper.CrossBenchArgumentParser:
    parser = subparsers.add_parser(
        cls.NAME,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        help=cls.cli_help(),
        description=cls.cli_description(),
        epilog=cls.cli_epilog(),
        aliases=aliases)
    assert isinstance(parser, cli_helper.CrossBenchArgumentParser)
    return parser

  @classmethod
  def describe(cls) -> Dict[str, Any]:
    return {
        "name": cls.NAME,
        "description": "\n".join(helper.wrap_lines(cls.cli_description(), 70)),
        "stories": [],
        "probes-default": {
            probe_cls.NAME: "\n".join(
                list(helper.wrap_lines((probe_cls.__doc__ or "").strip(), 70)))
            for probe_cls in cls.DEFAULT_STORY_CLS.PROBES
        }
    }

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    del args
    return {}

  @classmethod
  def from_cli_args(cls, args: argparse.Namespace) -> Benchmark:
    kwargs = cls.kwargs_from_cli(args)
    return cls(**kwargs)

  def __init__(self, stories: Sequence[Story]) -> None:
    assert self.NAME is not None, f"{self} has no .NAME property"
    assert self.DEFAULT_STORY_CLS != Story, (
        f"{self} has no .DEFAULT_STORY_CLS property")
    self.stories: List[Story] = self._validate_stories(stories)

  def _validate_stories(self, stories: Sequence[Story]) -> List[Story]:
    assert stories, "No stories provided"
    for story in stories:
      assert isinstance(story, self.DEFAULT_STORY_CLS), (
          f"story={story} should be a subclass/the same "
          f"class as {self.DEFAULT_STORY_CLS}")
    first_story = stories[0]
    expected_probes_cls_list = first_story.PROBES
    for story in stories:
      assert story.PROBES == expected_probes_cls_list, (
          f"story={story} has different PROBES than {first_story}")
    return list(stories)

  def setup(self, runner: Runner) -> None:
    del runner


StoryT = TypeVar("StoryT", bound=Story)


class StoryFilter(Generic[StoryT], metaclass=abc.ABCMeta):

  @classmethod
  def add_cli_parser(
      cls, parser: argparse.ArgumentParser) -> argparse.ArgumentParser:
    return parser

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    return {"patterns": args.stories.split(",")}

  @classmethod
  def from_cli_args(cls, story_cls: Type[StoryT],
                    args: argparse.Namespace) -> StoryFilter[StoryT]:
    kwargs = cls.kwargs_from_cli(args)
    return cls(story_cls, **kwargs)

  def __init__(self,
               story_cls: Type[StoryT],
               patterns: Sequence[str],
               separate: bool = False) -> None:
    self.story_cls = story_cls
    assert issubclass(
        story_cls, Story), (f"Subclass of {Story} expected, found {story_cls}")
    # Using order-preserving dict instead of set
    self._known_names: Dict[str,
                            None] = dict.fromkeys(story_cls.all_story_names())
    self.stories: Sequence[StoryT] = []
    self.process_all(patterns)
    self.stories = self.create_stories(separate)

  @abc.abstractmethod
  def process_all(self, patterns: Sequence[str]) -> None:
    pass

  @abc.abstractmethod
  def create_stories(self, separate: bool) -> Sequence[StoryT]:
    pass


class SubStoryBenchmark(Benchmark, metaclass=abc.ABCMeta):
  STORY_FILTER_CLS: Type[StoryFilter] = StoryFilter

  @classmethod
  def add_cli_parser(
      cls, subparsers, aliases: Sequence[str] = ()
  ) -> cli_helper.CrossBenchArgumentParser:
    parser = super().add_cli_parser(subparsers, aliases)
    # TODO: move these args to a dedicated SubStoryFilter class.
    parser.add_argument(
        "--stories",
        "--story",
        dest="stories",
        default="default",
        help="Comma-separated list of story names. "
        "Use 'all' for selecting all available stories. "
        "Use 'default' for the standard selection of stories.")
    is_combined_group = parser.add_mutually_exclusive_group()
    is_combined_group.add_argument(
        "--combined",
        dest="separate",
        default=False,
        action="store_false",
        help="Run each story in the same session. (default)")
    is_combined_group.add_argument(
        "--separate",
        action="store_true",
        help="Run each story in a fresh browser.")
    return parser

  @classmethod
  def cli_description(cls) -> str:
    desc = super().cli_description()
    desc += "\n\n"
    desc += ("Stories (alternatively use the 'describe benchmark "
             f"{cls.NAME}' command):\n")
    desc += ", ".join(cls.all_story_names())
    desc += "\n\n"
    desc += "Filtering (for --stories): "
    assert cls.STORY_FILTER_CLS.__doc__, (
        f"{cls.STORY_FILTER_CLS} has no doc string.")
    desc += cls.STORY_FILTER_CLS.__doc__.strip()

    return desc

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    kwargs = super().kwargs_from_cli(args)
    kwargs["stories"] = cls.stories_from_cli_args(args)
    return kwargs

  @classmethod
  def stories_from_cli_args(cls, args: argparse.Namespace) -> Sequence[Story]:
    return cls.STORY_FILTER_CLS.from_cli_args(cls.DEFAULT_STORY_CLS,
                                              args).stories

  @classmethod
  def describe(cls) -> Dict[str, Any]:
    data = super().describe()
    data["stories"] = cls.all_story_names()
    return data

  @classmethod
  def all_story_names(cls) -> Sequence[str]:
    return sorted(cls.DEFAULT_STORY_CLS.all_story_names())


PressBenchmarkStoryT = TypeVar(
    "PressBenchmarkStoryT", bound=PressBenchmarkStory)


class PressBenchmarkStoryFilter(Generic[PressBenchmarkStoryT],
                                StoryFilter[PressBenchmarkStoryT]):
  """
  Filter stories by name or regexp.

  Syntax:
    "all"     Include all stories (defaults to story_names).
    "name"    Include story with the given name.
    "-name"   Exclude story with the given name'
    "foo.*"   Include stories whose name matches the regexp.
    "-foo.*"  Exclude stories whose name matches the regexp.

  These patterns can be combined:
    [".*", "-foo", "-bar"] Includes all except the "foo" and "bar" story
  """

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    kwargs = super().kwargs_from_cli(args)
    kwargs["separate"] = args.separate
    kwargs["url"] = args.custom_benchmark_url
    return kwargs

  def __init__(self,
               story_cls: Type[PressBenchmarkStoryT],
               patterns: Sequence[str],
               separate: bool = False,
               url: Optional[str] = None):
    self.url = url
    # Using dict instead as ordered set
    self._selected_names: Dict[str, None] = {}
    super().__init__(story_cls, patterns, separate)
    assert issubclass(self.story_cls, PressBenchmarkStory)
    for name in self._known_names:
      assert name, "Invalid empty story name"
      assert not name.startswith("-"), (
          f"Known story names cannot start with '-', but got '{name}'.")
      assert not name == "all", "Known story name cannot match 'all'."

  def process_all(self, patterns: Sequence[str]) -> None:
    if not isinstance(patterns, (list, tuple)):
      raise ValueError("Expected Sequence of story name or patterns "
                       f"but got '{type(patterns)}'.")
    for pattern in patterns:
      self.process_pattern(pattern)

  def process_pattern(self, pattern: str) -> None:
    if pattern.startswith("-"):
      self.remove(pattern[1:])
    else:
      self.add(pattern)

  def add(self, pattern: str) -> None:
    self._check_processed_pattern(pattern)
    regexp = self._pattern_to_regexp(pattern)
    self._add_matching(regexp, pattern)

  def remove(self, pattern: str) -> None:
    self._check_processed_pattern(pattern)
    regexp = self._pattern_to_regexp(pattern)
    self._remove_matching(regexp, pattern)

  def _pattern_to_regexp(self, pattern: str) -> re.Pattern:
    if pattern == "all":
      return re.compile(".*")
    if pattern == "default":
      default_story_names = self.story_cls.default_story_names()
      if default_story_names == self.story_cls.all_story_names():
        return re.compile(".*")
      joined_names = "|".join(re.escape(name) for name in default_story_names)
      return re.compile(f"^({joined_names})$")
    if pattern in self._known_names:
      return re.compile(re.escape(pattern))
    return re.compile(pattern)

  def _check_processed_pattern(self, pattern: str) -> None:
    if not pattern:
      raise ValueError("Empty pattern is not allowed")
    if pattern == "-":
      raise ValueError(f"Empty remove pattern not allowed: '{pattern}'")
    if pattern[0] == "-":
      raise ValueError(f"Unprocessed negative pattern not allowed: '{pattern}'")

  def _add_matching(self, regexp: re.Pattern, original_pattern: str) -> None:
    substories = self._regexp_match(regexp, original_pattern)
    self._selected_names.update(dict.fromkeys(substories))

  def _remove_matching(self, regexp: re.Pattern, original_pattern: str) -> None:
    substories = self._regexp_match(regexp, original_pattern)
    for substory in substories:
      try:
        del self._selected_names[substory]
      except KeyError as e:
        raise ValueError(
            "Removing Story failed: "
            f"name='{substory}' extracted by pattern='{original_pattern}'"
            "is not in the filtered story list") from e

  def _regexp_match(self, regexp: re.Pattern,
                    original_pattern: str) -> List[str]:
    substories = [
        substory for substory in self._known_names if regexp.fullmatch(substory)
    ]
    if not substories:
      logging.warning(
          "No matching stories, using case-insensitive fallback regexp.")
      iregexp: re.Pattern = re.compile(regexp.pattern, flags=re.IGNORECASE)
      substories = [
          substory for substory in self._known_names
          if iregexp.fullmatch(substory)
      ]
    if not substories:
      raise ValueError(f"'{original_pattern}' didn't match any stories.")
    if len(substories) == len(self._known_names) and self._selected_names:
      raise ValueError(f"'{original_pattern}' matched all and overrode all"
                       "previously filtered story names.")
    return substories

  def create_stories(self, separate: bool) -> Sequence[PressBenchmarkStoryT]:
    logging.info("SELECTED STORIES: %s",
                 str(list(map(str, self._selected_names))))
    names = list(self._selected_names.keys())
    return self.create_stories_from_names(names, separate)

  def create_stories_from_names(
      self, names: List[str], separate: bool) -> Sequence[PressBenchmarkStoryT]:
    return self.story_cls.from_names(names, separate=separate, url=self.url)


class PressBenchmark(SubStoryBenchmark):
  STORY_FILTER_CLS = PressBenchmarkStoryFilter
  DEFAULT_STORY_CLS: Type[PressBenchmarkStory] = PressBenchmarkStory

  @classmethod
  @abc.abstractmethod
  def short_base_name(cls) -> str:
    raise NotImplementedError()

  @classmethod
  @abc.abstractmethod
  def base_name(cls) -> str:
    raise NotImplementedError()

  @classmethod
  @abc.abstractmethod
  def version(cls) -> Tuple[int, ...]:
    raise NotImplementedError()

  @classmethod
  def aliases(cls) -> Tuple[str, ...]:
    version = [str(v) for v in cls.version()]
    assert version, "Expected non-empty version tuple."
    version_names = []
    dot_version = ".".join(version)
    for name in (cls.short_base_name(), cls.base_name()):
      assert name, "Expected non-empty base name."
      version_names.append(f"{name}{dot_version}")
      version_names.append(f"{name}_{dot_version}")
    return tuple(version_names)

  @classmethod
  def add_cli_parser(
      cls, subparsers, aliases: Sequence[str] = ()
  ) -> cli_helper.CrossBenchArgumentParser:
    parser = super().add_cli_parser(subparsers, aliases)
    # TODO: Move story-related args to dedicated PressBenchmarkStoryFilter class
    benchmark_url_group = parser.add_mutually_exclusive_group()
    default_live_url = cls.DEFAULT_STORY_CLS.URL
    default_local_url = cls.DEFAULT_STORY_CLS.URL_LOCAL
    benchmark_url_group.add_argument(
        "--live",
        dest="custom_benchmark_url",
        const=None,
        action="store_const",
        help=f"Use live/online benchmark url ({default_live_url}).")
    benchmark_url_group.add_argument(
        "--local",
        "--custom-benchmark-url",
        type=cli_helper.parse_httpx_url_str,
        nargs="?",
        dest="custom_benchmark_url",
        const=default_local_url,
        help=(f"Use custom or locally (default={default_local_url}) "
              "hosted benchmark url."))
    cls.STORY_FILTER_CLS.add_cli_parser(parser)
    return parser

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    kwargs = super().kwargs_from_cli(args)
    kwargs["custom_url"] = args.custom_benchmark_url
    return kwargs

  @classmethod
  def describe(cls) -> Dict[str, Any]:
    data = super().describe()
    assert issubclass(cls.DEFAULT_STORY_CLS, PressBenchmarkStory)
    data["url"] = cls.DEFAULT_STORY_CLS.URL
    data["url-local"] = cls.DEFAULT_STORY_CLS.URL_LOCAL
    return data

  def __init__(self,
               stories: Sequence[Story],
               custom_url: Optional[str] = None):
    super().__init__(stories)
    self.custom_url = custom_url
    if custom_url:
      for story in stories:
        press_story = cast(PressBenchmarkStory, story)
        assert press_story.url == custom_url

  def setup(self, runner: Runner) -> None:
    super().setup(runner)
    self.validate_url(runner)

  def validate_url(self, runner: Runner) -> None:
    if self.custom_url:
      if runner.env.validate_url(self.custom_url):
        return
      raise ValueError(
          f"Could not reach custom benchmark URL: '{self.custom_url}'. "
          f"Please make sure your local web server is running.")
    first_story = cast(PressBenchmarkStory, self.stories[0])
    url = first_story.url
    if not url:
      raise ValueError("Invalid empty url")
    if not runner.env.validate_url(url):
      msg = [
          f"Could not reach live benchmark URL: '{url}'."
          f"Please make sure you're connected to the internet."
      ]
      local_url = first_story.URL_LOCAL
      if local_url:
        msg.append(
            f"Alternatively use --local for the default local URL: {local_url}")
      raise ValueError("\n".join(msg))
