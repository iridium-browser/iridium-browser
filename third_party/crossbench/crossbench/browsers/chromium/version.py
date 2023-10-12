# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import re
from typing import Dict, Tuple

from crossbench.browsers.version import (BrowserVersion, BrowserVersionChannel)


class ChromiumVersion(BrowserVersion):
  _VERSION_RE = re.compile(
      r'[^\d]+ (?P<version>\d+\.\d+\.\d+\.\d+)( (?P<channel>[a-zA-Z]+))?')
  _CHANNEL_LOOKUP: Dict[str, BrowserVersionChannel] = {
      "stable": BrowserVersionChannel.STABLE,
      "beta": BrowserVersionChannel.BETA,
      "dev": BrowserVersionChannel.ALPHA,
      "canary": BrowserVersionChannel.PRE_ALPHA
  }

  def _parse(
      self,
      version: str,
  ) -> Tuple[Tuple[int, ...], BrowserVersionChannel, str]:
    matches = self._VERSION_RE.fullmatch(version.strip())
    if not matches:
      raise ValueError(f"Could not extract version number from '{version}'")
    version_str = matches["version"]
    assert version_str
    channel_str: str = (matches["channel"] or "stable").lower()
    parts = tuple(map(int, version_str.split(".")))
    assert len(parts) == 4
    return parts, self._CHANNEL_LOOKUP[channel_str], version_str

  @property
  def build(self) -> int:
    return self._parts[2]

  @property
  def patch(self) -> int:
    return self._parts[3]

  @property
  def is_dev(self) -> bool:
    return self.is_alpha

  @property
  def is_canary(self) -> bool:
    return self.is_pre_alpha

  def _channel_name(self, channel: BrowserVersionChannel) -> str:
    for name, lookup_channel in self._CHANNEL_LOOKUP.items():
      if channel == lookup_channel:
        return name
    raise ValueError(f"Unsupported channel: {channel}")
