# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# TODO: remove after landing infra change.

import pytest
import pathlib
import sys

repo_dir = pathlib.Path(__file__).absolute().parents[2]
if repo_dir not in sys.path:
  sys.path.insert(0, str(repo_dir))

if __name__ == "__main__":
  test_file = (
      pathlib.Path(__file__).absolute().parents[1] / "end2end" / "cbb" /
      "test_cbb.py")
  sys.exit(pytest.main([str(test_file)]))
