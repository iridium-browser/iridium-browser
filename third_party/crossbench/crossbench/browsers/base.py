# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import html
import logging
import pathlib
import re
import shutil
import urllib.parse
import urllib.request
from typing import TYPE_CHECKING, Any, Dict, Optional, Sequence, Set

from crossbench import helper
from crossbench.flags import Flags

if TYPE_CHECKING:
  import datetime as dt

  from crossbench.probes.base import Probe
  from crossbench.runner import Run, Runner

# =============================================================================

BROWSERS_CACHE = pathlib.Path(__file__).parent.parent / ".browsers-cache"

# =============================================================================


class Browser(abc.ABC):

  @classmethod
  def default_flags(cls, initial_data: Flags.InitialDataType = None) -> Flags:
    return Flags(initial_data)

  def __init__(
      self,
      label: str,
      path: Optional[pathlib.Path],
      flags: Flags.InitialDataType = None,
      cache_dir: Optional[pathlib.Path] = None,
      type: Optional[str] = None,  # pylint: disable=redefined-builtin
      platform: Optional[helper.Platform] = None):
    self.platform = platform or helper.platform
    # Marked optional to make subclass constructor calls easier with pytype.
    assert type
    self.type: str = type
    self.label: str = label
    self._unique_name: str = ""
    self.path: Optional[pathlib.Path] = path
    self.app_name: str = type
    self.version: str = "custom"
    self.major_version: int = 0
    if path:
      self.path = self._resolve_binary(path)
      assert self.path.is_absolute()
      self.version = self._extract_version()
      self.major_version = int(self.version.split(".")[0])
      self.unique_name = f"{self.type}_v{self.major_version}_{self.label}"
    else:
      self.unique_name = f"{self.type}_{self.label}".lower()
    self.width: int = 1500
    self.height: int = 1000
    self.x: int = 10
    # Move down to avoid menu bars
    self.y: int = 50
    # TODO: Use WindowSize object
    self._start_fullscreen: bool = False
    self._is_running: bool = False
    self.cache_dir: Optional[pathlib.Path] = cache_dir
    self.clear_cache_dir: bool = True
    self._pid: Optional[int] = None
    self._probes: Set[Probe] = set()
    self._flags: Flags = self.default_flags(flags)
    self.log_file: Optional[pathlib.Path] = None

  @property
  def unique_name(self) -> str:
    return self._unique_name

  @unique_name.setter
  def unique_name(self, name: str) -> None:
    assert name
    # Replace any potentially unsafe chars in the name
    self._unique_name = re.sub(r"[^\w\d\-\.]", "_", name).lower()

  @property
  def is_headless(self) -> bool:
    return False

  @property
  def flags(self) -> Flags:
    return self._flags

  def user_agent(self, runner: Runner) -> str:
    return self.js(runner, "return window.navigator.userAgent")

  @property
  def pid(self) -> Optional[int]:
    return self._pid

  @property
  def is_local(self) -> bool:
    return True

  def set_log_file(self, path: pathlib.Path):
    self.log_file = path

  @property
  def stdout_log_file(self) -> pathlib.Path:
    assert self.log_file
    return self.log_file.with_suffix(".stdout.log")

  def _resolve_binary(self, path: pathlib.Path) -> pathlib.Path:
    path = path.absolute()
    assert path.exists(), f"Binary at path={path} does not exist."
    self.app_path = path
    self.app_name = self.app_path.stem
    if self.platform.is_macos:
      path = self._resolve_macos_binary(path)
    assert path.is_file(), (f"Binary at path={path} is not a file.")
    return path

  def _resolve_macos_binary(self, path: pathlib.Path) -> pathlib.Path:
    assert self.platform.is_macos
    candidate = self.platform.search_binary(path)
    if not candidate or not candidate.is_file():
      raise ValueError(f"Could not find browser executable in {path}")
    return candidate

  def attach_probe(self, probe: Probe) -> None:
    self._probes.add(probe)
    probe.attach(self)

  def details_json(self) -> Dict[str, Any]:
    return {
        "label": self.label,
        "browser": self.type,
        "unique_name": self.unique_name,
        "app_name": self.app_name,
        "version": self.version,
        "flags": tuple(self.flags.get_list()),
        "js_flags": tuple(),
        "path": str(self.path),
        "clear_cache_dir": self.clear_cache_dir,
        "major_version": self.major_version,
        "log": {}
    }

  def setup_binary(self, runner: Runner) -> None:
    pass

  def setup(self, run: Run) -> None:
    assert not self._is_running
    runner = run.runner
    self.clear_cache(runner)
    self.start(run)
    assert self._is_running
    self._prepare_temperature(run)
    self.show_url(runner, self.info_data_url(run))
    runner.wait(2)

  @abc.abstractmethod
  def _extract_version(self) -> str:
    pass

  def clear_cache(self, runner: Runner) -> None:
    del runner
    if self.clear_cache_dir and self.cache_dir and self.cache_dir.exists():
      shutil.rmtree(self.cache_dir)

  @abc.abstractmethod
  def start(self, run: Run) -> None:
    pass

  def _prepare_temperature(self, run: Run) -> None:
    """Warms up the browser by loading the page 3 times."""
    runner = run.runner
    if run.temperature != "cold" and run.temperature:
      for _ in range(3):
        # TODO(cbruni): add no_collect argument
        run.story.run(run)
        runner.wait(run.story.duration / 2)
        self.show_url(runner, "about:blank")
        runner.wait(1)

  def info_data_url(self, run: Run) -> str:
    page = ("<html><head>"
            "<title>Browser Details</title>"
            "<style>"
            """
            html { font-family: sans-serif; }
            dl {
              display: grid;
              grid-template-columns: max-content auto;
            }
            dt { grid-column-start: 1; }
            dd { grid-column-start: 2;  font-family: monospace; }
            """
            "</style>"
            "<head><body>"
            "<h1>"
            f"{html.escape(self.app_name.title())} {html.escape(self.version)}"
            "</h1>")
    page += (
        "<h2>Browser Details</h2>"
        "<dl>"
        f"<dt>UserAgent</dt><dd>{html.escape(self.user_agent(run.runner))}</dd>"
    )
    for property_name, value in self.details_json().items():
      page += f"<dt>{html.escape(property_name)}</dt>"
      page += f"<dd>{html.escape(str(value))}</dd>"
    page += "</dl>"
    page += "<h2>Run Details</h2><dl>"
    for property_name, value in run.details_json().items():
      page += f"<dt>{html.escape(property_name)}</dt>"
      page += f"<dd>{html.escape(str(value))}</dd>"
    page += "</dl>"
    page += "</body></html>"
    data_url = f"data:text/html;charset=utf-8,{urllib.parse.quote(page)}"
    return data_url

  def quit(self, runner: Runner) -> None:
    del runner
    assert self._is_running
    try:
      self.force_quit()
    finally:
      self._pid = None

  def force_quit(self) -> None:
    logging.info("QUIT")
    if self.platform.is_macos:
      self.platform.exec_apple_script(f"""
  tell application "{self.app_name}"
    quit
  end tell
      """)
    elif self._pid:
      self.platform.terminate(self._pid)
    self._is_running = False

  @abc.abstractmethod
  def js(self,
         runner: Runner,
         script: str,
         timeout: Optional[dt.timedelta] = None,
         arguments: Sequence[object] = ()) -> Any:
    pass

  @abc.abstractmethod
  def show_url(self, runner: Runner, url: str) -> None:
    pass


_FLAG_TO_PATH_RE = re.compile(r"[-/\\:\.]")


def convert_flags_to_label(*flags: str, index: Optional[int] = None) -> str:
  label = "default"
  if flags:
    label = _FLAG_TO_PATH_RE.sub("_", "_".join(flags).replace("--", ""))
  if index is None:
    return label
  return f"{str(index).rjust(2,'0')}_{label}"
