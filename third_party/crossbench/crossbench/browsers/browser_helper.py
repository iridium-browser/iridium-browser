# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import pathlib
import re
from typing import Optional

BROWSERS_CACHE = pathlib.Path(__file__).parents[2] / ".browsers-cache"

_FLAG_TO_PATH_RE = re.compile(r"[-/\\:.]")


def convert_flags_to_label(*flags: str, index: Optional[int] = None) -> str:
  label = "default"
  if flags:
    label = _FLAG_TO_PATH_RE.sub("_", "_".join(flags).replace("--", ""))
  if index is None:
    return label
  return f"{str(index).rjust(2,'0')}_{label}"
