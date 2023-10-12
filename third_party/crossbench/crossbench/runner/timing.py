# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import dataclasses
import datetime as dt
from typing import Union


# Arbitrary very large number that doesn't break any browser driver protocol.
# chromedriver likely uses an uint32 ms internally, 2**30ms == 12days.
SAFE_MAX_TIMEOUT_TIMEDELTA = dt.timedelta(milliseconds=2**30)


@dataclasses.dataclass(frozen=True)
class Timing:
  cool_down_time: dt.timedelta = dt.timedelta(seconds=1)
  # General purpose time unit.
  unit: dt.timedelta = dt.timedelta(seconds=1)
  # Used for upper bound / timeout limits independently.
  timeout_unit: dt.timedelta = dt.timedelta()
  run_timeout: dt.timedelta = dt.timedelta()

  def __post_init__(self) -> None:
    if self.cool_down_time.total_seconds() < 0:
      raise ValueError(
          f"Timing.cool_down_time must be >= 0, but got: {self.cool_down_time}")
    if self.unit.total_seconds() <= 0:
      raise ValueError(f"Timing.unit must be > 0, but got {self.unit}")
    if self.timeout_unit:
      if self.timeout_unit.total_seconds() <= 0:
        raise ValueError(
            f"Timing.timeout_unit must be > 0, but got {self.timeout_unit}")
      if self.timeout_unit < self.unit:
        raise ValueError(f"Timing.unit must be <= Timing.timeout_unit: "
                         f"{self.unit} vs. {self.timeout_unit}")
    if self.run_timeout.total_seconds() < 0:
      raise ValueError(
          f"Timing.run_timeout, must be >= 0, but got {self.run_timeout}")

  def units(self, time: Union[float, int, dt.timedelta]) -> float:
    if isinstance(time, dt.timedelta):
      seconds = time.total_seconds()
    else:
      seconds = time
    if seconds < 0:
      raise ValueError(f"Unexpected negative time: {seconds}s")
    return seconds / self.unit.total_seconds()

  def _convert_to_seconds(
      self, time_units: Union[float, int, dt.timedelta]) -> Union[float, int]:
    if isinstance(time_units, dt.timedelta):
      seconds = time_units.total_seconds()
    else:
      seconds = time_units
    assert isinstance(seconds, (float, int))
    if seconds < 0:
      raise ValueError(f"Time-units must be >= 0, but got {seconds}")
    return seconds

  def timedelta(self, time_units: Union[float, int,
                                        dt.timedelta]) -> dt.timedelta:
    seconds_f = self._convert_to_seconds(time_units)
    return self._to_safe_range(seconds_f * self.unit)

  def timeout_timedelta(
      self, time_units: Union[float, int, dt.timedelta]) -> dt.timedelta:
    if self.has_no_timeout:
      return SAFE_MAX_TIMEOUT_TIMEDELTA
    seconds_f = self._convert_to_seconds(time_units)
    return self._to_safe_range(seconds_f * (self.timeout_unit or self.unit))

  def _to_safe_range(self, result: dt.timedelta) -> dt.timedelta:
    if result > SAFE_MAX_TIMEOUT_TIMEDELTA:
      return SAFE_MAX_TIMEOUT_TIMEDELTA
    return result

  @property
  def has_no_timeout(self) -> bool:
    return self.timeout_unit == dt.timedelta.max
