# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime as dt
import pathlib
import unittest

from crossbench import helper
from crossbench import plt
from tests import run_helper
from tests.crossbench.mock_helper import CrossbenchFakeFsTestCase


class WaitTestCase(unittest.TestCase):

  def test_invalid_wait_ranges(self):
    with self.assertRaises(AssertionError):
      helper.WaitRange(min=-1)
    with self.assertRaises(AssertionError):
      helper.WaitRange(timeout=0)
    with self.assertRaises(AssertionError):
      helper.WaitRange(factor=0.2)

  def test_range(self):
    durations = list(
        helper.WaitRange(min=1, max=16, factor=2, max_iterations=5))
    self.assertListEqual(durations, [
        dt.timedelta(seconds=1),
        dt.timedelta(seconds=2),
        dt.timedelta(seconds=4),
        dt.timedelta(seconds=8),
        dt.timedelta(seconds=16)
    ])

  def test_range_extended(self):
    durations = list(
        helper.WaitRange(min=1, max=16, factor=2, max_iterations=5 + 4))
    self.assertListEqual(
        durations,
        [
            dt.timedelta(seconds=1),
            dt.timedelta(seconds=2),
            dt.timedelta(seconds=4),
            dt.timedelta(seconds=8),
            dt.timedelta(seconds=16),
            # After 5 iterations the interval is no longer increased
            dt.timedelta(seconds=16),
            dt.timedelta(seconds=16),
            dt.timedelta(seconds=16),
            dt.timedelta(seconds=16)
        ])

  def test_wait_with_backoff(self):
    data = []
    delta = 0.0005
    for time_spent, time_left in helper.wait_with_backoff(
        helper.WaitRange(min=0.01, max=0.05)):
      data.append((time_spent, time_left))
      if len(data) == 2:
        break
      plt.PLATFORM.sleep(delta)
    self.assertEqual(len(data), 2)
    first_time_spent, first_time_left = data[0]
    second_time_spent, second_time_left = data[1]
    self.assertLessEqual(first_time_spent + delta, second_time_spent)
    self.assertGreaterEqual(first_time_left, second_time_left + delta)


class DurationsTestCase(unittest.TestCase):

  def test_single(self):
    durations = helper.Durations()
    self.assertTrue(len(durations) == 0)
    self.assertDictEqual(durations.to_json(), {})
    with durations.measure("a"):
      pass
    self.assertGreaterEqual(durations["a"].total_seconds(), 0)
    self.assertTrue(len(durations) == 1)

  def test_invalid_twice(self):
    durations = helper.Durations()
    with durations.measure("a"):
      pass
    with self.assertRaises(AssertionError):
      with durations.measure("a"):
        pass
    self.assertTrue(len(durations) == 1)
    self.assertListEqual(list(durations.to_json().keys()), ["a"])

  def test_multiple(self):
    durations = helper.Durations()
    for name in ["a", "b", "c"]:
      with durations.measure(name):
        pass
    self.assertEqual(len(durations), 3)
    self.assertListEqual(list(durations.to_json().keys()), ["a", "b", "c"])


class ChangeCWDTestCase(CrossbenchFakeFsTestCase):

  def test_basic(self):
    old_cwd = pathlib.Path.cwd()
    new_cwd = pathlib.Path("/foo/bar").absolute()
    new_cwd.mkdir(parents=True)
    with helper.ChangeCWD(new_cwd):
      self.assertNotEqual(old_cwd, pathlib.Path.cwd())
      self.assertEqual(new_cwd, pathlib.Path.cwd())
    self.assertEqual(old_cwd, pathlib.Path.cwd())
    self.assertNotEqual(new_cwd, pathlib.Path.cwd())


class FileSizeTestCase(CrossbenchFakeFsTestCase):

  def test_empty(self):
    test_file = pathlib.Path("test.txt")
    test_file.touch()
    size = helper.get_file_size(test_file)
    self.assertEqual(size, "0.00 B")

  def test_bytes(self):
    test_file = pathlib.Path("test.txt")
    self.fs.create_file(test_file, st_size=501)
    size = helper.get_file_size(test_file)
    self.assertEqual(size, "501.00 B")

  def test_kib(self):
    test_file = pathlib.Path("test.txt")
    self.fs.create_file(test_file, st_size=1024 * 2)
    size = helper.get_file_size(test_file)
    self.assertEqual(size, "2.00 KiB")


class GroupByTestCase(unittest.TestCase):

  def test_empty(self):
    grouped = helper.group_by([], key=str)
    self.assertDictEqual({}, grouped)

  def test_basic(self):
    grouped = helper.group_by([1, 1, 1, 2, 2, 3], key=str)
    self.assertListEqual(list(grouped.keys()), ["1", "2", "3"])
    self.assertDictEqual({"1": [1, 1, 1], "2": [2, 2], "3": [3]}, grouped)

  def test_basic_out_of_order(self):
    grouped = helper.group_by([2, 3, 2, 1, 1, 1], key=str)
    self.assertListEqual(list(grouped.keys()), ["1", "2", "3"])
    self.assertDictEqual({"1": [1, 1, 1], "2": [2, 2], "3": [3]}, grouped)

  def test_basic_input_order(self):
    grouped = helper.group_by([2, 3, 2, 1, 1, 1], key=str, sort_key=None)
    self.assertListEqual(list(grouped.keys()), ["2", "3", "1"])
    self.assertDictEqual({"1": [1, 1, 1], "2": [2, 2], "3": [3]}, grouped)

  def test_basic_custom_order(self):
    grouped = helper.group_by([2, 3, 2, 1, 1, 1],
                              key=str,
                              sort_key=lambda item: int(item[0]))
    self.assertListEqual(list(grouped.keys()), ["1", "2", "3"])
    self.assertDictEqual({"1": [1, 1, 1], "2": [2, 2], "3": [3]}, grouped)
    # Try reverse sorting
    grouped = helper.group_by([2, 3, 2, 1, 1, 1],
                              key=str,
                              sort_key=lambda item: -int(item[0]))
    self.assertListEqual(list(grouped.keys()), ["3", "2", "1"])
    self.assertDictEqual({"1": [1, 1, 1], "2": [2, 2], "3": [3]}, grouped)

  def test_custom_key(self):
    grouped = helper.group_by([1.1, 1.2, 1.3, 2.1, 2.2, 3.1], key=int)
    self.assertDictEqual({1: [1.1, 1.2, 1.3], 2: [2.1, 2.2], 3: [3.1]}, grouped)

  def test_custom_value(self):
    grouped = helper.group_by([1, 1, 1, 2, 2, 3],
                              key=str,
                              value=lambda x: x * 100)
    self.assertDictEqual({
        "1": [100, 100, 100],
        "2": [200, 200],
        "3": [300]
    }, grouped)

  def test_custom_group(self):
    grouped = helper.group_by([1, 1, 1, 2, 2, 3],
                              key=str,
                              group=lambda key: ["custom"])
    self.assertDictEqual(
        {
            "1": ["custom", 1, 1, 1],
            "2": ["custom", 2, 2],
            "3": ["custom", 3]
        }, grouped)

  def test_custom_group_out_of_order(self):
    grouped = helper.group_by([1, 1, 1, 2, 2, 3],
                              key=str,
                              group=lambda key: ["custom"])
    self.assertDictEqual(
        {
            "1": ["custom", 1, 1, 1],
            "2": ["custom", 2, 2],
            "3": ["custom", 3]
        }, grouped)


class ConcatFilesTestCase(CrossbenchFakeFsTestCase):

  def setUp(self):
    super().setUp()
    self.platform = plt.PLATFORM

  def test_single(self):
    input_file = pathlib.Path("input")
    self.fs.create_file(input_file, contents="input content")
    output = pathlib.Path("ouput")
    self.platform.concat_files([input_file], output)
    self.assertEqual(output.read_text(encoding="utf-8"), "input content")

  def test_multiple(self):
    input_a = pathlib.Path("input_1")
    self.fs.create_file(input_a, contents="AAA")
    input_b = pathlib.Path("input_2")
    self.fs.create_file(input_b, contents="BBB")
    output = pathlib.Path("ouput")
    self.platform.concat_files([input_a, input_b], output)
    self.assertEqual(output.read_text(encoding="utf-8"), "AAABBB")


class EnumWithHelpTestCase(unittest.TestCase):

  class TestEnum(helper.EnumWithHelp):
    A = ("a", "help a")
    B = ("b", "help b")

  def test_lookup(self):
    self.assertIs(self.TestEnum("a"), self.TestEnum.A)
    self.assertIs(self.TestEnum("b"), self.TestEnum.B)
    self.assertIs(self.TestEnum["A"], self.TestEnum.A)
    self.assertIs(self.TestEnum["B"], self.TestEnum.B)

  def test_value_help(self):
    # pylint: disable=no-member
    self.assertEqual(self.TestEnum.A.name, "A")
    self.assertEqual(self.TestEnum.B.name, "B")
    self.assertEqual(self.TestEnum.A.value, "a")
    self.assertEqual(self.TestEnum.B.value, "b")
    self.assertEqual(self.TestEnum.A.help, "help a")
    self.assertEqual(self.TestEnum.B.help, "help b")

  def test_in(self):
    self.assertIn(self.TestEnum.A, self.TestEnum)
    self.assertIn(self.TestEnum.B, self.TestEnum)

  def test_str(self):
    self.assertEqual(str(self.TestEnum.A), "TestEnum.A")
    self.assertEqual(str(self.TestEnum.B), "TestEnum.B")

  def test_list(self):
    self.assertEqual(len(self.TestEnum), 2)
    self.assertListEqual(
        list(self.TestEnum), [self.TestEnum.A, self.TestEnum.B])

  def test_help_items(self):
    self.assertListEqual(self.TestEnum.help_text_items(), [("'a'", "help a"),
                                                           ("'b'", "help b")])


class StrEnumWithHelpTestCase(EnumWithHelpTestCase):

  class TestEnum(helper.StrEnumWithHelp):
    A = ("a", "help a")
    B = ("b", "help b")

  def test_str(self):
    self.assertEqual(str(self.TestEnum.A), "a")
    self.assertEqual(str(self.TestEnum.B), "b")


class UpdateUrlQueryTestCase(unittest.TestCase):

  def test_empty(self):
    self.assertEqual("http://test.com",
                     helper.update_url_query("http://test.com", {}))
    self.assertEqual("https://test.com",
                     helper.update_url_query("https://test.com", {}))
    self.assertEqual("https://test.com?foo=bar",
                     helper.update_url_query("https://test.com?foo=bar", {}))

  def test_empty_add(self):
    self.assertEqual("http://test.com?foo=bar",
                     helper.update_url_query("http://test.com", {"foo": "bar"}))
    self.assertEqual(
        "http://test.com?foo=bar#status",
        helper.update_url_query("http://test.com#status", {"foo": "bar"}))
    self.assertEqual(
        "http://test.com?xyz=10&foo=bar#status",
        helper.update_url_query("http://test.com?xyz=10#status",
                                {"foo": "bar"}))

  def test_override(self):
    self.assertEqual(
        "http://test.com?foo=bar",
        helper.update_url_query("http://test.com?foo=BAR", {"foo": "bar"}))
    self.assertEqual(
        "http://test.com?foo=bar#status",
        helper.update_url_query("http://test.com?foo=BAR#status",
                                {"foo": "bar"}))
    self.assertEqual(
        "http://test.com?foo=bar&xyz=10#status",
        helper.update_url_query("http://test.com?foo=BAR&xyz=10#status",
                                {"foo": "bar"}))

if __name__ == "__main__":
  run_helper.run_pytest(__file__)
