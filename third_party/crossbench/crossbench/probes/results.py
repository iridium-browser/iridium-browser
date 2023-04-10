# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import pathlib
from typing import TYPE_CHECKING, Any, Dict, Iterable, List, Optional

if TYPE_CHECKING:
  from crossbench.probes.base import Probe


class ProbeResult:

  def __init__(self,
               url: Optional[Iterable[str]] = None,
               file: Optional[Iterable[pathlib.Path]] = None,
               json: Optional[Iterable[pathlib.Path]] = None,
               csv: Optional[Iterable[pathlib.Path]] = None):
    self._url_list = tuple(url) if url else ()
    self._file_list = tuple(file) if file else ()
    self._json_list = tuple(json) if json else ()
    self._csv_list = tuple(csv) if csv else ()
    # TODO: Add Values object for keeping metrics in-memory instead of reloading
    # them from serialized JSON files for merging.
    self._values = None
    self._validate()

  @property
  def is_empty(self) -> bool:
    return not any(
        (self._url_list, self._file_list, self._json_list, self._csv_list))

  def merge(self, other: ProbeResult) -> ProbeResult:
    return ProbeResult(
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
      if not path.is_file():
        raise ValueError(f"ProbeResult file does not exist: {path}")

  def to_json(self) -> Dict[str, Any]:
    result = {}
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


class ProbeResultDict:
  """
  Maps Probes to their result files Paths.
  """

  def __init__(self, path: pathlib.Path):
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

  def to_json(self) -> Dict[str, Any]:
    data: Dict[str, Any] = {}
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
