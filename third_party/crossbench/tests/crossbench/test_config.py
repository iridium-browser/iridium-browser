# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import dataclasses
import json
import pathlib
from typing import Any, Dict, List, Optional
from frozendict import frozendict

from crossbench import cli_helper
from crossbench.config import ConfigObject, ConfigParser
from tests.crossbench.mock_helper import CrossbenchFakeFsTestCase


@dataclasses.dataclass(frozen=True)
class CustomNestedConfigObject(ConfigObject):
  name: str

  @classmethod
  def loads(cls, value: str) -> CustomNestedConfigObject:
    if ":" in value:
      raise ValueError("Invalid Config")
    if not value:
      raise ValueError("Got empty input")
    return cls(name=value)

  @classmethod
  def load_dict(cls, config: Dict[str, Any]) -> CustomNestedConfigObject:
    return cls.config_parser().parse(config)

  @classmethod
  def config_parser(cls) -> ConfigParser[CustomNestedConfigObject]:
    parser = ConfigParser("CustomNestedConfigObject parser", cls)
    parser.add_argument("name", type=str, required=True)
    return parser


@dataclasses.dataclass(frozen=True)
class CustomConfigObject(ConfigObject):

  name: str
  array: Optional[List[str]] = None
  integer: Optional[int] = None
  nested: Optional[CustomNestedConfigObject] = None

  @classmethod
  def loads(cls, value: str) -> CustomConfigObject:
    if ":" in value:
      raise ValueError("Invalid Config")
    if not value:
      raise ValueError("Got empty input")
    return cls(name=value)

  @classmethod
  def load_dict(cls, config: Dict[str, Any]) -> CustomConfigObject:
    return cls.config_parser().parse(config)

  @classmethod
  def config_parser(cls) -> ConfigParser[CustomConfigObject]:
    parser = ConfigParser("CustomConfigObject parser", cls)
    parser.add_argument("name", type=str, required=True)
    parser.add_argument("array", type=list)
    parser.add_argument("integer", type=cli_helper.parse_positive_int)
    parser.add_argument("nested", type=CustomNestedConfigObject)
    return parser


class ConfigObjectTestCase(CrossbenchFakeFsTestCase):

  def test_load_invalid_str(self):
    for invalid in ("", None, 1, []):
      with self.assertRaises(argparse.ArgumentTypeError):
        CustomConfigObject.parse(invalid)

  def test_load_dict_invalid(self):
    with self.assertRaises(argparse.ArgumentTypeError):
      CustomConfigObject.parse({})
    with self.assertRaises(argparse.ArgumentTypeError):
      CustomConfigObject.parse({"name": "foo", "array": 1})
    with self.assertRaises(argparse.ArgumentTypeError):
      CustomConfigObject.parse({"name": "foo", "array": [], "integer": "a"})
    with self.assertRaises(argparse.ArgumentTypeError):
      CustomConfigObject.load_dict({"name": "foo", "array": [], "integer": "a"})

  def test_load_dict(self):
    config = CustomConfigObject.parse({"name": "foo"})
    assert isinstance(config, CustomConfigObject)
    self.assertEqual(config.name, "foo")
    config = CustomConfigObject.parse({"name": "foo", "array": []})
    self.assertEqual(config.name, "foo")
    self.assertListEqual(config.array, [])
    data = {"name": "foo", "array": [1, 2, 3], "integer": 153}
    config = CustomConfigObject.parse(dict(data))
    assert isinstance(config, CustomConfigObject)
    self.assertEqual(config.name, "foo")
    self.assertListEqual(config.array, [1, 2, 3])
    self.assertEqual(config.integer, 153)
    config_2 = CustomConfigObject.load_dict(dict(data))
    assert isinstance(config, CustomConfigObject)
    self.assertEqual(config, config_2)

  def test_loads(self):
    config = CustomConfigObject.parse("a name")
    assert isinstance(config, CustomConfigObject)
    self.assertEqual(config.name, "a name")

  def test_load_path_missing_file(self):
    path = pathlib.Path("invalid.file")
    self.assertFalse(path.exists())
    with self.assertRaises(argparse.ArgumentTypeError):
      CustomConfigObject.parse(path)
    with self.assertRaises(argparse.ArgumentTypeError):
      CustomConfigObject.load_path(path)

  def test_load_path_empty_file(self):
    path = pathlib.Path("test_file.json")
    self.assertFalse(path.exists())
    path.touch()
    with self.assertRaises(argparse.ArgumentTypeError):
      CustomConfigObject.parse(path)
    with self.assertRaises(argparse.ArgumentTypeError):
      CustomConfigObject.load_path(path)

  def test_load_path_invalid_json_file(self):
    path = pathlib.Path("test_file.json")
    path.write_text("{{", encoding="utf-8")
    with self.assertRaises(argparse.ArgumentTypeError):
      CustomConfigObject.parse(path)
    with self.assertRaises(argparse.ArgumentTypeError):
      CustomConfigObject.load_path(path)

  def test_load_path_empty_json_object(self):
    path = pathlib.Path("test_file.json")
    with path.open("w", encoding="utf-8") as f:
      json.dump({}, f)
    with self.assertRaises(argparse.ArgumentTypeError) as cm:
      CustomConfigObject.parse(path)
    self.assertIn("non-empty data", str(cm.exception))

  def test_load_path_invalid_json_array(self):
    path = pathlib.Path("test_file.json")
    with path.open("w", encoding="utf-8") as f:
      json.dump([], f)
    with self.assertRaises(argparse.ArgumentTypeError) as cm:
      CustomConfigObject.parse(path)
    self.assertIn("non-empty data", str(cm.exception))

  def test_load_path_minimal(self):
    path = pathlib.Path("test_file.json")
    with path.open("w", encoding="utf-8") as f:
      json.dump({"name": "Config Name"}, f)
    config = CustomConfigObject.load_path(path)
    assert isinstance(config, CustomConfigObject)
    self.assertEqual(config.name, "Config Name")
    self.assertIsNone(config.array)
    self.assertIsNone(config.integer)
    self.assertIsNone(config.nested)
    config_2 = CustomConfigObject.parse(str(path))
    self.assertEqual(config, config_2)

  TEST_DICT = frozendict({
      "name": "Config Name",
      "array": [1, 3],
      "integer": 166
  })

  def test_load_path_full(self):
    path = pathlib.Path("test_file.json")
    with path.open("w", encoding="utf-8") as f:
      json.dump(dict(self.TEST_DICT), f)
    config = CustomConfigObject.load_path(path)
    assert isinstance(config, CustomConfigObject)
    self.assertEqual(config.name, "Config Name")
    self.assertListEqual(config.array, [1, 3])
    self.assertEqual(config.integer, 166)
    self.assertIsNone(config.nested)
    config_2 = CustomConfigObject.parse(str(path))
    self.assertEqual(config, config_2)

  def test_load_dict_full(self):
    config = CustomConfigObject.load_dict(dict(self.TEST_DICT))
    assert isinstance(config, CustomConfigObject)
    self.assertEqual(config.name, "Config Name")
    self.assertListEqual(config.array, [1, 3])
    self.assertEqual(config.integer, 166)
    self.assertIsNone(config.nested)

  TEST_DICT_NESTED = frozendict({"name": "a nested name"})

  def test_load_dict_nested(self):
    test_dict = dict(self.TEST_DICT)
    test_dict["nested"] = dict(self.TEST_DICT_NESTED)
    config = CustomConfigObject.load_dict(test_dict)
    assert isinstance(config, CustomConfigObject)
    self.assertEqual(config.name, "Config Name")
    self.assertListEqual(config.array, [1, 3])
    self.assertEqual(config.integer, 166)
    self.assertEqual(config.nested,
                     CustomNestedConfigObject(name="a nested name"))

  def test_load_dict_nested_file(self):
    path = pathlib.Path("nested.json")
    self.assertFalse(path.exists())
    with path.open("w", encoding="utf-8") as f:
      json.dump(dict(self.TEST_DICT_NESTED), f)
    test_dict = dict(self.TEST_DICT)
    test_dict["nested"] = str(path)
    config = CustomConfigObject.load_dict(test_dict)
    assert isinstance(config, CustomConfigObject)
    self.assertEqual(config.nested,
                     CustomNestedConfigObject(name="a nested name"))
