# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import pathlib
import re
from typing import Any, Dict, Iterator, List, Optional, Union

from crossbench.types import JsonDict

from .base import Environ, Platform, SubprocessError


class PosixPlatform(Platform, metaclass=abc.ABCMeta):
  # pylint: disable=locally-disabled, redefined-builtin

  def __init__(self) -> None:
    self._version = ""
    self._device = ""
    self._cpu = ""
    self._default_tmp_dir = pathlib.Path("")

  def app_version(self, app_or_bin: pathlib.Path) -> str:
    assert app_or_bin.exists(), f"Binary {app_or_bin} does not exist."
    return self.sh_stdout(app_or_bin, "--version")

  @property
  def version(self) -> str:
    if not self._version:
      self._version = self.sh_stdout("uname", "-r").strip()
    return self._version

  def _raw_machine_arch(self):
    if not self.is_remote:
      return super()._raw_machine_arch()
    return self.sh_stdout("arch").strip()

  _GET_CPONF_PROC_RE = re.compile(r".*PROCESSORS_CONF[^0-9]+(?P<cores>[0-9]+)")

  def cpu_details(self) -> Dict[str, Any]:
    if not self.is_remote:
      return super().cpu_details()
    cores = -1
    if self.which("nproc"):
      cores = int(self.sh_stdout("nproc"))
    elif self.which("getconf"):
      result = self._GET_CPONF_PROC_RE.search(self.sh_stdout("getconf", "-a"))
      if result:
        cores = int(result["cores"])
    return {
        "physical cores": cores,
        "info": self.cpu,
    }

  def os_details(self) -> JsonDict:
    if not self.is_remote:
      return super().os_details()
    return {
        "system": self.sh_stdout("uname").strip(),
        "release": self.sh_stdout("uname", "-r").strip(),
        "version": self.sh_stdout("uname", "-v").strip(),
        "platform": self.sh_stdout("uname", "-a").strip(),
    }

  _PY_VERSION = "import sys; print(64 if sys.maxsize > 2**32 else 32)"

  def python_details(self) -> JsonDict:
    if not self.is_remote:
      return super().python_details()
    if not self.which("python3"):
      return {"version": "unknown", "bits": 64}
    return {
        "version": self.sh_stdout("python3", "--version").strip(),
        "bits": int(self.sh_stdout("python3", "-c", self._PY_VERSION).strip())
    }

  @property
  def default_tmp_dir(self) -> pathlib.Path:
    if self._default_tmp_dir.parts:
      return self._default_tmp_dir
    if not self.is_remote:
      self._default_tmp_dir = super().default_tmp_dir
      return self._default_tmp_dir
    env = self.environ

    for tmp_var in ("TMPDIR", "TEMP", "TMP"):
      if tmp_var not in env:
        continue
      tmp_path = pathlib.Path(env[tmp_var])
      if self.is_dir(tmp_path):
        self._default_tmp_dir = tmp_path
        return tmp_path
    self._default_tmp_dir = pathlib.Path("/tmp")
    assert self._default_tmp_dir.is_dir(), (
        f"Fallback tmp dir does not exist: {self._default_tmp_dir}")
    return self._default_tmp_dir

  def which(self, binary_name: str) -> Optional[pathlib.Path]:
    if not self.is_remote:
      return super().which(binary_name)
    if not binary_name:
      raise ValueError("Got empty path")
    try:
      maybe_bin = pathlib.Path(self.sh_stdout("which", binary_name).strip())
      if maybe_bin.exists():
        return maybe_bin
    except SubprocessError:
      pass
    return None

  def cat(self, file: Union[str, pathlib.Path], encoding: str = "utf-8") -> str:
    if not self.is_remote:
      return super().cat(file, encoding)
    return self.sh_stdout("cat", file, encoding=encoding)

  def rm(self, path: Union[str, pathlib.Path], dir: bool = False) -> None:
    if not self.is_remote:
      super().rm(path, dir)
    elif dir:
      self.sh("rm", "-rf", path)
    else:
      self.sh("rm", path)

  def mkdir(self, path: Union[str, pathlib.Path]) -> None:
    if not self.is_remote:
      super().mkdir(path)
    else:
      self.sh("mkdir", "-p", path)

  def mkdtemp(self,
              prefix: Optional[str] = None,
              dir: Optional[Union[str, pathlib.Path]] = None) -> pathlib.Path:
    if not self.is_remote:
      return super().mkdtemp(prefix, dir)
    return self._mktemp_sh(is_dir=True, prefix=prefix, dir=dir)

  def mktemp(self,
             prefix: Optional[str] = None,
             dir: Optional[Union[str, pathlib.Path]] = None) -> pathlib.Path:
    if not self.is_remote:
      return super().mktemp(prefix, dir)
    return self._mktemp_sh(is_dir=False, prefix=prefix, dir=dir)

  def _mktemp_sh(self, is_dir: bool, prefix: Optional[str],
                 dir: Optional[Union[str, pathlib.Path]]) -> pathlib.Path:
    if not dir:
      dir = self.default_tmp_dir
    template = pathlib.Path(dir) / f"{prefix}.XXXXXXXXXXX"
    args: List[str] = ["mktemp"]
    if is_dir:
      args.append("-d")
    args.append(str(template))
    result = self.sh_stdout(*args)
    return pathlib.Path(result.strip())

  def exists(self, path: pathlib.Path) -> bool:
    if not self.is_remote:
      return super().exists(path)
    return self.sh("[", "-e", path, "]", check=False).returncode == 0

  def is_file(self, path: pathlib.Path) -> bool:
    if not self.is_remote:
      return super().is_file(path)
    return self.sh("[", "-f", path, "]", check=False).returncode == 0

  def is_dir(self, path: pathlib.Path) -> bool:
    if not self.is_remote:
      return super().is_dir(path)
    return self.sh("[", "-d", path, "]", check=False).returncode == 0

  @property
  def environ(self) -> Environ:
    if not self.is_remote:
      return super().environ
    return RemotePosixEnviron(self)


class RemotePosixEnviron(Environ):

  def __init__(self, platform: PosixPlatform) -> None:
    self._platform = platform
    self._environ = dict(
        line.split("=", maxsplit=1)
        for line in self._platform.sh_stdout("env").splitlines()
        if line)

  def __getitem__(self, key: str) -> str:
    return self._environ.__getitem__(key)

  def __setitem__(self, key: str, item: str) -> None:
    raise NotImplementedError("Unsupported")

  def __delitem__(self, key: str) -> None:
    raise NotImplementedError("Unsupported")

  def __iter__(self) -> Iterator[str]:
    return self._environ.__iter__()

  def __len__(self) -> int:
    return self._environ.__len__()
