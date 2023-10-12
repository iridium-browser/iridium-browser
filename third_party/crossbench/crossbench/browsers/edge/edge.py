# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import pathlib
from typing import TYPE_CHECKING, Optional

import crossbench
from crossbench.browsers.chromium.chromium import Chromium
import crossbench.exception
import crossbench.flags
from crossbench import helper

if TYPE_CHECKING:
  import crossbench.runner
  from crossbench.browsers.splash_screen import SplashScreen
  from crossbench.browsers.viewport import Viewport
  from crossbench import plt

FlagsInitialDataType = crossbench.flags.Flags.InitialDataType


class Edge(Chromium):
  DEFAULT_FLAGS = [
      "--enable-benchmarking",
      "--disable-extensions",
      "--no-first-run",
  ]

  @classmethod
  def default_path(cls) -> pathlib.Path:
    return cls.stable_path()

  @classmethod
  def stable_path(cls) -> pathlib.Path:
    return helper.search_app_or_executable(
        "Edge Stable",
        macos=["Microsoft Edge.app"],
        linux=["microsoft-edge"],
        win=["Microsoft/Edge/Application/msedge.exe"])

  @classmethod
  def beta_path(cls) -> pathlib.Path:
    return helper.search_app_or_executable(
        "Edge Beta",
        macos=["Microsoft Edge Beta.app"],
        linux=["microsoft-edge-beta"],
        win=["Microsoft/Edge Beta/Application/msedge.exe"])

  @classmethod
  def dev_path(cls) -> pathlib.Path:
    return helper.search_app_or_executable(
        "Edge Dev",
        macos=["Microsoft Edge Dev.app"],
        linux=["microsoft-edge-dev"],
        win=["Microsoft/Edge Dev/Application/msedge.exe"])

  @classmethod
  def canary_path(cls) -> pathlib.Path:
    return helper.search_app_or_executable(
        "Edge Canary",
        macos=["Microsoft Edge Canary.app"],
        linux=[],
        win=["Microsoft/Edge SxS/Application/msedge.exe"])

  def __init__(self,
               label: str,
               path: pathlib.Path,
               js_flags: FlagsInitialDataType = None,
               flags: FlagsInitialDataType = None,
               cache_dir: Optional[pathlib.Path] = None,
               viewport: Optional[Viewport] = None,
               splash_screen: Optional[SplashScreen] = None,
               platform: Optional[plt.Platform] = None):
    super().__init__(
        label,
        path,
        js_flags,
        flags,
        cache_dir,
        type="edge",
        viewport=viewport,
        splash_screen=splash_screen,
        platform=platform)
