#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o nounset
set -o errexit

VERIFY=${PREFIX:-yes}
PREFIX=${PREFIX:-}

python3 ../prepare_input.py --config $(basename $(pwd)) ref

${PREFIX} $1  ${DASHDASH} data/ref/input/input.source 58 > input.source.out  2>stderr1.out
${PREFIX} $1  ${DASHDASH} data/ref/input/input.graphic 58 > input.graphic.out  2>stderr2.out
${PREFIX} $1  ${DASHDASH} data/ref/input/input.program 58 > input.program.out  2>stder3.out

if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  cmp  input.source.out  data/ref/output/input.source.out
  cmp  input.graphic.out  data/ref/output/input.graphic.out
  cmp  input.program.out  data/ref/output/input.program.out
fi
echo OK
