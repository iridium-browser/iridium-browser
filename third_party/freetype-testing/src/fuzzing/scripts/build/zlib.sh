#!/bin/bash
set -euxo pipefail

# Copyright 2018-2020 by
# Armin Hasitzka and Ben Wagner.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

dir="${PWD}"
cd "$( dirname "$( readlink -f "${0}" )" )" # go to `/fuzzing/scripts/build'

path_to_src="$( readlink -f "../../../external/zlib" )"
# Cannot do out of source build with zprefix, too many zconf.h
path_to_build="${path_to_src}"
path_to_install="${path_to_src}/usr"

if [[ "${#}" -lt "1" || "${1}" != "--no-init" ]]; then

    git submodule update --init --depth 1 "${path_to_src}"

    cd "${path_to_src}"

    git clean -dfqx
    git reset --hard
    git rev-parse HEAD

    mkdir -p "${path_to_build}" && cd "${path_to_build}"
    # 'zlib' is a dependency of 'libpng'; the library must thus be installed
    # in the same directory as 'libpng'.  See `libpng.sh` for more
    # information why `--libdir` is necessary.
    sh ./configure --zprefix \
                   --prefix="${path_to_install}" \
                   --libdir="${path_to_install}/lib-asan" \
                   --static
fi

if [[ -f "${path_to_build}/Makefile" ]]; then
    cd "${path_to_build}"
    make -j$( nproc ) clean
    make -j$( nproc )
    make -j$( nproc ) install
fi

cd "${dir}"
