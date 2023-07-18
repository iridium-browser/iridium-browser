# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import pathlib
from typing import TYPE_CHECKING, Optional

from crossbench import helper

from crossbench.browsers.chromium import Chromium
from crossbench.browsers.viewport import Viewport

if TYPE_CHECKING:
  from crossbench.flags import Flags


class ChromePathMixin:

  @classmethod
  def default_path(cls) -> pathlib.Path:
    return cls.stable_path()

  @classmethod
  def stable_path(cls) -> pathlib.Path:
    return helper.search_app_or_executable(
        "Chrome Stable",
        macos=["Google Chrome.app"],
        linux=["google-chrome", "chrome"],
        win=["Google/Chrome/Application/chrome.exe"])

  @classmethod
  def beta_path(cls) -> pathlib.Path:
    return helper.search_app_or_executable(
        "Chrome Beta",
        macos=["Google Chrome Beta.app"],
        linux=["google-chrome-beta"],
        win=["Google/Chrome Beta/Application/chrome.exe"])

  @classmethod
  def dev_path(cls) -> pathlib.Path:
    return helper.search_app_or_executable(
        "Chrome Dev",
        macos=["Google Chrome Dev.app"],
        linux=["google-chrome-unstable"],
        win=["Google/Chrome Dev/Application/chrome.exe"])

  @classmethod
  def canary_path(cls) -> pathlib.Path:
    return helper.search_app_or_executable(
        "Chrome Canary",
        macos=["Google Chrome Canary.app"],
        win=["Google/Chrome SxS/Application/chrome.exe"])


class Chrome(ChromePathMixin, Chromium):

  def __init__(self,
               label: str,
               path: pathlib.Path,
               js_flags: Flags.InitialDataType = None,
               flags: Flags.InitialDataType = None,
               cache_dir: Optional[pathlib.Path] = None,
               viewport: Viewport = Viewport.DEFAULT,
               platform: Optional[helper.Platform] = None):
    super().__init__(
        label,
        path,
        js_flags,
        flags,
        cache_dir,
        type="chrome",
        viewport=viewport,
        platform=platform)
