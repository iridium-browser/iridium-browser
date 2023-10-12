# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations
import pathlib
from unittest import mock

from crossbench import plt
from crossbench.plt import android_adb

from crossbench.plt.android_adb import Adb
from crossbench.plt.arch import MachineArch
from tests import run_helper
from tests.crossbench.plt.helper import PosixPlatformTestCase

ADB_DEVICES_SAMPLE_OUTPUT = """List of devices attached
emulator-5556 device product:sdk_google_phone_x86_64 model:Android_SDK_built_for_x86_64 device:generic_x86_64
emulator-5554 device product:sdk_google_phone_x86 model:Android_SDK_built_for_x86 device:generic_x86
0a388e93      device usb:1-1 product:razor model:Nexus_7 device:flo"""


class AndroidAdbPlatformTest(PosixPlatformTestCase):
  __test__ = True
  DEVICE_ID = "emulator-5554"
  platform: plt.AndroidAdbPlatform

  def setUp(self) -> None:
    super().setUp()
    adb_patcher = mock.patch(
        "crossbench.plt.android_adb._find_adb_bin",
        return_value=pathlib.Path("adb"))
    adb_patcher.start()
    self.addCleanup(adb_patcher.stop)
    self.expect_sh(pathlib.Path("adb"), "start-server")
    self.expect_sh(
        pathlib.Path("adb"), "devices", "-l", result=ADB_DEVICES_SAMPLE_OUTPUT)
    self.adb = Adb(self.mock_platform, self.DEVICE_ID)
    self.platform = plt.AndroidAdbPlatform(
        self.mock_platform, self.DEVICE_ID, adb=self.adb)

  def expect_adb(self, *args, result=""):
    self.expect_sh(
        pathlib.Path("adb"), "-s", self.DEVICE_ID, *args, result=result)

  def test_is_remote(self):
    self.assertTrue(self.platform.is_remote)

  def test_name(self):
    self.assertEqual(self.platform.name, "android")

  def test_is_android(self):
    self.assertTrue(self.platform.is_android)

  def test_host_platform(self):
    self.assertIs(self.platform.host_platform, self.mock_platform)

  def test_version(self):
    self.expect_adb(
        "shell", "getprop", "ro.build.version.release", result="999")
    self.assertEqual(self.platform.version, "999")
    # Subsequent calls are cached.
    self.assertEqual(self.platform.version, "999")

  def test_device(self):
    self.expect_adb("shell", "getprop", "ro.product.model", result="Pixel 999")
    self.assertEqual(self.platform.device, "Pixel 999")
    # Subsequent calls are cached.
    self.assertEqual(self.platform.device, "Pixel 999")

  def test_cpu(self):
    self.expect_adb(
        "shell", "getprop", "dalvik.vm.isa.arm.variant", result="cortex-a999")
    self.expect_adb("shell", "getprop", "ro.board.platform", result="msmnile")
    self.assertEqual(self.platform.cpu, "cortex-a999 msmnile")
    # Subsequent calls are cached.
    self.assertEqual(self.platform.cpu, "cortex-a999 msmnile")

  def test_cpu_detailed(self):
    self.expect_adb(
        "shell", "getprop", "dalvik.vm.isa.arm.variant", result="cortex-a999")
    self.expect_adb("shell", "getprop", "ro.board.platform", result="msmnile")
    self.expect_adb(
        "shell", "cat", "/sys/devices/system/cpu/possible", result="0-998")
    self.assertEqual(self.platform.cpu, "cortex-a999 msmnile 999 cores")
    # Subsequent calls are cached.
    self.assertEqual(self.platform.cpu, "cortex-a999 msmnile 999 cores")

  def test_adb(self):
    self.assertIs(self.platform.adb, self.adb)

  def test_machine_unknown(self):
    self.expect_adb(
        "shell", "getprop", "ro.product.cpu.abi", result="arm37-XXX")
    with self.assertRaises(ValueError) as cm:
      self.assertEqual(self.platform.machine, MachineArch.ARM_64)
    self.assertIn("arm37-XXX", str(cm.exception))

  def test_machine_arm64(self):
    self.expect_adb(
        "shell", "getprop", "ro.product.cpu.abi", result="arm64-v8a")
    self.assertEqual(self.platform.machine, MachineArch.ARM_64)
    # Subsequent calls are cached.
    self.assertEqual(self.platform.machine, MachineArch.ARM_64)

  def test_machine_arm32(self):
    self.expect_adb(
        "shell", "getprop", "ro.product.cpu.abi", result="armeabi-v7a")
    self.assertEqual(self.platform.machine, MachineArch.ARM_32)
    # Subsequent calls are cached.
    self.assertEqual(self.platform.machine, MachineArch.ARM_32)

  def test_app_path_to_package_invalid_path(self):
    path = pathlib.Path("path/to/app.bin")
    with self.assertRaises(ValueError) as cm:
      self.platform.app_path_to_package(path)
    self.assertIn(str(path), str(cm.exception))

  def test_app_path_to_package_not_installed(self):
    with self.assertRaises(ValueError) as cm:
      self.expect_adb(
          "shell",
          "cmd",
          "package",
          "list",
          "packages",
          result=("package:com.google.android.wifi.resources\n"
                  "package:com.google.android.GoogleCamera"))
      self.platform.app_path_to_package(pathlib.Path("com.custom.app"))
    self.assertIn("com.custom.app", str(cm.exception))
    self.assertIn("not installed", str(cm.exception))

  def test_app_path_to_package(self):
    path = pathlib.Path("com.custom.app")
    self.expect_adb(
        "shell",
        "cmd",
        "package",
        "list",
        "packages",
        result=("package:com.google.android.wifi.resources\n"
                "package:com.custom.app"))
    self.assertEqual(self.platform.app_path_to_package(path), "com.custom.app")

  def test_app_version(self):
    path = pathlib.Path("com.custom.app")
    self.expect_adb(
        "shell",
        "cmd",
        "package",
        "list",
        "packages",
        result="package:com.custom.app")
    self.expect_adb(
        "shell",
        "dumpsys",
        "package",
        "com.custom.app",
        result="versionName=9.999")
    self.assertEqual(self.platform.app_version(path), "9.999")

  def test_app_version_unkown(self):
    path = pathlib.Path("com.custom.app")
    self.expect_adb(
        "shell",
        "cmd",
        "package",
        "list",
        "packages",
        result="package:com.custom.app")
    self.expect_adb(
        "shell", "dumpsys", "package", "com.custom.app", result="something")
    with self.assertRaises(ValueError) as cm:
      self.platform.app_version(path)
    self.assertIn("something", str(cm.exception))
    self.assertIn("com.custom.app", str(cm.exception))

  def test_get_relative_cpu_speed(self):
    self.assertGreater(self.platform.get_relative_cpu_speed(), 0)

  def test_check_autobrightness(self):
    self.assertTrue(self.platform.check_autobrightness())

  def get_main_display_brightness(self):
    display_info = ("BrightnessSynchronizer\n"
                    "mLatestFloatBrightness=0.5\n"
                    "mLatestIntBrightness=128\n"
                    "mPendingUpdate=null")
    self.expect_adb("shell", "dumpsys", "display", result=display_info)
    self.assertEqual(self.platform.get_main_display_brightness(), 50)
    # Values are not cached
    display_info = ("BrightnessSynchronizer\n"
                    "mLatestFloatBrightness=1.0\n"
                    "mLatestIntBrightness=255\n"
                    "mPendingUpdate=null")
    self.expect_adb("shell", "dumpsys", "display", result=display_info)
    self.assertEqual(self.platform.get_main_display_brightness(), 100)


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
