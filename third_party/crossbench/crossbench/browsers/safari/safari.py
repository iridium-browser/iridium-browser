# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import pathlib
from typing import TYPE_CHECKING, Optional, Tuple

from crossbench import helper
from crossbench.browsers.browser import Browser
from crossbench.browsers.viewport import Viewport

if TYPE_CHECKING:
  from crossbench.flags import Flags
  from crossbench.runner import Run, Runner


class Safari(Browser):

  @classmethod
  def default_path(cls) -> pathlib.Path:
    return pathlib.Path("/Applications/Safari.app")

  @classmethod
  def technology_preview_path(cls) -> pathlib.Path:
    return pathlib.Path("/Applications/Safari Technology Preview.app")

  def __init__(self,
               label: str,
               path: pathlib.Path,
               flags: Flags.InitialDataType = None,
               cache_dir: Optional[pathlib.Path] = None,
               viewport: Viewport = Viewport.DEFAULT,
               platform: Optional[helper.MacOSPlatform] = None):
    super().__init__(
        label, path, flags, type="safari", viewport=viewport, platform=platform)
    assert self.platform.is_macos, "Safari only works on MacOS"
    assert self.path
    self.bundle_name = self.path.stem.replace(" ", "")
    assert cache_dir is None, "Cannot set custom cache dir for Safari"
    self.cache_dir = pathlib.Path(
        f"~/Library/Containers/com.apple.{self.bundle_name}/Data/Library/Caches"
    ).expanduser()

  def clear_cache(self, runner: Runner) -> None:
    logging.info("CLEAR CACHE: %s", self)
    self.platform.exec_apple_script(f"""
      tell application "{self.app_path}" to activate
      tell application "System Events"
          keystroke "e" using {{command down, option down}}
      end tell""")

  def _extract_version(self) -> str:
    assert self.path
    app_path = self.path.parents[2]
    return self.platform.app_version(app_path)
