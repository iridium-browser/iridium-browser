#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o nounset
set -o errexit

PREFIX=${PREFIX:-}
VERIFY=${VERIFY:-yes}
EMU_HACK=${EMU_HACK:-yes}



LIST="166.i 200.i expr.i integrate.i  scilab.i"

python3 ../prepare_input.py --config $(basename $(pwd)) ref


for i in ${LIST} ; do
  out=${i%.*}.s
  if [[ "${EMU_HACK}" != "no" ]] ; then
    touch ${out}
  fi


  ${PREFIX} $1 ${DASHDASH} data/ref/input/$i -o ${out} \
    > $i.stdout.out 2> $i.stderr.out
done

if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  for i in ${LIST} ; do
    out=${i%.*}.s
    cmp  ${out}  data/ref/output/${out}
  done
fi
echo "OK"
