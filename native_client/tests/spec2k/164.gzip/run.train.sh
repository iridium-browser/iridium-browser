#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o nounset
set -o errexit

PREFIX=${PREFIX:-}
VERIFY=${VERIFY:-yes}
EMU_HACK=${EMU_HACK:-yes}

python3 ../prepare_input.py --config $(basename $(pwd)) train

${PREFIX} $1 ${DASHDASH} data/train/input/input.combined 32 >stdout.out 2>stderr.out

if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  cmp stdout.out  data/train/output/input.combined.out
fi
echo "OK"
