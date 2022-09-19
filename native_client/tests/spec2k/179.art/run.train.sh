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


${PREFIX} $1 ${DASHDASH} -scanfile c756hel.in\
                         -trainfile1 a10.img\
                         -stride 2\
                         -startx 134\
                         -starty 220\
                         -endx 184\
                         -endy 240\
                         -objects 3 >train.out 2>stderr.out


if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  ../specdiff.sh -r 0.01 -l 10 train.out data/train/output/train.out
fi

echo "OK"
