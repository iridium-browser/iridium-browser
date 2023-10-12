# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import abc
from typing import List, Sequence, Type

from crossbench.benchmarks import benchmark
from tests.crossbench.mock_helper import BaseCrossbenchTestCase


class BaseBenchmarkTestCase(BaseCrossbenchTestCase, metaclass=abc.ABCMeta):

  @property
  @abc.abstractmethod
  def benchmark_cls(self):
    pass

  @property
  def story_cls(self):
    return self.benchmark_cls.DEFAULT_STORY_CLS

  def setUp(self):
    super().setUp()
    self.assertTrue(
        issubclass(self.benchmark_cls, benchmark.Benchmark),
        f"Expected Benchmark subclass, but got: BENCHMARK={self.benchmark_cls}")

  def test_instantiate_no_stories(self):
    with self.assertRaises(AssertionError):
      self.benchmark_cls(stories=[])
    with self.assertRaises(AssertionError):
      self.benchmark_cls(stories="")
    with self.assertRaises(AssertionError):
      self.benchmark_cls(stories=["", ""])

  def test_describe(self):
    self.assertIsInstance(self.benchmark_cls.describe(), dict)


class SubStoryTestCase(BaseBenchmarkTestCase, metaclass=abc.ABCMeta):

  @property
  def story_filter_cls(self) -> Type[benchmark.StoryFilter]:
    return self.benchmark_cls.STORY_FILTER_CLS

  def story_filter(self, patterns: Sequence[str],
                   **kwargs) -> benchmark.StoryFilter:
    return self.story_filter_cls(  # pytype: disable=not-instantiable
        story_cls=self.story_cls,
        patterns=patterns,
        **kwargs)


  def test_stories_creation(self):
    for name in self.story_cls.all_story_names():
      stories = self.story_filter([name]).stories
      self.assertTrue(len(stories) == 1)
      story = stories[0]
      self.assertIsInstance(story, self.story_cls)
      self.assertIsInstance(story.details_json(), dict)
      self.assertTrue(len(str(story)) > 0)

  def test_instantiate_single_story(self):
    any_story_name = self.story_cls.all_story_names()[0]
    any_story = self.story_filter([any_story_name]).stories[0]
    # Instantiate with single story,
    with self.assertRaises(Exception):
      self.benchmark_cls(any_story)
    # with single story array
    self.benchmark_cls([any_story])
    with self.assertRaises(AssertionError):
      # Accidentally nested array.
      self.benchmark_cls([[any_story]])

  def test_instantiate_all_stories(self):
    stories = self.story_filter(self.story_cls.all_story_names()).stories
    self.benchmark_cls(stories)


class PressBaseBenchmarkTestCase(SubStoryTestCase, metaclass=abc.ABCMeta):

  def test_invalid_story_names(self):
    # Only StoryFilter can filter stories by regexp
    with self.assertRaises(Exception):
      self.story_cls.from_names(".*", separate=True)
    with self.assertRaises(Exception):
      self.story_cls.from_names([".*"], separate=True)
    with self.assertRaises(Exception):
      self.story_cls.from_names([".*", "name does not exist"], separate=True)
    with self.assertRaises(Exception):
      self.story_cls.from_names([""], separate=True)

  def test_all(self):
    all_stories = [story.name for story in self.story_cls.all(separate=True)]
    all_regexp = [
        story.name for story in self.story_filter([".*"], separate=True).stories
    ]
    all_string = [
        story.name
        for story in self.story_filter(["all"], separate=True).stories
    ]
    self.assertListEqual(all_stories, all_regexp)
    self.assertListEqual(all_stories, all_string)

  def test_default(self):
    default_stories = [
        story.name for story in self.story_cls.default(separate=True)
    ]
    default_string = [
        story.name
        for story in self.story_filter(["default"], separate=True).stories
    ]
    self.assertListEqual(default_stories, default_string)

  def test_remove(self):
    assert len(self.story_cls.all_story_names()) > 1
    story_name = self.story_cls.all_story_names()[0]
    all_stories = [story.name for story in self.story_cls.all(separate=True)]
    filtered_stories = [
        story.name for story in self.story_filter([".*", f"-{story_name}"],
                                                  separate=True).stories
    ]
    self.assertEqual(len(filtered_stories) + 1, len(all_stories))
    for name in filtered_stories:
      self.assertIn(name, all_stories)

  def test_remove_invalid(self):
    assert len(self.story_cls.all_story_names()) > 1
    story_name = self.story_cls.all_story_names()[0]
    with self.assertRaises(ValueError):
      self.story_filter(["-"])
    with self.assertRaises(ValueError):
      self.story_filter(["--"])
    with self.assertRaises(ValueError):
      self.story_filter(["-.*"])
    with self.assertRaises(ValueError):
      self.story_filter(["-all"])
    with self.assertRaises(ValueError):
      self.story_filter(["-does not exist name"])
    with self.assertRaises(ValueError):
      self.story_filter([f"-{story_name}"])

  def test_invalid_remove_all(self):
    assert len(self.story_cls.all_story_names()) > 1
    story_name = self.story_cls.all_story_names()[0]
    with self.assertRaises(ValueError):
      self.story_filter([story_name, f"-{story_name}"])
    with self.assertRaises(ValueError):
      self.story_filter([story_name, "-[^ ]+"])

  def test_invalid_add_all(self):
    assert len(self.story_cls.all_story_names()) > 1
    story_name = self.story_cls.all_story_names()[0]
    with self.assertRaises(ValueError):
      # Add all stories again after filtering out some
      self.story_filter([".*", f"-{story_name}", ".*|[^ ]+"])

  def test_remove_non_existent(self):
    assert len(self.story_cls.all_story_names()) > 1
    story_name = self.story_cls.all_story_names()[0]
    other_story_name = self.story_cls.all_story_names()[1]
    with self.assertRaises(ValueError):
      self.story_filter([other_story_name, f"-{story_name}"])
