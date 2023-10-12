# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
""" A collection of helpers that exist in future python versions. """

from __future__ import annotations

import enum
import pathlib
import sys

if sys.version_info >= (3, 11):
  from enum import StrEnum
else:

  class StrEnum(str, enum.Enum):

    def __str__(self) -> str:
      return str(self.value)


if sys.version_info >= (3, 9):

  def is_relative_to(path_a: pathlib.Path, path_b: pathlib.Path) -> bool:
    return path_a.is_relative_to(path_b)
else:

  def is_relative_to(path_a: pathlib.Path, path_b: pathlib.Path) -> bool:
    try:
      path_a.relative_to(path_b)
      return True
    except ValueError:
      return False
