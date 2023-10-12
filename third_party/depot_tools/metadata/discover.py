# Copyright 2023 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from typing import List

# The base names that are known to be Chromium metadata files.
_METADATA_FILES = {
    "README.chromium",
}


def is_metadata_file(path: str) -> bool:
    """Filter for metadata files."""
    return os.path.basename(path) in _METADATA_FILES


def find_metadata_files(root: str) -> List[str]:
    """Finds all metadata files within the given root directory,
    including subdirectories.

    Args:
      root: the absolute path to the root directory within which to
            search.

    Returns: the absolute full paths for all the metadata files within
             the root directory.
    """
    metadata_files = []
    for item in os.listdir(root):
        full_path = os.path.join(root, item)
        if is_metadata_file(item):
            metadata_files.append(full_path)
        elif os.path.isdir(full_path):
            metadata_files.extend(find_metadata_files(full_path))

    return metadata_files
