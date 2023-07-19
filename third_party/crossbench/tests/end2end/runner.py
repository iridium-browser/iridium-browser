#!/usr/bin/env python3
# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import pytest
import pathlib
import sys

end2end_test_dir = pathlib.Path(__file__).absolute().parent
repo_dir = pathlib.Path(__file__).absolute().parents[2]

if repo_dir not in sys.path:
  sys.path.insert(0, str(repo_dir))

if __name__ == '__main__':
  # Print output directly for easier debugging.
  return_code = pytest.main(
      ["--exitfirst", "--verbose", "--capture=tee-sys",
       str(end2end_test_dir)])
  sys.exit(return_code)
