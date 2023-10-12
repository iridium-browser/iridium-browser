# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import argparse
import collections
import enum
import inspect
import logging
import pathlib
import re
import textwrap
from typing import (Any, Callable, Dict, Generic, Iterable, List, Optional,
                    Tuple, Type, TypeVar, Union, cast)

import tabulate

from crossbench import cli_helper, helper
from crossbench import exception
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
      self._validate_enum_default()
      return
    # TODO: Remove once pytype can handle self.type
    maybe_class: ArgParserType = self.type
    if self.is_list:
      assert isinstance(self.default, collections.abc.Sequence), (
          f"List default must be a sequence, but got: {self.default}")
      assert not isinstance(self.default, str), (
          f"List default should not be a string, but got: {repr(self.default)}")
      if inspect.isclass(maybe_class):
        for default_item in self.default:
          if not isinstance(default_item, maybe_class):
            raise ValueError(
                f"Expected default list item of type={self.type}, "
                f"but got type={type(default_item)}: {default_item}")
    elif maybe_class and inspect.isclass(maybe_class):
      if not isinstance(self.default, maybe_class):
        raise ValueError(f"Expected default value of type={self.type}, "
                         f"but got type={type(self.default)}: {self.default}")

  def _validate_enum_default(self) -> None:
    enum_type: Type[enum.Enum] = cast(Type[enum.Enum], self.type)
    if self.is_list:
      default_list = self.default
    else:
      default_list = [self.default]
    for default in default_list:
      assert isinstance(default, enum_type), (
          f"Default must be a {enum_type} enum, but got: {self.default}")

  @property
  def cls(self) -> Type:
    return self.parser.cls

  @property
  def cls_name(self) -> str:
    return self.cls.__name__

  @property
  def help_text(self) -> str:
    items: List[Tuple[str, str]] = []
    if self.type is None:
      if self.is_list:
        items.append(("type", "list"))
    else:
      if self.is_list:
        items.append(("type", f"List[{self.type.__qualname__}]"))
      else:
        items.append(("type", str(self.type.__qualname__)))

    if self.default is None:
      items.append(("default", "not set"))
    else:
      if self.is_list:
        if not self.default:
          items.append(("default", "[]"))
        else:
          items.append(("default", ",".join(map(str, self.default))))
      else:
        items.append(("default", str(self.default)))
    if self.is_enum:
      items.extend(self._enum_help_text())
    elif self.choices:
      items.append(self._choices_help_text(self.choices))

    text = tabulate.tabulate(items, tablefmt="presto")
    if self.help:
      return f"{self.help}\n{text}"
    return text

  def _choices_help_text(self, choices: Iterable) -> Tuple[str, str]:
    return ("choices", ", ".join(map(str, choices)))

  def _enum_help_text(self) -> List[Tuple[str, str]]:
    if self.type and hasattr(self.type, "help_text_items"):
      # See helper.EnumWithHelp
      return [("choices", ""), *self.type.help_text_items()]
    assert self.choices
    return [self._choices_help_text(choice.value for choice in self.choices)]

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
    config_object_cls = self.type  # pytype needs a local for inspect.isclass.
    if (inspect.isclass(config_object_cls) and
        issubclass(config_object_cls, ConfigObject)):
      return config_object_cls.parse(data)
    return self.type(data)

  def parse_enum_data(self, data: Any) -> enum.Enum:
    assert self.is_enum
    assert self.choices
    for enum_instance in self.choices:
      if data in (enum_instance, enum_instance.value):
        return enum_instance
    raise ValueError("Expected enum {self.type}, but got {data}")


_PATH_PREFIX = re.compile(r"(\./|/|[a-zA-Z]:\\)[^\\/]")


class ConfigObject(abc.ABC):
  VALID_EXTENSIONS: Tuple[str, ...] = (".hjson", ".json")

  @classmethod
  def value_has_path_prefix(cls, value: str) -> bool:
    return _PATH_PREFIX.match(value) is not None

  @classmethod
  def parse(cls, value: Any) -> ConfigObject:
    if not value:
      raise argparse.ArgumentTypeError("Empty config value")
    if isinstance(value, dict):
      return cls.load_dict(value)
    if isinstance(value, (str, pathlib.Path)):
      maybe_config = cls.maybe_load_path(value)
      if maybe_config:
        return maybe_config
    if isinstance(value, str):
      return cls.loads(value)
    raise argparse.ArgumentTypeError(
        f"Invalid config input type {type(value).__name__}: {value}")

  @classmethod
  @abc.abstractmethod
  def loads(cls, value: str) -> ConfigObject:
    raise NotImplementedError()

  @classmethod
  def maybe_load_path(
      cls, value: Union[str, pathlib.Path]) -> Optional[ConfigObject]:
    maybe_config_path = pathlib.Path(value)
    if (maybe_config_path.suffix in cls.VALID_EXTENSIONS and
        maybe_config_path.is_file()):
      return cls.load_path(maybe_config_path)
    return None

  @classmethod
  def load_path(cls, path: pathlib.Path) -> ConfigObject:
    with exception.annotate_argparsing(f"Parsing {cls.__name__} file: {path}"):
      data = cli_helper.parse_dict_hjson_file(path)
      return cls.load_dict(data)

  @classmethod
  @abc.abstractmethod
  def load_dict(cls, config: Dict[str, Any]) -> ConfigObject:
    raise NotImplementedError()


ConfigResultObjectT = TypeVar("ConfigResultObjectT", bound="object")


class ConfigParser(Generic[ConfigResultObjectT]):

  def __init__(self, title: str, cls: Type[ConfigResultObjectT]) -> None:
    self.title = title
    assert title, "No title provided"
    self._cls = cls
    self._args: Dict[str, _ConfigArg] = {}

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

  def kwargs_from_config(self, config_data: Dict[str, Any]) -> Dict[str, Any]:
    with exception.annotate_argparsing(
        f"Parsing {self._cls.__name__} config dict:"):
      kwargs: Dict[str, Any] = {}
      for arg_parser in self._args.values():
        with exception.annotate(f"Parsing ...['{arg_parser.name}']:"):
          kwargs[arg_parser.name] = arg_parser.parse(config_data)
      return kwargs

  def parse(self, config_data: Dict[str, Any]) -> ConfigResultObjectT:
    kwargs = self.kwargs_from_config(config_data)
    if config_data:
      logging.debug("Got unused properties: %s", config_data.keys())
    return self.cls(**kwargs)

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
    wdith = 80
    if doc_string:
      parts.append("\n".join(textwrap.wrap(doc_string, width=wdith)))
      parts.append("")
    if not self._args:
      if parts:
        return parts[0]
      return ""
    parts.append(f"{self.title} Configuration:")
    parts.append("")
    for arg in self._args.values():
      parts.append(f"{arg.name}:")
      parts.extend(helper.wrap_lines(arg.help_text, width=wdith, indent="  "))
      parts.append("")
    return "\n".join(parts)
