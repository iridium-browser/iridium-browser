# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING

from ..applescript import AppleScriptBrowser
from .safari import Safari

if TYPE_CHECKING:
  from crossbench.runner.runner import Runner


class SafariAppleScript(Safari, AppleScriptBrowser):
  APPLE_SCRIPT_ALLOW_JS_MENU: str = (
      "Develop > Allow JavaScript from Apple Events")
  APPLE_SCRIPT_JS_COMMAND: str = (
      "tell current tab of front window to do javascript %(js_script)s")
  APPLE_SCRIPT_SET_URL: str = (
      "set URL of the current tab of front window to %(url)s")

  def _setup_window(self) -> None:
    self._exec_apple_script(f"""
      tell application "System Events"
          click menu item "New Private Window" of menu "File" of menu bar 1 of process "{self.bundle_name}"
      end tell
      set URL of current tab of front window to ""
    """)
    self.platform.sleep(0.5)
    if self.viewport.is_fullscreen:
      self._exec_apple_script("""
        tell application "System Events"
          keystroke "f" using {command down, control down}
        end tell""")
    elif self.viewport.is_maximized:
      self._exec_apple_script("""
        tell application "System Events"
          keystroke "m"
        end tell""")
    else:
      bounds = (f"{self.viewport.x},{self.viewport.y},"
                f"{self.viewport.width},{self.viewport.height}")
      self._exec_apple_script(f"set the bounds of the first window to {bounds}")

  def quit(self, runner: Runner) -> None:
    super().quit(runner)
    # Safari doesn't react to "quit" when using the full app path.
    self.platform.exec_apple_script(f"""
        tell application "{self.bundle_name}"
          quit
        end tell""")
