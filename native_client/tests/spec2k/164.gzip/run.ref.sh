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

LIST="input.source input.log input.graphic input.random input.program"

for i in  ${LIST} ; do
  ${PREFIX} $1 ${DASHDASH} data/ref/input/$i 60 > $i.out  2>stderr.out
done

if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  for i in  ${LIST} ; do
    cmp $i.out  data/ref/output/$i.out
  done
fi
echo "OK"
