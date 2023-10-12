# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import sys
from typing import Final

from .android_adb import AndroidAdbPlatform, adb_devices
from .arch import MachineArch
from .base import Platform, SubprocessError
from .linux import LinuxPlatform
from .macos import MacOSPlatform
from .posix import PosixPlatform
from .win import WinPlatform


def _get_default() -> Platform:
  if sys.platform == "linux":
    return LinuxPlatform()
  if sys.platform == "darwin":
    return MacOSPlatform()
  if sys.platform == "win32":
    return WinPlatform()
  raise NotImplementedError("Unsupported Platform")


PLATFORM: Final[Platform] = _get_default()
