#!/usr/bin/env bash

# Copyright 2020 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

YAJSV_BIN=$(which yajsv)
if [ "$YAJSV_BIN" == "" ]; then
  echo "Could not find yajsv, see the top-level README.md"
  exit 1
fi

for filename in $SCRIPT_DIR/streaming_examples/*.json; do
"$YAJSV_BIN" -s "$SCRIPT_DIR/streaming_schema.json" "$filename"
done

for filename in $SCRIPT_DIR/receiver_examples/*.json; do
"$YAJSV_BIN" -s "$SCRIPT_DIR/receiver_schema.json" "$filename"
done
