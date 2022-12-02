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

LIST="train.out train.twf train.pl1 train.pl2  train.pin "

if [[ "${EMU_HACK}" != "no" ]] ; then
  touch  ${LIST}
  touch  train.tmp
  touch  train.cel
  touch  train.sav
  touch  train.sv2
fi

${PREFIX} $1 ${DASHDASH} train >stdout.out 2>stderr.out

if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  ../specdiff.sh -o stdout.out data/train/output/train.stdout
  for i in ${LIST}; do
    ../specdiff.sh -o $i data/train/output/$i
  done
fi
echo "OK"
