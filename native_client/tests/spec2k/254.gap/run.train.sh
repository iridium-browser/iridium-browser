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

${PREFIX} $1 ${DASHDASH} -l data/all/input/ -q -m 128M \
    < data/train/input/train.in  > stdout.out 2> stderr.out

if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  cmp  stdout.out  data/train/output/train.out
fi

echo "OK"
