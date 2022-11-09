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

${PREFIX} $1 ${DASHDASH} -I./lib data/all/input/diffmail.pl 2 350 15 24 23 150 \
    > stdout1.out 2> stderr1.out

${PREFIX} $1 ${DASHDASH} -I./lib data/all/input/perfect.pl  b 3 \
  > stdout2.out 2> stderr2.out

${PREFIX} $1 ${DASHDASH} -I./lib  data/train/input/scrabbl.pl \
  <  data/train/input/scrabbl.in  > stdout3.out 2> stderr3.out


if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  cmp  stdout1.out  data/train/output/2.350.15.24.23.150.out
  cmp  stdout2.out  data/train/output/b.3.out
  cmp  stdout3.out  data/train/output/scrabbl.out
fi

echo "OK"
