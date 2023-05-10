#!/usr/bin/env python3
# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import pathlib
import platform

USE_PYTHON3 = True


def CheckChange(input_api, output_api, on_commit):
  tests = []
  results = []
  testing_env = dict(input_api.environ)
  testing_path = pathlib.Path(input_api.PresubmitLocalPath())
  crossbench_test_path = testing_path / "tests"
  testing_env["PYTHONPATH"] = input_api.os_path.pathsep.join(
      map(str, [testing_path, crossbench_test_path]))
  # ---------------------------------------------------------------------------
  # Validate the vpython spec
  # TODO: enable only for certain platforms due to pytype not being available
  # on windows. Currently this breaks as it validates the .vpython3 for all
  # platforms.
  # if platform.system() in ("Linux", "Darwin"):
  #   tests.append(
  #     input_api.canned_checks.CheckVPythonSpec(input_api, output_api))
  # ---------------------------------------------------------------------------
  # Pylint
  files_to_check = [r"^[^\.]+\.py$"]
  disabled_warnings = [
      "missing-module-docstring",
      "missing-class-docstring",
      "useless-super-delegation",
      "useless-return",
      "line-too-long",  # Annoying false-positives on URLs
      "cyclic-import",  # TODO: This is not working as expected with pytype
  ]
  tests += input_api.canned_checks.GetPylint(
      input_api,
      output_api,
      files_to_check=files_to_check,
      disabled_warnings=disabled_warnings)
  # ---------------------------------------------------------------------------
  # License header checks
  results += input_api.canned_checks.CheckLicense(input_api, output_api)
  # ---------------------------------------------------------------------------
  # Only run test_cli to speed up the presubmit checks
  if on_commit:
    dirs_to_check = crossbench_test_path.glob("**")
    files_to_check = [r".*test_.*\.py$"]
  else:
    # Only check a small subset on upload
    dirs_to_check = [crossbench_test_path]
    files_to_check = [r".*test_cli\.py$"]
  for dir_to_check in dirs_to_check:
    # CBB tests run end-to-end tests and require custom setup.
    if dir_to_check.name == "cbb":
      continue
    # Skip potentially empty dirs
    if dir_to_check.name == "__pycache__":
      continue
    tests += input_api.canned_checks.GetUnitTestsInDirectory(
        input_api,
        output_api,
        directory=dir_to_check,
        env=testing_env,
        files_to_check=files_to_check,
        skip_shebang_check=True,
        run_on_python2=False)
  # ---------------------------------------------------------------------------
  # Pytype (not supported on windows)
  if on_commit and platform.system() in ("Linux", "Darwin"):
    tests.append(
        input_api.Command(
            name="pytype",
            cmd=[
                input_api.python3_executable, "-m", "pytype", "--keep-going",
                "--jobs=auto",
                str(testing_path / "crossbench"),
                str(testing_path / "tests")
            ],
            message=output_api.PresubmitError,
            kwargs={},
            python3=True,
        ))
  # ---------------------------------------------------------------------------
  # Run all test
  results += input_api.RunTests(tests)
  return results


def CheckChangeOnUpload(input_api, output_api):
  return CheckChange(input_api, output_api, on_commit=False)


def CheckChangeOnCommit(input_api, output_api):
  return CheckChange(input_api, output_api, on_commit=True)
