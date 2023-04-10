# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import ctypes
import datetime as dt
import json
import logging
from math import floor, log10
import os
import pathlib
import platform as py_platform
import plistlib
import shlex
import shutil
import subprocess
import sys
import textwrap
import threading
import time
import traceback as tb
import urllib
import urllib.error
import urllib.request
from typing import (Any, Callable, Dict, Final, Iterable, Iterator, List,
                    Optional, Sequence, Tuple, TypeVar, Union)

import psutil

if not hasattr(shlex, "join"):
  raise Exception("Please update to python v3.8 that has shlex.join")


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
      logging.DEBUG: FORMAT + " (%(filename)s:%(lineno)d)",
      logging.INFO: TTYColor.GREEN + FORMAT + TTYColor.RESET,
      logging.WARNING: TTYColor.YELLOW + FORMAT + TTYColor.RESET,
      logging.ERROR: TTYColor.RED + FORMAT + TTYColor.RESET,
      logging.CRITICAL: TTYColor.BOLD + TTYColor.RED + FORMAT + TTYColor.RESET,
  }

  def format(self, record):
    log_fmt = self.FORMATS.get(record.levelno)
    formatter = logging.Formatter(log_fmt)
    return formatter.format(record)


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


def format_metric(value: Union[float, int],
                  stddev: Optional[float] = None) -> str:
  """Format value and stdev to only expose significant + 1 digits.
  Example outputs:
    100 ± 10%
    100.1 ± 1.2%
    100.12 ± 0.12%
    100.123 ± 0.012%
    100.1235 ± 0.0012%
  """
  if not stddev:
    return str(value)
  stddev = float(stddev)
  stddev_significant_digit = int(floor(log10(abs(stddev))))
  value_width = max(0, 1 - stddev_significant_digit)
  percent = stddev / value * 100
  percent_significant_digit = int(floor(log10(abs(percent))))
  percent_width = max(0, 1 - percent_significant_digit)
  return f"{value:.{value_width}f} ± {percent:.{percent_width}f}%"


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


class Platform(abc.ABC):

  @property
  @abc.abstractmethod
  def name(self) -> str:
    pass

  @property
  def is_remote(self) -> bool:
    return False

  @property
  def machine(self) -> str:
    return py_platform.machine()

  @property
  def is_ia32(self) -> bool:
    return self.machine in ("i386", "i686", "x86")

  @property
  def is_x64(self) -> bool:
    return self.machine in ("x86_64", "AMD64")

  @property
  def is_arm64(self) -> bool:
    return self.machine in ("arm64", "aarch64")

  @property
  def is_macos(self) -> bool:
    return False

  @property
  def is_linux(self) -> bool:
    return False

  @property
  def is_posix(self) -> bool:
    return self.is_macos or self.is_linux

  @property
  def is_win(self) -> bool:
    return False

  @property
  def is_battery_powered(self) -> bool:
    if not psutil.sensors_battery:
      return False
    status = psutil.sensors_battery()
    if not status:
      return False
    return status.power_plugged

  def search_app(self, app_path: pathlib.Path) -> Optional[pathlib.Path]:
    return self.search_binary(app_path)

  @abc.abstractmethod
  def search_binary(self, app_path: pathlib.Path) -> Optional[pathlib.Path]:
    pass

  @abc.abstractmethod
  def app_version(self, app_path: pathlib.Path) -> str:
    pass

  @property
  def has_display(self) -> bool:
    return True

  def sleep(self, seconds: Union[float, dt.timedelta]) -> None:
    if isinstance(seconds, dt.timedelta):
      seconds = seconds.total_seconds()
    if seconds == 0:
      return
    logging.debug("WAIT %ss", seconds)
    time.sleep(seconds)

  def which(self, binary):
    # TODO(cbruni): support remote platforms
    return shutil.which(binary)

  def processes(self,
                attrs: Optional[List[str]] = None) -> List[Dict[str, Any]]:
    assert not self.is_remote, "Only local platform supported"
    return [
        p.info  # pytype: disable=attribute-error
        for p in psutil.process_iter(attrs=attrs)
    ]

  def process_running(self, process_name_list: List[str]) -> Optional[str]:
    for proc in psutil.process_iter():
      try:
        if proc.name().lower() in process_name_list:
          return proc.name()
      except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
        pass
    return None

  def process_children(self, parent_pid: int,
                       recursive: bool = False) -> List[Dict[str, Any]]:
    return [
        p.as_dict()
        for p in psutil.Process(parent_pid).children(recursive=recursive)
    ]

  def foreground_process(self) -> Optional[Dict[str, Any]]:
    return None

  def sh_stdout(self,
                *args,
                shell: bool = False,
                quiet: bool = False,
                encoding: str = "utf-8") -> str:
    completed_process = self.sh(
        *args, shell=shell, capture_output=True, quiet=quiet)
    return completed_process.stdout.decode(encoding)

  def popen(self,
            *args,
            shell: bool = False,
            stdout=None,
            stderr=None,
            stdin=None,
            env=None,
            quiet: bool = False) -> subprocess.Popen:
    if not quiet:
      logging.debug("SHELL: %s", shlex.join(map(str, args)))
      logging.debug("CWD: %s", os.getcwd())
    return subprocess.Popen(
        args=args,
        shell=shell,
        stdin=stdin,
        stderr=stderr,
        stdout=stdout,
        env=env)

  def sh(self,
         *args,
         shell: bool = False,
         capture_output: bool = False,
         stdout=None,
         stderr=None,
         stdin=None,
         env=None,
         quiet=False) -> subprocess.CompletedProcess:
    if not quiet:
      logging.debug("SHELL: %s", shlex.join(map(str, args)))
      logging.debug("CWD: %s", os.getcwd())
    process = subprocess.run(
        args=args,
        shell=shell,
        stdin=stdin,
        stdout=stdout,
        stderr=stderr,
        env=env,
        capture_output=capture_output,
        check=False)
    if process.returncode != 0:
      raise SubprocessError(process)
    return process

  def exec_apple_script(self, script: str, quiet: bool = False):
    raise NotImplementedError("AppleScript is only available on MacOS")

  def terminate(self, proc_pid: int) -> None:
    process = psutil.Process(proc_pid)
    for proc in process.children(recursive=True):
      proc.terminate()
    process.terminate()

  def log(self, *messages: Any, level: int = 2) -> None:
    message_str = " ".join(map(str, messages))
    if level == 3:
      level = logging.DEBUG
    if level == 2:
      level = logging.INFO
    if level == 1:
      level = logging.WARNING
    if level == 0:
      level = logging.ERROR
    logging.log(level, message_str)

  # TODO(cbruni): split into separate list_system_monitoring and
  # disable_system_monitoring methods
  def check_system_monitoring(self, disable: bool = False) -> bool:
    # pylint: disable=unused-argument
    return True

  def get_relative_cpu_speed(self) -> float:
    return 1

  def is_thermal_throttled(self) -> bool:
    return self.get_relative_cpu_speed() < 1

  def disk_usage(self, path: pathlib.Path) -> psutil._common.sdiskusage:
    return psutil.disk_usage(str(path))

  def cpu_usage(self) -> float:
    return 1 - psutil.cpu_times_percent().idle / 100

  def cpu_details(self) -> Dict[str, Any]:
    details = {
        "physical cores":
            psutil.cpu_count(logical=False),
        "logical cores":
            psutil.cpu_count(logical=True),
        "usage":
            psutil.cpu_percent(  # pytype: disable=attribute-error
                percpu=True, interval=0.1),
        "total usage":
            psutil.cpu_percent(),
        "system load":
            psutil.getloadavg(),
    }
    try:
      cpu_freq = psutil.cpu_freq()
    except FileNotFoundError:
      # MacOS M1 fail for this some times
      return details
    details.update({
        "max frequency": f"{cpu_freq.max:.2f}Mhz",
        "min frequency": f"{cpu_freq.min:.2f}Mhz",
        "current frequency": f"{cpu_freq.current:.2f}Mhz",
    })
    return details

  def system_details(self) -> Dict[str, Any]:
    return {
        "machine": py_platform.machine(),
        "os": {
            "system": py_platform.system(),
            "release": py_platform.release(),
            "version": py_platform.version(),
            "platform": py_platform.platform(),
        },
        "python": {
            "version": py_platform.python_version(),
            "bits": "64" if sys.maxsize > 2**32 else "32",
        },
        "CPU": self.cpu_details(),
    }

  def download_to(self, url: str, path: pathlib.Path) -> pathlib.Path:
    logging.debug("DOWNLOAD: %s\n       TO: %s", url, path)
    assert not path.exists(), f"Download destination {path} exists already."
    try:
      urllib.request.urlretrieve(url, path)
    except (urllib.error.HTTPError, urllib.error.URLError) as e:
      raise OSError(f"Could not load {url}") from e
    assert path.exists(), (
        f"Downloading {url} failed. Downloaded file {path} doesn't exist.")
    return path

  def concat_files(self, inputs: Iterable[pathlib.Path],
                   output: pathlib.Path) -> pathlib.Path:
    with output.open("w", encoding="utf-8") as output_f:
      for input_file in inputs:
        assert input_file.is_file()
        with input_file.open(encoding="utf-8") as input_f:
          shutil.copyfileobj(input_f, output_f)
    return output

  def set_main_display_brightness(self, brightness_level: int) -> None:
    raise NotImplementedError(
        "Implementation is only available on MacOS for now")

  def get_main_display_brightness(self) -> int:
    raise NotImplementedError(
        "Implementation is only available on MacOS for now")

  def check_autobrightness(self) -> bool:
    raise NotImplementedError(
        "Implementation is only available on MacOS for now")


class SubprocessError(subprocess.CalledProcessError):
  """ Custom version that also prints stderr for debugging"""

  def __init__(self, process):
    super().__init__(process.returncode, shlex.join(map(str, process.args)),
                     process.stdout, process.stderr)

  def __str__(self) -> str:
    super_str = super().__str__()
    if not self.stderr:
      return super_str
    return f"{super_str}\nstderr:{self.stderr.decode()}"


class WinPlatform(Platform):
  SEARCH_PATHS = (
      pathlib.Path("."),
      pathlib.Path(os.path.expandvars("%ProgramFiles%")),
      pathlib.Path(os.path.expandvars("%ProgramFiles(x86)%")),
      pathlib.Path(os.path.expandvars("%APPDATA%")),
      pathlib.Path(os.path.expandvars("%LOCALAPPDATA%")),
  )

  @property
  def is_win(self) -> bool:
    return True

  @property
  def name(self) -> str:
    return "win"

  def search_binary(self, app_path: pathlib.Path) -> Optional[pathlib.Path]:
    if app_path.suffix != ".exe":
      raise ValueError("Expected executable path with '.exe' suffix, "
                       f"but got: '{app_path.name}'")
    for path in self.SEARCH_PATHS:
      # Recreate Path object for easier pyfakefs testing
      result_path = pathlib.Path(path) / app_path
      if result_path.exists():
        return result_path
    return None

  def app_version(self, app_path: pathlib.Path) -> str:
    assert app_path.exists(), f"Binary {app_path} does not exist."
    return self.sh_stdout(
        "powershell", "-command",
        f"(Get-Item '{app_path}').VersionInfo.ProductVersion")


class PosixPlatform(Platform, metaclass=abc.ABCMeta):

  def app_version(self, app_path: pathlib.Path) -> str:
    assert app_path.exists(), f"Binary {app_path} does not exist."
    return self.sh_stdout(app_path, "--version")


class MacOSPlatform(PosixPlatform):
  SEARCH_PATHS = (
      pathlib.Path("."),
      pathlib.Path("/Applications"),
      pathlib.Path.home() / "Applications",
  )

  @property
  def is_macos(self) -> bool:
    return True

  @property
  def name(self) -> str:
    return "macos"

  def _find_app_binary_path(self, app_path: pathlib.Path) -> pathlib.Path:
    assert app_path.suffix == ".app"
    bin_path = app_path / "Contents" / "MacOS" / app_path.stem
    if bin_path.exists():
      return bin_path
    binaries = [path for path in bin_path.parent.iterdir() if path.is_file()]
    if len(binaries) == 1:
      return binaries[0]
    # Fallback to read plist
    plist_path = app_path / "Contents" / "Info.plist"
    assert plist_path.is_file, (
        f"Could not find Info.plist in app bundle: {app_path}")
    with plist_path.open("rb") as f:
      plist = plistlib.load(f)
    bin_path = (
        app_path / "Contents" / "MacOS" /
        plist.get("CFBundleExecutable", app_path.stem))
    if bin_path.is_file():
      return bin_path
    raise Exception(f"Invalid number of binaries candidates found: {binaries}")

  def search_binary(self, app_path: pathlib.Path) -> Optional[pathlib.Path]:
    if app_path.suffix != ".app":
      raise ValueError("Expected app name with '.app' suffix, "
                       f"but got: '{app_path.name}'")
    for search_path in self.SEARCH_PATHS:
      # Recreate Path object for easier pyfakefs testing
      result_path = pathlib.Path(search_path) / app_path
      if not result_path.is_dir():
        continue
      result_path = self._find_app_binary_path(result_path)
      if result_path.exists():
        return result_path
    return None

  def search_app(self, app_path: pathlib.Path) -> Optional[pathlib.Path]:
    binary = self.search_binary(app_path)
    if not binary:
      return None
    # input: /Applications/Safari.app/Contents/MacOS/Safari
    # output: /Applications/Safari.app
    app_path = binary.parents[2]
    assert app_path.suffix == ".app"
    assert app_path.is_dir()
    return app_path

  def app_version(self, app_path: pathlib.Path) -> str:
    assert app_path.exists(), f"Binary {bin} does not exist."

    dot_app_path = None
    current = app_path
    while current != app_path.root:
      if current.suffix == ".app":
        dot_app_path = current
        break
      current = current.parent
    if not dot_app_path:
      # Most likely just a cli tool"
      return self.sh_stdout(app_path, "--version").strip()

    version_string = self.sh_stdout("mdls", "-name", "kMDItemVersion",
                                    dot_app_path).strip()
    # Filter output: 'kMDItemVersion = "14.1"' => '"14.1"'
    _, version_string = version_string.split(" = ", maxsplit=1)
    if version_string != "(null)":
      # Strip quotes: '"14.1"' => '14.1'
      return version_string[1:-1]
    # Backup solution with --version
    maybe_bin_path: Optional[pathlib.Path] = app_path
    if app_path.suffix == ".app":
      maybe_bin_path = self.search_binary(app_path)
    if maybe_bin_path:
      try:
        return self.sh_stdout(maybe_bin_path, "--version").strip()
      except SubprocessError as e:
        logging.debug("Could not use --version: %s", e)
    raise ValueError(f"Could not extract app version: {app_path}")

  def exec_apple_script(self, script: str, quiet: bool = False):
    if not quiet:
      logging.debug("AppleScript: %s", script)
    return self.sh("/usr/bin/osascript", "-e", script)

  def foreground_process(self) -> Optional[Dict[str, Any]]:
    foreground_process_info = self.sh_stdout("lsappinfo", "front").strip()
    if not foreground_process_info:
      return None
    foreground_info = self.sh_stdout("lsappinfo", "info", "-only", "pid",
                                     foreground_process_info).strip()
    _, pid = foreground_info.split("=")
    if pid and pid.isdigit():
      return psutil.Process(int(pid)).as_dict()
    return None

  def get_relative_cpu_speed(self) -> float:
    try:
      lines = self.sh_stdout("pmset", "-g", "therm").split()
      for index, line in enumerate(lines):
        if line == "CPU_Speed_Limit":
          return int(lines[index + 2]) / 100.0
    except SubprocessError:
      logging.debug("Could not get relative PCU speed: %s", tb.format_exc())
    return 1

  def system_details(self) -> Dict[str, Any]:
    details = super().system_details()
    details.update({
        "system_profiler":
            self.sh_stdout("system_profiler", "SPHardwareDataType"),
        "sysctl_machdep_cpu":
            self.sh_stdout("sysctl", "machdep.cpu"),
        "sysctl_hw":
            self.sh_stdout("sysctl", "hw"),
    })
    return details

  def check_system_monitoring(self, disable: bool = False) -> bool:
    return self.check_crowdstrike(disable)

  def check_autobrightness(self) -> bool:
    output = self.sh_stdout("system_profiler", "SPDisplaysDataType", "-json").strip()
    data = json.loads(output)
    if spdisplays_data := data.get("SPDisplaysDataType"):
      for data in spdisplays_data:
        if spdisplays_ndrvs := data.get("spdisplays_ndrvs"):
          for display in spdisplays_ndrvs:
            if auto_brightness := display.get("spdisplays_ambient_brightness"):
              return  auto_brightness == "spdisplays_yes"
        raise Exception("Could not find 'spdisplays_ndrvs' from SPDisplaysDataType")
    raise Exception("Could not get 'SPDisplaysDataType' form system profiler")

  def check_crowdstrike(self, disable: bool = False) -> bool:
    falconctl = pathlib.Path(
        "/Applications/Falcon.app/Contents/Resources/falconctl")
    if not falconctl.exists():
      logging.debug("You're fine, falconctl or %s are not installed.",
                    falconctl)
      return True
    if not disable:
      for process in self.processes(attrs=["exe"]):
        exe = process["exe"]
        if exe and exe.endswith("/com.crowdstrike.falcon.Agent"):
          return False
      return True
    try:
      logging.warning("Checking falcon sensor status:")
      status = self.sh_stdout("sudo", falconctl, "stats", "agent_info")
    except SubprocessError:
      return True
    if "operational: true" not in status:
      # Early return if not running, no need to disable the sensor.
      return True
    # Try disabling the process
    logging.warning("Disabling crowdstrike monitoring:")
    self.sh("sudo", falconctl, "unload")
    return True

  def _get_display_service(self):
    core_graphics = ctypes.CDLL(
        "/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics")
    main_display = core_graphics.CGMainDisplayID()
    display_services = ctypes.CDLL(
        "/System/Library/PrivateFrameworks/DisplayServices.framework"
        "/DisplayServices")
    display_services.DisplayServicesSetBrightness.argtypes = [
        ctypes.c_int, ctypes.c_float
    ]
    display_services.DisplayServicesGetBrightness.argtypes = [
        ctypes.c_int, ctypes.POINTER(ctypes.c_float)
    ]
    return display_services, main_display

  def set_main_display_brightness(self, brightness_level: int) -> None:
    """Sets the main display brightness at the specified percentage by
    brightness_level.

    This function imitates the open-source "brightness" tool at
    https://github.com/nriley/brightness.
    Since the benchmark doesn't care about older MacOSen, multiple displays
    or other complications that tool has to consider, setting the brightness
    level boils down to calling this function for the main display.

    Args:
      brightness_level: Percentage at which we want to set screen brightness.

    Raises:
      AssertionError: An error occurred when we tried to set the brightness
    """
    display_services, main_display = self._get_display_service()
    ret = display_services.DisplayServicesSetBrightness(main_display,
                                                        brightness_level / 100)
    assert ret == 0

  def get_main_display_brightness(self) -> int:
    """Gets the current brightness level of the main display .

    This function imitates the open-source "brightness" tool at
    https://github.com/nriley/brightness.
    Since the benchmark doesn't care about older MacOSen, multiple displays
    or other complications that tool has to consider, setting the brightness
    level boils down to calling this function for the main display.

    Returns:
      An int of the current percentage value of the main screen brightness

    Raises:
      AssertionError: An error occurred when we tried to set the brightness
    """

    display_services, main_display = self._get_display_service()
    display_brightness = ctypes.c_float()
    ret = display_services.DisplayServicesGetBrightness(
        main_display, ctypes.byref(display_brightness))
    assert ret == 0
    return round(display_brightness.value * 100)


class LinuxPlatform(PosixPlatform):
  SEARCH_PATHS = (
      pathlib.Path("."),
      pathlib.Path("/usr/local/sbin"),
      pathlib.Path("/usr/local/bin"),
      pathlib.Path("/usr/sbin"),
      pathlib.Path("/usr/bin"),
      pathlib.Path("/sbin"),
      pathlib.Path("/bin"),
      pathlib.Path("/opt/google"),
  )

  @property
  def is_linux(self) -> bool:
    return True

  @property
  def name(self) -> str:
    return "linux"

  def check_system_monitoring(self, disable: bool = False) -> bool:
    return True

  @property
  def has_display(self) -> bool:
    return "DISPLAY" in os.environ

  def system_details(self) -> Dict[str, Any]:
    details = super().system_details()
    for info_bin in ("lscpu", "inxi"):
      if self.which(info_bin):
        details[info_bin] = self.sh_stdout(info_bin)
    return details

  def search_binary(self, app_path: pathlib.Path) -> Optional[pathlib.Path]:
    for path in self.SEARCH_PATHS:
      # Recreate Path object for easier pyfakefs testing
      result_path = pathlib.Path(path) / app_path
      if result_path.exists():
        return result_path
    return None


if sys.platform == "linux":
  platform = LinuxPlatform()
elif sys.platform == "darwin":
  platform = MacOSPlatform()
elif sys.platform == "win32":
  platform = WinPlatform()
else:
  raise Exception("Unsupported Platform")

log = platform.log


def search_app_or_executable(name: str,
                             macos: Sequence[str] = (),
                             win: Sequence[str] = (),
                             linux: Sequence[str] = ()) -> pathlib.Path:
  executables: Sequence[str] = []
  if platform.is_macos:
    executables = macos
  elif platform.is_win:
    executables = win
  elif platform.is_linux:
    executables = linux

  if not executables:
    raise ValueError(
        f"Executable {name} not supported on platform {platform.name}")
  for name_or_path in executables:
    binary = platform.search_app(pathlib.Path(name_or_path))
    if binary and binary.exists():
      return binary
  raise Exception(f"Executable {name} not found on {platform.name}")

# =============================================================================


def urlopen(url: str):
  try:
    return urllib.request.urlopen(url)
  except urllib.error.HTTPError as e:
    log(f"Could not load url={url}")
    raise e


# =============================================================================


class ChangeCWD:

  def __init__(self, destination: pathlib.Path):
    self.new_dir = destination
    self.prev_dir: Optional[str] = None

  def __enter__(self):
    self.prev_dir = os.getcwd()
    os.chdir(self.new_dir)

  def __exit__(self, exc_type, exc_value, exc_traceback):
    os.chdir(self.prev_dir)


class SystemSleepPreventer:
  """
  Prevent the system from going to sleep while running the benchmark.
  """

  def __init__(self):
    self._process = None

  def __enter__(self):
    if platform.is_macos:
      self._process = platform.popen("caffeinate", "-imdsu")
    # TODO: Add linux support

  def __exit__(self, exc_type, exc_value, exc_traceback):
    if self._process is not None:
      self._process.kill()


class TimeScope:
  """
  Measures and logs the time spend during the lifetime of the TimeScope.
  """

  def __init__(self, message: str, level: int = 3):
    self._message = message
    self._level = level
    self._start: Optional[dt.datetime] = None

  @property
  def message(self) -> str:
    return self._message

  def __enter__(self):
    self._start = dt.datetime.now()

  def __exit__(self, exc_type, exc_value, exc_traceback):
    assert self._start
    diff = dt.datetime.now() - self._start
    log(f"{self._message} duration={diff}", level=self._level)


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
      max_iterations: Optional[int] = None):
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
  duration = 0
  for sleep_for in wait_range:
    duration = dt.datetime.now() - start
    if duration > wait_range.timeout:
      raise TimeoutError(f"Waited for {duration}")
    time_left = timeout - duration
    yield duration.total_seconds(), time_left.total_seconds()
    platform.sleep(sleep_for.total_seconds())


class Durations:
  """
  Helper object to track durations.
  """

  class _DurationMeasureContext:

    def __init__(self, durations: Durations, name: str):
      self._start_time = None
      self._durations = durations
      self._name = name

    def __enter__(self):
      self._start_time = dt.datetime.now()

    def __exit__(self, exc_type, exc_value, traceback):
      assert self._start_time
      delta = dt.datetime.now() - self._start_time
      self._durations[self._name] = delta

  def __init__(self):
    self._durations: Dict[str, dt.timedelta] = {}

  def __getitem__(self, name: str) -> dt.timedelta:
    return self._durations[name]

  def __setitem__(self, name: str, duration: dt.timedelta) -> None:
    assert name not in self._durations, (f"Cannot set '{name}' duration twice!")
    self._durations[name] = duration

  def __len__(self) -> int:
    return len(self._durations)

  def measure(self, name: str) -> _DurationMeasureContext:
    assert name not in self._durations, (
        f"Cannot measure '{name}' duration twice!")
    return self._DurationMeasureContext(self, name)

  def to_json(self) -> Dict[str, float]:
    return {
        name: self._durations[name].total_seconds()
        for name in sorted(self._durations.keys())
    }


def wrap_lines(body: str, width: int = 80, indent: str = "") -> Iterable[str]:
  for line in body.splitlines():
    for split in textwrap.wrap(line, width):
      yield f"{indent}{split}"


class Spinner:
  CURSORS = "◐◓◑◒"

  def __init__(self, sleep: float = 0.5):
    self._is_running = False
    self._sleep_time = sleep

  def __enter__(self):
    # Only enable the spinner if the output is an interactive terminal.
    is_atty = hasattr(sys.stdout, "isatty") and sys.stdout.isatty()
    if is_atty:
      self._is_running = True
      threading.Thread(target=self._spin).start()

  def __exit__(self, exc_type, exc_value, traceback):
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
