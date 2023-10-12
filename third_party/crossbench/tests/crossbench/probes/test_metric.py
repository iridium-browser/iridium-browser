# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import pathlib
import unittest

from crossbench.probes import metric
from tests import run_helper
from tests.crossbench.mock_helper import CrossbenchFakeFsTestCase


class FormatMetricTestCase(unittest.TestCase):

  def test_no_stdev(self):
    self.assertEqual(metric.format_metric(100), "100")
    self.assertEqual(metric.format_metric(0), "0")
    self.assertEqual(metric.format_metric(1.5), "1.5")
    self.assertEqual(metric.format_metric(100, 0), "100")
    self.assertEqual(metric.format_metric(0, 0), "0")
    self.assertEqual(metric.format_metric(1.5, 0), "1.5")

  def test_stdev(self):
    self.assertEqual(metric.format_metric(100, 10), "100 ± 10%")
    self.assertEqual(metric.format_metric(100, 1), "100.0 ± 1.0%")
    self.assertEqual(metric.format_metric(100, 1.5), "100.0 ± 1.5%")
    self.assertEqual(metric.format_metric(100, 0.1), "100.00 ± 0.10%")
    self.assertEqual(metric.format_metric(100, 0.12), "100.00 ± 0.12%")
    self.assertEqual(metric.format_metric(100, 0.125), "100.00 ± 0.12%")

  def test_round_stdev(self):
    value = 100.123456789
    percent = value / 100
    self.assertEqual(
        metric.format_metric(value, percent * 10.1234), "100 ± 10%")
    self.assertEqual(
        metric.format_metric(value, percent * 1.2345), "100.1 ± 1.2%")
    self.assertEqual(
        metric.format_metric(value, percent * 0.12345), "100.12 ± 0.12%")
    self.assertEqual(
        metric.format_metric(value, percent * 0.012345), "100.123 ± 0.012%")
    self.assertEqual(
        metric.format_metric(value, percent * 0.0012345), "100.1235 ± 0.0012%")
    self.assertEqual(
        metric.format_metric(value, percent * 0.00012345),
        "100.12346 ± 0.00012%")


class MetricTestCase(unittest.TestCase):

  def test_empty(self):
    values = metric.Metric()
    self.assertTrue(values.is_numeric)
    self.assertEqual(len(values), 0)

  def test_is_numeric(self):
    values = metric.Metric([1, 2, 3, 4])
    self.assertTrue(values.is_numeric)
    values.append(5)
    self.assertTrue(values.is_numeric)
    values.append("6")
    self.assertFalse(values.is_numeric)

    values = metric.Metric([1, 2, 3, "4"])
    self.assertFalse(values.is_numeric)

  def test_to_json_empty(self):
    json_data = metric.Metric().to_json()
    self.assertDictEqual(json_data, {"values": []})

  def test_to_json_any(self):
    json_data = metric.Metric(["a", "b", "c"]).to_json()
    self.assertDictEqual(json_data, {"values": ["a", "b", "c"]})

  def test_to_json_repeated(self):
    json_data = metric.Metric(["a", "a", "a"]).to_json()
    self.assertEqual(json_data, "a")

  def test_to_json_numeric_repeated(self):
    json_data = metric.Metric([1, 1, 1]).to_json()
    self.assertListEqual(json_data["values"], [1, 1, 1])
    self.assertEqual(json_data["min"], 1)
    self.assertEqual(json_data["max"], 1)
    self.assertEqual(json_data["geomean"], 1)
    self.assertEqual(json_data["average"], 1)
    self.assertEqual(json_data["stddevPercent"], 0)

  def test_to_json_numeric_average_0(self):
    json_data = metric.Metric([-1, 0, 1]).to_json()
    self.assertListEqual(json_data["values"], [-1, 0, 1])
    self.assertEqual(json_data["min"], -1)
    self.assertEqual(json_data["max"], 1)
    self.assertEqual(json_data["geomean"], 0)
    self.assertEqual(json_data["average"], 0)
    self.assertEqual(json_data["stddevPercent"], 0)


class MetricsMergerTestCase(CrossbenchFakeFsTestCase):

  def test_empty(self):
    merger = metric.MetricsMerger()
    self.assertDictEqual(merger.to_json(), {})
    self.assertListEqual(merger.to_csv(), [])

  def test_add_flat(self):
    input_data = {"a": 1, "b": 2}
    merger = metric.MetricsMerger()
    merger.add(input_data)
    data = merger.data
    self.assertEqual(len(data), 2)
    self.assertIsInstance(data["a"], metric.Metric)
    self.assertIsInstance(data["b"], metric.Metric)
    self.assertListEqual(data["a"].values, [1])
    self.assertListEqual(data["b"].values, [2])

    merger.add(input_data)
    data = merger.data
    self.assertEqual(len(data), 2)
    self.assertListEqual(data["a"].values, [1, 1])
    self.assertListEqual(data["b"].values, [2, 2])

  def test_add_hierarchical(self):
    input_data = {
        "a": {
            "a": {
                "a": 1,
                "b": 2
            }
        },
        "b": 2,
    }
    merger = metric.MetricsMerger()
    merger.add(input_data)
    data = merger.data
    self.assertListEqual(list(data.keys()), ["a/a/a", "a/a/b", "b"])
    self.assertIsInstance(data["a/a/a"], metric.Metric)
    self.assertIsInstance(data["a/a/b"], metric.Metric)
    self.assertIsInstance(data["b"], metric.Metric)

  def test_repeated_numeric(self):
    merger = metric.MetricsMerger()
    input_data = {
        "a": {
            "aa": 1,
            "ab": 2
        },
        "b": 3,
        "c": {
            "cc": {
                "ccc": 4
            }
        },
    }
    merger.add(input_data)
    merger.add(input_data)
    data = merger.data
    self.assertEqual(len(data), 4)
    self.assertListEqual(data["a/aa"].values, [1, 1])
    self.assertListEqual(data["a/ab"].values, [2, 2])
    self.assertListEqual(data["b"].values, [3, 3])
    self.assertListEqual(data["c/cc/ccc"].values, [4, 4])

  BASIC_NESTED_DATA = {
      "a": {
          "a": {
              "a": 1,
              "b": 2
          }
      },
      "b": 3,
  }

  def test_custom_key_fn(self):

    def under_join(segments):
      return "_".join(segments)

    merger = metric.MetricsMerger(key_fn=under_join)
    merger.add(self.BASIC_NESTED_DATA)
    data = merger.data
    self.assertListEqual(list(data.keys()), ["a_a_a", "a_a_b", "b"])

  def test_merge_serialized_same(self):
    merger = metric.MetricsMerger()
    merger.add(self.BASIC_NESTED_DATA)
    self.assertListEqual(list(merger.data.keys()), ["a/a/a", "a/a/b", "b"])
    path_a = pathlib.Path("merged_a.json")
    path_b = pathlib.Path("merged_b.json")
    with path_a.open("w", encoding="utf-8") as f:
      json.dump(merger.to_json(), f)
    with path_b.open("w", encoding="utf-8") as f:
      json.dump(merger.to_json(), f)

    merger = metric.MetricsMerger.merge_json_list([path_a, path_b],
                                                  merge_duplicate_paths=True)
    data = merger.data
    self.assertListEqual(list(data.keys()), ["a/a/a", "a/a/b", "b"])
    self.assertListEqual(data["a/a/a"].values, [1, 1])
    self.assertListEqual(data["a/a/b"].values, [2, 2])
    self.assertListEqual(data["b"].values, [3, 3])

    # All duplicate entries are ignored
    merger = metric.MetricsMerger.merge_json_list([path_a, path_b],
                                                  merge_duplicate_paths=False)
    self.assertListEqual(list(merger.data.keys()), [])

  def test_merge_serialized_different_data(self):
    merger_a = metric.MetricsMerger({"a": {"a": 1}})
    merger_b = metric.MetricsMerger({"a": {"b": 2}})
    path_a = pathlib.Path("merged_a.json")
    path_b = pathlib.Path("merged_b.json")
    with path_a.open("w", encoding="utf-8") as f:
      json.dump(merger_a.to_json(), f)
    with path_b.open("w", encoding="utf-8") as f:
      json.dump(merger_b.to_json(), f)

    merger = metric.MetricsMerger.merge_json_list([path_a, path_b],
                                                  merge_duplicate_paths=True)
    data = merger.data
    self.assertListEqual(list(data.keys()), ["a/a", "a/b"])
    self.assertListEqual(data["a/a"].values, [1])
    self.assertListEqual(data["a/b"].values, [2])

    merger = metric.MetricsMerger.merge_json_list([path_a, path_b],
                                                  merge_duplicate_paths=False)
    data = merger.data
    self.assertListEqual(list(data.keys()), ["a/a", "a/b"])

  def test_to_csv_no_path(self) -> None:
    merger = metric.MetricsMerger()
    merger.add(self.BASIC_NESTED_DATA)
    csv = merger.to_csv(lambda metric: metric.geomean, include_path=False)
    self.assertListEqual(csv, [
        ["a"],
        ["a"],
        ["a", 1.0],
        ["b", 2.0],
        ["b", 3.0],
    ])

  def test_to_csv_path(self) -> None:
    merger = metric.MetricsMerger()
    merger.add(self.BASIC_NESTED_DATA)
    csv = merger.to_csv(lambda metric: metric.geomean, include_path=True)
    self.assertListEqual(csv, [
        ["a", "a"],
        ["a/a", "a"],
        ["a/a/a", "a", 1.0],
        ["a/a/b", "b", 2.0],
        ["b", "b", 3.0],
    ])

  def test_to_csv_header(self) -> None:
    merger = metric.MetricsMerger()
    merger.add({"a": 1, "b": 2})
    headers = [
        ["a", "custom", "header", "line"],
        [1, 2, 3, 4, 5],
    ]
    csv = merger.to_csv(
        lambda metric: metric.geomean, headers=headers, include_path=True)
    self.assertListEqual(csv, [
        ["a", "custom", "header", "line"],
        [1, 2, 3, 4, 5],
        ["a", "a", 1.0],
        ["b", "b", 2.0],
    ])


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
