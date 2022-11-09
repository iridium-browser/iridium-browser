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

LIST="1 2 3"
if [[ "${EMU_HACK}" != "no" ]] ; then
  touch  vortex.msg
  touch  vortex1.out vortex2.out vortex3.out
fi


for i in ${LIST} ; do
  ${PREFIX} $1 ${DASHDASH} lendian$i.raw     > $i.stdout.out 2> $i.stderr.out
done

if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  for i in ${LIST} ; do
    cmp  vortex$i.out   data/ref/output/vortex$i.out
  done
fi

echo "OK"
