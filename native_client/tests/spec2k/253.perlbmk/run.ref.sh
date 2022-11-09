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

${PREFIX} $1 ${DASHDASH} -I./lib data/all/input/diffmail.pl 2 550 15 24 23 100 \
    > stdout1.out 2> stderr1.out

${PREFIX} $1 ${DASHDASH} -I./lib data/all/input/perfect.pl  b 3 m 4\
  > stdout2.out 2> stderr2.out

${PREFIX} $1 ${DASHDASH} -I./lib data/ref/input/makerand.pl \
  > stdout3.out 2> stderr3.out

${PREFIX} $1 ${DASHDASH} -I./lib data/ref/input/splitmail.pl 850 5 19 18 1500\
  > stdout4.out 2> stderr4.out

${PREFIX} $1 ${DASHDASH} -I./lib data/ref/input/splitmail.pl 704 12 26 16 836\
  > stdout5.out 2> stderr5.out

${PREFIX} $1 ${DASHDASH} -I./lib data/ref/input/splitmail.pl 535 13 25 24 1091\
  > stdout6.out 2> stderr6.out

${PREFIX} $1 ${DASHDASH} -I./lib data/ref/input/splitmail.pl 957 12 23 26 1014\
  > stdout7.out 2> stderr7.out



if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  cmp  stdout1.out  data/ref/output/2.550.15.24.23.100.out
  cmp  stdout2.out  data/ref/output/b.3.m.4.out
  cmp  stdout3.out  data/ref/output/makerand.out
  cmp  stdout4.out  data/ref/output/850.5.19.18.1500.out
  cmp  stdout5.out  data/ref/output/704.12.26.16.836.out
  cmp  stdout6.out  data/ref/output/535.13.25.24.1091.out
  cmp  stdout7.out  data/ref/output/957.12.23.26.1014.out
fi

echo "OK"
