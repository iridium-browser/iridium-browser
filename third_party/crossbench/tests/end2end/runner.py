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
  pass_through_args = sys.argv[1:]
  return_code = pytest.main([
      "--exitfirst", "--verbose", "--dist=loadgroup",
      str(end2end_test_dir), *pass_through_args
  ])
  sys.exit(return_code)
