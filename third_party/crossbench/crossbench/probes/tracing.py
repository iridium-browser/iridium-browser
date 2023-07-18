# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations
import argparse
import enum
import logging
import pathlib

from typing import TYPE_CHECKING, Dict, List, Optional, Sequence, Set, cast
from crossbench import cli_helper

from crossbench.browsers.chromium import Chromium
from crossbench.helper import Platform
from crossbench.probes.probe import Probe, ProbeConfigParser
from crossbench.probes.results import ProbeResult
from crossbench.probes import helper as probe_helper

if TYPE_CHECKING:
  from crossbench.flags import ChromeFlags
  from crossbench.runner import Run
  from crossbench.browsers.browser import Browser

MINIMAL_CONFIG = {
    "toplevel",
    "v8",
    "v8.execute",
}
DEVTOOLS_TRACE_CONFIG = {
    "blink.console",
    "blink.user_timing",
    "devtools.timeline",
    "disabled-by-default-devtools.screenshot",
    "disabled-by-default-devtools.timeline",
    "disabled-by-default-devtools.timeline.frame",
    "disabled-by-default-devtools.timeline.layers",
    "disabled-by-default-devtools.timeline.picture",
    "disabled-by-default-devtools.timeline.stack",
    "disabled-by-default-lighthouse",
    "disabled-by-default-v8.compile",
    "disabled-by-default-v8.cpu_profiler",
    "disabled-by-default-v8.cpu_profiler.hires"
    "latencyInfo",
    "toplevel",
    "v8.execute",
}
# TODO: go over these again and clean the categories.
V8_TRACE_CONFIG = {
    "blink",
    "browser",
    "cc",
    "disabled-by-default-ipc.flow",
    "disabled-by-default-power",
    "disabled-by-default-v8.compile",
    "disabled-by-default-v8.cpu_profiler",
    "disabled-by-default-v8.cpu_profiler.hires",
    "disabled-by-default-v8.gc",
    "disabled-by-default-v8.gc_stats",
    "disabled-by-default-v8.inspector",
    "disabled-by-default-v8.runtime",
    "disabled-by-default-v8.runtime_stats",
    "disabled-by-default-v8.runtime_stats_sampling",
    "disabled-by-default-v8.stack_trace",
    "disabled-by-default-v8.turbofan",
    "disabled-by-default-v8.wasm.detailed",
    "disabled-by-default-v8.wasm.turbofan",
    "gpu",
    "io",
    "ipc",
    "latency",
    "latencyInfo",
    "loading",
    "log",
    "mojom",
    "navigation",
    "net",
    "netlog",
    "toplevel",
    "toplevel.flow",
    "v8",
    "v8.execute",
    "wayland",
}

TRACE_PRESETS: Dict[str, Set[str]] = {
    "minimal": MINIMAL_CONFIG,
    "devtools": DEVTOOLS_TRACE_CONFIG,
    "v8": V8_TRACE_CONFIG,
}


class RecordMode(enum.Enum):
  CONTINUOUSLY = "record-continuously"
  UNTIL_FULL = "record-until-full"
  AS_MUCH_AS_POSSIBLE = "record-as-much-as-possible"


class RecordFormat(enum.Enum):
  JSON = "json"
  PROTO = "proto"


class TracingProbe(Probe):
  """
  Chromium-only Probe to collect tracing / perfetto data that can be used by
  chrome://tracing or https://ui.perfetto.dev/.

  Currently WIP
  """
  NAME = "tracing"
  CHROMIUM_FLAGS = ("--enable-perfetto",)

  HELP_URL = "https://www.chromium.org/developers/how-tos/trace-event-profiling-tool/"

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "preset",
        type=str,
        default="minimal",
        choices=TRACE_PRESETS.keys(),
        help="Use predefined trace categories")
    parser.add_argument(
        "categories",
        is_list=True,
        default=[],
        type=str,
        help=("A list of trace categories to enable. "
              f"See chrome's {cls.HELP_URL} for more details"))
    parser.add_argument(
        "trace_config",
        type=cli_helper.parse_json_file_path,
        help=("Sets Chromium's --trace-config-file to the given json config."))
    parser.add_argument(
        "startup_duration",
        type=cli_helper.parse_positive_float,
        help="Stop recording tracing after a certain time")
    parser.add_argument(
        "record_mode",
        default=RecordMode.CONTINUOUSLY,
        type=RecordMode,
        help="")
    parser.add_argument(
        "traceconv",
        default=None,
        type=cli_helper.parse_file_path,
        help=(
            "Path to the 'traceconv.py' helper to convert "
            "'.proto' traces to legacy '.json'. "
            "If not specified, tries to find it in a v8 or chromium checkout."))
    return parser

  def __init__(self,
               preset: Optional[str] = None,
               categories: Optional[Sequence[str]] = None,
               trace_config: Optional[pathlib.Path] = None,
               startup_duration: float = 0,
               record_mode: RecordMode = RecordMode.CONTINUOUSLY,
               traceconv: Optional[pathlib.Path] = None) -> None:
    super().__init__()
    self._trace_config = trace_config
    self._categories: Set[str] = set(categories or MINIMAL_CONFIG)
    if preset:
      self._categories.update(TRACE_PRESETS[preset])
    if self._trace_config:
      if self._categories != set(MINIMAL_CONFIG):
        raise argparse.ArgumentTypeError(
            "TracingProbe requires either a list of "
            "trace categories or a trace_config file.")
      self._categories = set()

    self._startup_duration = startup_duration
    self._record_mode = record_mode
    self._record_format = RecordFormat.PROTO
    self._traceconv = traceconv

  @property
  def results_file_name(self) -> str:
    return f"trace.{self._record_format.value}"

  @property
  def traceconv(self) -> Optional[pathlib.Path]:
    return self._traceconv

  def is_compatible(self, browser: Browser) -> bool:
    return isinstance(browser, Chromium)

  def attach(self, browser: Chromium) -> None:
    flags: ChromeFlags = browser.flags
    flags.update(self.CHROMIUM_FLAGS)
    # Force proto file so we can convert it to legacy json as well.
    flags["--trace-startup-format"] = self._record_format.value
    if self._trace_config:
      flags["--trace-config-file"] = str(self._trace_config)
    else:
      if self._startup_duration:
        flags["--trace-startup-duration"] = str(self._startup_duration)
      flags["--trace-startup-record-mode"] = self._record_mode.value
      assert self._categories, "No trace categories provided."
      flags["--trace-startup"] = ",".join(self._categories)
    super().attach(browser)

  class Scope(Probe.Scope):
    _traceconv: Optional[pathlib.Path]

    def setup(self, run: Run) -> None:
      run.extra_flags["--trace-startup-file"] = str(self.results_file)
      self._traceconv = self.probe.traceconv or TraceconvFinder(
          self.browser_platform).traceconv

    def start(self, run: Run) -> None:
      del run

    def stop(self, run: Run) -> None:
      del run

    def tear_down(self, run: Run) -> ProbeResult:
      if not self._traceconv:
        logging.info(
            "No traceconv binary: skipping converting proto to legacy traces")
        return ProbeResult(file=(self.results_file,))
      logging.info("Converting to legacy .json trace: %s", self.results_file)
      json_trace_file = self.results_file.with_suffix(".json")
      self.browser_platform.sh(self._traceconv, "json", self.results_file,
                               json_trace_file)
      return ProbeResult(json=(json_trace_file,), file=(self.results_file,))


class TraceconvFinder(probe_helper.V8CheckoutFinder):

  def __init__(self, platform: Platform) -> None:
    super().__init__(platform)
    self.traceconv: Optional[pathlib.Path] = None
    if self.v8_checkout:
      candidate = (
          self.v8_checkout / "third_party" / "perfetto" / "tools" / "traceconv")
      if candidate.is_file():
        self.traceconv = candidate
