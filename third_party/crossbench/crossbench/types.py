# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Union, Dict, List, Tuple

JSON = Union["JsonDict", "JsonList", "JsonTuple", str, int, float, bool, None]
JsonDict = Dict[str, "JSON"]
JsonList = List["JSON"]
JsonTuple = Tuple["JSON"]