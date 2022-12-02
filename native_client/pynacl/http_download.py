#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Download a file from a URL to a file on disk.

This module supports username and password with basic authentication.
"""

import base64
import os
import os.path
import sys
import time

try:
  import urllib2 as urllib
except ImportError:  # For Py3 compatibility
  import urllib.request as urllib

import pynacl.download_utils


def _CreateDirectory(path):
  """Create a directory tree, ignore if it's already there."""
  try:
    os.makedirs(path)
    return True
  except os.error:
    return False


def HttpDownload(url, target, username=None, password=None, verbose=True,
    logger=None):
  """Download a file from a remote server.

  Args:
    url: A URL to download from.
    target: Filename to write download to.
    username: Optional username for download.
    password: Optional password for download (ignored if no username).
    logger: Function to log events to.
  """

  # Log to stdout by default.
  if logger is None:
    logger = sys.stdout.write
  headers = [('Accept', '*/*')]
  if username:
    if password:
      auth_code = base64.b64encode(username + ':' + password)
    else:
      auth_code = base64.b64encode(username)
    headers.append(('Authorization', 'Basic ' + auth_code))
  if os.environ.get('http_proxy'):
    proxy = os.environ.get('http_proxy')
    proxy_handler = urllib.ProxyHandler({'http': proxy, 'https': proxy})
    opener = urllib.build_opener(proxy_handler)
  else:
    opener = urllib.build_opener()
  opener.addheaders = headers
  urllib.install_opener(opener)
  _CreateDirectory(os.path.split(target)[0])
  # Retry up to 4 times (appengine logger is flaky).
  for i in range(4):
    if i:
      time.sleep(10**(i-1))
      logger('Download failed on %s, retrying... (%d)\n' % (url, i))
    try:
      # Exponential timeout to ensure we fail and retry on stalled connections.
      src = urllib.urlopen(url, timeout=30)
      try:
        pynacl.download_utils.WriteDataFromStream(
            target, src, chunk_size=2**20, verbose=verbose)
        content_len = src.headers.get('Content-Length')
        if content_len:
          content_len = int(content_len)
          file_size = os.path.getsize(target)
          if content_len != file_size:
            logger('Filesize:%d does not match Content-Length:%d' % (
                file_size, content_len))
            continue
      finally:
        src.close()
      break
    except urllib.HTTPError as e:
      if e.code == 404:
        logger('Resource does not exist.\n')
        raise
      logger('Failed to open.\n')
    except urllib.URLError:
      logger('Failed mid stream.\n')
    except Exception as e:
      logger('Download failed: %s' % e)
  else:
    logger('Download failed on %s, giving up.\n' % url)
    raise
