#!/usr/bin/env python3
# Copyright 2023 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
import unittest

_THIS_DIR = os.path.abspath(os.path.dirname(__file__))
# The repo's root directory.
_ROOT_DIR = os.path.abspath(os.path.join(_THIS_DIR, "..", ".."))

# Add the repo's root directory for clearer imports.
sys.path.insert(0, _ROOT_DIR)

import gclient_utils
import metadata.validate

# Common paths for tests.
_SOURCE_FILE_DIR = os.path.join(_THIS_DIR, "data")
_VALID_METADATA_FILEPATH = os.path.join(_THIS_DIR, "data",
                                        "README.chromium.test.multi-valid")
_INVALID_METADATA_FILEPATH = os.path.join(
    _THIS_DIR, "data", "README.chromium.test.multi-invalid")


class ValidateContentTest(unittest.TestCase):
    """Tests for the validate_content function."""
    def test_empty(self):
        # Validate empty content (should result in a validation error).
        results = metadata.validate.validate_content(
            content="",
            source_file_dir=_SOURCE_FILE_DIR,
            repo_root_dir=_THIS_DIR,
        )
        self.assertEqual(len(results), 1)
        self.assertTrue(results[0].is_fatal())

    def test_valid(self):
        # Validate valid file content (no errors or warnings).
        results = metadata.validate.validate_content(
            content=gclient_utils.FileRead(_VALID_METADATA_FILEPATH),
            source_file_dir=_SOURCE_FILE_DIR,
            repo_root_dir=_THIS_DIR,
        )
        self.assertEqual(len(results), 0)

    def test_invalid(self):
        # Validate invalid file content (both errors and warnings).
        results = metadata.validate.validate_content(
            content=gclient_utils.FileRead(_INVALID_METADATA_FILEPATH),
            source_file_dir=_SOURCE_FILE_DIR,
            repo_root_dir=_THIS_DIR,
        )
        self.assertEqual(len(results), 11)
        error_count = 0
        warning_count = 0
        for result in results:
            if result.is_fatal():
                error_count += 1
            else:
                warning_count += 1
        self.assertEqual(error_count, 9)
        self.assertEqual(warning_count, 2)


class ValidateFileTest(unittest.TestCase):
    """Tests for the validate_file function."""
    def test_missing(self):
        # Validate a file that does not exist.
        results = metadata.validate.validate_file(
            filepath=os.path.join(_THIS_DIR, "data", "MISSING.chromium"),
            repo_root_dir=_THIS_DIR,
        )
        # There should be exactly 1 error returned.
        self.assertEqual(len(results), 1)
        self.assertTrue(results[0].is_fatal())

    def test_valid(self):
        # Validate a valid file (no errors or warnings).
        results = metadata.validate.validate_file(
            filepath=_VALID_METADATA_FILEPATH,
            repo_root_dir=_THIS_DIR,
        )
        self.assertEqual(len(results), 0)

    def test_invalid(self):
        # Validate an invalid file (both errors and warnings).
        results = metadata.validate.validate_file(
            filepath=_INVALID_METADATA_FILEPATH,
            repo_root_dir=_THIS_DIR,
        )
        self.assertEqual(len(results), 11)
        error_count = 0
        warning_count = 0
        for result in results:
            if result.is_fatal():
                error_count += 1
            else:
                warning_count += 1
        self.assertEqual(error_count, 9)
        self.assertEqual(warning_count, 2)


class CheckFileTest(unittest.TestCase):
    """Tests for the check_file function."""
    def test_missing(self):
        # Check a file that does not exist.
        errors, warnings = metadata.validate.check_file(
            filepath=os.path.join(_THIS_DIR, "data", "MISSING.chromium"),
            repo_root_dir=_THIS_DIR,
        )
        # TODO(aredulla): update this test once validation errors can be
        # returned as errors. Bug: b/285453019.
        # self.assertEqual(len(errors), 1)
        # self.assertEqual(len(warnings), 0)
        self.assertEqual(len(errors), 0)
        self.assertEqual(len(warnings), 1)

    def test_valid(self):
        # Check file with valid content (no errors or warnings).
        errors, warnings = metadata.validate.check_file(
            filepath=_VALID_METADATA_FILEPATH,
            repo_root_dir=_THIS_DIR,
        )
        self.assertEqual(len(errors), 0)
        self.assertEqual(len(warnings), 0)

    def test_invalid(self):
        # Check file with invalid content (both errors and warnings).
        errors, warnings = metadata.validate.check_file(
            filepath=_INVALID_METADATA_FILEPATH,
            repo_root_dir=_THIS_DIR,
        )
        # TODO(aredulla): update this test once validation errors can be
        # returned as errors. Bug: b/285453019.
        # self.assertEqual(len(errors), 9)
        # self.assertEqual(len(warnings), 2)
        self.assertEqual(len(errors), 0)
        self.assertEqual(len(warnings), 11)


if __name__ == "__main__":
    unittest.main()
