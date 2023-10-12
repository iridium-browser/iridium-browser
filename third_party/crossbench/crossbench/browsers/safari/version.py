# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import re
from typing import Tuple

from crossbench.browsers.version import (BrowserVersion, BrowserVersionChannel)


class SafariVersion(BrowserVersion):
  _VERSION_RE = re.compile(r"(?P<major_minor>\d+\.\d+)"
                           r"[^(]+ \((?P<version>"
                           r"(Release (?P<release>\d+), )?"
                           r"(?P<parts>([\d.]+)+)"
                           r")\)")

  def _parse(
      self,
      version: str,
  ) -> Tuple[Tuple[int, ...], BrowserVersionChannel, str]:
    matches = self._VERSION_RE.fullmatch(version.strip())
    if not matches:
      raise ValueError(f"Could not extract version number from '{version}'")
    version_str = matches["version"]
    parts_str = matches["parts"]
    major_minor_str = matches["major_minor"]
    assert version_str and parts_str and major_minor_str
    channel: BrowserVersionChannel = BrowserVersionChannel.STABLE
    if "Safari Technology Preview" in version:
      channel = BrowserVersionChannel.BETA
    major, minor = tuple(map(int, major_minor_str.split(".")))
    release = 0
    if release_str := matches["release"]:
      release = int(release_str)
    try:
      parts = tuple(map(int, parts_str.split(".")))
    except ValueError as e:
      raise ValueError("Could not parse version number parts.") from e
    if len(parts) < 4:
      raise ValueError(f"Invalid number of version number parts in '{version}'")
    parts = (major, minor, release) + parts
    return parts, channel, f"{major_minor_str} ({version_str})"

  @property
  def is_tech_preview(self) -> bool:
    return self.channel == BrowserVersionChannel.BETA

  @property
  def release(self) -> int:
    return self._parts[2]

  @property
  def channel_name(self) -> str:
    return self._channel_name(self.channel)

  def _channel_name(self, channel: BrowserVersionChannel) -> str:
    if channel == BrowserVersionChannel.STABLE:
      return "stable"
    if channel == BrowserVersionChannel.BETA:
      return "technology preview"
    raise ValueError(f"Unsupported channel: {channel}")
