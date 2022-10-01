# Copyright 2021 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Enforces generated //infra/config files are up to date.

See http://www.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details on the presubmit API built into depot_tools.
"""

PRESUBMIT_VERSION = '2.0.0'
USE_PYTHON3 = True


def CheckLucicfgGenOutputMain(input_api, output_api):
  return input_api.RunTests(input_api.canned_checks.CheckLucicfgGenOutput(
      input_api, output_api, 'main.star'))


def CheckChangedLUCIConfigs(input_api, output_api):
  return input_api.canned_checks.CheckChangedLUCIConfigs(
      input_api, output_api)
