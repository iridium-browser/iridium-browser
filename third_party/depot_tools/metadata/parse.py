#!/usr/bin/env python3
# Copyright 2023 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import re
import sys
from typing import List

_THIS_DIR = os.path.abspath(os.path.dirname(__file__))
# The repo's root directory.
_ROOT_DIR = os.path.abspath(os.path.join(_THIS_DIR, ".."))

# Add the repo's root directory for clearer imports.
sys.path.insert(0, _ROOT_DIR)

import metadata.fields.known as known_fields
import metadata.dependency_metadata as dm

# Line used to separate dependencies within the same metadata file.
DEPENDENCY_DIVIDER = re.compile(r"^-{20} DEPENDENCY DIVIDER -{20}$")

# Delimiter used to separate a field's name from its value.
FIELD_DELIMITER = ":"

# Pattern used to check if a line from a metadata file declares a new
# field.
_PATTERN_FIELD_DECLARATION = re.compile(
    "^({}){}".format("|".join(known_fields.ALL_FIELD_NAMES), FIELD_DELIMITER),
    re.IGNORECASE)


def parse_content(content: str) -> List[dm.DependencyMetadata]:
    """Reads and parses the metadata from the given string.

    Args:
        content: the string to parse metadata from.

    Returns: all the metadata, which may be for zero or more
             dependencies, from the given string.
  """
    dependencies = []
    current_metadata = dm.DependencyMetadata()
    current_field_name = None
    current_field_value = ""
    for line in content.splitlines(keepends=True):
        # Check if a new dependency is being described.
        if DEPENDENCY_DIVIDER.match(line):
            if current_field_name:
                # Save the field value for the previous dependency.
                current_metadata.add_entry(current_field_name,
                                           current_field_value)
            if current_metadata.has_entries():
                # Add the previous dependency to the results.
                dependencies.append(current_metadata)
            # Reset for the new dependency's metadata,
            # and reset the field state.
            current_metadata = dm.DependencyMetadata()
            current_field_name = None
            current_field_value = ""

        elif _PATTERN_FIELD_DECLARATION.match(line):
            # Save the field value to the current dependency's metadata.
            if current_field_name:
                current_metadata.add_entry(current_field_name,
                                           current_field_value)

            current_field_name, current_field_value = line.split(
                FIELD_DELIMITER, 1)
            field = known_fields.get_field(current_field_name)
            if field and field.is_one_liner():
                # The field should be on one line, so add it now.
                current_metadata.add_entry(current_field_name,
                                           current_field_value)
                # Reset the field state.
                current_field_name = None
                current_field_value = ""

        elif current_field_name:
            # The field is on multiple lines, so add this line to the
            # field value.
            current_field_value += line

    # At this point, the end of the file has been reached.
    # Save any remaining field data and metadata.
    if current_field_name:
        current_metadata.add_entry(current_field_name, current_field_value)
    if current_metadata.has_entries():
        dependencies.append(current_metadata)

    return dependencies
