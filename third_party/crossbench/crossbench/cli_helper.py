# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import contextlib
import datetime as dt
import json
import math
import pathlib
import re
import sys
from typing import Any, Iterator, Optional, Union
from urllib.parse import urlparse

import hjson
from crossbench import plt


def parse_path(value: Union[str, pathlib.Path]) -> pathlib.Path:
  if not value:
    raise argparse.ArgumentTypeError("Invalid empty path.")
  try:
    path = pathlib.Path(value).expanduser()
  except RuntimeError as e:
    raise argparse.ArgumentTypeError(f"Invalid Path '{value}': {e}") from e
  if not path.exists():
    raise argparse.ArgumentTypeError(f"Path '{path}' does not exist.")
  return path


def parse_existing_file_path(value: Union[str, pathlib.Path]) -> pathlib.Path:
  path = parse_path(value)
  if not path.is_file():
    raise argparse.ArgumentTypeError(f"Path '{path}' is not a file.")
  return path


def parse_non_empty_file_path(value: Union[str, pathlib.Path]) -> pathlib.Path:
  path: pathlib.Path = parse_existing_file_path(value)
  if path.stat().st_size == 0:
    raise argparse.ArgumentTypeError(f"Path '{path}' is an empty file.")
  return path


def parse_file_path(value: Union[str, pathlib.Path]) -> pathlib.Path:
  return parse_non_empty_file_path(value)


def parse_dir_path(value: Union[str, pathlib.Path]) -> pathlib.Path:
  path = parse_path(value)
  if not path.is_dir():
    raise argparse.ArgumentTypeError(f"Path '{path}', is not a folder.")
  return path


def parse_existing_path(value: Union[str, pathlib.Path]) -> pathlib.Path:
  path = parse_path(value)
  if not path.exists():
    raise argparse.ArgumentTypeError(f"Path '{path}' does not exist.")
  return path


def parse_binary_path(value: str,
                      platform: Optional[plt.Platform] = None) -> pathlib.Path:
  maybe_path = pathlib.Path(value)
  if maybe_path.is_file():
    return maybe_path
  maybe_bin = (platform or plt.PLATFORM).search_binary(maybe_path)
  if not maybe_bin:
    raise argparse.ArgumentTypeError(f"Unknown binary: {value}")
  return maybe_bin


def parse_inline_hjson(value: Any) -> Any:
  value_str = parse_non_empty_str(value, hjson.__name__)
  if value_str[0] != "{" or value_str[-1] != "}":
    raise argparse.ArgumentTypeError(
        f"Invalid inline {hjson.__name__}, missing braces: '{value_str}'")
  try:
    return hjson.loads(value_str)
  except ValueError as e:
    message = _extract_decoding_error(
        f"Could not decode inline {hjson.__name__}", value_str, e)
    if "eof" in message:
      message += "\n   Likely missing quotes."
    raise argparse.ArgumentTypeError(message) from e


def _extract_decoding_error(message: str, value: Union[str, pathlib.Path],
                            e: ValueError) -> str:
  lineno = getattr(e, "lineno", -1) - 1
  colno = getattr(e, "colno", -1) - 1
  if lineno < 0 or colno < 0:
    if isinstance(value, pathlib.Path):
      return f"{message}\n    {str(e)}"
    return f"{message}: {value}\n    {str(e)}"
  if isinstance(value, pathlib.Path):
    line = value.open().readlines()[lineno]
  else:
    line = value.splitlines()[lineno]
  MAX_LEN = 70
  if len(line) > MAX_LEN:
    # Only show line around error:
    start = colno - MAX_LEN // 2
    end = colno + MAX_LEN // 2
    prefix = "..."
    suffix = "..."
    if start < 0:
      start = 0
      end = MAX_LEN
      prefix = ""
    elif end > len(line):
      end = len(line)
      start = len(line) - MAX_LEN
      suffix = ""
    colno -= start
    line = prefix + line[start:end] + suffix
    marker_space = (" " * len(prefix)) + (" " * colno)
  else:
    marker_space = (" " * colno)
  marker = "_▲_"
  # Adjust line to be aligned with marker size
  line = (" " * (len(marker) // 2)) + line
  return f"{message}\n    {line}\n    {marker_space}{marker}\n({str(e)})"


def parse_json_file_path(value: Union[str, pathlib.Path]) -> pathlib.Path:
  path = parse_file_path(value)
  with path.open(encoding="utf-8") as f:
    try:
      json.load(f)
    except ValueError as e:
      message = _extract_decoding_error(f"Invalid json file '{path}':", path, e)
      raise argparse.ArgumentTypeError(message) from e
  return path


def parse_hjson_file_path(value: Union[str, pathlib.Path]) -> pathlib.Path:
  path = parse_file_path(value)
  with path.open(encoding="utf-8") as f:
    try:
      hjson.load(f)
    except ValueError as e:
      message = _extract_decoding_error(
          f"Invalid {hjson.__name__} file '{path}':", path, e)
      raise argparse.ArgumentTypeError(message) from e
  return path


def parse_json_file(value: Union[str, pathlib.Path]) -> Any:
  path = parse_file_path(value)
  with path.open(encoding="utf-8") as f:
    try:
      return json.load(f)
    except ValueError as e:
      message = _extract_decoding_error(f"Invalid json file '{path}':", path, e)
      raise argparse.ArgumentTypeError(message) from e


def parse_hjson_file(value: Union[str, pathlib.Path]) -> Any:
  path = parse_file_path(value)
  with path.open(encoding="utf-8") as f:
    try:
      return hjson.load(f)
    except ValueError as e:
      message = _extract_decoding_error(
          f"Invalid {hjson.__name__} file '{path}':", path, e)
      raise argparse.ArgumentTypeError(message) from e


def parse_non_empty_hjson_file(value: Union[str, pathlib.Path]) -> Any:
  data = parse_hjson_file(value)
  if not data:
    raise argparse.ArgumentTypeError(
        f"Expected {hjson.__name__} file with non-empty data, "
        f"but got: {hjson.dumps(data)}")
  return data


def parse_dict_hjson_file(value: Union[str, pathlib.Path]) -> Any:
  data = parse_non_empty_hjson_file(value)
  if not isinstance(data, dict):
    raise argparse.ArgumentTypeError(
        f"Expected object in {hjson.__name__} config '{value}', "
        f"but got {type(data).__name__}: {data}")
  return data


def parse_positive_zero_float(value: Any) -> float:
  try:
    value_f = float(value)
  except ValueError as e:
    raise argparse.ArgumentTypeError(f"Invalid float: '{value}'") from e
  if not math.isfinite(value_f) or value_f < 0:
    raise argparse.ArgumentTypeError(f"Expected float >= 0, but got: {value_f}")
  return value_f


def parse_positive_zero_int(value: Any) -> int:
  try:
    positive_int = int(value)
  except ValueError as e:
    raise argparse.ArgumentTypeError(f"Invalid integer: '{value}'") from e
  if positive_int < 0:
    raise argparse.ArgumentTypeError(
        f"Expected int >= 0, but got: {positive_int}")
  return positive_int


def parse_positive_int(value: str, msg: str = "") -> int:
  try:
    value_i = int(value)
  except ValueError as e:
    raise argparse.ArgumentTypeError(f"Invalid integer: '{value}'") from e
  if not math.isfinite(value_i) or value_i <= 0:
    raise argparse.ArgumentTypeError(
        f"Expected int > 0 {msg}, but got: {value_i}")
  return value_i


def parse_non_empty_str(value: Any, name: str = "string") -> str:
  if value is None:
    raise argparse.ArgumentTypeError(f"Expected non-empty {name}, but got None")
  if not isinstance(value, str):
    raise argparse.ArgumentTypeError(
        f"Expected non-empty {name}, but got {type(value)}: {value}")
  if not value:
    raise argparse.ArgumentTypeError("Non-empty {name} expected.")
  return value


def parse_url_str(value: str) -> str:
  # TODO: improve
  url_str: str = parse_non_empty_str(value, "url")
  return url_str


def parse_httpx_url_str(value: Any) -> str:
  try:
    url_str: str = parse_url_str(value)
    parsed = urlparse(url_str)
    if parsed.scheme not in ("http", "https"):
      raise argparse.ArgumentTypeError(
          "Expected 'http' or 'https' scheme, "
          f"but got '{parsed.scheme}' for url '{value}'")
    if not parsed.hostname:
      raise argparse.ArgumentTypeError(f"Missing hostname in url: '{value}'")
  except ValueError as e:
    raise argparse.ArgumentTypeError(f"Invalid URL: {value}, {e}") from e
  return value


def parse_bool(value: Any) -> bool:
  if isinstance(value, bool):
    return value
  value = value.lower()
  if value == "true":
    return True
  if value == "false":
    return False
  raise TypeError(f"Expected bool but got {type(value)}: {value}")


class CrossBenchArgumentError(argparse.ArgumentError):
  """Custom class that also prints the argument.help if available.
  """

  def __init__(self, argument: Any, message: str) -> None:
    self.help: str = ""
    super().__init__(argument, message)
    if self.argument_name:
      self.help = getattr(argument, "help", "")

  def __str__(self) -> str:
    formatted = super().__str__()
    if not self.help:
      return formatted
    return (f"argument error {self.argument_name}:\n\n"
            f"Help {self.argument_name}:\n{self.help}\n\n"
            f"{formatted}")

# Needed to gap the diff between 3.8 and 3.9 default args that change throwing
# behavior.
class _BaseCrossBenchArgumentParser(argparse.ArgumentParser):

  def fail(self, message) -> None:
    super().error(message)


if sys.version_info < (3, 9, 0):

  class CrossBenchArgumentParser(_BaseCrossBenchArgumentParser):

    def error(self, message) -> None:
      # Let the CrossBenchCLI handle all errors and simplify testing.
      exception = sys.exc_info()[1]
      if isinstance(exception, BaseException):
        raise exception
      raise argparse.ArgumentError(None, message)

else:

  class CrossBenchArgumentParser(_BaseCrossBenchArgumentParser):

    def __init__(self, *args, **kwargs) -> None:
      kwargs["exit_on_error"] = False
      super().__init__(*args, **kwargs)


class LateArgumentError(argparse.ArgumentTypeError):
  """Signals argument parse errors after parser.parse_args().
  This is used to map errors back to the original argument, much like
  argparse.ArgumentError does internally. However, since this happens after
  the internal argument parsing we need this custom implementation to print
  more descriptive error messages.
  """

  def __init__(self, flag: str, message: str) -> None:
    super().__init__(message)
    self.flag = flag
    self.message = message


@contextlib.contextmanager
def late_argument_type_error_wrapper(flag: str) -> Iterator[None]:
  """Converts raised ValueError and ArgumentTypeError to LateArgumentError
  that are associated with the given flag.
  """
  try:
    yield
  except Exception as e:
    raise LateArgumentError(flag, str(e)) from e


class Duration:

  @classmethod
  def help(cls) -> str:
    return "'12.5' == '12.5s',  units=['ms', 's', 'm', 'h']"

  _DURATION_RE = re.compile(r"(?P<value>(-?\d+(\.\d+)?)) ?(?P<unit>[^0-9\.]+)?")

  @classmethod
  def _to_timedelta(cls, value: float, suffix: str) -> dt.timedelta:
    if suffix in {"ms", "millis", "milliseconds"}:
      return dt.timedelta(milliseconds=value)
    if suffix in {"s", "sec", "secs", "second", "seconds"}:
      return dt.timedelta(seconds=value)
    if suffix in {"m", "min", "mins", "minute", "minutes"}:
      return dt.timedelta(minutes=value)
    if suffix in {"h", "hrs", "hour", "hours"}:
      return dt.timedelta(hours=value)
    raise argparse.ArgumentTypeError(
        f"Error: {suffix} is not supported for duration. "
        "Make sure to use a supported time unit/suffix")

  @classmethod
  def parse(cls, time_value: Union[float, int, str]) -> dt.timedelta:
    return cls.parse_non_zero(time_value)

  @classmethod
  def parse_non_zero(cls, time_value: Union[float, int, str]) -> dt.timedelta:
    duration: dt.timedelta = cls.parse_any(time_value)
    if duration.total_seconds() <= 0:
      raise argparse.ArgumentTypeError(
          f"Expected non-zero duration, but got {duration}")
    return duration

  @classmethod
  def parse_zero(cls, time_value: Union[float, int, str]) -> dt.timedelta:
    duration: dt.timedelta = cls.parse_any(time_value)
    if duration.total_seconds() < 0:
      raise argparse.ArgumentTypeError(
          f"Expected positive duration, but got {duration}")
    return duration

  @classmethod
  def parse_any(cls, time_value: Union[float, int, str]) -> dt.timedelta:
    """
    This function will parse the measurement and the value from string value.

    For example:
    5s => dt.timedelta(seconds=5)
    5m => 5*60 = dt.timedelta(minutes=5)

    """
    if isinstance(time_value, (int, float)):
      if time_value < 0:
        raise argparse.ArgumentTypeError(
            f"Duration must be positive, but got: {time_value}")
      return dt.timedelta(seconds=time_value)

    if not time_value:
      raise argparse.ArgumentTypeError("duration.")

    match = cls._DURATION_RE.fullmatch(time_value)
    if match is None:
      raise argparse.ArgumentTypeError(
          f"Unknown Duration format: '{time_value}'")

    value = match.group("value")
    if not value:
      raise argparse.ArgumentTypeError(
          "Error: Duration value not found."
          f"Make sure to include a valid duration value: '{time_value}'")
    time_unit = match.group("unit")
    try:
      time_value = float(value)
    except ValueError as e:
      raise argparse.ArgumentTypeError(f"Duration must be a valid number, {e}")
    if math.isnan(time_value) or math.isinf(time_value):
      raise argparse.ArgumentTypeError(
          f"Duration must be positive, but got: {time_value}")

    if not time_unit:
      # If no time unit provided we assume it is in seconds.
      return dt.timedelta(seconds=time_value)
    return cls._to_timedelta(time_value, time_unit)
