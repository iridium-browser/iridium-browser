#!/bin/bash -eu
# Copyright 2021 The Wuffs-Mirror-Release-C Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# ----------------

# This script synchronizes this repository's release/c directory with the
# upstream one, assumed to be "../wuffs/release/c", and updates "sync.txt".
#
# It does not ensure that ../wuffs is synchronized to the latest version (at
# https://github.com/google/wuffs). That is a separate responsibility.

echo -n "script/sync.sh ran on " > sync.txt
date --iso-8601 >> sync.txt
echo -n "Sibling directory (../wuffs) git revision is " >> sync.txt
cd ../wuffs
git rev-parse HEAD >> ../wuffs-mirror-release-c/sync.txt
cd ../wuffs-mirror-release-c
echo "Manifest (sha256sum values, filenames, versions):" >> sync.txt

for f in ../wuffs/release/c/*; do
  f=${f##*/}

  # Skip the unsupported snapshot.
  if [ $f = "wuffs-unsupported-snapshot.c" ]; then
    continue
  fi

  cp ../wuffs/release/c/$f release/c
  sha256sum release/c/$f >> sync.txt
  set +e
  VER=$(grep "^#define WUFFS_VERSION_STRING " release/c/$f)
  set -e
  if [ -n "$VER" ]; then
    echo "    $VER" >> sync.txt
  fi
done
