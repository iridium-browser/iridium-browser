# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import pathlib
from typing import TYPE_CHECKING, Optional

from crossbench.browsers.chromium.chromium import Chromium

from .helper import ChromePathMixin

if TYPE_CHECKING:
  from crossbench.browsers.splash_screen import SplashScreen
  from crossbench.browsers.viewport import Viewport
  from crossbench.flags import Flags
  from crossbench import plt


class Chrome(ChromePathMixin, Chromium):

  def __init__(
      self,
      label: str,
      path: pathlib.Path,
      flags: Optional[Flags.InitialDataType] = None,
      js_flags: Optional[Flags.InitialDataType] = None,
      cache_dir: Optional[pathlib.Path] = None,
      type: str = "chrome",  # pylint: disable=redefined-builtin
      driver_path: Optional[pathlib.Path] = None,
      viewport: Optional[Viewport] = None,
      splash_screen: Optional[SplashScreen] = None,
      platform: Optional[plt.Platform] = None):
    super().__init__(
        label,
        path,
        flags,
        js_flags,
        cache_dir,
        type=type,
        driver_path=driver_path,
        viewport=viewport,
        splash_screen=splash_screen,
        platform=platform)
