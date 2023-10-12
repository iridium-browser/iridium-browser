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

# Pattern for CPE 2.3 URI binding (also compatible with CPE 2.2).
_PATTERN_CPE_URI = re.compile(
    r"^c[pP][eE]:/[AHOaho]?(:[A-Za-z0-9._\-~%]*){0,6}$")

# Pattern that will match CPE 2.3 formatted string binding.
_PATTERN_CPE_FORMATTED_STRING = re.compile(r"^cpe:2\.3:[aho\-\*](:[^:]+){10}$")


def is_uri_cpe(value: str) -> bool:
    """Returns whether the value conforms to the CPE 2.3 URI binding
    (which is compatible with CPE 2.2), with the additional constraint
    that at least one component other than "part" has been specified.

    For reference, see section 6.1 of the CPE Naming Specification
    Version 2.3 at
    https://nvlpubs.nist.gov/nistpubs/Legacy/IR/nistir7695.pdf.
    """
    if not util.matches(_PATTERN_CPE_URI, value):
        return False

    components = value.split(":")
    if len(components) < 3:
        # At most, only part was provided.
        return False

    # Check at least one component other than "part" has been specified.
    for component in components[2:]:
        if component:
            return True

    return False


def is_formatted_string_cpe(value: str) -> bool:
    """Returns whether the value conforms to the CPE 2.3 formatted
    string binding.

    For reference, see section 6.2 of the CPE Naming Specification
    Version 2.3 at
    https://nvlpubs.nist.gov/nistpubs/Legacy/IR/nistir7695.pdf.
    """
    return util.matches(_PATTERN_CPE_FORMATTED_STRING, value)


class CPEPrefixField(field_types.MetadataField):
    """Custom field for the package's CPE."""
    def __init__(self):
        super().__init__(name="CPEPrefix", one_liner=True)

    def validate(self, value: str) -> Union[vr.ValidationResult, None]:
        """Checks the given value is either 'unknown', or conforms to
        either the CPE 2.3 or 2.2 format.
        """
        if (util.is_unknown(value) or is_formatted_string_cpe(value)
                or is_uri_cpe(value)):
            return None

        return vr.ValidationError(
            reason=f"{self._name} is invalid.",
            additional=[
                "This field should be a CPE (version 2.3 or 2.2), "
                "or 'unknown'.",
                "Search for a CPE tag for the package at "
                "https://nvd.nist.gov/products/cpe/search.",
                f"Current value: '{value}'.",
            ])
