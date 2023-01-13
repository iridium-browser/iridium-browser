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

if [[ "${EMU_HACK}" != "no" ]] ; then
  touch  vortex.msg
  touch  vortex.out
fi

${PREFIX} $1 ${DASHDASH} lendian.raw     > stdout.out 2> stderr.out

if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  cmp  vortex.out   data/train/output/vortex.out
fi

echo "OK"
