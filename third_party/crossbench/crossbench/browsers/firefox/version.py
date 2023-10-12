# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import re
from typing import Dict, Tuple

from crossbench.browsers.version import BrowserVersion, BrowserVersionChannel


class FirefoxVersion(BrowserVersion):
  _VERSION_RE = re.compile(r"[^\d]+ (?P<version>"
                           r"(?P<parts>\d+\.\d+(?P<channel>[ab.])\d+)"
                           r")(?P<channel_esr>esr)?")
  _SPLIT_RE = re.compile(r'[ab.]')
  _CHANNEL_LOOKUP: Dict[str, Tuple[BrowserVersionChannel, int]] = {
      "esr": (BrowserVersionChannel.LTS, 3),
      ".": (BrowserVersionChannel.STABLE, 2),
      # IRL Firefox version numbers do not distinct beta from stable, so we
      # remap Firefox Dev => beta.
      "b": (BrowserVersionChannel.BETA, 1),
      "a": (BrowserVersionChannel.ALPHA, 0),
  }

  def _parse(
      self,
      version: str,
  ) -> Tuple[Tuple[int, ...], BrowserVersionChannel, str]:
    matches = self._VERSION_RE.fullmatch(version.strip())
    if not matches:
      raise ValueError(f"Could not extract version number from '{version}'")
    version_str = matches["version"]
    version_parts = matches["parts"]
    assert version_parts and version_str
    if matches["channel_esr"] and matches["channel"] != ".":
      raise ValueError(f"Invalid ESR version: {version}")
    channel: str = (matches["channel_esr"] or matches["channel"] or
                    "stable").lower()
    browser_channel, channel_id = self._CHANNEL_LOOKUP[channel]
    parts = tuple(map(int, self._SPLIT_RE.split(version_parts)))
    if len(parts) != 3:
      raise ValueError(f"Invalid number of version number parts in '{version}'")
    # Inject browser_channel into the version parts to make it unique
    parts = parts[:-1] + (channel_id, parts[-1])
    return parts, browser_channel, version_str

  def _channel_name(self, channel: BrowserVersionChannel) -> str:
    if channel == BrowserVersionChannel.LTS:
      return "esr"
    if channel == BrowserVersionChannel.STABLE:
      return "stable"
    if channel == BrowserVersionChannel.BETA:
      return "dev"
    if channel == BrowserVersionChannel.ALPHA:
      return "nightly"
    raise ValueError(f"Unsupported channel: {channel}")
