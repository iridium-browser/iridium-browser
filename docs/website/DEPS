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
#
#
# This file is used to manage the dependencies of the chromium website tooling.
# It is used by gclient to determine what version of each dependency to
# check out, and where.
#
# For more information, please refer to the official documentation:
#   https://sites.google.com/a/chromium.org/dev/developers/how-tos/get-the-code
#
# When adding a new dependency, please update the top-level .gitignore file
# to list the dependency's destination directory.

allowed_hosts = [
  'chromium.googlesource.com',
]

use_relative_paths = True

deps = {
}

hooks = [
  {
    'name': 'node_linux64',
    'pattern': '.',
    'condition': 'host_os == "linux"',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--extract',
                '--no_auth',
                '--bucket', 'chromium-nodejs/14.15.4',
                '-s', 'third_party/node/linux/node-linux-x64.tar.gz.sha1',
    ],
  },
  {
    'name': 'node_mac',
    'pattern': '.',
    'condition': 'host_os == "mac" and host_cpu == "x64"',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--extract',
                '--no_auth',
                '--bucket', 'chromium-nodejs/14.15.4',
                '-s', 'third_party/node/mac/node-darwin-x64.tar.gz.sha1',
    ],
  },
  {
    # TODO(crbug.com/1457689): Node 16.0 will likely ship with an official
    # universal node binary on macOS. Once node 16.0 is released,
    # collapse this into the node_mac hook above again and use the
    # universal binary on mac independent of host_cpu.
    'name': 'node_mac_arm64',
    'pattern': '.',
    'condition': 'host_os == "mac" and host_cpu == "arm64"',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--extract',
                '--no_auth',
                '--bucket', 'chromium-nodejs/16.0.0-pre',
                '-s', 'third_party/node/mac/node-darwin-arm64.tar.gz.sha1',
    ],
  },
  {
    'name': 'node_win',
    'pattern': '.',
    'condition': 'host_os == "win"',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--no_auth',
                '--bucket', 'chromium-nodejs/14.15.4',
                '-s', 'third_party/node/win/node.exe.sha1',
    ],
  },
  {
    'name': 'fetch_node_modules',
    'pattern': '.',
    'action': [ 'python3',
                'scripts/fetch_node_modules.py'
    ],
  },
  {
    'name': 'fetch_lobs',
    'pattern': '.',
    'action': [
      'vpython3',
      'scripts/fetch_lobs.py',
    ],
  },
]
