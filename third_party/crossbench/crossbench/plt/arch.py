# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import enum


class MachineArch(enum.Enum):
  IA32 = ("ia32", "intel", 32)
  X64 = ("x64", "intel", 64)
  ARM_32 = ("arm32", "arm", 32)
  ARM_64 = ("arm64", "arm", 64)

  def __init__(self, name: str, arch: str, bits: int) -> None:
    self.identifier = name
    self.arch = arch
    self.bits = bits

  @property
  def is_arm(self) -> bool:
    return self.arch == "arm"

  @property
  def is_intel(self) -> bool:
    return self.arch == "intel"

  @property
  def is_32bit(self) -> bool:
    return self.bits == 32

  @property
  def is_64bit(self) -> bool:
    return self.bits == 64

  def __str__(self) -> str:
    return self.identifier
