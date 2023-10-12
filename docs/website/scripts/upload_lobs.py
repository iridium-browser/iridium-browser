#!/usr/bin/env python3
# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import optparse
import os

for path in os.environ['PATH'].split(os.path.pathsep):
    if path.endswith('depot_tools') and path not in sys.path:
        sys.path.insert(0, path)

import upload_to_google_storage

import common

# This list must be kept in sync with the lists in //.eleventy.js and
# //PRESUBMIT.py.
# TODO(crbug.com/1457683): Figure out how to share these lists to eliminate the
# duplication and need to keep them in sync.

LOB_EXTENSIONS = [
                  '.ai',
                  '.bin',
                  '.bmp',
                  '.brd',
                  '.bz2',
                  '.config',
                  '.crx',
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

def main():
  parser = optparse.OptionParser(upload_to_google_storage.USAGE_STRING)
  parser.add_option('-b', '--bucket',
                    default='chromium-website-lob-storage',
                    help='Google Storage bucket to upload to.')
  parser.add_option('-e', '--boto', help='Specify a custom boto file.')
  parser.add_option('-f', '--force', action='store_true',
                    help='Force upload even if remote file exists.')
  parser.add_option('-g', '--gsutil_path',
                    default=upload_to_google_storage.GSUTIL_DEFAULT_PATH,
                    help='Path to the gsutil script.')
  parser.add_option('-m', '--use_md5', action='store_true',
                    help='Generate MD5 files when scanning, and don\'t check '
                    'the MD5 checksum if a .md5 file is found.')
  parser.add_option('-t', '--num_threads', default=1, type='int',
                    help='Number of uploader threads to run.')
  parser.add_option('-s', '--skip_hashing', action='store_true',
                    help='Skip hashing if .sha1 file exists.')
  parser.add_option('-0', '--use_null_terminator', action='store_true',
                    help='Use \\0 instead of \\n when parsing '
                    'the file list from stdin.  This is useful if the input '
                    'is coming from "find ... -print0".')
  parser.add_option('-z', '--gzip', metavar='ext',
                    help='Gzip files which end in ext. '
                         'ext is a comma-separated list')
  parser.add_option('-d', '--directory',
                    help='The target is a directory.  ')
  parser.add_option('-r', '--remove', action='store_true',
                    help='Removes the file that was uploaded to storage.  ')
  (options, args) = parser.parse_args()

  if options.directory:
    input_filenames = get_lobs_from_dir(options.directory)
    if len(input_filenames) == 0:
      print("No LOB files found in directory to upload")
      return 0
  else:
    # Enumerate our inputs.
    input_filenames = upload_to_google_storage.get_targets(args, parser,
                                                    options.use_null_terminator)

  # Make sure we can find a working instance of gsutil.
  if os.path.exists(upload_to_google_storage.GSUTIL_DEFAULT_PATH):
    gsutil = upload_to_google_storage.Gsutil(
      upload_to_google_storage.GSUTIL_DEFAULT_PATH, boto_path=options.boto)
  else:
    parser.error('gsutil not found in %s, bad depot_tools checkout?' %
                   upload_to_google_storage.GSUTIL_DEFAULT_PATH)

  base_url = 'gs://%s' % options.bucket

  add_to_ignore(input_filenames)

  upload_status = upload_to_google_storage.upload_to_google_storage(
      input_filenames, base_url, gsutil, options.force, options.use_md5,
      options.num_threads, options.skip_hashing, options.gzip)

  if upload_status:
    return upload_status

  if options.remove:
    remove_lobs(input_filenames)

  return 0

def remove_lobs(lob_files):
  for lob_file in lob_files:
    if os.path.exists(lob_file):
      os.remove(lob_file)

def add_to_ignore(lob_files):
  with open(common.SITE_DIR + "/.gitignore", 'r') as ignore_file:
    file_lines = list(line.rstrip() for line in ignore_file.readlines())

  end_tag_index = file_lines.index('#end_lob_ignore')
  lob_ignores = set(file_lines[
    file_lines.index('#start_lob_ignore') + 1 :
    end_tag_index])

  for lob_file in lob_files:
    rel_path = os.path.relpath(lob_file, common.SITE_DIR)

    if os.path.exists(lob_file) and not rel_path in lob_ignores:
      file_lines.insert(end_tag_index, rel_path)
      end_tag_index+=1

  with open(common.SITE_DIR + "/.gitignore", 'w') as ignore_file:
    ignore_file.writelines(line + '\n' for line in file_lines)

def get_lobs_from_dir(directory):
    lobs = []
    for (dirpath, _, filenames) in os.walk(directory):
      for filename in filenames:
        absolute_filename = os.path.join(dirpath, filename)
        if os.path.isfile(absolute_filename):
          for ext in LOB_EXTENSIONS:
            if filename.endswith(ext):
              lobs.append(absolute_filename)
              break
    return lobs

if __name__ == '__main__':
  try:
    sys.exit(main())
  except KeyboardInterrupt:
    sys.stderr.write('interrupted\n')
    sys.exit(1)
