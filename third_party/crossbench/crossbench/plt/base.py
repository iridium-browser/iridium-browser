# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import collections.abc
import datetime as dt
import logging
import os
import pathlib
import platform as py_platform
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
from typing import (TYPE_CHECKING, Any, Dict, Iterable, Iterator, List, Mapping,
                    Optional, Tuple, Union)

import psutil

from .arch import MachineArch

if TYPE_CHECKING:
  from crossbench.types import JsonDict


class Environ(collections.abc.MutableMapping, metaclass=abc.ABCMeta):
  pass


class LocalEnviron(Environ):

  def __init__(self) -> None:
    self._environ = os.environ

  def __getitem__(self, key: str) -> str:
    return self._environ.__getitem__(key)

  def __setitem__(self, key: str, item: str) -> None:
    self._environ.__setitem__(key, item)

  def __delitem__(self, key: str) -> None:
    self._environ.__delitem__(key)

  def __iter__(self) -> Iterator[str]:
    return self._environ.__iter__()

  def __len__(self) -> int:
    return self._environ.__len__()


class SubprocessError(subprocess.CalledProcessError):
  """ Custom version that also prints stderr for debugging"""

  def __init__(self, platform: Platform, process) -> None:
    self.platform = platform
    super().__init__(process.returncode, shlex.join(map(str, process.args)),
                     process.stdout, process.stderr)

  def __str__(self) -> str:
    super_str = super().__str__()
    if not self.stderr:
      return super_str
    return f"{self.platform}: {super_str}\nstderr:{self.stderr.decode()}"


class Platform(abc.ABC):
  # pylint: disable=locally-disabled, redefined-builtin

  @property
  @abc.abstractmethod
  def name(self) -> str:
    pass

  @property
  @abc.abstractmethod
  def version(self) -> str:
    pass

  @property
  @abc.abstractmethod
  def device(self) -> str:
    pass

  @property
  @abc.abstractmethod
  def cpu(self) -> str:
    pass

  @property
  def full_version(self) -> str:
    return f"{self.name} {self.version} {self.machine}"

  def __str__(self) -> str:
    return ".".join(self.key) + (".remote" if self.is_remote else ".local")

  @property
  def is_remote(self) -> bool:
    return False

  @property
  def host_platform(self) -> Platform:
    return self

  @property
  def machine(self) -> MachineArch:
    raw = self._raw_machine_arch()
    if raw in ("i386", "i686", "x86", "ia32"):
      return MachineArch.IA32
    if raw in ("x86_64", "AMD64"):
      return MachineArch.X64
    if raw in ("arm64", "aarch64"):
      return MachineArch.ARM_64
    if raw in ("arm"):
      return MachineArch.ARM_32
    raise NotImplementedError(f"Unsupported machine type: {raw}")

  def _raw_machine_arch(self) -> str:
    assert not self.is_remote, "Unsupported operation on remote platform"
    return py_platform.machine()

  @property
  def is_ia32(self) -> bool:
    return self.machine == MachineArch.IA32

  @property
  def is_x64(self) -> bool:
    return self.machine == MachineArch.X64

  @property
  def is_arm64(self) -> bool:
    return self.machine == MachineArch.ARM_64

  @property
  def key(self) -> Tuple[str, str]:
    return (self.name, str(self.machine))

  @property
  def is_macos(self) -> bool:
    return False

  @property
  def is_linux(self) -> bool:
    return False

  @property
  def is_android(self) -> bool:
    return False

  @property
  def is_posix(self) -> bool:
    return self.is_macos or self.is_linux or self.is_android

  @property
  def is_win(self) -> bool:
    return False

  @property
  def environ(self) -> Environ:
    assert not self.is_remote, "Unsupported operation on remote platform"
    return LocalEnviron()

  @property
  def is_battery_powered(self) -> bool:
    assert not self.is_remote, "Unsupported operation on remote platform"
    if not psutil.sensors_battery:
      return False
    status = psutil.sensors_battery()
    if not status:
      return False
    return not status.power_plugged

  def search_app(self, app_or_bin: pathlib.Path) -> Optional[pathlib.Path]:
    """Look up a application bundle (macos) or binary (all other platforms) in 
    the common search paths.
    """
    return self.search_binary(app_or_bin)

  @abc.abstractmethod
  def search_binary(self, app_or_bin: pathlib.Path) -> Optional[pathlib.Path]:
    """Look up a binary in the common search paths based of a path or a single
    segment path with just the binary name.
    Returns the location of the binary (and not the .app bundle on macOS).
    """

  @abc.abstractmethod
  def app_version(self, app_or_bin: pathlib.Path) -> str:
    pass

  @property
  def has_display(self) -> bool:
    """Return a bool whether the platform has an active display.
    This can be false on linux without $DISPLAY, true an all other platforms."""
    return True

  def sleep(self, seconds: Union[int, float, dt.timedelta]) -> None:
    if isinstance(seconds, dt.timedelta):
      seconds = seconds.total_seconds()
    if seconds == 0:
      return
    logging.debug("WAIT %ss", seconds)
    time.sleep(seconds)

  def which(self, binary_name: str) -> Optional[pathlib.Path]:
    if not binary_name:
      raise ValueError("Got empty path")
    assert not self.is_remote, "Unsupported operation on remote platform"
    result = shutil.which(binary_name)
    if not result:
      return None
    return pathlib.Path(result)

  def processes(self,
                attrs: Optional[List[str]] = None) -> List[Dict[str, Any]]:
    # TODO(cbruni): support remote platforms
    assert not self.is_remote, "Only local platform supported"
    return [
        p.info  # pytype: disable=attribute-error
        for p in psutil.process_iter(attrs=attrs)
    ]

  def process_running(self, process_name_list: List[str]) -> Optional[str]:
    assert not self.is_remote, "Unsupported operation on remote platform"
    # TODO(cbruni): support remote platforms
    for proc in psutil.process_iter():
      try:
        if proc.name().lower() in process_name_list:
          return proc.name()
      except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
        pass
    return None

  def process_children(self,
                       parent_pid: int,
                       recursive: bool = False) -> List[Dict[str, Any]]:
    assert not self.is_remote, "Unsupported operation on remote platform"
    # TODO(cbruni): support remote platforms
    try:
      process = psutil.Process(parent_pid)
    except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
      return []
    return [p.as_dict() for p in process.children(recursive=recursive)]

  def process_info(self, pid: int) -> Optional[Dict[str, Any]]:
    assert not self.is_remote, "Unsupported operation on remote platform"
    # TODO(cbruni): support remote platforms
    try:
      return psutil.Process(pid).as_dict()
    except psutil.NoSuchProcess:
      return None

  def foreground_process(self) -> Optional[Dict[str, Any]]:
    return None

  def terminate(self, proc_pid: int) -> None:
    assert not self.is_remote, "Unsupported operation on remote platform"
    # TODO(cbruni): support remote platforms
    process = psutil.Process(proc_pid)
    for proc in process.children(recursive=True):
      proc.terminate()
    process.terminate()

  @property
  def default_tmp_dir(self) -> pathlib.Path:
    assert not self.is_remote, "Unsupported operation on remote platform"
    return pathlib.Path(tempfile.gettempdir())

  def cat(self, file: Union[str, pathlib.Path], encoding: str = "utf-8") -> str:
    """Meow! I return the file contents as a str."""
    assert not self.is_remote, "Unsupported operation on remote platform"
    with pathlib.Path(file).open(encoding=encoding) as f:
      return f.read()

  def rsync(self, from_path: pathlib.Path,
            to_path: pathlib.Path) -> pathlib.Path:
    """ Convenience implementation that works for copying local dirs """
    assert not self.is_remote, "Unsupported operation on remote platform"
    if not from_path.exists():
      raise ValueError(f"Cannot copy non-existing source path: {from_path}")
    to_path.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(from_path, to_path)
    return to_path

  def rm(self, path: Union[str, pathlib.Path], dir: bool = False) -> None:
    """Remove a single file on this platform."""
    assert not self.is_remote, "Unsupported operation on remote platform"
    if dir:
      shutil.rmtree(path)
    else:
      pathlib.Path(path).unlink()

  def mkdir(self, path: Union[str, pathlib.Path]) -> None:
    assert not self.is_remote, "Unsupported operation on remote platform"
    pathlib.Path(path).mkdir(parents=True, exist_ok=True)

  def mkdtemp(self,
              prefix: Optional[str] = None,
              dir: Optional[Union[str, pathlib.Path]] = None) -> pathlib.Path:
    assert not self.is_remote, "Unsupported operation on remote platform"
    return pathlib.Path(tempfile.mkdtemp(prefix=prefix, dir=dir))

  def mktemp(self,
             prefix: Optional[str] = None,
             dir: Optional[Union[str, pathlib.Path]] = None) -> pathlib.Path:
    assert not self.is_remote, "Unsupported operation on remote platform"
    fd, name = tempfile.mkstemp(prefix=prefix, dir=dir)
    os.close(fd)
    return pathlib.Path(name)

  def exists(self, path: pathlib.Path) -> bool:
    assert not self.is_remote, "Unsupported operation on remote platform"
    return path.exists()

  def is_file(self, path: pathlib.Path) -> bool:
    assert not self.is_remote, "Unsupported operation on remote platform"
    return path.is_file()

  def is_dir(self, path: pathlib.Path) -> bool:
    assert not self.is_remote, "Unsupported operation on remote platform"
    return path.is_dir()

  def sh_stdout(self,
                *args: Union[str, pathlib.Path],
                shell: bool = False,
                quiet: bool = False,
                encoding: str = "utf-8",
                env: Optional[Mapping[str, str]] = None,
                check: bool = True) -> str:
    completed_process = self.sh(
        *args,
        shell=shell,
        capture_output=True,
        quiet=quiet,
        env=env,
        check=check)
    return completed_process.stdout.decode(encoding)

  def popen(self,
            *args: Union[str, pathlib.Path],
            shell: bool = False,
            stdout=None,
            stderr=None,
            stdin=None,
            env: Optional[Mapping[str, str]] = None,
            quiet: bool = False) -> subprocess.Popen:
    assert not self.is_remote, "Unsupported operation on remote platform"
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
         *args: Union[str, pathlib.Path],
         shell: bool = False,
         capture_output: bool = False,
         stdout=None,
         stderr=None,
         stdin=None,
         env: Optional[Mapping[str, str]] = None,
         quiet: bool = False,
         check: bool = True) -> subprocess.CompletedProcess:
    assert not self.is_remote, "Unsupported operation on remote platform"
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
    if check and process.returncode != 0:
      raise SubprocessError(self, process)
    return process

  def exec_apple_script(self, script: str) -> str:
    raise NotImplementedError("AppleScript is only available on MacOS")

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
    assert not self.is_remote, "Unsupported operation on remote platform"
    return psutil.disk_usage(str(path))

  def cpu_usage(self) -> float:
    assert not self.is_remote, "Unsupported operation on remote platform"
    return 1 - psutil.cpu_times_percent().idle / 100

  def cpu_details(self) -> Dict[str, Any]:
    assert not self.is_remote, "Unsupported operation on remote platform"
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
        "info":
            self.cpu,
    }
    try:
      cpu_freq = psutil.cpu_freq()
    except FileNotFoundError as e:
      logging.debug("psutil.cpu_freq() failed (normal on macOS M1): %s", e)
      return details
    details.update({
        "max frequency": f"{cpu_freq.max:.2f}Mhz",
        "min frequency": f"{cpu_freq.min:.2f}Mhz",
        "current frequency": f"{cpu_freq.current:.2f}Mhz",
    })
    return details

  def system_details(self) -> Dict[str, Any]:
    return {
        "machine": str(self.machine),
        "os": self.os_details(),
        "python": self.python_details(),
        "CPU": self.cpu_details(),
    }

  def os_details(self) -> JsonDict:
    assert not self.is_remote, "Unsupported operation on remote platform"
    return {
        "system": py_platform.system(),
        "release": py_platform.release(),
        "version": py_platform.version(),
        "platform": py_platform.platform(),
    }

  def python_details(self) -> JsonDict:
    assert not self.is_remote, "Unsupported operation on remote platform"
    return {
        "version": py_platform.python_version(),
        "bits": 64 if sys.maxsize > 2**32 else 32,
    }

  def download_to(self, url: str, path: pathlib.Path) -> pathlib.Path:
    assert not self.is_remote, "Unsupported operation on remote platform"
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
    assert not self.is_remote, "Unsupported operation on remote platform"
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
