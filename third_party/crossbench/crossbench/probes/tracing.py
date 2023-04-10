# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING, Iterable

from crossbench.browsers.chromium import Chromium
from crossbench.probes.base import Probe

if TYPE_CHECKING:
  from crossbench.browsers.base import Browser


class TracingProbe(Probe):
  """
  Chromium-only Probe to collect tracing / perfetto data that can be used by
  chrome://tracing or https://ui.perfetto.dev/.

  Currently WIP
  """
  NAME = "tracing"
  FLAGS = (
      "--enable-perfetto",
      "--disable-fre",
  )

  def __init__(self,
               categories: Iterable[str],
               startup_duration: float = 0,
               output_format: str = "json"):
    super().__init__()
    self._categories = categories
    self._startup_duration = startup_duration
    self._format = output_format
    assert self._format in ("json", "proto"), (
        f"Invalid trace output output_format={self._format}")

  def is_compatible(self, browser: Browser) -> bool:
    return isinstance(browser, Chromium)

  def attach(self, browser: Browser) -> None:
    # "--trace-startup-format"
    # --trace-startup-duration=
    # --trace-startup=categories
    # v--trace-startup-file=" + file_name
    super().attach(browser)
