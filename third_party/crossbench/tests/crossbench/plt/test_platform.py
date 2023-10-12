# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import unittest

from crossbench.plt import MachineArch
from tests import run_helper


class MachineArchTestCase(unittest.TestCase):

  def test_is_arm(self):
    self.assertFalse(MachineArch.IA32.is_arm)
    self.assertFalse(MachineArch.X64.is_arm)
    self.assertTrue(MachineArch.ARM_32.is_arm)
    self.assertTrue(MachineArch.ARM_64.is_arm)

  def test_is_intel(self):
    self.assertTrue(MachineArch.IA32.is_intel)
    self.assertTrue(MachineArch.X64.is_intel)
    self.assertFalse(MachineArch.ARM_32.is_intel)
    self.assertFalse(MachineArch.ARM_64.is_intel)

  def test_is_32bit(self):
    self.assertTrue(MachineArch.IA32.is_32bit)
    self.assertFalse(MachineArch.X64.is_32bit)
    self.assertTrue(MachineArch.ARM_32.is_32bit)
    self.assertFalse(MachineArch.ARM_64.is_32bit)

  def test_is_64bit(self):
    self.assertFalse(MachineArch.IA32.is_64bit)
    self.assertTrue(MachineArch.X64.is_64bit)
    self.assertFalse(MachineArch.ARM_32.is_64bit)
    self.assertTrue(MachineArch.ARM_64.is_64bit)

  def test_str(self):
    self.assertEqual(str(MachineArch.IA32), "ia32")
    self.assertEqual(str(MachineArch.X64), "x64")
    self.assertEqual(str(MachineArch.ARM_32), "arm32")
    self.assertEqual(str(MachineArch.ARM_64), "arm64")


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
