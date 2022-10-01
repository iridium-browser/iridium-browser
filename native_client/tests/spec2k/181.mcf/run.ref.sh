#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o nounset
set -o errexit

PREFIX=${PREFIX:-}
VERIFY=${VERIFY:-yes}
EMU_HACK=${EMU_HACK:-yes}

python3 ../prepare_input.py --config $(basename $(pwd)) ref

if [[ "${EMU_HACK}" != "no" ]] ; then
  touch mcf.out
fi

${PREFIX} $1  ${DASHDASH} data/ref/input/inp.in >stdout.out 2>stderr.out

if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  cmp stdout.out  data/ref/output/inp.out
  cmp mcf.out data/ref/output/mcf.out
fi

echo "OK"
