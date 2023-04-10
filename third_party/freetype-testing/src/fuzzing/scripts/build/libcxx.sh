#!/bin/bash
set -euxo pipefail

# Copyright 2019, 2021 by
# Armin Hasitzka, Ben Wagner.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

dir="${PWD}"
path_to_self="$( dirname "$( readlink -f "${0}" )" )"
cd "${path_to_self}" # go to `/fuzzing/scripts/build'

path_to_src="$( readlink -f "../../../external/llvm-project" )"
path_to_build="${path_to_src}/build"

if [[ "${#}" == "0" || "${1}" != "--no-init" ]]; then

    git submodule update --init --depth 1 "${path_to_src}"

    cd "${path_to_src}"

    git clean -dfqx
    git reset --hard
    git rev-parse HEAD

    # See https://github.com/google/oss-fuzz/pull/7033
    # See https://reviews.llvm.org/D116050
    git apply "${path_to_self}/0001-Add-trace-pc-guard-to-fno-sanitize-coverage.patch"

    mkdir "${path_to_build}" && cd "${path_to_build}"

    case "${SANITIZER}" in
      "address") LLVM_SANITIZER="Address" ;;
      "address;undefined") LLVM_SANITIZER="Address;Undefined" ;;
      "undefined") LLVM_SANITIZER="Undefined" ;;
      "memory") LLVM_SANITIZER="MemoryWithOrigins" ;;
      "thread") LLVM_SANITIZER="Thread" ;;
      *) LLVM_SANITIZER="" ;;
    esac

    env | sort
    cmake \
      -GNinja ../llvm \
      -DCMAKE_BUILD_TYPE=Release \
      -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi" \
      -DLLVM_USE_SANITIZER="${LLVM_SANITIZER}" \
      -DLIBCXX_ENABLE_SHARED=OFF \
      -DLIBCXXABI_ENABLE_SHARED=OFF
fi

if [[ -f "${path_to_build}/build.ninja" ]]; then
    cd "${path_to_build}"
    cmake --build . -- cxx cxxabi
fi

cd "${dir}"
