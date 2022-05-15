#!/usr/bin/env bash

# Copyright 2020 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# NOTE: this is based on this script running from cast/protocol/castv2
YAJSV_BIN="$SCRIPT_DIR/../../../tools/yajsv"

if [ ! -f "$YAJSV_BIN" ]; then
    echo "Could not find yajsv, please run tools/download-yajsv.py"
fi


for filename in $SCRIPT_DIR/streaming_examples/*.json; do
"$YAJSV_BIN" -s "$SCRIPT_DIR/streaming_schema.json" "$filename"
done

for filename in $SCRIPT_DIR/receiver_examples/*.json; do
"$YAJSV_BIN" -s "$SCRIPT_DIR/receiver_schema.json" "$filename"
done
