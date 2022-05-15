# Copyright (c) 2021 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

PRESUBMIT_VERSION = '2.0.0'

USE_PYTHON3 = True


def CheckUsePython3(input_api, output_api):
  results = []

  if sys.version_info.major != 3:
    results.append(output_api.PresubmitError(
        'Did not use Python3 for //tests/PRESUBMIT.py.'))

  return results
