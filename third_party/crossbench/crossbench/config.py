# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import collections
import inspect
import pathlib
import textwrap
from typing import Any, Callable, Dict, List, Optional, Sequence, Type, Union

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
      choices: Optional[Sequence[Any]] = None,
      help: Optional[str] = None,
      is_list: bool = False,
      required: bool = False):
    self.parser = parser
    self.name = name
    self.type = type
    self.default = default
    self.choices = choices
    self.help = help
    self.is_list = is_list
    self.required = required
    if self.type:
      assert callable(self.type), (
          f"Expected type to be a class or a callable, but got: {self.type}")
    if self.default is not None:
      self._validate_default()

  def _validate_default(self) -> None:
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
  def help_text(self) -> str:
    items: List[str] = []
    if self.help:
      items.append(self.help)
    if self.type is None:
      if self.is_list:
        items.append("type    = list")
    else:
      if self.is_list:
        items.append(f"type    = List[{self.type}]")
      else:
        items.append(f"type    = {self.type}")

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
            f"{self.cls}: "
            f"No value provided for required config option '{self.name}'")
      data = self.default
    if data is None:
      return None
    if self.is_list:
      return self.parse_list_data(data)
    return self.parse_data(data)

  def parse_list_data(self, data: Any) -> List[Any]:
    if not isinstance(data, (list, tuple)):
      raise ValueError(f"{self.cls}.{self.name}: "
                       f"Expected sequence got {type(data)}")
    return [self.parse_data(value) for value in data]

  def parse_data(self, data: Any) -> Any:
    if self.type is None:
      return data
    if self.type is bool:
      if not isinstance(data, bool):
        raise ValueError(f"Expected bool, but got {data}")
    elif self.type in (float, int):
      if not isinstance(data, (float, int)):
        raise ValueError(f"Expected number, got {data}")
    return self.type(data)


class ConfigParser:

  # TODO: combine with the similar method in cli.py and find a better name
  def existing_file_type(self, str_value: str) -> pathlib.Path:
    try:
      path = pathlib.Path(str_value).expanduser()
    except RuntimeError as e:
      raise ValueError(f"Invalid Path '{str_value}': {e}") from e
    if not path.exists():
      raise ValueError(f"Path '{path}', does not exist.")
    if not path.is_file():
      raise ValueError(f"Path '{path}', is not a file.")
    return path

  def __init__(self, title: str, cls: Type[object]):
    self.title = title
    assert title, "No title provided"
    self._cls = cls
    self._args: Dict[str, _ConfigArg] = dict()

  def add_argument(  # pylint: disable=redefined-builtin
      self,
      name: str,
      type: Optional[ArgParserType],
      default: Any = None,
      choices: Optional[Sequence[Any]] = None,
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
