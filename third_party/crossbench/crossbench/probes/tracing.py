# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import logging
import pathlib
from typing import TYPE_CHECKING, Dict, Optional, Sequence, Set

from crossbench import cli_helper, compat, helper
from crossbench.browsers.chromium.chromium import Chromium
from crossbench.probes import helper as probe_helper
from crossbench.probes.probe import (Probe, ProbeConfigParser, ProbeScope,
                                     ResultLocation)
from crossbench.probes.results import ProbeResult

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.flags import ChromeFlags
  from crossbench import plt
  from crossbench.runner.run import Run

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

class RecordMode(compat.StrEnum):
  CONTINUOUSLY = "record-continuously"
  UNTIL_FULL = "record-until-full"
  AS_MUCH_AS_POSSIBLE = "record-as-much-as-possible"


class RecordFormat(helper.StrEnumWithHelp):
  JSON = ("json", "Old about://tracing compatible file format.")
  PROTO = ("proto", "New https://ui.perfetto.dev/ compatible format")


def parse_trace_config_file_path(value: str) -> pathlib.Path:
  data = cli_helper.parse_json_file(value)
  if "trace_config" not in data:
    raise argparse.ArgumentTypeError("Missing 'trace_config' property.")
  cli_helper.parse_positive_int(
      data.get("startup_duration", "0"), "for 'startup_duration'")
  if "result_file" in data:
    raise argparse.ArgumentTypeError(
        "Explicit 'result_file' is not allowed with crossbench. "
        "--probe=tracing sets a results location automatically.")
  config = data["trace_config"]
  if "included_categories" not in config and (
      "excluded_categories" not in config) and ("memory_dump_config"
                                                not in config):
    raise argparse.ArgumentTypeError(
        "Empty trace config: no trace categories or memory dumps configured.")
  record_mode = config.get("record_mode", RecordMode.CONTINUOUSLY)
  try:
    RecordMode(record_mode)
  except ValueError as e:
    # pytype: disable=missing-parameter
    raise argparse.ArgumentTypeError(
        f"Invalid record_mode: '{record_mode}'. "
        f"Choices are: {', '.join(str(e) for e in RecordMode)}") from e
    # pytype: enable=missing-parameter
  return pathlib.Path(value)


ANDROID_TRACE_CONFIG_PATH = pathlib.Path("/data/local/chrome-trace-config.json")


class TracingProbe(Probe):
  """
  Chromium-only Probe to collect tracing / perfetto data that can be used by
  chrome://tracing or https://ui.perfetto.dev/.

  Currently WIP
  """
  NAME = "tracing"
  RESULT_LOCATION = ResultLocation.BROWSER
  CHROMIUM_FLAGS = ("--enable-perfetto",)

  HELP_URL = "https://bit.ly/chrome-about-tracing"

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
        help=f"A list of trace categories to enable.\n{cls.HELP_URL}")
    parser.add_argument(
        "trace_config",
        type=parse_trace_config_file_path,
        help=("Sets Chromium's --trace-config-file to the given json config.\n"
              "https://bit.ly/chromium-memory-startup-tracing "))
    parser.add_argument(
        "startup_duration",
        default=0,
        type=cli_helper.parse_positive_zero_int,
        help=("Stop recording tracing after a given number of seconds. "
              "Use 0 (default) for unlimited recording time."))
    parser.add_argument(
        "record_mode",
        default=RecordMode.CONTINUOUSLY,
        type=RecordMode,
        help="")
    parser.add_argument(
        "record_format",
        default=RecordFormat.PROTO,
        type=RecordFormat,
        help=("Choose between 'json' or the default 'proto' format. "
              "Perfetto proto output is converted automatically to the "
              "legacy json format."))
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
               startup_duration: int = 0,
               record_mode: RecordMode = RecordMode.CONTINUOUSLY,
               record_format: RecordFormat = RecordFormat.PROTO,
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

    self._startup_duration: int = startup_duration
    self._record_mode: RecordMode = record_mode
    self._record_format: RecordFormat = record_format
    self._traceconv = traceconv

  @property
  def result_path_name(self) -> str:
    return f"trace.{self._record_format.value}"  # pylint: disable=no-member

  @property
  def traceconv(self) -> Optional[pathlib.Path]:
    return self._traceconv

  @property
  def record_format(self) -> RecordFormat:
    return self._record_format

  def is_compatible(self, browser: Browser) -> bool:
    return isinstance(browser, Chromium)

  def attach(self, browser: Browser) -> None:
    assert isinstance(browser, Chromium)
    flags: ChromeFlags = browser.flags
    flags.update(self.CHROMIUM_FLAGS)
    # Force proto file so we can convert it to legacy json as well.
    flags["--trace-startup-format"] = str(self._record_format)
    # pylint: disable=no-member
    flags["--trace-startup-duration"] = str(self._startup_duration)
    if self._trace_config:
      # TODO: use ANDROID_TRACE_CONFIG_PATH
      assert not browser.platform.is_android, (
          "Trace config files not supported on android yet")
      flags["--trace-config-file"] = str(self._trace_config.absolute())
    else:
      flags["--trace-startup-record-mode"] = str(self._record_mode)
      assert self._categories, "No trace categories provided."
      flags["--enable-tracing"] = ",".join(self._categories)
    super().attach(browser)

  def get_scope(self, run: Run) -> TracingProbeScope:
    return TracingProbeScope(self, run)


class TracingProbeScope(ProbeScope[TracingProbe]):
  _traceconv: Optional[pathlib.Path]
  _record_format: RecordFormat

  def setup(self, run: Run) -> None:
    run.extra_flags["--trace-startup-file"] = str(self.result_path)
    self._record_format = self.probe.record_format
    if self._record_format == RecordFormat.PROTO:
      self._traceconv = self.probe.traceconv or TraceconvFinder(
          self.browser_platform).traceconv
    else:
      self._traceconv = None

  def start(self, run: Run) -> None:
    del run

  def stop(self, run: Run) -> None:
    del run

  def tear_down(self, run: Run) -> ProbeResult:
    if self._record_format == RecordFormat.JSON:
      return self.browser_result(json=(self.result_path,))
    if not self._traceconv:
      logging.info(
          "No traceconv binary: skipping converting proto to legacy traces")
      return self.browser_result(file=(self.result_path,))

    logging.info("Converting to legacy .json trace on local machine: %s",
                 self.result_path)
    json_trace_file = self.result_path.with_suffix(".json")
    self.browser_platform.sh(self._traceconv, "json", self.result_path,
                             json_trace_file)
    return self.browser_result(
        json=(json_trace_file,), file=(self.result_path,))


class TraceconvFinder(probe_helper.V8CheckoutFinder):

  def __init__(self, platform: plt.Platform) -> None:
    super().__init__(platform)
    self.traceconv: Optional[pathlib.Path] = None
    if self.v8_checkout:
      candidate = (
          self.v8_checkout / "third_party" / "perfetto" / "tools" / "traceconv")
      if candidate.is_file():
        self.traceconv = candidate
