# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import collections
import enum
import inspect
import textwrap
from typing import (Any, Callable, Dict, Iterable, List, Optional, Type, Union,
                    cast)

from crossbench import helper
from crossbench.exception import ExceptionAnnotator

ArgParserType = Union[Callable[[Any], Any], Type]


class _ConfigArg:

  def __init__(  # pylint: disable=redefined-builtin
      self,
      parser: ConfigParser,
      name: str,
      type: Optional[ArgParserType],
      default: Any = None,
      choices: Optional[frozenset[Any]] = None,
      help: Optional[str] = None,
      is_list: bool = False,
      required: bool = False):
    self.parser = parser
    self.name = name
    self.type = type
    self.default = default
    self.help = help
    self.is_list = is_list
    self.required = required
    self.is_enum: bool = inspect.isclass(type) and issubclass(type, enum.Enum)
    if self.type:
      assert callable(self.type), (
          f"Expected type to be a class or a callable, but got: {self.type}")
    self.choices: Optional[frozenset] = self._validate_choices(choices)
    if self.default is not None:
      self._validate_default()

  def _validate_choices(
      self, choices: Optional[frozenset[Any]]) -> Optional[frozenset]:
    if self.is_enum:
      return self._validate_enum_choices(choices)
    if choices is None:
      return None
    choices_list = list(choices)
    assert choices_list, f"Got empty choices: {choices}"
    frozen_choices = frozenset(choices_list)
    if len(frozen_choices) != len(choices_list):
      raise ValueError("Choices must be unique, but got: {choices}")
    return frozen_choices

  def _validate_enum_choices(
      self, choices: Optional[frozenset[Any]]) -> Optional[frozenset]:
    assert self.is_enum
    assert self.type
    enum_type: Type[enum.Enum] = cast(Type[enum.Enum], self.type)
    if choices is None:
      return frozenset(enum for enum in enum_type)
    for choice in choices:
      assert isinstance(
          choice,
          enum_type), (f"Enum choices must be {enum_type}, but got: {choice}")
    return frozenset(choices)

  def _validate_default(self) -> None:
    if self.is_enum:
      enum_type: Type[enum.Enum] = cast(Type[enum.Enum], self.type)
      assert isinstance(self.default, enum_type), (
          f"Default must a {enum_type} enum, but got: {self.default}")
    # TODO: Remove once pytype can handle self.type
    maybe_class: ArgParserType = self.type
    if self.is_list:
      assert isinstance(self.default, collections.abc.Sequence), (
          f"List default must be a sequence, but got: {self.default}")
      assert not isinstance(self.default, str), (
          f"List default should not be a string, but got: {repr(self.default)}")
      if inspect.isclass(maybe_class):
        for default_item in self.default:
          assert isinstance(
              default_item,
              self.type), (f"Expected default list item of type={self.type}, "
                           f"but got type={type(default_item)}: {default_item}")
    elif self.type and inspect.isclass(maybe_class):
      assert isinstance(
          self.default,
          self.type), (f"Expected default value of type={self.type}, "
                       f"but got type={type(self.default)}: {self.default}")

  @property
  def cls(self) -> Type:
    return self.parser.cls

  @property
  def cls_name(self) -> str:
    return self.cls.__name__

  @property
  def help_text(self) -> str:
    items: List[str] = []
    if self.help:
      items.append(self.help)
    if self.type is None:
      if self.is_list:
        items.append("type    = list")
    else:
      if self.is_list:
        items.append(f"type    = List[{self.type.__qualname__}]")
      else:
        items.append(f"type    = {self.type.__qualname__}")

    if self.default is None:
      items.append("default = not set")
    else:
      if self.is_list:
        if not self.default:
          items.append("default = []")
        else:
          items.append(f"default = {','.join(map(str, self.default))}")
      else:
        items.append(f"default = {self.default}")
    if self.choices:
      items.append(f"choices = {', '.join(map(str, self.choices))}")

    return "\n".join(items)

  def parse(self, config_data: Dict[str, Any]) -> Any:
    data = config_data.pop(self.name, None)
    if data is None:
      if self.required and self.default is None:
        raise ValueError(
            f"{self.cls_name}: "
            f"No value provided for required config option '{self.name}'")
      data = self.default
    if data is None:
      return None
    if self.is_list:
      return self.parse_list_data(data)
    return self.parse_data(data)

  def parse_list_data(self, data: Any) -> List[Any]:
    if not isinstance(data, (list, tuple)):
      raise ValueError(f"{self.cls_name}.{self.name}: "
                       f"Expected sequence got {type(data)}")
    return [self.parse_data(value) for value in data]

  def parse_data(self, data: Any) -> Any:
    if self.is_enum:
      return self.parse_enum_data(data)
    if self.choices and data not in self.choices:
      raise ValueError(f"{self.cls_name}.{self.name}: "
                       f"Invalid choice '{data}', choices are {self.choices}")
    if self.type is None:
      return data
    if self.type is bool:
      if not isinstance(data, bool):
        raise ValueError(
            f"{self.cls_name}.{self.name}: Expected bool, but got {data}")
    elif self.type in (float, int):
      if not isinstance(data, (float, int)):
        raise ValueError(
            f"{self.cls_name}.{self.name}: Expected number, got {data}")
    return self.type(data)

  def parse_enum_data(self, data: Any) -> enum.Enum:
    assert self.is_enum
    assert self.choices
    if data in self.choices:
      return data
    for enum_instance in self.choices:
      if data == enum_instance.value:
        return enum_instance
    raise ValueError("Expected enum {self.type}, but got {data}")


class ConfigParser:

  def __init__(self, title: str, cls: Type[object]):
    self.title = title
    assert title, "No title provided"
    self._cls = cls
    self._args: Dict[str, _ConfigArg] = dict()

  def add_argument(  # pylint: disable=redefined-builtin
      self,
      name: str,
      type: Optional[ArgParserType],
      default: Optional[Any] = None,
      choices: Optional[Iterable[Any]] = None,
      help: Optional[str] = None,
      is_list: bool = False,
      required: bool = False) -> None:
    assert name not in self._args, f"Duplicate argument: {name}"
    self._args[name] = _ConfigArg(self, name, type, default, choices, help,
                                  is_list, required)

  def kwargs_from_config(self, config_data: Dict[str, Any],
                         throw: bool = False) -> Dict[str, Any]:
    kwargs: Dict[str, Any] = {}
    exceptions = ExceptionAnnotator(throw=throw)
    for arg in self._args.values():
      with exceptions.capture(f"Parsing ...['{arg.name}']:"):
        kwargs[arg.name] = arg.parse(config_data)
    exceptions.assert_success("Failed to parse config: {}", log=False)
    return kwargs

  @property
  def cls(self) -> Type:
    return self._cls

  @property
  def doc(self) -> str:
    if not self._cls.__doc__:
      return ""
    return self._cls.__doc__.strip()

  def __str__(self) -> str:
    parts: List[str] = []
    doc_string = self.doc
    if doc_string:
      parts.append("\n".join(textwrap.wrap(doc_string, width=60)))
      parts.append("")
    if not self._args:
      if parts:
        return parts[0]
      return ""
    parts.append(f"{self.title} Configuration:")
    parts.append("")
    for arg in self._args.values():
      parts.append(f"{arg.name}:")
      parts.extend(helper.wrap_lines(arg.help_text, width=58, indent="  "))
      parts.append("")
    return "\n".join(parts)
