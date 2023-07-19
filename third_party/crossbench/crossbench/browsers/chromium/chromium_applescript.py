# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from ..applescript import AppleScriptBrowser
from .chromium import Chromium


# TODO: fix https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/browser_commands_mac.mm;drc=ddf482c0cf47fc8e47e5cfc5c112e2313e066cb8;bpv=1;bpt=1;l=38
# TODO: Auto-set: prefs::kAllowJavascriptAppleEvents
# TODO: add --enable-automation flag
class ChromiumAppleScript(Chromium, AppleScriptBrowser):
  APPLE_SCRIPT_ALLOW_JS_MENU: str = (
      "View > Developer > Allow JavaScript from Apple Events")
  APPLE_SCRIPT_JS_COMMAND: str = (
      "tell the active tab of front window to execute javascript %(js_script)s")
  APPLE_SCRIPT_SET_URL: str = (
      "set URL of the active tab of front window to %(url)s")

  def _setup_window(self) -> None:
    pass
