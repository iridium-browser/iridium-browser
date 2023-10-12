# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pytype: disable=attribute-error

from __future__ import annotations

import argparse
import datetime as dt
import pathlib
import unittest
from typing import Sequence, cast

import hjson

import crossbench
import crossbench.env
import crossbench.runner
from crossbench.benchmarks.loading.loading_benchmark import LoadingPageFilter, PageLoadBenchmark
from crossbench.benchmarks.loading.page import (PAGE_LIST, PAGE_LIST_SMALL,
                                                CombinedPage, InteractivePage,
                                                LivePage)
from crossbench.benchmarks.loading.page_config import PageConfig
from crossbench.benchmarks.loading.playback_controller import (
    PlaybackController, RepeatPlaybackController, TimeoutPlaybackController)
from crossbench.runner.runner import Runner
from crossbench.stories.story import Story
from tests import run_helper
from tests.crossbench.benchmarks import helper
from tests.crossbench.mock_helper import CrossbenchFakeFsTestCase

cb = crossbench


class PlaybackControllerTest(unittest.TestCase):

  def test_parse_invalid(self):
    for invalid in [
        "11", "something", "1.5x", "4.3.h", "4.5.x", "-1x", "-1.4x", "-2h",
        "-2.1h", "1h30", "infx", "infh", "nanh", "nanx"
    ]:
      with self.subTest(pattern=invalid):
        with self.assertRaises((argparse.ArgumentTypeError, ValueError)):
          PlaybackController.parse(invalid)

  def test_parse_repeat(self):
    playback = PlaybackController.parse("once")
    self.assertIsInstance(playback, RepeatPlaybackController)
    assert isinstance(playback, RepeatPlaybackController)
    self.assertEqual(playback.count, 1)
    self.assertEqual(len(list(playback)), 1)

    playback = PlaybackController.parse("1x")
    self.assertIsInstance(playback, RepeatPlaybackController)
    assert isinstance(playback, RepeatPlaybackController)
    self.assertEqual(playback.count, 1)
    self.assertEqual(len(list(playback)), 1)

    playback = PlaybackController.parse("11x")
    self.assertIsInstance(playback, RepeatPlaybackController)
    assert isinstance(playback, RepeatPlaybackController)
    self.assertEqual(playback.count, 11)
    self.assertEqual(len(list(playback)), 11)

  def test_parse_forever(self):
    playback = PlaybackController.parse("forever")
    self.assertIsInstance(playback, PlaybackController)
    playback = PlaybackController.parse("inf")
    self.assertIsInstance(playback, PlaybackController)
    playback = PlaybackController.parse("infinity")
    self.assertIsInstance(playback, PlaybackController)

  def test_parse_duration(self):
    playback = PlaybackController.parse("5s")
    self.assertIsInstance(playback, TimeoutPlaybackController)
    assert isinstance(playback, TimeoutPlaybackController)
    self.assertEqual(playback.duration, dt.timedelta(seconds=5))

    playback = PlaybackController.parse("5m")
    self.assertIsInstance(playback, TimeoutPlaybackController)
    assert isinstance(playback, TimeoutPlaybackController)
    self.assertEqual(playback.duration, dt.timedelta(minutes=5))

    playback = PlaybackController.parse("5.5m")
    self.assertIsInstance(playback, TimeoutPlaybackController)
    assert isinstance(playback, TimeoutPlaybackController)
    self.assertEqual(playback.duration, dt.timedelta(minutes=5.5))

    playback = PlaybackController.parse("5.5m")
    self.assertIsInstance(playback, TimeoutPlaybackController)
    assert isinstance(playback, TimeoutPlaybackController)
    self.assertEqual(playback.duration, dt.timedelta(minutes=5.5))

class TestPageLoadBenchmark(helper.SubStoryTestCase):

  @property
  def benchmark_cls(self):
    return PageLoadBenchmark

  def story_filter(self, patterns: Sequence[str],
                   **kwargs) -> LoadingPageFilter:
    return cast(LoadingPageFilter, super().story_filter(patterns, **kwargs))

  def test_all_stories(self):
    stories = self.story_filter(["all"]).stories
    self.assertGreater(len(stories), 1)
    for story in stories:
      self.assertIsInstance(story, LivePage)
    names = set(story.name for story in stories)
    self.assertEqual(len(names), len(stories))
    self.assertEqual(names, set(page.name for page in PAGE_LIST))

  def test_default_stories(self):
    stories = self.story_filter(["default"]).stories
    self.assertGreater(len(stories), 1)
    for story in stories:
      self.assertIsInstance(story, LivePage)
    names = set(story.name for story in stories)
    self.assertEqual(len(names), len(stories))
    self.assertEqual(names, set(page.name for page in PAGE_LIST_SMALL))

  def test_combined_stories(self):
    stories = self.story_filter(["all"], separate=False).stories
    self.assertEqual(len(stories), 1)
    combined = stories[0]
    self.assertIsInstance(combined, CombinedPage)

  def test_filter_by_name(self):
    for page in PAGE_LIST:
      stories = self.story_filter([page.name]).stories
      self.assertListEqual([p.url for p in stories], [page.url])
    self.assertListEqual(self.story_filter([]).stories, [])

  def test_filter_by_name_with_duration(self):
    pages = PAGE_LIST
    filtered_pages = self.story_filter([pages[0].name, pages[1].name,
                                        "1001"]).stories
    self.assertListEqual([p.url for p in filtered_pages],
                         [pages[0].url, pages[1].url])
    self.assertEqual(filtered_pages[0].duration, pages[0].duration)
    self.assertEqual(filtered_pages[1].duration, dt.timedelta(seconds=1001))

  def test_page_by_url(self):
    url1 = "http://example.com/test1"
    url2 = "http://example.com/test2"
    stories = self.story_filter([url1, url2]).stories
    self.assertEqual(len(stories), 2)
    self.assertEqual(stories[0].url, url1)
    self.assertEqual(stories[1].url, url2)

  def test_page_by_url_www(self):
    url1 = "www.example.com/test1"
    url2 = "www.example.com/test2"
    stories = self.story_filter([url1, url2]).stories
    self.assertEqual(len(stories), 2)
    self.assertEqual(stories[0].url, f"https://{url1}")
    self.assertEqual(stories[1].url, f"https://{url2}")

  def test_page_by_url_combined(self):
    url1 = "http://example.com/test1"
    url2 = "http://example.com/test2"
    stories = self.story_filter([url1, url2], separate=False).stories
    self.assertEqual(len(stories), 1)
    combined = stories[0]
    self.assertIsInstance(combined, CombinedPage)

  def test_run_combined(self):
    stories = [
        CombinedPage(PAGE_LIST),
    ]
    self._test_run(stories)
    self._assert_urls_loaded([story.url for story in PAGE_LIST])

  def test_run_default(self):
    stories = PAGE_LIST
    self._test_run(stories)
    self._assert_urls_loaded([story.url for story in stories])

  def test_run_throw(self):
    stories = PAGE_LIST
    self._test_run(stories)
    self._assert_urls_loaded([story.url for story in stories])

  def test_run_repeat(self):
    url1 = "https://www.example.com/test1"
    url2 = "https://www.example.com/test2"
    stories = self.story_filter([url1, url2],
                                separate=False,
                                playback=PlaybackController.repeat(3)).stories
    self._test_run(stories)
    urls = [url1, url2] * 3
    self._assert_urls_loaded(urls)

  def test_run_repeat_separate(self):
    url1 = "https://www.example.com/test1"
    url2 = "https://www.example.com/test2"
    stories = self.story_filter([url1, url2],
                                separate=True,
                                playback=PlaybackController.repeat(3)).stories
    self._test_run(stories)
    urls = [url1] * 3 + [url2] * 3
    self._assert_urls_loaded(urls)

  def _test_run(self, stories, throw: bool = False):
    benchmark = self.benchmark_cls(stories)
    self.assertTrue(len(benchmark.describe()) > 0)
    runner = Runner(
        self.out_dir,
        self.browsers,
        benchmark,
        env_config=cb.env.HostEnvironmentConfig(),
        env_validation_mode=cb.env.ValidationMode.SKIP,
        platform=self.platform,
        throw=throw)
    runner.run()
    self.assertTrue(runner.is_success)
    self.assertTrue(self.browsers[0].did_run)
    self.assertTrue(self.browsers[1].did_run)

  def _assert_urls_loaded(self, story_urls):
    browser_1_urls = self.filter_data_urls(self.browsers[0].url_list)
    self.assertEqual(browser_1_urls, story_urls)
    browser_2_urls = self.filter_data_urls(self.browsers[1].url_list)
    self.assertEqual(browser_2_urls, story_urls)


class TestExamplePageConfig(unittest.TestCase):

  @unittest.skipIf(hjson.__name__ != "hjson", "hjson not available")
  def test_parse_example_page_config_file(self):
    example_config_file = pathlib.Path(
        __file__).parents[3] / "config" / "page.config.example.hjson"
    with example_config_file.open(encoding="utf-8") as f:
      file_config = PageConfig()
      file_config.load(f)
    with example_config_file.open(encoding="utf-8") as f:
      data = hjson.load(f)
    dict_config = PageConfig()
    dict_config.load_dict(data)
    self.assertTrue(dict_config.stories)
    self.assertTrue(file_config.stories)
    for story in dict_config.stories:
      json_data = story.details_json()
      self.assertIn("actions", json_data)


class TestPageConfig(CrossbenchFakeFsTestCase):

  def test_example(self):
    config_data = {
        "pages": {
            "Google Story": [
                {
                    "action": "get",
                    "url": "https://www.google.com"
                },
                {
                    "action": "wait",
                    "duration": 5
                },
                {
                    "action": "scroll",
                    "direction": "down",
                    "duration": 3
                },
            ],
        }
    }
    config = PageConfig()
    config.load_dict(config_data)
    self.assert_single_google_story(config.stories)
    # Loading the same config from a file should result in the same actions.
    file = pathlib.Path("page.config.hjson")
    assert not file.exists()
    with file.open("w", encoding="utf-8") as f:
      hjson.dump(config_data, f)
    stories = PageConfig.parse(str(file))
    self.assert_single_google_story(stories)

  def assert_single_google_story(self, stories: Sequence[Story]):
    self.assertTrue(len(stories), 1)
    story = stories[0]
    assert isinstance(story, InteractivePage)
    self.assertEqual(story.name, "Google Story")
    self.assertListEqual([action.TYPE for action in story.actions],
                         ["get", "wait", "scroll"])

  def test_no_scenarios(self):
    with self.assertRaises(argparse.ArgumentTypeError):
      PageConfig().load_dict({})
    with self.assertRaises(argparse.ArgumentTypeError):
      PageConfig().load_dict({"pages": {}})

  def test_scenario_invalid_actions(self):
    invalid_actions = [None, "", [], {}, "invalid string", 12]
    for invalid_action in invalid_actions:
      config_dict = {"pages": {"name": invalid_action}}
      with self.subTest(invalid_action=invalid_action):
        with self.assertRaises(argparse.ArgumentTypeError):
          PageConfig().load_dict(config_dict)

  def test_missing_action(self):
    with self.assertRaises(argparse.ArgumentTypeError) as cm:
      PageConfig().load_dict(
          {"pages": {
              "TEST": [{
                  "action___": "wait",
                  "duration": 5.0
              }]
          }})
    self.assertIn("Missing 'action'", str(cm.exception))

  def test_invalid_action(self):
    invalid_actions = [None, "", [], {}, "unknown action name", 12]
    for invalid_action in invalid_actions:
      config_dict = {
          "pages": {
              "TEST": [{
                  "action": invalid_action,
                  "duration": 5.0
              }]
          }
      }
      with self.subTest(invalid_action=invalid_action):
        with self.assertRaises(argparse.ArgumentTypeError):
          PageConfig().load_dict(config_dict)

  def test_missing_get_action_scenario(self):
    with self.assertRaises(argparse.ArgumentTypeError):
      PageConfig().load_dict(
          {"pages": {
              "TEST": [{
                  "action": "wait",
                  "duration": 5.0
              }]
          }})

  def test_get_action_durations(self):
    durations = [
        ("5", 5),
        ("5.5", 5.5),
        (6, 6),
        (6.1, 6.1),
        ("5.5", 5.5),
        ("170ms", 0.17),
        ("170milliseconds", 0.17),
        ("170.4ms", 0.1704),
        ("170.4 millis", 0.1704),
        ("8s", 8),
        ("8.1s", 8.1),
        ("8.1seconds", 8.1),
        ("1 second", 1),
        ("1.1 seconds", 1.1),
        ("9m", 9 * 60),
        ("9.5m", 9.5 * 60),
        ("9.5 minutes", 9.5 * 60),
        ("9.5 mins", 9.5 * 60),
        ("1 minute", 60),
        ("1 min", 60),
        ("1h", 3600),
        ("1 h", 3600),
        ("1 hour", 3600),
        ("0.5h", 1800),
        ("0.5 hours", 1800),
    ]
    for input_value, duration in durations:
      with self.subTest(duration=duration):
        page_config = PageConfig()
        page_config.load_dict({
            "pages": {
                "TEST": [
                    {
                        "action": "get",
                        "url": 'google.com'
                    },
                    {
                        "action": "wait",
                        "duration": input_value
                    },
                ]
            }
        })
        self.assertEqual(len(page_config.stories), 1)
        story = page_config.stories[0]
        assert isinstance(story, InteractivePage)
        self.assertEqual(len(story.actions), 2)
        self.assertEqual(story.actions[1].duration,
                         dt.timedelta(seconds=duration))

  def test_action_invalid_duration(self):
    invalid_durations = [
        "1.1.1",
        None,
        "",
        -1,
        "-1",
        "-1ms",
        "1msss",
        "1ss",
        "2hh",
        "asdfasd",
        "---",
        "1.1.1",
        "1_123ms",
        "1'200h",
        (),
        [],
        {},
        "-1h",
    ]
    for invalid_duration in invalid_durations:
      with self.subTest(duration=invalid_duration), self.assertRaises(
          (AssertionError, ValueError, argparse.ArgumentTypeError)):
        PageConfig().load_dict({
            "pages": {
                "TEST": [
                    {
                        "action": "get",
                        "url": 'google.com'
                    },
                    {
                        "action": "wait",
                        "duration": invalid_duration
                    },
                ]
            }
        })


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
