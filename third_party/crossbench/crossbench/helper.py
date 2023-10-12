# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
import enum
import logging
import os
import pathlib
import shlex
import sys
import textwrap
import threading
import time
import urllib
import urllib.error
import urllib.parse as urlparse
import urllib.request
from typing import (Any, Callable, Dict, Final, Iterable, Iterator, List,
                    Optional, Sequence, Tuple, Type, TypeVar, Union)

import tabulate

from crossbench import plt

assert hasattr(shlex,
               "join"), ("Please update to python v3.8 that has shlex.join")


class TTYColor:
  CYAN = "\033[1;36;6m"
  PURPLE = "\033[1;35;5m"
  BLUE = "\033[38;5;4m"
  YELLOW = "\033[38;5;3m"
  GREEN = "\033[38;5;2m"
  RED = "\033[38;5;1m"
  BLACK = "\033[38;5;0m"

  BOLD = "\033[1m"
  UNDERLINE = "\033[4m"
  REVERSED = "\033[7m"
  RESET = "\033[0m"


class ColoredLogFormatter(logging.Formatter):

  FORMAT = "%(message)s"

  FORMATS = {
      logging.DEBUG: FORMAT,
      logging.INFO: TTYColor.GREEN + FORMAT + TTYColor.RESET,
      logging.WARNING: TTYColor.YELLOW + FORMAT + TTYColor.RESET,
      logging.ERROR: TTYColor.RED + FORMAT + TTYColor.RESET,
      logging.CRITICAL: TTYColor.BOLD + FORMAT + TTYColor.RESET,
  }

  def format(self, record: logging.LogRecord) -> str:
    log_fmt = self.FORMATS.get(record.levelno)
    formatter = logging.Formatter(log_fmt)
    return formatter.format(record)

  def formatException(self, ei):
    return ""

  def formatStack(self, stack_info):
    return ""


InputT = TypeVar("InputT")
KeyT = TypeVar("KeyT")
GroupT = TypeVar("GroupT")


def group_by(collection: Iterable[InputT],
             key: Callable[[InputT], KeyT],
             value: Optional[Callable[[InputT], Any]] = None,
             group: Optional[Callable[[KeyT], GroupT]] = None,
             sort_key: Optional[Callable[[Tuple[KeyT, GroupT]], Any]] = str
            ) -> Dict[KeyT, GroupT]:
  """
  Works similar to itertools.groupby but does a global, SQL-style grouping
  instead of a line-by-line basis like uniq.

  key:   a function that returns the grouping key for a group
  group: a function that accepts a group_key and returns a group object that
    has an append() method.
  """
  assert key, "No key function provided"
  key_fn = key
  value_fn = value or (lambda item: item)
  group_fn: Callable[[KeyT], GroupT] = group or (lambda key: [])
  groups: Dict[KeyT, GroupT] = {}
  for input_item in collection:
    group_key: KeyT = key_fn(input_item)
    group_item = value_fn(input_item)
    if group_key not in groups:
      new_group: GroupT = group_fn(group_key)
      groups[group_key] = new_group
      new_group.append(group_item)
    else:
      groups[group_key].append(group_item)
  if sort_key:
    # sort keys as well for more predictable behavior
    items = sorted(groups.items(), key=sort_key)
  else:
    items = groups.items()
  return dict(items)


def sort_by_file_size(files: Iterable[pathlib.Path]) -> List[pathlib.Path]:
  return sorted(files, key=lambda f: (-f.stat().st_size, f.name))


SIZE_UNITS: Final[Tuple[str, ...]] = ("B", "KiB", "MiB", "GiB", "TiB")


def get_file_size(file: pathlib.Path, digits: int = 2) -> str:
  size = file.stat().st_size
  unit_index = 0
  divisor = 1024
  while (unit_index < len(SIZE_UNITS)) and size >= divisor:
    unit_index += 1
    size /= divisor
  return f"{size:.{digits}f} {SIZE_UNITS[unit_index]}"


def _search_executable(
    name: str,
    macos: Sequence[str],
    win: Sequence[str],
    linux: Sequence[str],
    platform: plt.Platform,
    lookup_callable: Callable[[pathlib.Path], Optional[pathlib.Path]],
) -> pathlib.Path:
  executables: Sequence[str] = []
  if platform.is_macos:
    executables = macos
  elif platform.is_win:
    executables = win
  elif platform.is_linux:
    executables = linux
  if not executables:
    raise ValueError(f"Executable {name} not supported on {platform}")
  for name_or_path in executables:
    path = pathlib.Path(name_or_path).expanduser()
    binary = lookup_callable(path)
    if binary and binary.exists():
      return binary
  raise ValueError(f"Executable {name} not found on {platform}")


def search_app_or_executable(
    name: str,
    macos: Sequence[str] = (),
    win: Sequence[str] = (),
    linux: Sequence[str] = (),
    platform: Optional[plt.Platform] = None) -> pathlib.Path:
  platform = platform or plt.PLATFORM
  return _search_executable(name, macos, win, linux, platform,
                            platform.search_app)


def search_binary(name: str,
                  macos: Sequence[str] = (),
                  win: Sequence[str] = (),
                  linux: Sequence[str] = (),
                  platform: Optional[plt.Platform] = None) -> pathlib.Path:
  platform = platform or plt.PLATFORM
  return _search_executable(name, macos, win, linux, platform,
                            platform.search_binary)

# =============================================================================


def urlopen(url: str):
  try:
    logging.debug("Opening url: %s", url)
    return urllib.request.urlopen(url)
  except urllib.error.HTTPError as e:
    logging.info("Could not load url=%s", url)
    raise e


# =============================================================================


class ChangeCWD:

  def __init__(self, destination: pathlib.Path) -> None:
    self.new_dir = destination
    self.prev_dir: Optional[str] = None

  def __enter__(self) -> None:
    self.prev_dir = os.getcwd()
    os.chdir(self.new_dir)

  def __exit__(self, exc_type, exc_value, exc_traceback) -> None:
    assert self.prev_dir, "ChangeCWD was not entered correctly."
    os.chdir(self.prev_dir)


class SystemSleepPreventer:
  """
  Prevent the system from going to sleep while running the benchmark.
  """

  def __init__(self) -> None:
    self._process = None

  def __enter__(self) -> None:
    if plt.PLATFORM.is_macos:
      self._process = plt.PLATFORM.popen("caffeinate", "-imdsu")
    # TODO: Add linux support

  def __exit__(self, exc_type, exc_value, exc_traceback) -> None:
    if self._process:
      self._process.kill()


class TimeScope:
  """
  Measures and logs the time spend during the lifetime of the TimeScope.
  """

  def __init__(self, message: str, level: int = 3) -> None:
    self._message = message
    self._level = level
    self._start: Optional[dt.datetime] = None
    self._duration = dt.timedelta()

  @property
  def message(self) -> str:
    return self._message

  @property
  def duration(self) -> dt.timedelta:
    return self._duration

  def __enter__(self) -> TimeScope:
    self._start = dt.datetime.now()
    return self

  def __exit__(self, exc_type, exc_value, exc_traceback) -> None:
    assert self._start
    self._duration = dt.datetime.now() - self._start
    logging.log(self._level, "%s duration=%s", self._message, self._duration)


class WaitRange:
  min: dt.timedelta
  max: dt.timedelta
  current: dt.timedelta

  def __init__(
      self,
      min: Union[float, dt.timedelta] = 0.1,  # pylint: disable=redefined-builtin
      timeout: Union[float, dt.timedelta] = 10,
      factor: float = 1.01,
      max: Optional[Union[float, dt.timedelta]] = None,  # pylint: disable=redefined-builtin
      max_iterations: Optional[int] = None
  ) -> None:
    if isinstance(min, dt.timedelta):
      self.min = min
    else:
      self.min = dt.timedelta(seconds=min)
    assert self.min.total_seconds() > 0
    if not max:
      self.max = self.min * 10
    elif isinstance(max, dt.timedelta):
      self.max = max
    else:
      self.max = dt.timedelta(seconds=max)
    assert self.min <= self.max
    assert 1.0 < factor
    self.factor = factor
    if isinstance(timeout, dt.timedelta):
      self.timeout = timeout
    else:
      self.timeout = dt.timedelta(seconds=timeout)
    assert 0 < self.timeout.total_seconds()
    self.current = self.min
    assert max_iterations is None or max_iterations > 0
    self.max_iterations = max_iterations

  def __iter__(self) -> Iterator[dt.timedelta]:
    assert self.current
    i = 0
    while self.max_iterations is None or i < self.max_iterations:
      yield self.current
      self.current = min(self.current * self.factor, self.max)
      i += 1


def wait_with_backoff(wait_range: WaitRange) -> Iterator[Tuple[float, float]]:
  assert isinstance(wait_range, WaitRange)
  start = dt.datetime.now()
  timeout = wait_range.timeout
  for sleep_for in wait_range:
    duration = dt.datetime.now() - start
    if duration > wait_range.timeout:
      raise TimeoutError(f"Waited for {duration}")
    time_left = timeout - duration
    yield duration.total_seconds(), time_left.total_seconds()
    plt.PLATFORM.sleep(sleep_for.total_seconds())


class DurationMeasureContext:

  def __init__(self, durations: Durations, name: str) -> None:
    self._start_time = dt.datetime.utcfromtimestamp(0)
    self._durations = durations
    self._name = name

  def __enter__(self) -> DurationMeasureContext:
    self._start_time = dt.datetime.now()
    return self

  def __exit__(self, exc_type, exc_value, traceback) -> None:
    assert self._start_time
    delta = dt.datetime.now() - self._start_time
    self._durations[self._name] = delta


class Durations:
  """
  Helper object to track durations.
  """

  def __init__(self) -> None:
    self._durations: Dict[str, dt.timedelta] = {}

  def __getitem__(self, name: str) -> dt.timedelta:
    return self._durations[name]

  def __setitem__(self, name: str, duration: dt.timedelta) -> None:
    assert name not in self._durations, (f"Cannot set '{name}' duration twice!")
    self._durations[name] = duration

  def __len__(self) -> int:
    return len(self._durations)

  def measure(self, name: str) -> DurationMeasureContext:
    assert name not in self._durations, (
        f"Cannot measure '{name}' duration twice!")
    return DurationMeasureContext(self, name)

  def to_json(self) -> Dict[str, float]:
    return {
        name: self._durations[name].total_seconds()
        for name in sorted(self._durations.keys())
    }


def wrap_lines(body: str, width: int = 80, indent: str = "") -> Iterable[str]:
  for line in body.splitlines():
    for split in textwrap.wrap(line, width):
      yield f"{indent}{split}"


def type_name(t: Type) -> str:
  module = t.__module__
  if not module:
    return t.__qualname__
  return f"{module}.{t.__qualname__}"


class Spinner:
  CURSORS = "◐◓◑◒"

  def __init__(self, sleep: float = 0.5) -> None:
    self._is_running = False
    self._sleep_time = sleep

  def __enter__(self) -> None:
    # Only enable the spinner if the output is an interactive terminal.
    is_atty = hasattr(sys.stdout, "isatty") and sys.stdout.isatty()
    if is_atty:
      self._is_running = True
      threading.Thread(target=self._spin).start()

  def __exit__(self, exc_type, exc_value, traceback) -> None:
    if self._is_running:
      self._is_running = False
      self._sleep()

  def _cursors(self) -> Iterable[str]:
    while True:
      yield from Spinner.CURSORS

  def _spin(self) -> None:
    stdout = sys.stdout
    for cursor in self._cursors():
      if not self._is_running:
        return
      # Print the current wait-cursor and send a carriage return to move to the
      # start of the line.
      stdout.write(f" {cursor}\r")
      stdout.flush()
      self._sleep()

  def _sleep(self) -> None:
    time.sleep(self._sleep_time)


class EnumWithHelp(enum.Enum):

  def __new__(cls, value, help_str: str = ""):
    del help_str
    obj = object.__new__(cls)
    obj._value_ = value
    return obj

  def __init__(self, value, help_str: str = "") -> None:
    del value
    assert help_str, "Missing help_str"
    self._help = help_str

  @property
  def help(self) -> str:
    return self._help

  @classmethod
  def help_text_items(cls) -> List[Tuple[str, str]]:
    return [(repr(instance.value), instance.help) for instance in cls]

  @classmethod
  def help_text(cls, indent: int = 0) -> str:
    text: str = tabulate.tabulate(cls.help_text_items(), tablefmt="plain")
    if indent:
      return textwrap.indent(text, " " * indent)
    return text


class StrEnumWithHelp(EnumWithHelp):

  def __str__(self) -> str:
    return str(self.value)


def update_url_query(url: str, query_params: Dict[str, str]) -> str:
  parsed_url = urlparse.urlparse(url)
  query = dict(urlparse.parse_qsl(parsed_url.query))
  query.update(query_params)
  parsed_url = parsed_url._replace(query=urlparse.urlencode(query, doseq=True))
  return parsed_url.geturl()
