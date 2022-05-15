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

path_to_src=$( readlink -f "../../../external/brotli" )
path_to_build="${path_to_src}/build"

if [[ "${#}" == "0" || "${1}" != "--no-init" ]]; then

    git submodule update --init "${path_to_src}"

    cd "${path_to_src}"

    git clean -dfqx
    git reset --hard
    git rev-parse HEAD

    mkdir "${path_to_build}" && cd "${path_to_build}"

    cmake -DCMAKE_BUILD_TYPE=Release ..
fi

if [[ -d "${path_to_build}" ]]; then
    cd "${path_to_build}"
    make -j$( nproc )
fi

cd "${dir}"
