#!/bin/bash
set -euxo pipefail

# Copyright 2018-2019 by
# Armin Hasitzka.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

dir="${PWD}"
cd "$( dirname "$( readlink -f "${0}" )" )" # go to `/fuzzing/scripts/build'

path_to_src="$( readlink -f "../../../external/libarchive" )"

if [[ "${#}" -lt "1" || "${1}" != "--no-init" ]]; then

    git submodule update --init --depth 1 "${path_to_src}"

    cd "${path_to_src}"

    git clean -dfqx
    git reset --hard
    git rev-parse HEAD

    sh build/autogen.sh

    # Build `libarchive' as slim as possible:

    sh configure                     \
       --disable-dependency-tracking \
       --disable-shared              \
       --enable-static               \
       --disable-bsdtar              \
       --disable-bsdcat              \
       --disable-bsdcpio             \
       --enable-posix-regex-lib=libc \
       --disable-xattr               \
       --disable-acl                 \
       --disable-largefile           \
       --without-zlib                \
       --without-bz2lib              \
       --without-iconv               \
       --without-libiconv-prefix     \
       --without-lz4                 \
       --without-zstd                \
       --without-lzma                \
       --with-lzo2                   \
       --without-cng                 \
       --without-nettle              \
       --without-openssl             \
       --without-xml2                \
       --without-expat
fi

if [[ -f "${path_to_src}/Makefile" ]]; then
    cd "${path_to_src}"
    make -j$( nproc )
fi

cd "${dir}"
