# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import pathlib
from typing import TYPE_CHECKING

from selenium import webdriver
from selenium.webdriver.chrome.options import Options as ChromeOptions
from selenium.webdriver.chrome.service import Service as ChromeService

from crossbench import helper
from crossbench.browsers.viewport import Viewport
from crossbench.browsers.chromium import ChromiumAppleScript

from .chrome import ChromePathMixin

if TYPE_CHECKING:
  from typing import Optional

  from selenium.webdriver.chromium.webdriver import ChromiumDriver

  from crossbench.flags import Flags


class ChromeAppleScript(ChromePathMixin, ChromiumAppleScript):

  WEB_DRIVER_OPTIONS = ChromeOptions
  WEB_DRIVER_SERVICE = ChromeService

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

  def _create_driver(self, options, service) -> ChromiumDriver:
    return webdriver.Chrome(  # pytype: disable=wrong-keyword-args
        options=options, service=service)
