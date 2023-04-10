# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from crossbench.probes import Probe, ProbeConfigParser

import sys
import pytest


class MockProbe(Probe):
  """
  Probe DOC Text
  """


class CustomArgType:

  def __init__(self, value):
    self.value = value


def custom_arg_type(value):
  return CustomArgType(value)


class TestProbeConfig(unittest.TestCase):

  def test_help_text(self):
    parser = ProbeConfigParser(MockProbe)
    parser.add_argument("bool", type=bool)
    parser.add_argument("bool_default", type=bool, default=False)
    parser.add_argument("str_empty_default", type=str, default="")
    parser.add_argument("bool_list", type=bool, default=(False,), is_list=True)
    parser.add_argument("any_list", type=None, default=(1,), is_list=True)
    parser.add_argument(
        "empty_default_list", type=None, default=[], is_list=True)
    parser.add_argument("custom_type", type=custom_arg_type)
    parser.add_argument("custom_help", type=bool, help="custom help")
    help_text = str(parser)
    self.assertIn("Probe DOC Text", help_text)
    self.assertIn("bool_default", help_text)
    self.assertIn("str_empty_default", help_text)
    self.assertIn("bool_list", help_text)
    self.assertIn("any_list", help_text)
    self.assertIn("empty_default_list", help_text)
    self.assertIn("custom_type", help_text)
    self.assertIn("custom_help", help_text)

  def test_invalid_config_duplicate(self):
    parser = ProbeConfigParser(MockProbe)
    parser.add_argument("bool", type=bool)
    with self.assertRaises(AssertionError):
      parser.add_argument("bool", type=bool)

  def test_config_defaults(self):
    parser = ProbeConfigParser(MockProbe)
    with self.assertRaises(AssertionError):
      parser.add_argument("bool", type=bool, default=1)
    parser.add_argument("any", type=object, default=1)

  def test_config_defaults_list(self):
    parser = ProbeConfigParser(MockProbe)
    with self.assertRaises(AssertionError):
      parser.add_argument("bool", type=bool, is_list=True, default=True)
    with self.assertRaises(AssertionError):
      parser.add_argument("str", type=str, is_list=True, default="str")
    with self.assertRaises(AssertionError):
      parser.add_argument(
          "bool", type=bool, is_list=True, default=(
              1,
              1,
          ))
    parser.add_argument(
        "bool", type=bool, is_list=True, default=(
            True,
            False,
        ))
    parser.add_argument(
        "custom_list",
        type=lambda x: x + 1,
        is_list=True,
        default=(
            True,
            False,
        ))

  def test_bool_missing_property(self):
    parser = ProbeConfigParser(MockProbe)
    parser.add_argument("bool_argument_name", type=bool, required=True)
    with self.assertRaises(ValueError):
      parser.kwargs_from_config({})
    with self.assertRaises(ValueError):
      parser.kwargs_from_config({"other": True})

  def test_bool_invalid_value(self):
    parser = ProbeConfigParser(MockProbe)
    parser.add_argument("bool_argument_name", type=bool)
    parser_required = ProbeConfigParser(MockProbe)
    parser_required.add_argument("bool_argument_name", type=bool, required=True)
    with self.assertRaises(ValueError):
      parser.kwargs_from_config({"bool_argument_name": "not a bool"})
    with self.assertRaises(ValueError):
      parser.kwargs_from_config({"bool_argument_name": ""})
    with self.assertRaises(ValueError):
      parser.kwargs_from_config({"bool_argument_name": {}})
    with self.assertRaises(ValueError):
      parser.kwargs_from_config({"bool_argument_name": []})
    # Argument is not required, this should pass
    parser.kwargs_from_config({"bool_argument_name": None})
    with self.assertRaises(ValueError):
      parser_required.kwargs_from_config({"bool_argument_name": None})
    with self.assertRaises(ValueError):
      parser.kwargs_from_config({"bool_argument_name": 0})

  def test_bool_default(self):
    parser = ProbeConfigParser(MockProbe)
    parser.add_argument("bool_argument_name", type=bool, default=False)

    config_data = {}
    kwargs = parser.kwargs_from_config(config_data)
    self.assertDictEqual(config_data, {})
    self.assertDictEqual(kwargs, {"bool_argument_name": False})

    config_data = {"bool_argument_name": True}
    kwargs = parser.kwargs_from_config(config_data)
    self.assertDictEqual(config_data, {})
    self.assertDictEqual(kwargs, {"bool_argument_name": True})

  def test_bool(self):
    parser = ProbeConfigParser(MockProbe)
    parser.add_argument("bool_argument_name", type=bool)
    config_data = {"bool_argument_name": True}
    kwargs = parser.kwargs_from_config(config_data)
    self.assertDictEqual(config_data, {})
    self.assertDictEqual(kwargs, {"bool_argument_name": True})

  def test_int_list_invalid(self):
    parser = ProbeConfigParser(MockProbe)
    parser.add_argument("int_list", type=int, is_list=True, default=[111, 222])
    with self.assertRaises(ValueError):
      parser.kwargs_from_config({"int_list": 9})
    with self.assertRaises(ValueError):
      parser.kwargs_from_config({"int_list": ["0", "1"]})
    with self.assertRaises(ValueError):
      parser.kwargs_from_config({"int_list": "0,1"})

  def test_int_list(self):
    parser = ProbeConfigParser(MockProbe)
    parser.add_argument("int_list", type=int, is_list=True, default=[111, 222])
    kwargs = parser.kwargs_from_config({})
    self.assertDictEqual(kwargs, {"int_list": [111, 222]})

    config_data = {"int_list": [0, 1]}
    kwargs = parser.kwargs_from_config(config_data)
    self.assertDictEqual(config_data, {})
    self.assertDictEqual(kwargs, {"int_list": [0, 1]})

  def test_custom_type(self):
    parser = ProbeConfigParser(MockProbe)
    parser.add_argument("custom", type=custom_arg_type)
    config_data = {"custom": [1, 2, "stuff"]}
    kwargs = parser.kwargs_from_config(config_data)
    self.assertDictEqual(config_data, {})
    result = kwargs["custom"]
    self.assertIsInstance(result, CustomArgType)
    self.assertListEqual(result.value, [1, 2, "stuff"])

  def test_no_type(self):
    parser = ProbeConfigParser(MockProbe)
    parser.add_argument("custom", type=None)
    for data in ["", "a", {"a": 1}, set(), []]:
      config_data = {"custom": data}
      kwargs = parser.kwargs_from_config(config_data)
      self.assertIs(kwargs["custom"], data)


if __name__ == "__main__":
  sys.exit(pytest.main([__file__]))
