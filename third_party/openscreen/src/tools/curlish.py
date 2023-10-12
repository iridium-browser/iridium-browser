# Copyright 2020 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
This file contains curlish(), a CURL-ish method that downloads things without
needing CURL installed.
"""

import os
from urllib.error import HTTPError, URLError
from urllib.request import urlopen


def curlish(download_url, output_path):
    """Basically curl, but doesn't require the developer to have curl installed
    locally. Returns True if succeeded at downloading file."""

    if not output_path or not download_url:
        print('need both output path and download URL to download, exiting.')
        return False

    print('downloading from "{}" to "{}"'.format(download_url, output_path))
    script_contents = ''
    try:
        response = urlopen(download_url)
        script_contents = response.read()
    except HTTPError as e:
        print(e.code)
        print(e.read())
        return False
    except URLError as e:
        print('Download failed. Reason: ', e.reason)
        return False

    directory = os.path.dirname(output_path)
    if not os.path.exists(directory):
        os.makedirs(directory)

    with open(output_path, 'wb') as script_file:
        script_file.write(script_contents)

    return True
