# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import pathlib
import unittest
import datetime as dt
import pyfakefs.fake_filesystem_unittest

from crossbench import helper

import sys
import pytest


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
      helper.platform.sleep(delta)
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


class ChangeCWDTestCase(pyfakefs.fake_filesystem_unittest.TestCase):

  def setUp(self):
    self.setUpPyfakefs()

  def test_basic(self):
    old_cwd = pathlib.Path.cwd()
    new_cwd = pathlib.Path("/foo/bar").absolute()
    new_cwd.mkdir(parents=True)
    with helper.ChangeCWD(new_cwd):
      self.assertNotEqual(old_cwd, pathlib.Path.cwd())
      self.assertEqual(new_cwd, pathlib.Path.cwd())
    self.assertEqual(old_cwd, pathlib.Path.cwd())
    self.assertNotEqual(new_cwd, pathlib.Path.cwd())


class FileSizeTestCase(pyfakefs.fake_filesystem_unittest.TestCase):

  def setUp(self):
    self.setUpPyfakefs()

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


class ConcatFilesTestCase(pyfakefs.fake_filesystem_unittest.TestCase):

  def setUp(self):
    self.setUpPyfakefs()
    self.platform = helper.platform

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


class FormatMetricTestCase(unittest.TestCase):

  def test_no_stdev(self):
    self.assertEqual(helper.format_metric(100), "100")
    self.assertEqual(helper.format_metric(0), "0")
    self.assertEqual(helper.format_metric(1.5), "1.5")
    self.assertEqual(helper.format_metric(100, 0), "100")
    self.assertEqual(helper.format_metric(0, 0), "0")
    self.assertEqual(helper.format_metric(1.5, 0), "1.5")

  def test_stdev(self):
    self.assertEqual(helper.format_metric(100, 10), "100 ± 10%")
    self.assertEqual(helper.format_metric(100, 1), "100.0 ± 1.0%")
    self.assertEqual(helper.format_metric(100, 1.5), "100.0 ± 1.5%")
    self.assertEqual(helper.format_metric(100, 0.1), "100.00 ± 0.10%")
    self.assertEqual(helper.format_metric(100, 0.12), "100.00 ± 0.12%")
    self.assertEqual(helper.format_metric(100, 0.125), "100.00 ± 0.12%")

  def test_round_stdev(self):
    value = 100.123456789
    percent = value / 100
    self.assertEqual(
        helper.format_metric(value, percent * 10.1234), "100 ± 10%")
    self.assertEqual(
        helper.format_metric(value, percent * 1.2345), "100.1 ± 1.2%")
    self.assertEqual(
        helper.format_metric(value, percent * 0.12345), "100.12 ± 0.12%")
    self.assertEqual(
        helper.format_metric(value, percent * 0.012345), "100.123 ± 0.012%")
    self.assertEqual(
        helper.format_metric(value, percent * 0.0012345), "100.1235 ± 0.0012%")
    self.assertEqual(
        helper.format_metric(value, percent * 0.00012345),
        "100.12346 ± 0.00012%")


class PlatformTestCase(unittest.TestCase):

  def setUp(self):
    self.platform = helper.platform

  def test_sleep(self):
    self.platform.sleep(0)
    self.platform.sleep(0.01)
    self.platform.sleep(dt.timedelta())
    self.platform.sleep(dt.timedelta(seconds=0.1))

  def test_cpu_details(self):
    details = self.platform.cpu_details()
    self.assertLess(0, details["physical cores"])

  def test_get_relative_cpu_speed(self):
    self.assertGreater(self.platform.get_relative_cpu_speed(), 0)

  def test_is_thermal_throttled(self):
    self.assertIsInstance(self.platform.is_thermal_throttled(), bool)

  def test_is_battery_powered(self):
    self.assertIsInstance(self.platform.is_battery_powered, bool)

  def test_cpu_usage(self):
    self.assertGreaterEqual(self.platform.cpu_usage(), 0)

  def test_system_details(self):
    self.assertIsNotNone(self.platform.system_details())


@unittest.skipIf(not helper.platform.is_win, "Incompatible platform")
class WinxPlatformUnittest(unittest.TestCase):
  platform: helper.WinPlatform

  def setUp(self):
    super().setUp()
    assert isinstance(helper.platform, helper.WinPlatform)
    self.platform = helper.platform

  def test_sh(self):
    ls = self.platform.sh_stdout("ls")
    self.assertTrue(ls)

  def test_search_binary(self):
    with self.assertRaises(ValueError):
      self.platform.search_binary(pathlib.Path("does not exist"))
    path = self.platform.search_binary(
        pathlib.Path("Windows NT/Accessories/wordpad.exe"))
    self.assertTrue(path and path.exists())

  def test_app_version(self):
    path = self.platform.search_binary(
        pathlib.Path("Windows NT/Accessories/wordpad.exe"))
    self.assertTrue(path and path.exists())
    version = self.platform.app_version(path)
    self.assertIsNotNone(version)

  def test_is_macos(self):
    self.assertFalse(self.platform.is_macos)
    self.assertFalse(self.platform.is_linux)
    self.assertTrue(self.platform.is_win)
    self.assertFalse(self.platform.is_remote)

@unittest.skipIf(not helper.platform.is_posix, "Incompatible platform")
class PosixPlatformUnittest(unittest.TestCase):

  def setUp(self):
    super().setUp()
    self.platform: helper.PosixPlatform = helper.platform

  def test_sh(self):
    ls = self.platform.sh_stdout("ls")
    self.assertTrue(ls)
    lsa = self.platform.sh_stdout("ls", "-a")
    self.assertTrue(lsa)
    self.assertNotEqual(ls, lsa)

  def test_which(self):
    ls_bin = self.platform.which("ls")
    bash_bin = self.platform.which("bash")
    self.assertNotEqual(ls_bin, bash_bin)
    self.assertTrue(pathlib.Path(ls_bin).exists())
    self.assertTrue(pathlib.Path(bash_bin).exists())

  def test_system_details(self):
    details = self.platform.system_details()
    self.assertTrue(details)


@unittest.skipIf(not helper.platform.is_macos, "Incompatible platform")
class MacOSPlatformHelperTestCase(unittest.TestCase):

  def setUp(self):
    super().setUp()
    self.platform = helper.platform

  def test_search_binary_not_found(self):
    with self.assertRaises(ValueError):
      self.platform.search_binary(pathlib.Path("Invalid App Name"))
    binary = self.platform.search_binary(pathlib.Path("Non-existent App.app"))
    self.assertIsNone(binary)

  def test_search_binary(self):
    binary = self.platform.search_binary(pathlib.Path("Safari.app"))
    self.assertTrue(binary and binary.is_file())

  def test_search_app(self):
    binary = self.platform.search_app(pathlib.Path("Safari.app"))
    self.assertTrue(binary and binary.exists())
    self.assertTrue(binary and binary.is_dir())

  def test_name(self):
    self.assertEqual(self.platform.name, "macos")

  def test_is_macos(self):
    self.assertTrue(self.platform.is_macos)
    self.assertFalse(self.platform.is_linux)
    self.assertFalse(self.platform.is_win)
    self.assertFalse(self.platform.is_remote)

  def test_set_main_screen_brightness(self):
    prev_level = helper.platform.get_main_display_brightness()
    brightness_level = 32
    helper.platform.set_main_display_brightness(brightness_level)
    self.assertEqual(brightness_level,
                     helper.platform.get_main_display_brightness())
    helper.platform.set_main_display_brightness(prev_level)
    self.assertEqual(prev_level, helper.platform.get_main_display_brightness())


if __name__ == "__main__":
  sys.exit(pytest.main([__file__]))
