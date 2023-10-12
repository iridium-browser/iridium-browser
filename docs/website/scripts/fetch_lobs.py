#!/usr/bin/env vpython3
# Copyright 2022 Google LLC
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

"""Download all the LOBS in //site."""

import argparse
import hashlib
import io
import os
import sys
import time

import requests

import common

# This is used to hold a global requests.Session object so that the
# process can reuse a single connection across multiple requests.
http_session = None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--force', action='store_true')
    parser.add_argument('-j', '--jobs', type=int, default=os.cpu_count())
    args = parser.parse_args()
    q = common.JobQueue(_download, args.jobs)
    paths = [path.replace('.sha1', '')
             for path in common.walk(common.SITE_DIR)
             if path.endswith('.sha1')]

    for path in paths:
        with open(os.path.join(common.SITE_DIR, path + '.sha1'), 'r') as fp:
            expected_sha1 = fp.read().strip()

        if not args.force and os.path.exists(
                os.path.join(common.SITE_DIR, path)):
            with open(os.path.join(common.SITE_DIR, path), 'rb') as fp:
                s = hashlib.sha1()
                s.update(fp.read())
                actual_sha1 = s.hexdigest()
            if args.force or (actual_sha1 != expected_sha1):
                q.request(path, expected_sha1)
        else:
            q.request(path, expected_sha1)

    if len(q.all_tasks()) == 0:
        return 0

    start = time.time()
    updated = 0
    failed = False
    total_bytes = 0
    for path, res, resp in q.results():
        did_update, num_bytes = resp
        if res:
            print('%s failed: %s' % (path, res))
            failed = True
        if did_update:
            updated += 1
        total_bytes += num_bytes
    end = time.time()

    print('Fetched %d LOBs (%.1fMB) in %.3f seconds (%.1fMbps).'  %
          (updated,
           (total_bytes / 1_000_000),
           (end - start),
           (total_bytes * 8 / (end - start) / 1_000_000)))
    return 1 if failed else 0


def _url(expected_sha1):
    return 'https://storage.googleapis.com/%s/%s' % (
        'chromium-website-lob-storage', expected_sha1)


def _download(path, expected_sha1):
    """This routine downloads a given file if needed.

    If there is no file at `path`, or if the file's SHA-1 hash doesn't
    match the expected hash, we download it from the cloud storage bucket.
    """

    # This is used to hold a global requests.Session object so that the
    # process can reuse a single connection across multiple requests.
    global http_session
    if http_session is None:
        http_session = requests.Session()

    url = _url(expected_sha1)
    for i in range(4):
      try:
        resp = http_session.get(url)
        s = hashlib.sha1()
        s.update(resp.content)
        actual_sha1 = s.hexdigest()
        if actual_sha1 != expected_sha1:
            return ('sha1 mismatch: expected %s, got %s' % (
                expected_sha1, actual_sha1), (False, len(resp.content)))
        with open(os.path.join(common.SITE_DIR, path), 'wb') as fp:
            fp.write(resp.content)
      except (requests.HTTPError, requests.ConnectionError,
              requests.Timeout) as e:
          if i < 4:
            time.sleep(1)
          else:
            return str(e), (False, 0)
      except Exception as e:
          return str(e), (False, 0)
    return '', (True, len(resp.content))

if __name__ == '__main__':
    sys.exit(main())
