#!/usr/bin/env python3
# Copyright 2023 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import re
import sys
from typing import Union

_THIS_DIR = os.path.abspath(os.path.dirname(__file__))
# The repo's root directory.
_ROOT_DIR = os.path.abspath(os.path.join(_THIS_DIR, "..", "..", ".."))

# Add the repo's root directory for clearer imports.
sys.path.insert(0, _ROOT_DIR)

import metadata.fields.field_types as field_types
import metadata.fields.util as util
import metadata.validation_result as vr

_PATTERN_URL_ALLOWED = re.compile(r"^(https?|ftp|git):\/\/\S+$")
_PATTERN_URL_CANONICAL_REPO = re.compile(
    r"^This is the canonical (public )?repo(sitory)?\.?$", re.IGNORECASE)


class URLField(field_types.MetadataField):
    """Custom field for the package URL(s)."""
    def __init__(self):
        super().__init__(name="URL", one_liner=False)

    def validate(self, value: str) -> Union[vr.ValidationResult, None]:
        """Checks the given value has acceptable URL values only.

        Note: this field supports multiple values.
        """
        if util.matches(_PATTERN_URL_CANONICAL_REPO, value):
            return None

        invalid_values = []
        for url in value.split(self.VALUE_DELIMITER):
            url = url.strip()
            if not util.matches(_PATTERN_URL_ALLOWED, url):
                invalid_values.append(url)

        if invalid_values:
            return vr.ValidationError(
                reason=f"{self._name} is invalid.",
                additional=[
                    "URLs must use a protocol scheme in "
                    "[http, https, ftp, git].",
                    f"Separate URLs using a '{self.VALUE_DELIMITER}'.",
                    f"Invalid values: {util.quoted(invalid_values)}.",
                ])

        return None
