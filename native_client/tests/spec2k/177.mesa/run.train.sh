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

${PREFIX} $1 ${DASHDASH} -frames 500 -meshfile mesa.in -ppmfile mesa.ppm \
  > mesa.out 2> mesa.err

if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  cmp  mesa.log  data/train/output/mesa.log
  ../specdiff.sh -a 6 -l 10 mesa.ppm data/train/output/mesa.ppm
fi


echo "OK"
