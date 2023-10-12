# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
import pathlib
import tempfile
import unittest

from crossbench import compat, plt
from tests import run_helper


class PlatformTestCase(unittest.TestCase):

  def setUp(self):
    self.platform: plt.Platform = plt.PLATFORM

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
    self.assertEqual(
        self.platform.is_battery_powered,
        plt.PLATFORM.is_battery_powered,
    )

  def test_cpu_usage(self):
    self.assertGreaterEqual(self.platform.cpu_usage(), 0)

  def test_system_details(self):
    self.assertIsNotNone(self.platform.system_details())

  def test_environ(self):
    env = self.platform.environ
    self.assertTrue(env)

  def test_which_none(self):
    with self.assertRaises(ValueError):
      self.platform.which("")

  def test_which_invalid_binary(self):
    with tempfile.TemporaryDirectory() as tmp_dirname:
      self.assertIsNone(self.platform.which(tmp_dirname))

  def test_search_binary_empty_path(self):
    with self.assertRaises(ValueError) as cm:
      self.platform.search_binary(pathlib.Path())
    self.assertIn("empty", str(cm.exception))
    with self.assertRaises(ValueError) as cm:
      self.platform.search_binary(pathlib.Path(""))
    self.assertIn("empty", str(cm.exception))

  def test_search_app_empty_path(self):
    with self.assertRaises(ValueError) as cm:
      self.platform.search_app(pathlib.Path())
    self.assertIn("empty", str(cm.exception))
    with self.assertRaises(ValueError) as cm:
      self.platform.search_app(pathlib.Path(""))
    self.assertIn("empty", str(cm.exception))

  def test_cat(self):
    with tempfile.TemporaryDirectory() as tmp_dirname:
      file = pathlib.Path(tmp_dirname) / "test.txt"
      with file.open("w") as f:
        f.write("a b c d e f 11")
      result = self.platform.cat(file)
      self.assertEqual(result, "a b c d e f 11")

  def test_mkdir(self):
    with tempfile.TemporaryDirectory() as tmp_dirname:
      path = pathlib.Path(tmp_dirname) / "foo" / "bar"
      self.assertFalse(path.exists())
      self.platform.mkdir(path)
      self.assertTrue(path.is_dir())

  def test_rm_file(self):
    with tempfile.TemporaryDirectory() as tmp_dirname:
      path = pathlib.Path(tmp_dirname) / "foo.txt"
      path.touch()
      self.assertTrue(path.is_file())
      self.platform.rm(path)
      self.assertFalse(path.exists())

  def test_rm_dir(self):
    with tempfile.TemporaryDirectory() as tmp_dirname:
      path = pathlib.Path(tmp_dirname) / "foo" / "bar"
      path.mkdir(parents=True, exist_ok=False)
      self.assertTrue(path.is_dir())
      with self.assertRaises(Exception):
        self.platform.rm(path.parent)
      self.platform.rm(path.parent, dir=True)
      self.assertFalse(path.exists())
      self.assertFalse(path.parent.exists())

  def test_mkdtemp(self):
    result = self.platform.mkdtemp(prefix="a_custom_prefix")
    self.assertTrue(result.is_dir())
    self.assertIn("a_custom_prefix", result.name)
    self.platform.rm(result, dir=True)
    self.assertFalse(result.exists())

  def test_mkdtemp_dir(self):
    with tempfile.TemporaryDirectory() as tmp_dirname:
      tmp_dir = pathlib.Path(tmp_dirname)
      result = self.platform.mkdtemp(dir=tmp_dir)
      self.assertTrue(result.is_dir())
      self.assertTrue(compat.is_relative_to(result, tmp_dir))
    self.assertFalse(result.exists())

  def test_mktemp(self):
    result = self.platform.mktemp(prefix="a_custom_prefix")
    self.assertTrue(result.is_file())
    self.assertIn("a_custom_prefix", result.name)
    self.platform.rm(result)
    self.assertFalse(result.exists())

  def test_mktemp_dir(self):
    with tempfile.TemporaryDirectory() as tmp_dirname:
      tmp_dir = pathlib.Path(tmp_dirname)
      result = self.platform.mktemp(dir=tmp_dir)
      self.assertTrue(result.is_file())
      self.assertTrue(compat.is_relative_to(result, tmp_dir))
    self.assertFalse(result.exists())

  def test_exists(self):
    with tempfile.TemporaryDirectory() as tmp_dirname:
      tmp_dir = pathlib.Path(tmp_dirname)
      self.assertTrue(self.platform.exists(tmp_dir))
      self.assertFalse(self.platform.exists(tmp_dir / "foo"))

  def test_path_tests(self):
    with tempfile.TemporaryDirectory() as tmp_dirname:
      tmp_dir = pathlib.Path(tmp_dirname)
      self.assertTrue(self.platform.exists(tmp_dir))
      self.assertTrue(self.platform.is_dir(tmp_dir))
      self.assertFalse(self.platform.is_file(tmp_dir))

      foo_dir = tmp_dir / "foo"
      self.assertFalse(self.platform.exists(foo_dir))
      self.assertFalse(self.platform.is_dir(foo_dir))
      self.assertFalse(self.platform.is_file(foo_dir))
      foo_dir.mkdir()
      self.assertTrue(self.platform.exists(foo_dir))
      self.assertTrue(self.platform.is_dir(foo_dir))
      self.assertFalse(self.platform.is_file(foo_dir))

      bar_file = tmp_dir / "bar.txt"
      self.assertFalse(self.platform.exists(bar_file))
      self.assertFalse(self.platform.is_dir(bar_file))
      self.assertFalse(self.platform.is_file(bar_file))
      bar_file.touch()
      self.assertTrue(self.platform.exists(bar_file))
      self.assertFalse(self.platform.is_dir(bar_file))
      self.assertTrue(self.platform.is_file(bar_file))


@unittest.skipIf(not plt.PLATFORM.is_posix, "Incompatible platform")
class PosixPlatformTestCase(PlatformTestCase):
  platform: plt.PosixPlatform

  def setUp(self):
    super().setUp()
    assert isinstance(plt.PLATFORM, plt.PosixPlatform)
    self.platform: plt.PosixPlatform = plt.PLATFORM

  def test_sh(self):
    ls = self.platform.sh_stdout("ls")
    self.assertTrue(ls)
    lsa = self.platform.sh_stdout("ls", "-a")
    self.assertTrue(lsa)
    self.assertNotEqual(ls, lsa)

  def test_which(self):
    ls_bin = self.platform.which("ls")
    self.assertIsNotNone(ls_bin)
    bash_bin = self.platform.which("bash")
    self.assertIsNotNone(bash_bin)
    self.assertNotEqual(ls_bin, bash_bin)
    self.assertTrue(pathlib.Path(ls_bin).exists())
    self.assertTrue(pathlib.Path(bash_bin).exists())

  def test_system_details(self):
    details = self.platform.system_details()
    self.assertTrue(details)

  def test_search_binary(self):
    result_path = self.platform.search_binary(pathlib.Path("ls"))
    self.assertIsNotNone(result_path)
    self.assertIn("ls", result_path.parts)

  def test_environ(self):
    env = self.platform.environ
    self.assertTrue(env)
    self.assertIn("PATH", env)
    self.assertTrue(list(env))

  def test_environ_set_proprty(self):
    env = self.platform.environ
    custom_key = f"CROSSBENCH_TEST_KEY_{len(env)}"
    self.assertNotIn(custom_key, env)
    with self.assertRaises(Exception):
      env[custom_key] = 1234
    env[custom_key] = "1234"
    self.assertEqual(env[custom_key], "1234")
    self.assertIn(custom_key, env)
    del env[custom_key]
    self.assertNotIn(custom_key, env)


class MockRemotePosixPlatform(type(plt.PLATFORM)):

  def is_remote(self) -> bool:
    return True

  def sh(self, *args, **kwargs):
    return plt.PLATFORM.sh(*args, **kwargs)

  def sh_stdout(self, *args, **kwargs):
    return plt.PLATFORM.sh_stdout(*args, **kwargs)


@unittest.skipIf(not plt.PLATFORM.is_posix, "Incompatible platform")
class MockRemotePosixPlatformTestCase(PosixPlatformTestCase):
  """All Posix operations should also work on a remote platform (e.g. via SSH).
  This test fakes this by temporarily moving the current PLATFORM's is_remove
  getter to return True"""

  def setUp(self):
    super().setUp()
    self.platform = MockRemotePosixPlatform()

  def tests_default_tmp_dir(self):
    self.assertEqual(self.platform.default_tmp_dir,
                     plt.PLATFORM.default_tmp_dir)

  def test_environ_set_proprty(self):
    raise self.skipTest("Not supported on remote platforms")

  def test_cpu_usage(self):
    raise self.skipTest("Not supported on remote platforms")


@unittest.skipIf(not plt.PLATFORM.is_macos, "Incompatible platform")
class MacOSPlatformTestCase(PosixPlatformTestCase):
  platform: plt.MacOSPlatform

  def setUp(self):
    super().setUp()
    assert isinstance(plt.PLATFORM, plt.MacOSPlatform)
    self.platform = plt.PLATFORM

  def test_search_binary_not_found(self):
    binary = self.platform.search_binary(pathlib.Path("Invalid App Name"))
    self.assertIsNone(binary)
    binary = self.platform.search_binary(pathlib.Path("Non-existent App.app"))
    self.assertIsNone(binary)

  def test_search_binary(self):
    binary = self.platform.search_binary(pathlib.Path("Safari.app"))
    self.assertTrue(binary and binary.is_file())

  def test_search_app_invalid(self):
    with self.assertRaises(ValueError):
      self.platform.search_app(pathlib.Path("Invalid App Name"))

  def test_search_app_none(self):
    self.assertIsNone(self.platform.search_app(pathlib.Path("No App.app")))

  def test_search_app(self):
    binary = self.platform.search_app(pathlib.Path("Safari.app"))
    self.assertTrue(binary and binary.exists())
    self.assertTrue(binary and binary.is_dir())

  def test_app_version_app(self):
    app = self.platform.search_app(pathlib.Path("Safari.app"))
    self.assertIsNotNone(app)
    self.assertTrue(app.is_dir())
    version = self.platform.app_version(app)
    self.assertRegex(version, r"[0-9]+\.[0-9]+")

  def test_app_version_app_binary(self):
    binary = self.platform.search_binary(pathlib.Path("Safari.app"))
    self.assertIsNotNone(binary)
    self.assertTrue(binary.is_file())
    version = self.platform.app_version(binary)
    self.assertRegex(version, r"[0-9]+\.[0-9]+")

  def test_app_version_binary(self):
    binary = pathlib.Path("/usr/bin/safaridriver")
    self.assertTrue(binary.is_file())
    version = self.platform.app_version(binary)
    self.assertRegex(version, r"[0-9]+\.[0-9]+")

  def test_name(self):
    self.assertEqual(self.platform.name, "macos")

  def test_version(self):
    self.assertTrue(self.platform.version)
    self.assertRegex(self.platform.version, r"[0-9]+\.[0-9]")

  def test_device(self):
    self.assertTrue(self.platform.device)
    self.assertRegex(self.platform.device, r"[a-zA-Z]+[0-9]+,[0-9]+")

  def test_cpu(self):
    self.assertTrue(self.platform.cpu)
    self.assertRegex(self.platform.cpu, r".* [0-9]+ cores")

  def test_foreground_process(self):
    self.assertTrue(self.platform.foreground_process())

  def test_is_macos(self):
    self.assertTrue(self.platform.is_macos)
    self.assertFalse(self.platform.is_linux)
    self.assertFalse(self.platform.is_win)
    self.assertFalse(self.platform.is_remote)

  def test_set_main_screen_brightness(self):
    prev_level = plt.PLATFORM.get_main_display_brightness()
    brightness_level = 32
    plt.PLATFORM.set_main_display_brightness(brightness_level)
    self.assertEqual(brightness_level,
                     plt.PLATFORM.get_main_display_brightness())
    plt.PLATFORM.set_main_display_brightness(prev_level)
    self.assertEqual(prev_level, plt.PLATFORM.get_main_display_brightness())

  def test_check_autobrightness(self):
    self.platform.check_autobrightness()

  def test_exec_apple_script(self):
    self.assertEqual(
        self.platform.exec_apple_script('copy "a value" to stdout').strip(),
        "a value")

  def test_exec_apple_script_args(self):
    result = self.platform.exec_apple_script("copy item 1 of argv to stdout",
                                             "a value", "b")
    self.assertEqual(result.strip(), "a value")
    result = self.platform.exec_apple_script("copy item 2 of argv to stdout",
                                             "a value", "b")
    self.assertEqual(result.strip(), "b")

  def test_exec_apple_script_invalid(self):
    with self.assertRaises(plt.SubprocessError):
      self.platform.exec_apple_script('something is not right 11')


@unittest.skipIf(not plt.PLATFORM.is_win, "Incompatible platform")
class WinPlatformTestCase(PlatformTestCase):
  platform: plt.WinPlatform

  def setUp(self):
    super().setUp()
    assert isinstance(plt.PLATFORM, plt.WinPlatform)
    self.platform = plt.PLATFORM

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

  def test_has_display(self):
    self.assertIn(self.platform.has_display, (True, False))

  def test_version(self):
    self.assertTrue(self.platform.version)


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
