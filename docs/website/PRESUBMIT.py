# Copyright (c) 2021 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Top-level presubmit script for the Git repo backing chromium.org.

See http://www.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

import os


PRESUBMIT_VERSION = '2.0.0'

# This line is 'magic' in that git-cl looks for it to decide whether to
# use Python3 instead of Python2 when running the code in this file.
USE_PYTHON3 = True


# This list must be kept in sync with the lists in //.eleventy.js and
# //scripts/upload_lobs.py.
# TODO(crbug.com/1457683): Figure out how to share these lists to eliminate
# the duplication and need to keep them in sync.

LOB_EXTENSIONS = [
                  '.ai',
                  '.bin',
                  '.bmp',
                  '.brd',
                  '.bz2',
                  '.crx',
                  '.config',
                  '.dia',
                  '.gif',
                  '.graffle',
                  '.ico',
                  '.jpg',
                  'jpg',  # Some files are missing the '.' :(.
                  '.jpeg',
                  '.mp4',
                  '.msi',
                  '.pdf',
                  'pdf',  # Some files are missing the '.' :(.
                  '.png',
                  'png',  # Some files are missing the '.' :(.
                  '.PNG',
                  '.swf',
                  '.svg',
                  '.tar.gz',
                  '.tiff',
                  '_trace',
                  '.webp',
                  '.xcf',
                  '.xlsx',
                  '.zip'
                  ]


def CheckPatchFormatted(input_api, output_api):
  return input_api.canned_checks.CheckPatchFormatted(input_api, output_api)


def CheckChangeHasDescription(input_api, output_api):
  return input_api.canned_checks.CheckChangeHasDescription(
      input_api, output_api)


def CheckForLobs(input_api, output_api):
  output_status = []
  for file in input_api.change.AffectedFiles():
    # The tar.gz for example prevents using a hashmap to look up the extension
    for ext in LOB_EXTENSIONS:
      if str(file).endswith(ext) and file.Action() != 'D':
        error_msg = ('The file \'{file_name}\' is a binary that has not been '
          'uploaded to GCE. Please run:\n\tscripts/upload_lobs.py '
          '"{file_name}"\nand commit {file_name}.sha1 instead\n'
          'Run:\n\tgit rm --cached "{file_name}"\nto remove the lob from git'
          .format(file_name = file.LocalPath()))

        error = output_api.PresubmitError(error_msg)
        output_status.append(error)
        break

  return output_status


def CheckLobIgnores(input_api, output_api):
  del input_api
  output_status = []
  with open("site/.gitignore", 'r') as ignore_file:
    ignored_lobs = list(line.rstrip() for line in ignore_file.readlines())
    ignored_lobs = set(ignored_lobs[
      ignored_lobs.index('#start_lob_ignore') + 1 :
      ignored_lobs.index('#end_lob_ignore')])

    for ignored_lob in ignored_lobs:
      lob_sha_file = os.path.join('site', ignored_lob + '.sha1')
      if not lob_sha_file.startswith('#') and not os.path.exists(lob_sha_file):
        error_msg = ('The sha1 file \'{removed_file}\' no longer exists, '
          'please remove "{ignored_file}" from site/.gitignore'
          .format(removed_file = lob_sha_file, ignored_file = ignored_lob))

        error = output_api.PresubmitError(error_msg)
        output_status.append(error)
  return output_status
