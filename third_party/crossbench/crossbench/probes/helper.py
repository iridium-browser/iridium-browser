# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import csv
import json
import logging
import math
import pathlib
from typing import (Any, Callable, Dict, Iterable, List, Optional, Sequence,
                    Set, Tuple, Union)

_KeyFnType = Callable[[Tuple[str, ...]], Optional[str]]


def _default_flatten_key_fn(path: Tuple[str, ...]) -> str:
  return "/".join(path)


class Flatten:
  """
  Creates a sorted flat list of (key-path, Values) from hierarchical data.

  input = {"a" : {"aa1":1, "aa2":2}, "b": 12 }
  Flatten(input).data == {
    "a/aa1":  1,
    "a/aa2":  2,
    "b":     12,
  }
  """
  _key_fn: _KeyFnType
  _accumulator: Dict[str, Any]

  def __init__(self, *args: Dict, key_fn: Optional[_KeyFnType] = None):
    """_summary_

    Args:
        *args (optional): Optional hierarchical data to be flattened
        key_fn (optional): Maps property paths (Tuple[str,...]) to strings used
          as final result keys, or None to skip property paths.
    """
    self._accumulator = {}
    self._key_fn = key_fn or _default_flatten_key_fn
    self.append(*args)

  @property
  def data(self) -> Dict[str, Any]:
    items = sorted(self._accumulator.items(), key=lambda item: item[0])
    return dict(items)

  def append(self, *args: Dict, ignore_toplevel: bool = False) -> None:
    toplevel_path: Tuple[str, ...] = tuple()
    for merged_data in args:
      self._flatten(toplevel_path, merged_data, ignore_toplevel)

  def _is_leaf_item(self, item) -> bool:
    if isinstance(item, (str, float, int, list)):
      return True
    if "values" in item and isinstance(item["values"], list):
      return True
    return False

  def _flatten(self,
               parent_path: Tuple[str, ...],
               data,
               ignore_toplevel: bool = False) -> None:
    for name, item in data.items():
      path = parent_path + (name,)
      if self._is_leaf_item(item):
        if ignore_toplevel and parent_path == ():
          continue
        key = self._key_fn(path)
        if key is None:
          continue
        assert isinstance(key, str)
        if key in self._accumulator:
          raise ValueError(f"Duplicate key='{key}' path={path}")
        self._accumulator[key] = item
      else:
        self._flatten(path, item)


def is_number(value: Any) -> bool:
  return isinstance(value, (int, float))


class Values:
  """
  A collection of values that is use as an accumulator in the ValuesMerger.

  Values provides simple statistical getters if the collected values are
  ints or floats only.
  """

  @classmethod
  def from_json(cls, json_data):
    return cls(json_data["values"])

  def __init__(self, values=None):
    self.values = values or []
    self._is_numeric: bool = all(map(is_number, self.values))

  def __len__(self) -> int:
    return len(self.values)

  @property
  def is_numeric(self) -> bool:
    return self._is_numeric

  @property
  def min(self) -> float:
    assert self._is_numeric
    return min(self.values)

  @property
  def max(self) -> float:
    assert self._is_numeric
    return max(self.values)

  @property
  def average(self) -> float:
    assert self._is_numeric
    return sum(self.values) / len(self.values)

  @property
  def geomean(self) -> float:
    assert self._is_numeric
    return geomean(self.values)

  @property
  def stddev(self) -> float:
    assert self._is_numeric
    # We're ignoring here any actual distribution of the data and use this as a
    # rough estimate of the quality of the data
    average = self.average
    variance = 0.0
    for value in self.values:
      variance += (average - value)**2
    variance /= len(self.values)
    return math.sqrt(variance)

  def append(self, value: Any) -> None:
    self.values.append(value)
    self._is_numeric = self._is_numeric and is_number(value)

  def to_json(self) -> Dict[str, Any]:
    json_data = {"values": self.values}
    if not self.values:
      return json_data
    if self.is_numeric:
      json_data["min"] = self.min
      average = json_data["average"] = self.average
      json_data["geomean"] = self.geomean
      json_data["max"] = self.max
      stddev = json_data["stddev"] = self.stddev
      if average == 0:
        json_data["stddevPercent"] = 0
      else:
        json_data["stddevPercent"] = (stddev / average) * 100
      return json_data
    # Simplify repeated non-numeric values
    if len(set(self.values)) == 1:
      return self.values[0]
    return json_data


def geomean(values: Sequence) -> float:
  product = 1
  for value in values:
    product *= value
  return product**(1 / len(values))


class ValuesMerger:
  """
  Merges hierarchical data into 1-level aggregated data;

  Input:
  data_1 ={
    "a": {
      "aa": 1.1,
      "ab": 2
    }
    "b": 2.1
  }
  data_2 = {
    "a": {
      "aa": 1.2
    }
    "b": 2.2,
    "c": 2
  }

  The merged data maps str => Values():

  ValuesMerger(data_1, data_2).data == {
    "a/aa": Values(1.1, 1.2)
    "a/ab": Values(2)
    "b":    Values(2.1, 2.2)
    "c":    Values(2)
  }
  """

  @classmethod
  def merge_json_list(cls,
                      files: Iterable[pathlib.Path],
                      key_fn: Optional[_KeyFnType] = None,
                      merge_duplicate_paths: bool = False) -> ValuesMerger:
    merger = cls(key_fn=key_fn)
    for file in files:
      with file.open(encoding="utf-8") as f:
        merger.merge_values(
            json.load(f), merge_duplicate_paths=merge_duplicate_paths)
    return merger

  def __init__(self,
               *args: Union[Dict, List[Dict]],
               key_fn: Optional[_KeyFnType] = None):
    """Create a new ValuesMerger

    Args:
        *args (optional): Optional hierarchical data to be merged.
        key_fn (optional): Maps property paths (Tuple[str,...]) to strings used
          as keys to group/merge values, or None to skip property paths.
    """
    self._data: Dict[str, Values] = {}
    self._key_fn: _KeyFnType = key_fn or _default_flatten_key_fn
    self._ignored_keys: Set[str] = set()
    for data in args:
      self.add(data)

  @property
  def data(self) -> Dict[str, Values]:
    return self._data

  def merge_values(self,
                   data: Dict[str, Dict],
                   prefix_path: Tuple[str, ...] = (),
                   merge_duplicate_paths: bool = False) -> None:
    """Merge a previously json-serialized ValuesMerger object"""
    for property_name, item in data.items():
      path = prefix_path + (property_name,)
      key = self._key_fn(path)
      if key is None or key in self._ignored_keys:
        continue
      if key in self._data:
        if merge_duplicate_paths:
          values = self._data[key]
          for value in item["values"]:
            values.append(value)
        else:
          logging.debug(
              "Removing Values with the same key-path='%s', key='%s"
              "from multiple files.", path, key)
          del self._data[key]
          self._ignored_keys.add(key)
      else:
        self._data[key] = Values.from_json(item)

  def add(self, data: Union[Dict, List[Dict]]) -> None:
    """ Merge "arbitrary" hierarchical data that ends up having primitive leafs.
    Anything that is not a dict is considered a leaf node.
    """
    if isinstance(data, list):
      # Assume that top-level lists are repetitions of the same data
      for item in data:
        self._merge(item)
    else:
      self._merge(data)

  def _merge(self, data, parent_path: Tuple[str, ...] = ()) -> None:
    assert isinstance(data, dict)
    for property_name, value in data.items():
      path = parent_path + (property_name,)
      key: Optional[str] = self._key_fn(path)
      if key is None:
        continue
      if isinstance(value, dict):
        self._merge(value, path)
      else:
        if key in self._data:
          values = self._data[key]
        else:
          values = self._data[key] = Values()
        if isinstance(value, list):
          for v in value:
            values.append(v)
        else:
          values.append(value)

  def to_json(self, value_fn: Optional[Callable[[Any], Any]] = None
             ) -> Dict[str, Any]:
    items = []
    for key, value in self._data.items():
      assert isinstance(value, Values)
      if value_fn is None:
        value = value.to_json()
      else:
        value = value_fn(value)
      items.append((key, value))
    # Make sure the data is always in the same order, independent of the input
    # order
    items.sort()
    return dict(items)

  def to_csv(self,
             value_fn: Optional[Callable[[Any], Any]] = None,
             headers: Sequence[Sequence[Any]] = ()) -> List[Sequence[Any]]:
    """
    Input: {
        "VanillaJS-TodoMVC/Adding100Items/Async": 1
        "VanillaJS-TodoMVC/Adding100Items/Sync": 2
        "Total": 3
      }
    output: [
      ["VanillaJS-TodoMVC"],
      ["Adding100Items"],
      ["Async", 1]
      [],
      ["Sync", 2]
      ["Total", 3]
    ]
    """
    converted = self.to_json(value_fn)
    lookup: Dict[str, Any] = {}
    # Use Dict as ordered-set
    toplevel: Dict[str, None] = {}
    for key, value in converted.items():
      path = None
      segments = key.split("/")
      for segment in segments:
        if path:
          path += "/" + segment
        else:
          path = segment
        if path not in lookup:
          lookup[path] = None
      if len(segments) == 1:
        toplevel[key] = None
      lookup[key] = value
    csv_data: List[Sequence[Any]] = []
    for header in headers:
      assert isinstance(header, Sequence), (
          f"Additional CSV headers must be Sequences, got {type(header)}: "
          f"{header}")
      csv_data.append(header)
    for path, value in lookup.items():
      if path in toplevel:
        continue
      name = path.split("/")[-1]
      if value is None:
        csv_data.append([name])
      else:
        csv_data.append([name, value])
    # Write toplevel entries last
    for key in toplevel:
      csv_data.append([key, lookup[key]])

    return csv_data


def _ljust(sequence: List, n: int, fillvalue: Any = ""):
  return sequence + ([fillvalue] * (n - len(sequence)))


def merge_csv(csv_list: Sequence[pathlib.Path],
              headers: Optional[List[str]] = None,
              delimiter: str = "\t") -> List[List[Any]]:
  """
  Merge multiple CSV files.
  File 1:
    Header,     Col Header 1.1, Col Header  1.2
    Row Header, Data 1.1,       Data 1.2
  File 2:
    Header,     Col Header 2.1, Col Header 2.2
    Row Header, Data 2.1,       Data 2.2

  The first Col has to contain the same data:

  Merged:
    Header,     Col Header 1.1, Col Header 1.2,  Col Header 2.1, Col Header 2.2
    Row Header, Data 1.1,       Data 1.2,        Data 2.1,       Data 2.2


  If no column header is available, filename_as_header=True can be used.

  Merged with file name header:
            , File 1,           , File 2,
  Row Header, Data 1.1, Data 1.2, Data 2.1, Data 2.2
  """
  # Fill in the header column taken from the first file
  table: List[List[Any]] = []
  if headers:
    table_headers = [""]
  else:
    table_headers = []
  with csv_list[0].open(encoding="utf-8") as first_file:
    for row in csv.reader(first_file, delimiter=delimiter):
      assert row, "Mergeable CSV files musth have row names."
      metric_name = row[0]
      table.append([metric_name])

  for csv_file in csv_list:
    with csv_file.open(encoding="utf-8") as f:
      csv_data = list(csv.reader(f, delimiter=delimiter))
      # Find the max width
      max_rows_with_row_header = max(len(row) for row in csv_data)
      max_rows = max_rows_with_row_header - 1
      if headers:
        col_header = [headers.pop(0)]
        table_headers.extend(_ljust(col_header, max_rows))
      for table_row, row in zip(table, csv_data):
        metric_name = row[0]
        padded_row = _ljust(row[1:], max_rows)
        assert table_row[0] == metric_name, (f"{table_row[0]} != {metric_name}"
                                             f"\n{csv_data}\n{table}")
        table_row.extend(padded_row)

  if table_headers:
    return [table_headers] + table
  return table
