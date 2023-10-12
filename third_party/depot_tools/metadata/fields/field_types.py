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
_ROOT_DIR = os.path.abspath(os.path.join(_THIS_DIR, "..", ".."))

# Add the repo's root directory for clearer imports.
sys.path.insert(0, _ROOT_DIR)

import metadata.fields.util as util
import metadata.validation_result as vr

# Pattern used to check if the entire string is either "yes" or "no",
# case-insensitive.
_PATTERN_YES_OR_NO = re.compile(r"^(yes|no)$", re.IGNORECASE)

# Pattern used to check if the string starts with "yes" or "no",
# case-insensitive. e.g. "No (test only)", "Yes?"
_PATTERN_STARTS_WITH_YES_OR_NO = re.compile(r"^(yes|no)", re.IGNORECASE)


class MetadataField:
    """Base class for all metadata fields."""

    # The delimiter used to separate multiple values.
    VALUE_DELIMITER = ","

    def __init__(self, name: str, one_liner: bool = True):
        self._name = name
        self._one_liner = one_liner

    def __eq__(self, other):
        if not isinstance(other, MetadataField):
            return False

        return self._name.lower() == other._name.lower()

    def __hash__(self):
        return hash(self._name.lower())

    def get_name(self):
        return self._name

    def is_one_liner(self):
        return self._one_liner

    def validate(self, value: str) -> Union[vr.ValidationResult, None]:
        """Checks the given value is acceptable for the field.

        Raises: NotImplementedError if called. This method must be
                overridden with the actual validation of the field.
        """
        raise NotImplementedError(
            f"{self._name} field validation not defined.")


class FreeformTextField(MetadataField):
    """Field where the value is freeform text."""
    def validate(self, value: str) -> Union[vr.ValidationResult, None]:
        """Checks the given value has at least one non-whitespace
        character.
        """
        if util.is_empty(value):
            return vr.ValidationError(reason=f"{self._name} is empty.")

        return None


class YesNoField(MetadataField):
    """Field where the value must be yes or no."""
    def __init__(self, name: str):
        super().__init__(name=name, one_liner=True)

    def validate(self, value: str) -> Union[vr.ValidationResult, None]:
        """Checks the given value is either yes or no."""
        if util.matches(_PATTERN_YES_OR_NO, value):
            return None

        if util.matches(_PATTERN_STARTS_WITH_YES_OR_NO, value):
            return vr.ValidationWarning(
                reason=f"{self._name} is invalid.",
                additional=[
                    f"This field should be only {util.YES} or {util.NO}.",
                    f"Current value is '{value}'.",
                ])

        return vr.ValidationError(
            reason=f"{self._name} is invalid.",
            additional=[
                f"This field must be {util.YES} or {util.NO}.",
                f"Current value is '{value}'.",
            ])
