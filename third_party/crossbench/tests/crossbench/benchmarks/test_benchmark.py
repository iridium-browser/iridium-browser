# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import datetime as dt

from crossbench.benchmarks.benchmark import PressBenchmarkStoryFilter
from crossbench.runner.run import Run
from crossbench.stories.press_benchmark import PressBenchmarkStory


class TestStory(PressBenchmarkStory):
  NAME = "TestStory"
  URL = "http://test.com"
  SUBSTORIES = (
      "Story-1",
      "Story-2",
      "Story-3",
      "Story-4",
  )

  @property
  def substory_duration(self) -> dt.timedelta:
    return dt.timedelta(seconds=0.1)

  def run(self, run: Run) -> None:
    pass


class PressBenchmarkStoryFilterTestCase(unittest.TestCase):

  def test_empty(self):
    with self.assertRaises(ValueError):
      _ = PressBenchmarkStoryFilter(TestStory, [])

  def test_all(self):
    stories = PressBenchmarkStoryFilter(TestStory, ["all"]).stories
    self.assertEqual(len(stories), 1)
    story: TestStory = stories[0]
    self.assertSequenceEqual(story.substories, TestStory.SUBSTORIES)

  def test_all_separate(self):
    stories = PressBenchmarkStoryFilter(
        TestStory, ["all"], separate=True).stories
    self.assertSequenceEqual([story.substories[0] for story in stories],
                             TestStory.SUBSTORIES)
    for story in stories:
      self.assertTrue(len(story.substories), 1)

  def test_match_regexp_none(self):
    with self.assertRaises(ValueError) as cm:
      _ = PressBenchmarkStoryFilter(TestStory, ["Story"]).stories
    self.assertIn("Story", str(cm.exception))

  def test_match_regexp_some(self):
    stories = PressBenchmarkStoryFilter(TestStory, [".*-3"]).stories
    self.assertEqual(len(stories), 1)
    story: TestStory = stories[0]
    self.assertSequenceEqual(story.substories, ["Story-3"])

  def test_match_regexp_all(self):
    stories = PressBenchmarkStoryFilter(TestStory, ["Story.*"]).stories
    self.assertEqual(len(stories), 1)
    story: TestStory = stories[0]
    self.assertSequenceEqual(story.substories, TestStory.SUBSTORIES)

  def test_match_regexp_all_wrong_case(self):
    stories = PressBenchmarkStoryFilter(TestStory, ["StOrY.*"]).stories
    self.assertEqual(len(stories), 1)
    story: TestStory = stories[0]
    self.assertSequenceEqual(story.substories, TestStory.SUBSTORIES)
