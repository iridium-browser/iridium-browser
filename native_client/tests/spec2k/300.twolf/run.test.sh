 #!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o nounset
set -o errexit

PREFIX=${PREFIX:-}
VERIFY=${VERIFY:-yes}
EMU_HACK=${EMU_HACK:-yes}


rm -f *.out
cp  data/test/input/* .


LIST="test.out test.twf test.pl1 test.pl2 test.sav test.pin test.sv2"

if [[ "${EMU_HACK}" != "no" ]] ; then
  touch  ${LIST}
  touch  test.tmp
  touch  test.cel
fi

${PREFIX} $1 ${DASHDASH} test >stdout.out 2>stderr.out

if [[ "${VERIFY}" != "no" ]] ; then
  echo "VERIFY"
  #../specdiff.sh -o stdout.out data/test/output/test.stdout
  echo "SKIP STDOUT"
  for i in ${LIST}; do
    ../specdiff.sh -o $i data/test/output/$i
  done
fi
echo "OK"

