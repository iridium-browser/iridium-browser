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

${PREFIX} $1 ${DASHDASH}  <inp.in  >inp.out 2>stderr.out

if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  ../specdiff.sh -r 0.00001 -l 10 inp.out data/ref/output/inp.out
fi

echo "OK"
