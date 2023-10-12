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
import metadata.parse


class ParseTest(unittest.TestCase):
    def test_parse_single(self):
        """Check parsing works for a single dependency's metadata."""
        filepath = os.path.join(_THIS_DIR, "data",
                                "README.chromium.test.single-valid")
        content = gclient_utils.FileRead(filepath)
        all_metadata = metadata.parse.parse_content(content)

        self.assertEqual(len(all_metadata), 1)
        self.assertListEqual(
            all_metadata[0].get_entries(),
            [
                ("Name", "Test-A README for Chromium metadata"),
                ("Short Name", "metadata-test-valid"),
                ("URL", "https://www.example.com/metadata,\n"
                 "     https://www.example.com/parser"),
                ("Version", "1.0.12"),
                ("Date", "2020-12-03"),
                ("License", "Apache, 2.0 and MIT"),
                ("License File", "LICENSE"),
                ("Security Critical", "yes"),
                ("Shipped", "yes"),
                ("CPEPrefix", "unknown"),
                ("Description", "A test metadata file, with a\n"
                 " multi-line description."),
                ("Local Modifications", "None,\nEXCEPT:\n* nothing."),
            ],
        )

    def test_parse_multiple(self):
        """Check parsing works for multiple dependencies' metadata."""
        filepath = os.path.join(_THIS_DIR, "data",
                                "README.chromium.test.multi-invalid")
        content = gclient_utils.FileRead(filepath)
        all_metadata = metadata.parse.parse_content(content)

        # Dependency metadata with no entries at all are ignored.
        self.assertEqual(len(all_metadata), 3)

        # Check entries are added according to fields being one-liners.
        self.assertListEqual(
            all_metadata[0].get_entries(),
            [
                ("Name",
                 "Test-A README for Chromium metadata (0 errors, 0 warnings)"),
                ("Short Name", "metadata-test-valid"),
                ("URL", "https://www.example.com/metadata,\n"
                 "     https://www.example.com/parser"),
                ("Version", "1.0.12"),
                ("Date", "2020-12-03"),
                ("License", "Apache, 2.0 and MIT"),
                ("License File", "LICENSE"),
                ("Security Critical", "yes"),
                ("Shipped", "yes"),
                ("CPEPrefix", "unknown"),
                ("Description", "A test metadata file, with a\n"
                 " multi-line description."),
                ("Local Modifications", "None,\nEXCEPT:\n* nothing."),
            ],
        )

        # Check the parser handles different casing for field names, and
        # strips leading and trailing whitespace from values.
        self.assertListEqual(
            all_metadata[1].get_entries(),
            [
                ("Name",
                 "Test-B README for Chromium metadata (4 errors, 1 warning)"),
                ("SHORT NAME", "metadata-test-invalid"),
                ("URL", "file://home/drive/chromium/src/metadata"),
                ("Version", "0"),
                ("Date", "2020-12-03"),
                ("License", "MIT"),
                ("Security critical", "yes"),
                ("Shipped", "Yes"),
                ("Description", ""),
                ("Local Modifications", "None."),
            ],
        )

        # Check repeated fields persist in the metadata's entries.
        self.assertListEqual(
            all_metadata[2].get_entries(),
            [
                ("Name",
                 "Test-C README for Chromium metadata (5 errors, 1 warning)"),
                ("URL", "https://www.example.com/first"),
                ("URL", "https://www.example.com/second"),
                ("Version", "N/A"),
                ("Date", "2020-12-03"),
                ("License", "Custom license"),
                ("Security Critical", "yes"),
                ("Description", "Test metadata with multiple entries for one "
                 "field, and\nmissing a mandatory field."),
            ],
        )


if __name__ == "__main__":
    unittest.main()
