# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import collections
import logging
from typing import (Dict, Final, Iterable, Iterator, Optional, Set, Tuple,
                    Union)


class Flags(collections.UserDict):
  """Basic implementation for command line flags (similar to Dic[str, str].

  This class is mostly used to make sure command-line flags for browsers
  don't end up having contradicting values.
  """

  InitialDataType = Optional[
      Union[Dict[str, str], "Flags", Iterable[Union[Tuple[str, str], str]]]]

  @classmethod
  def split(cls, flag_str: str) -> Tuple[str, Optional[str]]:
    if "=" in flag_str:
      flag_name, flag_value = flag_str.split("=", maxsplit=1)
      return (flag_name, flag_value)
    return (flag_str, None)

  def __init__(self, initial_data: Flags.InitialDataType = None) -> None:
    super().__init__(initial_data)

  def __setitem__(self, flag_name: str, flag_value: Optional[str]) -> None:
    return self.set(flag_name, flag_value)

  def set(self,
          flag_name: str,
          flag_value: Optional[str] = None,
          override: bool = False) -> None:
    self._set(flag_name, flag_value, override)

  def _set(self,
           flag_name: str,
           flag_value: Optional[str] = None,
           override: bool = False) -> None:
    assert flag_name, "Cannot set empty flag"
    assert "=" not in flag_name, (
        f"Flag name contains '=': {flag_name}, please split")
    assert flag_name.startswith("-"), f"Invalid flag name: {flag_name}"
    assert flag_value is None or isinstance(flag_value, str), (
        f"Expected None or string flag-value for flag '{flag_name}', "
        f"but got: {repr(flag_value)}")
    if not override and flag_name in self:
      old_value = self[flag_name]
      assert flag_value == old_value, (
          f"Flag {flag_name}={flag_value} was already set "
          f"with a different previous value: '{old_value}'")
      return
    self.data[flag_name] = flag_value

  # pylint: disable=arguments-differ
  def update(self,
             initial_data: Flags.InitialDataType = None,
             override: bool = False) -> None:
    # pylint: disable=arguments-differ
    if initial_data is None:
      return
    if isinstance(initial_data, (Flags, dict)):
      for flag_name, flag_value in initial_data.items():
        self.set(flag_name, flag_value, override)
    else:
      for flag_name_or_items in initial_data:
        if isinstance(flag_name_or_items, str):
          self.set(flag_name_or_items, None, override)
        else:
          flag_name, flag_value = flag_name_or_items
          self.set(flag_name, flag_value, override)

  def copy(self) -> Flags:
    return self.__class__(self)

  def _describe(self, flag_name: str) -> str:
    value = self.get(flag_name)
    if value is None:
      return flag_name
    return f"{flag_name}={value}"

  def get_list(self) -> Iterator[str]:
    return (k if v is None else f"{k}={v}" for k, v in self.items())

  def __str__(self) -> str:
    return " ".join(self.get_list())


class JSFlags(Flags):
  """Custom flags implementation for V8 flags (--js-flags in chrome)

  Additionally to the base Flag implementation it asserts that bool flags
  with the --no-.../--no... prefix are not contradicting each other.
  """
  _NO_PREFIX = "--no"

  def copy(self) -> JSFlags:
    return self.__class__(self)

  def _set(self,
           flag_name: str,
           flag_value: Optional[str] = None,
           override: bool = False) -> None:
    if flag_value is not None:
      if "," in flag_value:
        raise ValueError(
            "--js-flags: Comma in V8 flag value, flag escaping for chrome's "
            f"--js-flags might not work: {flag_name}={flag_value}")
    if not flag_name.startswith("--"):
      raise ValueError("--js-flags: Only long-form flag names allowed, "
                       f"but got '{flag_name}'")
    self._check_negated_flag(flag_name, override)
    super()._set(flag_name, flag_value, override)

  def _check_negated_flag(self, flag_name: str, override: bool) -> None:
    if flag_name.startswith(self._NO_PREFIX):
      enabled = flag_name[len(self._NO_PREFIX):]
      # Check for --no-foo form
      if enabled.startswith("-"):
        enabled = enabled[1:]
      enabled = "--" + enabled
      if override:
        del self[enabled]
      else:
        assert enabled not in self, (
            f"Conflicting flag '{flag_name}', "
            f"it has already been enabled by '{self._describe(enabled)}'")
    else:
      # --foo => --no-foo
      disabled = f"--no-{flag_name[2:]}"
      if disabled not in self:
        # Try compact version: --foo => --nofoo
        disabled = f"--no{flag_name[2:]}"
        if disabled not in self:
          return
      if override:
        del self[disabled]
      else:
        assert False, (
            f"Conflicting flag '{flag_name}', "
            f"it has previously been disabled by '{self._describe(flag_name)}'")

  def __str__(self) -> str:
    return ",".join(self.get_list())


class ChromeFlags(Flags):
  """Specialized Flags for Chrome/Chromium-based browser.

  This has special treatment for --js-flags and the feature flags:
  --enable-features/--disable-features
  """
  _JS_FLAG = "--js-flags"

  def __init__(self, initial_data: Flags.InitialDataType = None) -> None:
    self._features = ChromeFeatures()
    self._js_flags = JSFlags()
    super().__init__(initial_data)

  def _set(self,
           flag_name: str,
           flag_value: Optional[str] = None,
           override: bool = False) -> None:
    # pylint: disable=signature-differs
    if flag_name == ChromeFeatures.ENABLE_FLAG:
      if flag_value is None:
        raise ValueError(f"{ChromeFeatures.ENABLE_FLAG} cannot be None")
      for feature in flag_value.split(","):
        self._features.enable(feature)
    elif flag_name == ChromeFeatures.DISABLE_FLAG:
      if flag_value is None:
        raise ValueError(f"{ChromeFeatures.DISABLE_FLAG} cannot be None")
      for feature in flag_value.split(","):
        self._features.disable(feature)
    elif flag_name == self._JS_FLAG:
      if flag_value is None:
        raise ValueError(f"{self._JS_FLAG} cannot be None")
      new_js_flags = JSFlags(self._js_flags)
      for js_flag in flag_value.split(","):
        js_flag_name, js_flag_value = Flags.split(js_flag.lstrip())
        new_js_flags.set(js_flag_name, js_flag_value, override=override)
      self._js_flags.update(new_js_flags)
    else:
      self._verify_flag(flag_name, flag_value)
      super()._set(flag_name, flag_value, override)

  def _verify_flag(self, name: str, value: Optional[str]) -> None:
    if name == "--enable-feature":
      logging.error(
          "Potentially misspelled flag: '%s'. "
          "Did you mean to use --enable-features, with an 's'?", name)
    elif name == "--disable-feature":
      logging.error(
          "Potentially misspelled flag:  '%s'. "
          "Did you mean to use --disable-features, with an 's'?", name)
    del value

  @property
  def features(self) -> ChromeFeatures:
    return self._features

  @property
  def js_flags(self) -> JSFlags:
    return self._js_flags

  def items(self) -> Iterable[Tuple[str, Optional[str]]]:
    yield from super().items()
    if self._js_flags:
      yield (self._JS_FLAG, str(self.js_flags))
    yield from self.features.items()


class ChromeFeatures:
  """
  Chrome Features set, throws if features are enabled and disabled at the same
  time.
  Examples:
    --disable-features="MyFeature1"
    --enable-features="MyFeature1,MyFeature2"
    --enable-features="MyFeature1:k1/v1/k2/v2,MyFeature2"
    --enable-features="MyFeature3<Trial2:k1/v1/k2/v2"
  """

  ENABLE_FLAG: Final[str] = "--enable-features"
  DISABLE_FLAG: Final[str] = "--disable-features"

  def __init__(self) -> None:
    self._enabled: Dict[str, Optional[str]] = {}
    # Use dict as ordered set.
    self._disabled: Dict[str, None] = {}

  @property
  def is_empty(self) -> bool:
    return len(self._enabled) == 0 and len(self._disabled) == 0

  @property
  def enabled(self) -> Dict[str, Optional[str]]:
    return self._enabled

  @property
  def disabled(self) -> Set[str]:
    return set(self._disabled.keys())

  def _parse_feature(self, feature: str) -> Tuple[str, Optional[str]]:
    assert feature, "Cannot parse empty feature"
    assert "," not in feature, (
        f"'{feature}' contains multiple features. Please split them first.")
    parts = feature.split("<")
    if len(parts) == 2:
      return (parts[0], "<" + parts[1])
    assert len(parts) == 1
    parts = feature.split(":")
    if len(parts) == 2:
      return (parts[0], ":" + parts[1])
    assert len(parts) == 1
    return (feature, None)

  def enable(self, feature: str) -> None:
    name, value = self._parse_feature(feature)
    if name in self._disabled:
      raise ValueError(f"Cannot enable previously disabled feature={name}")
    if name in self._enabled:
      prev_value = self._enabled[name]
      if value != prev_value:
        raise ValueError(
            f"Cannot set conflicting values ('{prev_value}', vs. '{value}') "
            f"for the same feature={name}")
    else:
      self._enabled[name] = value

  def disable(self, feature: str) -> None:
    name, _ = self._parse_feature(feature)
    if name in self._enabled:
      raise ValueError(f"Cannot disable previously enabled feature={name}")
    self._disabled[name] = None

  def items(self) -> Iterable[Tuple[str, str]]:
    if self._enabled:
      joined = ",".join(
          k if v is None else f"{k}{v}" for k, v in self._enabled.items())
      yield (self.ENABLE_FLAG, joined)
    if self._disabled:
      joined = ",".join(self._disabled.keys())
      yield (self.DISABLE_FLAG, joined)

  def get_list(self) -> Iterable[str]:
    for flag_name, features_str in self.items():
      yield f"{flag_name}={features_str}"

  def __str__(self) -> str:
    result = " ".join(self.get_list())
    return result
