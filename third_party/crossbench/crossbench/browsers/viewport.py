# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from argparse import ArgumentTypeError
from typing import Tuple

from crossbench import compat


class ViewportMode(compat.StrEnum):
  SIZE = "size"
  MAXIMIZED = "maximized"
  FULLSCREEN = "fullscreen"
  HEADLESS = "headless"


class Viewport:
  DEFAULT: Viewport
  MAXIMIZED: Viewport
  FULLSCREEN: Viewport
  HEADLESS: Viewport

  @classmethod
  def parse(cls, value: str) -> Viewport:
    if not value:
      return cls.DEFAULT
    if value in ("m", "max", "maximised", ViewportMode.MAXIMIZED):
      return cls.MAXIMIZED
    if value in ("f", "full", ViewportMode.FULLSCREEN):
      return cls.FULLSCREEN
    if value == ViewportMode.HEADLESS:
      return cls.HEADLESS
    size, _, position = value.partition(",")
    width, _, height = size.partition("x")
    if not height:
      raise ArgumentTypeError(f"Missing viewport height in input: {value}")
    x = str(cls.DEFAULT.x)
    y = str(cls.DEFAULT.y)
    if position:
      x, _, y = position.partition("x")
      if not y:
        raise ArgumentTypeError(
            f"Missing viewport y position in input: {value}")
    return Viewport(int(width), int(height), int(x), int(y))

  def __init__(self,
               width: int = 1500,
               height: int = 1000,
               x: int = 10,
               y: int = 50,
               mode: ViewportMode = ViewportMode.SIZE):
    self._width = width
    self._height = height
    self._x = x
    self._y = y
    self._mode = mode
    self._validate()

  def _validate(self) -> None:
    if self._mode == ViewportMode.SIZE:
      if self._width <= 0:
        raise ArgumentTypeError(f"width must be > 0, but got {self._width}")
      if self._height <= 0:
        raise ArgumentTypeError(f"height must be > 0, but got {self._height}")
      if self._x < 0:
        raise ArgumentTypeError(f"x must be >= 0, but got {self._x}")
      if self._y < 0:
        raise ArgumentTypeError(f"y must be >= 0, but got {self._y}")
    else:
      if self._width != 0:
        raise ArgumentTypeError(
            "Non-zero width only allowed with ViewportMode.SIZE")
      if self._height != 0:
        raise ArgumentTypeError(
            "Non-zero height only allowed with ViewportMode.SIZE")
      if self._x != 0:
        raise ArgumentTypeError(
            "Non-zero x only allowed with ViewportMode.SIZE")
      if self._y != 0:
        raise ArgumentTypeError(
            "Non-zero y only allowed with ViewportMode.SIZE")

  @property
  def is_default(self) -> bool:
    return self == Viewport.DEFAULT

  @property
  def is_maximized(self) -> bool:
    return self._mode == ViewportMode.MAXIMIZED

  @property
  def is_fullscreen(self) -> bool:
    return self._mode == ViewportMode.FULLSCREEN

  @property
  def is_headless(self) -> bool:
    return self._mode == ViewportMode.HEADLESS

  @property
  def has_size(self) -> bool:
    return self._mode == ViewportMode.SIZE

  @property
  def position(self) -> Tuple[int, int]:
    assert self.has_size, f"Viewport has no explicit size: {self._mode}"
    return (self._x, self._y)

  @property
  def size(self) -> Tuple[int, int]:
    assert self.has_size, f"Viewport has no explicit size: {self._mode}"
    return (self._width, self._height)

  @property
  def width(self) -> int:
    assert self.has_size, f"Viewport has no explicit size: {self._mode}"
    return self._width

  @property
  def height(self) -> int:
    assert self.has_size, f"Viewport has no explicit size: {self._mode}"
    return self._height

  @property
  def x(self) -> int:
    assert self.has_size, f"Viewport has no explicit size: {self._mode}"
    return self._x

  @property
  def y(self) -> int:
    assert self.has_size, f"Viewport has no explicit size: {self._mode}"
    return self._y

  @property
  def mode(self) -> ViewportMode:
    return self._mode

  def __str__(self) -> str:
    return f"Viewport({self.width}x{self.height},{self.x}x{self.y})"


Viewport.DEFAULT = Viewport()
Viewport.MAXIMIZED = Viewport(0, 0, 0, 0, ViewportMode.MAXIMIZED)
Viewport.FULLSCREEN = Viewport(0, 0, 0, 0, ViewportMode.FULLSCREEN)
Viewport.HEADLESS = Viewport(0, 0, 0, 0, ViewportMode.HEADLESS)
