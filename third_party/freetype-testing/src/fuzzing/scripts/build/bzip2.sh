#!/bin/bash
set -euxo pipefail

# Copyright 2019 by
# Armin Hasitzka.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

dir="${PWD}"
cd $( dirname $( readlink -f "${0}" ) ) # go to `/fuzzing/scripts/build'

path_to_src=$( readlink -f "../../../external/bzip2" )

if [[ "${#}" == "0" || "${1}" != "--no-init" ]]; then

    git submodule update --init "${path_to_src}"

    cd "${path_to_src}"

    git clean -dfqx
    git reset --hard
    git rev-parse HEAD
fi

make -j$( nproc )

cd "${dir}"
