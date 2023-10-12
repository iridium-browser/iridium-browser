# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import datetime as dt
import logging
from typing import Iterator

from crossbench import cli_helper


class PlaybackController:

  @classmethod
  def parse(cls, value: str) -> PlaybackController:
    if not value or value == "once":
      return cls.once()
    if value in ("inf", "infinity", "forever"):
      return cls.forever()
    if value[-1].isnumeric():
      raise argparse.ArgumentTypeError(
          f"Missing unit suffix: '{value}'\n"
          "Use 'x' for repetitions or time unit 's', 'm', 'h'")
    if value[-1] == "x":
      try:
        loops = int(value[:-1])
      except ValueError as e:
        raise argparse.ArgumentTypeError(
            f"Repeat-count must be a valid int, {e}")
      if loops <= 0:
        raise argparse.ArgumentTypeError(
            f"Repeat-count must be positive: {value}")
      try:
        return cls.repeat(loops)
      except ValueError as e:
        logging.debug("{%s}.repeat failed, falling back to timeout: %s", cls, e)
    duration = dt.timedelta()
    try:
      duration = cli_helper.Duration.parse_zero(value)
    except argparse.ArgumentTypeError as e:
      raise argparse.ArgumentTypeError(f"Invalid cycle argument: {e}") from e
    if duration.total_seconds() == 0:
      return cls.forever()
    return cls.timeout(duration)

  @classmethod
  def once(cls) -> RepeatPlaybackController:
    return RepeatPlaybackController(1)

  @classmethod
  def repeat(cls, count: int) -> RepeatPlaybackController:
    return RepeatPlaybackController(count)

  @classmethod
  def forever(cls) -> PlaybackController:
    return PlaybackController()

  @classmethod
  def timeout(cls, duration: dt.timedelta) -> TimeoutPlaybackController:
    return TimeoutPlaybackController(duration)

  def __iter__(self) -> Iterator[None]:
    while True:
      yield None


class TimeoutPlaybackController(PlaybackController):

  def __init__(self, duration: dt.timedelta) -> None:
    # TODO: support --time-unit
    self._duration = duration

  @property
  def duration(self) -> dt.timedelta:
    return self._duration

  def __iter__(self) -> Iterator[None]:
    end = dt.datetime.now() + self.duration
    while dt.datetime.now() <= end:
      yield None


class RepeatPlaybackController(PlaybackController):

  def __init__(self, count: int) -> None:
    assert count > 0, f"Invalid page playback count: {count}"
    self._count = count

  def __iter__(self) -> Iterator[None]:
    for _ in range(self._count):
      yield None

  @property
  def count(self) -> int:
    return self._count
