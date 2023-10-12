# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import pathlib
from typing import Union
import pytest
import sys


def run_pytest(path: Union[str, pathlib.Path], *args):
  extra_args = [*args, *sys.argv[1:]]
  # Run tests single-threaded by default when running the test file directly.
  if "-n" not in extra_args:
    extra_args.extend(["-n", "1"])
  sys.exit(pytest.main([str(path), *extra_args]))
