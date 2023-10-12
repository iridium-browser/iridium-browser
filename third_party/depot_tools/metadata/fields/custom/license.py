#!/usr/bin/env python3
# Copyright 2023 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import re
import sys
from typing import List, Union, Tuple

_THIS_DIR = os.path.abspath(os.path.dirname(__file__))
# The repo's root directory.
_ROOT_DIR = os.path.abspath(os.path.join(_THIS_DIR, "..", "..", ".."))

# Add the repo's root directory for clearer imports.
sys.path.insert(0, _ROOT_DIR)

import metadata.fields.field_types as field_types
import metadata.fields.util as util
import metadata.validation_result as vr

# Copied from ANDROID_ALLOWED_LICENSES in
# https://chromium.googlesource.com/chromium/src/+/refs/heads/main/third_party/PRESUBMIT.py
_ANDROID_ALLOWED_LICENSES = [
    "A(pple )?PSL 2(\.0)?",
    "Android Software Development Kit License",
    "Apache( License)?,?( Version)? 2(\.0)?",
    "(New )?([23]-Clause )?BSD( [23]-Clause)?( with advertising clause)?",
    "GNU Lesser Public License",
    "L?GPL ?v?2(\.[01])?( or later)?( with the classpath exception)?",
    "(The )?MIT(/X11)?(-like)?( License)?",
    "MPL 1\.1 ?/ ?GPL 2(\.0)? ?/ ?LGPL 2\.1",
    "MPL 2(\.0)?",
    "Microsoft Limited Public License",
    "Microsoft Permissive License",
    "Public Domain",
    "Python",
    "SIL Open Font License, Version 1.1",
    "SGI Free Software License B",
    "Unicode, Inc. License",
    "University of Illinois\/NCSA Open Source",
    "X11",
    "Zlib",
]
_PATTERN_LICENSE_ALLOWED = re.compile(
    "^({})$".format("|".join(_ANDROID_ALLOWED_LICENSES)),
    re.IGNORECASE,
)

_PATTERN_VERBOSE_DELIMITER = re.compile(r" and | or | / ")


def process_license_value(value: str,
                          atomic_delimiter: str) -> List[Tuple[str, bool]]:
    """Process a license field value, which may list multiple licenses.

    Args:
        value: the value to process, which may include both verbose and
               atomic delimiters, e.g. "Apache, 2.0 and MIT and custom"
        atomic_delimiter: the delimiter to use as a final step; values
                          will not be further split after using this
                          delimiter.

    Returns: a list of the constituent licenses within the given value,
             and whether the constituent license is on the allowlist.
             e.g. [("Apache, 2.0", True), ("MIT", True),
                   ("custom", False)]
    """
    # Check if the value is on the allowlist as-is, and thus does not
    # require further processing.
    if is_license_allowlisted(value):
        return [(value, True)]

    breakdown = []
    if re.search(_PATTERN_VERBOSE_DELIMITER, value):
        # Split using the verbose delimiters.
        for component in re.split(_PATTERN_VERBOSE_DELIMITER, value):
            breakdown.extend(
                process_license_value(component.strip(), atomic_delimiter))
    else:
        # Split using the standard value delimiter. This results in
        # atomic values; there is no further splitting possible.
        for atomic_value in value.split(atomic_delimiter):
            atomic_value = atomic_value.strip()
            breakdown.append(
                (atomic_value, is_license_allowlisted(atomic_value)))

    return breakdown


def is_license_allowlisted(value: str) -> bool:
    """Returns whether the value is in the allowlist for license
    types.
    """
    return util.matches(_PATTERN_LICENSE_ALLOWED, value)


class LicenseField(field_types.MetadataField):
    """Custom field for the package's license type(s).

    e.g. Apache 2.0, MIT, BSD, Public Domain.
    """
    def __init__(self):
        super().__init__(name="License", one_liner=False)

    def validate(self, value: str) -> Union[vr.ValidationResult, None]:
        """Checks the given value consists of recognized license types.

        Note: this field supports multiple values.
        """
        not_allowlisted = []
        licenses = process_license_value(value,
                                         atomic_delimiter=self.VALUE_DELIMITER)
        for license, allowed in licenses:
            if util.is_empty(license):
                return vr.ValidationError(
                    reason=f"{self._name} has an empty value.")
            if not allowed:
                not_allowlisted.append(license)

        if not_allowlisted:
            return vr.ValidationWarning(
                reason=f"{self._name} has a license not in the allowlist.",
                additional=[
                    f"Separate licenses using a '{self.VALUE_DELIMITER}'.",
                    "Licenses not allowlisted: "
                    f"{util.quoted(not_allowlisted)}.",
                ])

        # Suggest using the standard value delimiter when possible.
        if (re.search(_PATTERN_VERBOSE_DELIMITER, value)
                and self.VALUE_DELIMITER not in value):
            return vr.ValidationWarning(
                reason=f"Separate licenses using a '{self.VALUE_DELIMITER}'.")

        return None
