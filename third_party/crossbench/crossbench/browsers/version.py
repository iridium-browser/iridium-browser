# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import functools
from typing import Any, Tuple
import enum


class BrowserVersionChannel(enum.Enum):
  LTS = "lts"
  STABLE = "stable"
  BETA = "beta"
  ALPHA = "alpha"
  PRE_ALPHA = "pre-alpha"


@functools.total_ordering
class BrowserVersion(abc.ABC):

  _parts: Tuple[int, ...]
  _channel: BrowserVersionChannel
  _version_str: str

  def __init__(self, version: str) -> None:
    (self._parts, self._channel, self._version_str) = self._parse(version)
    if len(self._parts) < 2:
      raise ValueError("Invalid version format")
    for part in self._parts:
      if part < 0:
        raise ValueError(
            f"Version parts must be positive, but got {self._parts}")

  @abc.abstractmethod
  def _parse(
      self,
      version: str,
  ) -> Tuple[Tuple[int, ...], BrowserVersionChannel, str]:
    pass

  @property
  def major(self) -> int:
    return self._parts[0]

  @property
  def minor(self) -> int:
    return self._parts[1]

  @property
  def channel(self) -> BrowserVersionChannel:
    return self._channel

  @property
  def is_lts(self) -> bool:
    return self.channel == BrowserVersionChannel.LTS

  @property
  def is_stable(self) -> bool:
    return self.channel == BrowserVersionChannel.STABLE

  @property
  def is_beta(self) -> bool:
    return self.channel == BrowserVersionChannel.BETA

  @property
  def is_alpha(self) -> bool:
    return self.channel == BrowserVersionChannel.ALPHA

  @property
  def is_pre_alpha(self) -> bool:
    return self.channel == BrowserVersionChannel.PRE_ALPHA

  @property
  def channel_name(self) -> str:
    return self._channel_name(self.channel)

  @abc.abstractmethod
  def _channel_name(self, channel: BrowserVersionChannel) -> str:
    pass

  def __str__(self) -> str:
    return f"{self._version_str} {self.channel_name}"

  def __eq__(self, other: Any) -> bool:
    if not isinstance(other, BrowserVersion):
      return False
    return str(self) == str(other)

  def __lt__(self, other: Any) -> bool:
    assert isinstance(
        other, BrowserVersion), (f"Expected BrowserVersion, but got {other}.")
    return self._parts < other._parts
