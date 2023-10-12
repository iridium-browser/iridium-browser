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

_PATTERN_DATE = re.compile(r"^\d{4}-(0|1)\d-[0-3]\d$")


class DateField(field_types.MetadataField):
    """Custom field for the date when the package was updated."""
    def __init__(self):
        super().__init__(name="Date", one_liner=True)

    def validate(self, value: str) -> Union[vr.ValidationResult, None]:
        """Checks the given value is a YYYY-MM-DD date."""
        if util.matches(_PATTERN_DATE, value):
            return None

        return vr.ValidationError(reason=f"{self._name} is invalid.",
                                  additional=[
                                      "The correct format is YYYY-MM-DD.",
                                      f"Current value is '{value}'.",
                                  ])
