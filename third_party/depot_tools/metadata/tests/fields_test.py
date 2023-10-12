#!/usr/bin/env vpython3
# Copyright (c) 2023 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
from typing import List
import unittest

_THIS_DIR = os.path.abspath(os.path.dirname(__file__))
# The repo's root directory.
_ROOT_DIR = os.path.abspath(os.path.join(_THIS_DIR, "..", ".."))

# Add the repo's root directory for clearer imports.
sys.path.insert(0, _ROOT_DIR)

import metadata.fields.known as known_fields
import metadata.fields.field_types as field_types
import metadata.validation_result as vr


class FieldValidationTest(unittest.TestCase):
    def _run_field_validation(self,
                              field: field_types.MetadataField,
                              valid_values: List[str],
                              error_values: List[str],
                              warning_values: List[str] = []):
        """Helper to run a field's validation for different values."""
        for value in valid_values:
            self.assertIsNone(field.validate(value), value)

        for value in error_values:
            self.assertIsInstance(field.validate(value), vr.ValidationError,
                                  value)

        for value in warning_values:
            self.assertIsInstance(field.validate(value), vr.ValidationWarning,
                                  value)

    def test_freeform_text_validation(self):
        # Check validation of a freeform text field that should be on
        # one line.
        self._run_field_validation(
            field=field_types.FreeformTextField("Freeform single",
                                                one_liner=True),
            valid_values=["Text on single line", "a", "1"],
            error_values=["", "\n", " "],
        )

        # Check validation of a freeform text field that can span
        # multiple lines.
        self._run_field_validation(
            field=field_types.FreeformTextField("Freeform multi",
                                                one_liner=False),
            valid_values=[
                "This is text spanning multiple lines:\n"
                "    * with this point\n"
                "    * and this other point",
                "Text on single line",
                "a",
                "1",
            ],
            error_values=["", "\n", " "],
        )

    def test_yes_no_field_validation(self):
        self._run_field_validation(
            field=field_types.YesNoField("Yes/No test"),
            valid_values=["yes", "no", "No", "YES"],
            error_values=["", "\n", "Probably yes"],
            warning_values=["Yes?", "not"],
        )

    def test_cpe_prefix_validation(self):
        self._run_field_validation(
            field=known_fields.CPE_PREFIX,
            valid_values=[
                "unknown",
                "cpe:2.3:a:sqlite:sqlite:3.0.0:*:*:*:*:*:*:*",
                "cpe:2.3:a:sqlite:sqlite:*:*:*:*:*:*:*:*",
                "cpe:/a:vendor:product:version:update:edition:lang",
                "cpe:/a::product:",
                "cpe:/:vendor::::edition",
                "cpe:/:vendor",
            ],
            error_values=[
                "",
                "\n",
                "cpe:2.3:a:sqlite:sqlite:3.0.0",
                "cpe:2.3:a:sqlite:sqlite::::::::",
                "cpe:/",
                "cpe:/a:vendor:product:version:update:edition:lang:",
            ],
        )

    def test_date_validation(self):
        self._run_field_validation(
            field=known_fields.DATE,
            valid_values=["2012-03-04"],
            error_values=["", "\n", "April 3, 2012", "2012/03/04"],
        )

    def test_license_validation(self):
        self._run_field_validation(
            field=known_fields.LICENSE,
            valid_values=[
                "Apache, 2.0 / MIT / MPL 2",
                "LGPL 2.1",
                "Apache, Version 2 and Public domain",
            ],
            error_values=["", "\n", ",", "Apache 2.0 / MIT / "],
            warning_values=[
                "Custom license",
                "Custom / MIT",
                "Public domain or MPL 2",
                "APSL 2 and the MIT license",
            ],
        )

    def test_license_file_validation(self):
        self._run_field_validation(
            field=known_fields.LICENSE_FILE,
            valid_values=[
                "LICENSE", "src/LICENSE.txt",
                "LICENSE, //third_party_test/LICENSE-TEST",
                "src/MISSING_LICENSE"
            ],
            error_values=["", "\n", ","],
            warning_values=["NOT_SHIPPED"],
        )

        # Check relative path from README directory, and multiple
        # license files.
        result = known_fields.LICENSE_FILE.validate_on_disk(
            value="LICENSE, src/LICENSE.txt",
            source_file_dir=os.path.join(_THIS_DIR, "data"),
            repo_root_dir=_THIS_DIR,
        )
        self.assertIsNone(result)

        # Check relative path from Chromium src directory.
        result = known_fields.LICENSE_FILE.validate_on_disk(
            value="//data/LICENSE",
            source_file_dir=os.path.join(_THIS_DIR, "data"),
            repo_root_dir=_THIS_DIR,
        )
        self.assertIsNone(result)

        # Check missing file.
        result = known_fields.LICENSE_FILE.validate_on_disk(
            value="MISSING_LICENSE",
            source_file_dir=os.path.join(_THIS_DIR, "data"),
            repo_root_dir=_THIS_DIR,
        )
        self.assertIsInstance(result, vr.ValidationError)

        # Check deprecated NOT_SHIPPED.
        result = known_fields.LICENSE_FILE.validate_on_disk(
            value="NOT_SHIPPED",
            source_file_dir=os.path.join(_THIS_DIR, "data"),
            repo_root_dir=_THIS_DIR,
        )
        self.assertIsInstance(result, vr.ValidationWarning)

    def test_url_validation(self):
        self._run_field_validation(
            field=known_fields.URL,
            valid_values=[
                "https://www.example.com/a",
                "http://www.example.com/b",
                "ftp://www.example.com/c,git://www.example.com/d",
                "This is the canonical public repository",
            ],
            error_values=[
                "",
                "\n",
                "ghttps://www.example.com/e",
                "https://www.example.com/ f",
                "Https://www.example.com/g",
                "This is an unrecognized message for the URL",
            ],
        )

    def test_version_validation(self):
        self._run_field_validation(
            field=known_fields.VERSION,
            valid_values=["n / a", "123abc", "unknown forked version"],
            error_values=["", "\n"],
            warning_values=["0", "unknown"],
        )


if __name__ == "__main__":
    unittest.main()
