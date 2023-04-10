# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pytype: disable=attribute-error

from __future__ import annotations
import json

import sys
import unittest
import pathlib
from unittest import mock
import hjson
import pyfakefs.fake_filesystem_unittest
from typing import Sequence, cast

import pytest

import crossbench
import crossbench.env
import crossbench.runner
from crossbench.benchmarks import loading
from tests.benchmarks import helper

#TODO: fix imports
cb = crossbench

class TestPageLoadBenchmark(helper.SubStoryTestCase):

  @property
  def benchmark_cls(self):
    return loading.PageLoadBenchmark

  def story_filter(self, patterns: Sequence[str],
                   **kwargs) -> loading.LoadingPageFilter:
    return cast(loading.LoadingPageFilter,
                super().story_filter(patterns, **kwargs))

  def test_all_stories(self):
    stories = self.story_filter(["all"]).stories
    self.assertGreater(len(stories), 1)
    for story in stories:
      self.assertIsInstance(story, loading.LivePage)
    names = set(story.name for story in stories)
    self.assertEqual(len(names), len(stories))
    self.assertEqual(names, set(page.name for page in loading.PAGE_LIST))

  def test_default_stories(self):
    stories = self.story_filter(["default"]).stories
    self.assertGreater(len(stories), 1)
    for story in stories:
      self.assertIsInstance(story, loading.LivePage)
    names = set(story.name for story in stories)
    self.assertEqual(len(names), len(stories))
    self.assertEqual(names, set(page.name for page in loading.PAGE_LIST_SMALL))

  def test_combined_stories(self):
    stories = self.story_filter(["all"], separate=False).stories
    self.assertEqual(len(stories), 1)
    combined = stories[0]
    self.assertIsInstance(combined, loading.CombinedPage)

  def test_filter_by_name(self):
    for page in loading.PAGE_LIST:
      stories = self.story_filter([page.name]).stories
      self.assertListEqual(stories, [page])
    self.assertListEqual(self.story_filter([]).stories, [])

  def test_filter_by_name_with_duration(self):
    pages = loading.PAGE_LIST
    filtered_pages = self.story_filter([pages[0].name, pages[1].name,
                                        "1001"]).stories
    self.assertListEqual(filtered_pages, [pages[0], pages[1]])
    self.assertEqual(filtered_pages[0].duration, pages[0].duration)
    self.assertEqual(filtered_pages[1].duration, 1001)

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
    self.assertIsInstance(combined, loading.CombinedPage)

  def test_run_combined(self):
    stories = [
        loading.CombinedPage(loading.PAGE_LIST),
    ]
    self._test_run(stories)
    self._assert_urls_loaded([story.url for story in loading.PAGE_LIST])

  def test_run_default(self):
    stories = loading.PAGE_LIST
    self._test_run(stories)
    self._assert_urls_loaded([story.url for story in stories])

  def test_run_throw(self):
    stories = loading.PAGE_LIST
    self._test_run(stories, throw=True)
    self._assert_urls_loaded([story.url for story in stories])

  def _test_run(self, stories, throw: bool = False):
    benchmark = self.benchmark_cls(stories)
    self.assertTrue(len(benchmark.describe()) > 0)
    runner = cb.runner.Runner(
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


class TestPageConfig(pyfakefs.fake_filesystem_unittest.TestCase):

  def setUp(self):
    # TODO: Move to separate common helper class
    self.setUpPyfakefs(modules_to_reload=[crossbench])

  @unittest.skipIf(hjson.__name__ != "hjson", "hjson not available")
  def test_parse_example_page_config_file(self):
    example_config_file = pathlib.Path(
        __file__).parents[2] / "config" / "page.config.example.hjson"
    if not example_config_file.exists():
      raise unittest.SkipTest(f"Test file {example_config_file} does not exist")
    with example_config_file.open(encoding="utf-8") as f:
      file_config = loading.PageConfig()
      file_config.load(f)
    with example_config_file.open(encoding="utf-8") as f:
      data = hjson.load(f)
    dict_config = loading.PageConfig()
    dict_config.load_dict(data)
    self.assertTrue(dict_config.stories)
    self.assertTrue(file_config.stories)

  def test_example(self):
    config_data = {
        "pages": {
            "Google Story": [
                {
                    "action": "get",
                    "value": "https://www.google.com"
                },
                {
                    "action": "wait",
                    "duration": 5
                },
                {
                    "action": "scroll",
                    "value": "down",
                    "duration": 3
                },
            ],
        }
    }
    config = loading.PageConfig()
    config.load_dict(config_data, throw=True)
    self.assert_single_google_story(config)
    # Loading the same config from a file should result in the same actions.
    file = pathlib.Path("page.config.hjson")
    assert not file.exists()
    with file.open("w", encoding="utf-8") as f:
      hjson.dump(config_data, f)
    args = mock.Mock(page_config=file, wraps=False)
    config = loading.PageConfig.from_cli_args(args)
    self.assert_single_google_story(config)

  def assert_single_google_story(self, config):
    self.assertTrue(len(config.stories), 1)
    story = config.stories[0]
    assert isinstance(story, loading.InteractivePage)
    self.assertEqual(story.name, "Google Story")
    self.assertListEqual([action.action_type for action in story.actions],
                         ["get", "wait", "scroll"])

  def test_no_scenarios(self):
    with self.assertRaises(ValueError):
      loading.PageConfig().load_dict({}, throw=True)
    with self.assertRaises(ValueError):
      loading.PageConfig().load_dict({"pages": {}}, throw=True)

  def test_scenario_invalid_actions(self):
    invalid_actions = [None, "", [], {}, "invalid string", 12]
    for invalid_action in invalid_actions:
      config_dict = {"pages": {"name": invalid_action}}
      with self.subTest(invalid_action=invalid_action):
        with self.assertRaises(ValueError):
          loading.PageConfig().load_dict(config_dict, throw=True)

  def test_missing_action(self):
    with self.assertRaises(ValueError):
      loading.PageConfig().load_dict(
          {"pages": {
              "TEST": [{
                  "action___": "wait",
                  "duration": 5.0
              }]
          }},
          throw=True)

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
        with self.assertRaises(ValueError):
          loading.PageConfig().load_dict(config_dict, throw=True)

  def test_missing_get_action_scenario(self):
    with self.assertRaises(AssertionError):
      loading.PageConfig().load_dict(
          {"pages": {
              "TEST": [{
                  "action": "wait",
                  "duration": 5.0
              }]
          }},
          throw=True)

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
        page_config = loading.PageConfig()
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
        assert isinstance(story, loading.InteractivePage)
        self.assertEqual(len(story.actions), 2)
        self.assertEqual(story.actions[1].duration, duration)

  def test_action_invalid_duration(self):
    invalid_durations = [
        "1.1.1", None, "", -1, "-1", "-1ms", "1msss", "1ss", "2hh", "asdfasd",
        "---", "1.1.1", "1_123ms", "1'200h", (), [], {}
    ]
    for invalid_duration in invalid_durations:
      with self.subTest(duration=invalid_duration), self.assertRaises(
          (AssertionError, ValueError)):
        loading.PageConfig().load_dict(
            {
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
            },
            throw=True)


if __name__ == "__main__":
  sys.exit(pytest.main([__file__]))
