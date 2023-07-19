# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import csv
import json
import pathlib
import sys
import unittest

import pyfakefs.fake_filesystem_unittest
import pytest

from crossbench.probes import helper


class TestMergeCSV(pyfakefs.fake_filesystem_unittest.TestCase):

  def setUp(self):
    self.setUpPyfakefs()

  def test_merge_single(self):
    file = pathlib.Path("test.csv")
    data = [
        ["Metric", "Run1"],
        ["Total", "200"],
    ]
    for delimiter in ["\t", ","]:
      with file.open("w", newline="", encoding="utf-8") as f:
        csv.writer(f, delimiter=delimiter).writerows(data)
      merged = helper.merge_csv([file], delimiter=delimiter)
      self.assertListEqual(merged, data)

  def test_merge_single_padding(self):
    file = pathlib.Path("test.csv")
    data = [
        ["Metric", "Run1", "Run2"],
        ["marker"],
        ["Total", "200", "300"],
    ]
    with file.open("w", newline="", encoding="utf-8") as f:
      csv.writer(f, delimiter="\t").writerows(data)
    merged = helper.merge_csv([file], headers=None)
    self.assertListEqual(merged, [
        ["Metric", "Run1", "Run2"],
        ["marker", "", ""],
        ["Total", "200", "300"],
    ])

  def test_merge_single_file_header(self):
    file = pathlib.Path("test.csv")
    data = [
        ["Total", "200"],
    ]
    for delimiter in ["\t", ","]:
      with file.open("w", newline="", encoding="utf-8") as f:
        csv.writer(f, delimiter=delimiter).writerows(data)
      merged = helper.merge_csv([file],
                                delimiter=delimiter,
                                headers=[file.name])
      self.assertListEqual(merged, [
          ["", file.name],
          ["Total", "200"],
      ])

  def test_merge_two_padding(self):
    file_1 = pathlib.Path("test_1.csv")
    file_2 = pathlib.Path("test_2.csv")
    data_1 = [
        ["marker"],
        ["Total", "101", "102"],
    ]
    data_2 = [
        ["marker"],
        ["Total", "201"],
    ]
    with file_1.open("w", newline="", encoding="utf-8") as f:
      csv.writer(f, delimiter="\t").writerows(data_1)
    with file_2.open("w", newline="", encoding="utf-8") as f:
      csv.writer(f, delimiter="\t").writerows(data_2)
    merged = helper.merge_csv([file_1, file_2], headers=["col_1", "col_2"])
    self.assertListEqual(merged, [
        ["", "col_1", "", "col_2"],
        ["marker", "", "", ""],
        ["Total", "101", "102", "201"],
    ])


class TestFlatten(unittest.TestCase):

  def flatten(self, *data, key_fn=None):
    return helper.Flatten(*data, key_fn=key_fn).data

  def test_single(self):
    data = {
        "a": 1,
        "b": 2,
    }
    flattened = self.flatten(data)
    self.assertDictEqual(flattened, data)

  def test_single_nested(self):
    data = {
        "a": 1,
        "b": {
            "a": 2,
            "b": 3
        },
    }
    flattened = self.flatten(data)
    self.assertDictEqual(flattened, {"a": 1, "b/a": 2, "b/b": 3})

  def test_single_key_fn(self):
    data = {
        "a": 1,
        "b": 2,
    }
    flattened = self.flatten(data, key_fn=lambda path: "prefix_" + path[0])
    self.assertDictEqual(flattened, {
        "prefix_a": 1,
        "prefix_b": 2,
    })

  def test_single_key_fn_filtering(self):
    data = {
        "a": 1,
        "b": 2,
    }
    flattened = self.flatten(
        data,
        key_fn=lambda path: None if path[0] == "a" else "prefix_" + path[0])
    self.assertDictEqual(flattened, {
        "prefix_b": 2,
    })

  def test_single_nested_key_fn(self):
    data = {
        "a": 1,
        "b": {
            "a": 2,
            "b": 3
        },
    }
    with self.assertRaises(ValueError):
      # Fail on duplicate entries
      self.flatten(data, key_fn=lambda path: "prefix_" + path[0])

    flattened = self.flatten(
        data, key_fn=lambda path: "prefix_" + "/".join(path))
    self.assertDictEqual(flattened, {
        "prefix_a": 1,
        "prefix_b/a": 2,
        "prefix_b/b": 3,
    })

  def test_single_nested_key_fn_filtering(self):
    data = {
        "a": 1,
        "b": {
            "a": 2,
            "b": 3
        },
    }
    flattened = self.flatten(
        data,
        key_fn=lambda path: None
        if path[-1] == "a" else "prefix_" + "/".join(path))
    self.assertDictEqual(flattened, {
        "prefix_b/b": 3,
    })

  def test_multiple_flat(self):
    data_1 = {
        "a": 1,
        "b": 2,
    }
    with self.assertRaises(ValueError):
      # duplicate entries
      self.flatten(data_1, data_1)
    data_2 = {
        "c": 3,
        "d": 4,
    }
    flattened = self.flatten(data_1, data_2)
    self.assertDictEqual(flattened, {
        "a": 1,
        "b": 2,
        "c": 3,
        "d": 4,
    })


class ValuesTestCase(unittest.TestCase):

  def test_empty(self):
    values = helper.Values()
    self.assertTrue(values.is_numeric)
    self.assertEqual(len(values), 0)

  def test_is_numeric(self):
    values = helper.Values([1, 2, 3, 4])
    self.assertTrue(values.is_numeric)
    values.append(5)
    self.assertTrue(values.is_numeric)
    values.append("6")
    self.assertFalse(values.is_numeric)

    values = helper.Values([1, 2, 3, "4"])
    self.assertFalse(values.is_numeric)

  def test_to_json_empty(self):
    json_data = helper.Values().to_json()
    self.assertDictEqual(json_data, {"values": []})

  def test_to_json_any(self):
    json_data = helper.Values(["a", "b", "c"]).to_json()
    self.assertDictEqual(json_data, {"values": ["a", "b", "c"]})

  def test_to_json_repeated(self):
    json_data = helper.Values(["a", "a", "a"]).to_json()
    self.assertEqual(json_data, "a")

  def test_to_json_numeric_repeated(self):
    json_data = helper.Values([1, 1, 1]).to_json()
    self.assertListEqual(json_data["values"], [1, 1, 1])
    self.assertEqual(json_data["min"], 1)
    self.assertEqual(json_data["max"], 1)
    self.assertEqual(json_data["geomean"], 1)
    self.assertEqual(json_data["average"], 1)
    self.assertEqual(json_data["stddevPercent"], 0)

  def test_to_json_numeric_average_0(self):
    json_data = helper.Values([-1, 0, 1]).to_json()
    self.assertListEqual(json_data["values"], [-1, 0, 1])
    self.assertEqual(json_data["min"], -1)
    self.assertEqual(json_data["max"], 1)
    self.assertEqual(json_data["geomean"], 0)
    self.assertEqual(json_data["average"], 0)
    self.assertEqual(json_data["stddevPercent"], 0)


class ValuesMergerTestCase(pyfakefs.fake_filesystem_unittest.TestCase):

  def setUp(self):
    self.setUpPyfakefs()

  def test_empty(self):
    merger = helper.ValuesMerger()
    self.assertDictEqual(merger.to_json(), {})
    self.assertListEqual(merger.to_csv(), [])

  def test_add_flat(self):
    input_data = {"a": 1, "b": 2}
    merger = helper.ValuesMerger()
    merger.add(input_data)
    data = merger.data
    self.assertEqual(len(data), 2)
    self.assertIsInstance(data["a"], helper.Values)
    self.assertIsInstance(data["b"], helper.Values)
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
    merger = helper.ValuesMerger()
    merger.add(input_data)
    data = merger.data
    self.assertListEqual(list(data.keys()), ["a/a/a", "a/a/b", "b"])
    self.assertIsInstance(data["a/a/a"], helper.Values)
    self.assertIsInstance(data["a/a/b"], helper.Values)
    self.assertIsInstance(data["b"], helper.Values)

  def test_repeated_numeric(self):
    merger = helper.ValuesMerger()
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

  def test_custom_key_fn(self):
    input_data = {
        "a": {
            "a": {
                "a": 1,
                "b": 2
            }
        },
        "b": 2,
    }

    def under_join(segments):
      return "_".join(segments)

    merger = helper.ValuesMerger(key_fn=under_join)
    merger.add(input_data)
    data = merger.data
    self.assertListEqual(list(data.keys()), ["a_a_a", "a_a_b", "b"])

  def test_merge_serialized_same(self):
    input_data = {
        "a": {
            "a": {
                "a": 1,
                "b": 2
            }
        },
        "b": 3,
    }
    merger = helper.ValuesMerger()
    merger.add(input_data)
    self.assertListEqual(list(merger.data.keys()), ["a/a/a", "a/a/b", "b"])
    path_a = pathlib.Path("merged_a.json")
    path_b = pathlib.Path("merged_b.json")
    with path_a.open("w", encoding="utf-8") as f:
      json.dump(merger.to_json(), f)
    with path_b.open("w", encoding="utf-8") as f:
      json.dump(merger.to_json(), f)

    merger = helper.ValuesMerger.merge_json_list([path_a, path_b],
                                                 merge_duplicate_paths=True)
    data = merger.data
    self.assertListEqual(list(data.keys()), ["a/a/a", "a/a/b", "b"])
    self.assertListEqual(data["a/a/a"].values, [1, 1])
    self.assertListEqual(data["a/a/b"].values, [2, 2])
    self.assertListEqual(data["b"].values, [3, 3])

    # All duplicate entries are ignored
    merger = helper.ValuesMerger.merge_json_list([path_a, path_b],
                                                 merge_duplicate_paths=False)
    self.assertListEqual(list(merger.data.keys()), [])

  def test_merge_serialized_different_data(self):
    merger_a = helper.ValuesMerger({"a": {"a": 1}})
    merger_b = helper.ValuesMerger({"a": {"b": 2}})
    path_a = pathlib.Path("merged_a.json")
    path_b = pathlib.Path("merged_b.json")
    with path_a.open("w", encoding="utf-8") as f:
      json.dump(merger_a.to_json(), f)
    with path_b.open("w", encoding="utf-8") as f:
      json.dump(merger_b.to_json(), f)

    merger = helper.ValuesMerger.merge_json_list([path_a, path_b],
                                                 merge_duplicate_paths=True)
    data = merger.data
    self.assertListEqual(list(data.keys()), ["a/a", "a/b"])
    self.assertListEqual(data["a/a"].values, [1])
    self.assertListEqual(data["a/b"].values, [2])

    merger = helper.ValuesMerger.merge_json_list([path_a, path_b],
                                                 merge_duplicate_paths=False)
    data = merger.data
    self.assertListEqual(list(data.keys()), ["a/a", "a/b"])


if __name__ == "__main__":
  sys.exit(pytest.main([__file__]))
