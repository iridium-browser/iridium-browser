# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import logging
import pathlib
from typing import TYPE_CHECKING, Any, Dict, Iterable, List, Optional

if TYPE_CHECKING:
  from crossbench.types import JsonDict
  from crossbench.probes.probe import Probe
  from crossbench.runner.run import Run


class ProbeResult(abc.ABC):

  def __init__(self,
               url: Optional[Iterable[str]] = None,
               file: Optional[Iterable[pathlib.Path]] = None,
               json: Optional[Iterable[pathlib.Path]] = None,
               csv: Optional[Iterable[pathlib.Path]] = None):
    self._url_list = tuple(url) if url else ()
    self._file_list = tuple(file) if file else ()
    self._json_list = tuple(json) if json else ()
    self._csv_list = tuple(csv) if csv else ()
    # TODO: Add Metric object for keeping metrics in-memory instead of reloading
    # them from serialized JSON files for merging.
    self._values = None
    self._validate()

  @property
  def is_empty(self) -> bool:
    return not any(
        (self._url_list, self._file_list, self._json_list, self._csv_list))

  def __bool__(self) -> bool:
    return not self.is_empty

  def merge(self, other: ProbeResult) -> ProbeResult:
    if self.is_empty:
      return other
    if other.is_empty:
      return self
    return LocalProbeResult(
        url=self.url_list + other.url_list,
        file=self.file_list + other.file_list,
        json=self.json_list + other.json_list,
        csv=self.csv_list + other.csv_list)

  def _validate(self) -> None:
    for path in self._file_list:
      if path.suffix in (".csv", ".json"):
        raise ValueError(f"Use specific parameter for result: {path}")
    for path in self._json_list:
      if path.suffix != ".json":
        raise ValueError(f"Expected .json file but got: {path}")
    for path in self._csv_list:
      if path.suffix != ".csv":
        raise ValueError(f"Expected .csv file but got: {path}")
    for path in self.all_files():
      if not path.exists():
        raise ValueError(f"ProbeResult path does not exist: {path}")

  def to_json(self) -> JsonDict:
    result: JsonDict = {}
    if self._url_list:
      result["url"] = self._url_list
    if self._file_list:
      result["file"] = list(map(str, self._file_list))
    if self._json_list:
      result["json"] = list(map(str, self._json_list))
    if self._csv_list:
      result["csv"] = list(map(str, self._csv_list))
    return result

  def all_files(self) -> Iterable[pathlib.Path]:
    yield from self._file_list
    yield from self._json_list
    yield from self._csv_list

  @property
  def url(self) -> str:
    assert len(self._url_list) == 1
    return self._url_list[0]

  @property
  def url_list(self) -> List[str]:
    return list(self._url_list)

  @property
  def file(self) -> pathlib.Path:
    assert len(self._file_list) == 1
    return self._file_list[0]

  @property
  def file_list(self) -> List[pathlib.Path]:
    return list(self._file_list)

  @property
  def json(self) -> pathlib.Path:
    assert len(self._json_list) == 1
    return self._json_list[0]

  @property
  def json_list(self) -> List[pathlib.Path]:
    return list(self._json_list)

  @property
  def csv(self) -> pathlib.Path:
    assert len(self._csv_list) == 1
    return self._csv_list[0]

  @property
  def csv_list(self) -> List[pathlib.Path]:
    return list(self._csv_list)


class EmptyProbeResult(ProbeResult):

  def __init__(self) -> None:
    super().__init__()

  def __bool__(self) -> bool:
    return False


class LocalProbeResult(ProbeResult):
  """LocalProbeResult can be used for files that are always available on the
  runner/local machine."""


class BrowserProbeResult(ProbeResult):
  """BrowserProbeResult are stored on the device where the browser runs.
  Result files will be automatically transferred to the local run's results
  folder.
  """

  def __init__(self,
               run: Run,
               url: Optional[Iterable[str]] = None,
               file: Optional[Iterable[pathlib.Path]] = None,
               json: Optional[Iterable[pathlib.Path]] = None,
               csv: Optional[Iterable[pathlib.Path]] = None):
    self._browser_file = file
    self._browser_json = json
    self._browser_csv = csv

    file = self._copy_files(run, file)
    json = self._copy_files(run, json)
    csv = self._copy_files(run, csv)

    super().__init__(url, file, json, csv)

  def _copy_files(
      self, run: Run, paths: Optional[Iterable[pathlib.Path]]
  ) -> Optional[Iterable[pathlib.Path]]:
    if not paths or not run.is_remote:
      return paths
    # Copy result files from remote tmp dir to local results dir
    browser_platform = run.browser_platform
    remote_tmp_dir = run.browser_tmp_dir
    out_dir = run.out_dir
    result_paths: List[pathlib.Path] = []
    for remote_path in paths:
      relative_path = remote_path.relative_to(remote_tmp_dir)
      result_path = out_dir / relative_path
      browser_platform.rsync(remote_path, result_path)
      assert result_path.exists(), "Failed to copy result file."
      result_paths.append(result_path)
    return result_paths


class ProbeResultDict:
  """
  Maps Probes to their result files Paths.
  """

  def __init__(self, path: pathlib.Path) -> None:
    self._path = path
    self._dict: Dict[str, ProbeResult] = {}

  def __setitem__(self, probe: Probe, result: ProbeResult) -> None:
    assert isinstance(result, ProbeResult)
    self._dict[probe.name] = result

  def __getitem__(self, probe: Probe) -> ProbeResult:
    name = probe.name
    if name not in self._dict:
      raise KeyError(f"No results for probe='{name}'")
    return self._dict[name]

  def __contains__(self, probe: Probe) -> bool:
    return probe.name in self._dict

  def get(self, probe: Probe, default: Any = None) -> ProbeResult:
    return self._dict.get(probe.name, default)

  def get_by_name(self, name: str, default: Any = None) -> ProbeResult:
    # Debug helper only.
    # Use bracket `results[probe]` or `results.get(probe)` instead.
    return self._dict.get(name, default)

  def to_json(self) -> JsonDict:
    data: JsonDict = {}
    for probe_name, results in self._dict.items():
      if isinstance(results, (pathlib.Path, str)):
        data[probe_name] = str(results)
      else:
        if results.is_empty:
          logging.debug("probe=%s did not produce any data.", probe_name)
          data[probe_name] = None
        else:
          data[probe_name] = results.to_json()
    return data
