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

if [[ "${EMU_HACK}" != "no" ]] ; then
  touch costs.out place.out route.out
fi

ARGS1="-nodisp -place_only -init_t 5 -exit_t 0.005 -alpha_t 0.9412 -inner_num 2"
ARGS2="-nodisp -route_only -route_chan_width 15 -pres_fac_mult 2 -acc_fac 1 -first_iter_pres_fac 4 -initial_pres_fac 8"


${PREFIX} $1  ${DASHDASH} net.in arch.in place.out dum.out ${ARGS1} \
  >place_log.out 2>stderr1.out

${PREFIX} $1  ${DASHDASH} net.in arch.in place.in route.out ${ARGS2} \
  >route_log.out 2>stderr2.out

LIST="place_log.out route_log.out costs.out route.out"

if [[ "${VERIFY}" != "no" ]] ; then
   echo "VERIFY"
   for i in ${LIST} ; do
     # NOTE: we are a little more conservative than spec with regard to reltol.
     # (the normal harness uses weaker reltols for some files).
     ../specdiff.sh -r 0.015 -l 10 $i data/ref/output/$i
   done
fi
echo "OK"
