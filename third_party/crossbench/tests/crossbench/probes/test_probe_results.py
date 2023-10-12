# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import pathlib
from unittest import mock

from crossbench import plt
from crossbench.probes.results import EmptyProbeResult, LocalProbeResult
from tests import run_helper
from tests.crossbench.mock_helper import CrossbenchFakeFsTestCase


class BrowserProbeResultTestCase(CrossbenchFakeFsTestCase):

  def setUp(self) -> None:
    super().setUp()
    self.run = mock.Mock(platform=plt.PLATFORM)

  # TODO add remote platform test



class ProbeResultTestCase(CrossbenchFakeFsTestCase):

  def create_file(self, path_str: str) -> pathlib.Path:
    path = pathlib.Path(path_str)
    self.fs.create_file(path)
    return path

  def test_is_empty(self):
    empty = EmptyProbeResult()
    self.assertTrue(empty.is_empty)
    self.assertFalse(empty)
    combined = empty.merge(EmptyProbeResult())
    self.assertTrue(combined.is_empty)
    url = LocalProbeResult(url=["http://test.com"])
    self.assertFalse(url.is_empty)
    self.assertTrue(url)
    combined = empty.merge(url)
    self.assertFalse(combined.is_empty)
    self.assertTrue(combined)

  def test_invalid_files(self):
    with self.assertRaises(ValueError):
      LocalProbeResult(file=[pathlib.Path("foo.json")])
    with self.assertRaises(ValueError):
      LocalProbeResult(file=[pathlib.Path("foo.csv")])
    with self.assertRaises(ValueError):
      LocalProbeResult(json=[pathlib.Path("foo.csv")])
    with self.assertRaises(ValueError):
      LocalProbeResult(csv=[pathlib.Path("foo.json")])

  def test_inexistent_files(self):
    with self.assertRaises(ValueError):
      LocalProbeResult(file=[pathlib.Path("not_there.txt")])
    with self.assertRaises(ValueError):
      LocalProbeResult(csv=[pathlib.Path("not_there.csv")])
    with self.assertRaises(ValueError):
      LocalProbeResult(json=[pathlib.Path("not_there.json")])

  def test_result_url(self):
    url = "http://foo.bar.com"
    result = LocalProbeResult(url=[url])
    self.assertFalse(result.is_empty)
    self.assertEqual(result.url, url)
    self.assertListEqual(result.url_list, [url])
    self.assertListEqual(list(result.all_files()), [])
    failed = None
    with self.assertRaises(AssertionError):
      failed = result.file
    with self.assertRaises(AssertionError):
      failed = result.json
    with self.assertRaises(AssertionError):
      failed = result.csv
    self.assertIsNone(failed)

  def test_result_any_file(self):
    path = self.create_file("result.txt")
    result = LocalProbeResult(file=[path])
    self.assertFalse(result.is_empty)
    self.assertEqual(result.file, path)
    self.assertListEqual(result.file_list, [path])
    self.assertListEqual(list(result.all_files()), [path])
    failed = None
    with self.assertRaises(AssertionError):
      failed = result.url
    with self.assertRaises(AssertionError):
      failed = result.json
    with self.assertRaises(AssertionError):
      failed = result.csv
    self.assertIsNone(failed)

  def test_result_csv(self):
    path = self.create_file("result.csv")
    result = LocalProbeResult(csv=[path])
    self.assertFalse(result.is_empty)
    self.assertEqual(result.csv, path)
    self.assertListEqual(result.csv_list, [path])
    self.assertListEqual(list(result.all_files()), [path])
    failed = None
    with self.assertRaises(AssertionError):
      failed = result.file
    with self.assertRaises(AssertionError):
      failed = result.file
    with self.assertRaises(AssertionError):
      failed = result.json
    self.assertIsNone(failed)

  def test_result_json(self):
    path = self.create_file("result.json")
    result = LocalProbeResult(json=[path])
    self.assertFalse(result.is_empty)
    self.assertEqual(result.json, path)
    self.assertListEqual(result.json_list, [path])
    self.assertListEqual(list(result.all_files()), [path])
    failed = None
    with self.assertRaises(AssertionError):
      failed = result.url
    with self.assertRaises(AssertionError):
      failed = result.file
    with self.assertRaises(AssertionError):
      failed = result.csv
    self.assertIsNone(failed)

  def test_merge(self):
    file = self.create_file("result.custom")
    json = self.create_file("result.json")
    csv = self.create_file("result.csv")
    url = "http://foo.bar.com"

    result = LocalProbeResult(url=[url], file=[file], json=[json], csv=[csv])
    self.assertFalse(result.is_empty)
    self.assertListEqual(list(result.all_files()), [file, json, csv])
    self.assertListEqual(result.url_list, [url])

    merged = result.merge(EmptyProbeResult())
    self.assertFalse(merged.is_empty)
    self.assertListEqual(list(merged.all_files()), [file, json, csv])
    self.assertListEqual(result.url_list, [url])

    file_2 = self.create_file("result.2.custom")
    json_2 = self.create_file("result.2.json")
    csv_2 = self.create_file("result.2.csv")
    url_2 = "http://foo.bar.com/2"
    other = LocalProbeResult(
        url=[url_2], file=[file_2], json=[json_2], csv=[csv_2])
    merged = result.merge(other)
    self.assertFalse(merged.is_empty)
    self.assertListEqual(
        list(merged.all_files()), [file, file_2, json, json_2, csv, csv_2])
    self.assertListEqual(merged.url_list, [url, url_2])
    # result is unchanged:
    self.assertFalse(result.is_empty)
    self.assertListEqual(list(result.all_files()), [file, json, csv])
    self.assertListEqual(result.url_list, [url])
    # other is unchanged:
    self.assertFalse(other.is_empty)
    self.assertListEqual(list(other.all_files()), [file_2, json_2, csv_2])
    self.assertListEqual(other.url_list, [url_2])


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
