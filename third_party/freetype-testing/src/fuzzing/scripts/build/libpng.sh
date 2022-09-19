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

path_to_zlib="$( readlink -f "../../../external/zlib" )"

path_to_src="$( readlink -f "../../../external/libpng" )"
path_to_build="${path_to_src}/build"
path_to_install="${path_to_src}/usr"

if [[ "${#}" -lt "1" || "${1}" != "--no-init" ]]; then

    git submodule update --init --depth 1 "${path_to_src}"

    cd "${path_to_src}"

    git clean -dfqx
    git reset --hard
    git rev-parse HEAD

    # Based on how oss-fuzz/libpng builds, file at
    # https://github.com/glennrp/libpng/blob/libpng16/contrib/oss-fuzz/build.sh
    # Disable logging via library build configuration control.
    cat scripts/pnglibconf.dfa | \
      sed -e "s/option STDIO/option STDIO disabled/" \
          -e "s/option WARNING /option WARNING disabled/" \
          -e "s/option WRITE enables WRITE_INT_FUNCTIONS/option WRITE disabled/" \
      > scripts/pnglibconf.dfa.temp
    mv scripts/pnglibconf.dfa.temp scripts/pnglibconf.dfa

    autoreconf -f -i

    mkdir -p "${path_to_build}" && cd "${path_to_build}"

    # Depending on the OS, the library gets installed in `.../lib` or
    # `.../lib64` (or maybe even somewhere else).  For simplicity, however,
    # we want a location that is independent of the OS, thus the `--libdir`
    # argument.
    CPPFLAGS="-I${path_to_zlib}/usr/include" \
    LDFLAGS="-L${path_to_zlib}/usr/lib-asan" \
    sh ../configure --with-libpng-prefix=OSS_FUZZ_ \
                    --prefix="${path_to_install}" \
                    --libdir="${path_to_install}/lib-asan" \
                    --enable-static \
                    --disable-shared
fi

if [[ -f "${path_to_build}/Makefile" ]]; then
    cd "${path_to_build}"
    make -j$(nproc) clean
    make -j$(nproc)
    # Do full install to ensure OSS_FUZZ_ prefix.
    make -j$(nproc) install
fi

cd "${dir}"
