# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Documentation on PRESUBMIT.py can be found at:
# http://www.chromium.org/developers/how-tos/depottools/presubmit-scripts

from __future__ import print_function

import os
import subprocess
import sys

USE_PYTHON3 = True

# List of directories to not apply presubmit project checks, relative
# to the NaCl top directory
EXCLUDE_PROJECT_CHECKS = [
    # The following contain test data (including automatically generated),
    # and do not follow our conventions.
    'src/trusted/validator_ragel/testdata/32/',
    'src/trusted/validator_ragel/testdata/64/',
    'src/trusted/validator_x86/testdata/32/',
    'src/trusted/validator_x86/testdata/64/',
    'src/trusted/validator/x86/decoder/generator/testdata/32/',
    'src/trusted/validator/x86/decoder/generator/testdata/64/',
    # The following directories contains automatically generated source,
    # which may not follow our conventions.
    'src/trusted/validator_x86/gen/',
    'src/trusted/validator/x86/decoder/gen/',
    'src/trusted/validator/x86/decoder/generator/gen/',
    'src/trusted/validator/x86/ncval_seg_sfi/gen/',
    'src/trusted/validator_arm/gen/',
    'src/trusted/validator_ragel/gen/',
    # The following files contain code from outside native_client (e.g. newlib)
    # or are known the contain style violations.
    'src/trusted/service_runtime/include/sys/',
    'src/include/win/port_win.h',
    'src/trusted/service_runtime/include/machine/_types.h'
    ]

NACL_TOP_DIR = os.getcwd()
while not os.path.isfile(os.path.join(NACL_TOP_DIR, 'PRESUBMIT.py')):
  NACL_TOP_DIR = os.path.dirname(NACL_TOP_DIR)
  assert len(NACL_TOP_DIR) >= 3, "Could not find NaClTopDir"


def _CommonChecks(input_api, output_api):
  """Checks for both upload and commit."""
  results = []

  results.extend(input_api.canned_checks.PanProjectChecks(
      input_api, output_api, project_name='Native Client',
      excluded_paths=tuple(EXCLUDE_PROJECT_CHECKS)))

  # The commit queue assumes PRESUBMIT.py is standalone.
  # TODO(bradnelson): Migrate code_hygiene to a common location so that
  # it can be used by the commit queue.
  old_sys_path = list(sys.path)
  try:
    sys.path.append(os.path.join(NACL_TOP_DIR, 'tools'))
    sys.path.append(os.path.join(NACL_TOP_DIR, 'build'))
    import code_hygiene
  finally:
    sys.path = old_sys_path
    del old_sys_path

  affected_files = input_api.AffectedFiles(include_deletes=False)
  exclude_dirs = [ NACL_TOP_DIR + '/' + x for x in EXCLUDE_PROJECT_CHECKS ]
  for filename in affected_files:
    filename = filename.AbsoluteLocalPath()
    if filename in exclude_dirs:
      continue
    if not IsFileInDirectories(filename, exclude_dirs):
      errors, warnings = code_hygiene.CheckFile(filename, False)
      for e in errors:
        results.append(output_api.PresubmitError(e, items=errors[e]))
      for w in warnings:
        results.append(output_api.PresubmitPromptWarning(w, items=warnings[w]))

  return results


def IsFileInDirectories(f, dirs):
  """ Returns true if f is in list of directories"""
  for d in dirs:
    if d is os.path.commonprefix([f, d]):
      return True
  return False


def CheckChangeOnUpload(input_api, output_api):
  """Verifies all changes in all files.
  Args:
    input_api: the limited set of input modules allowed in presubmit.
    output_api: the limited set of output modules allowed in presubmit.
  """
  report = []
  report.extend(_CommonChecks(input_api, output_api))
  return report


def CheckChangeOnCommit(input_api, output_api):
  """Verifies all changes in all files and verifies that the
  tree is open and can accept a commit.
  Args:
    input_api: the limited set of input modules allowed in presubmit.
    output_api: the limited set of output modules allowed in presubmit.
  """
  report = []
  report.extend(_CommonChecks(input_api, output_api))
  return report
