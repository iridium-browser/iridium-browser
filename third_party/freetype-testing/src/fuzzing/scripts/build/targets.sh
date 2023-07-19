#!/bin/bash
set -euxo pipefail

# Copyright 2018 by
# Armin Hasitzka.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

dir="${PWD}"
cd "$( dirname "$( readlink -f "${0}" )" )" # go to `/fuzzing/scripts/build'

path_to_build="$( readlink -f "../../build" )"

if [[ "${#}" == "0" || "${1}" != "--no-init" ]]; then

    mkdir -p "${path_to_build}" && cd "${path_to_build}"

    # Remove all files and folders in `fuzzing/build' except for the folder
    # `bin':

    find .              \
         -maxdepth 1    \
         -type f        \
         -exec rm {} \;

    find .                 \
         -maxdepth 1       \
         -type d           \
         ! -name bin       \
         -exec rm -r {} \;

    env | sort
    cmake -GNinja -DCMAKE_BUILD_TYPE=Debug ..
fi

if [[ -f "${path_to_build}/build.ninja" ]]; then
   cd "${path_to_build}"
   ninja
   cd "bin"

   # link ./driver -> ./target (if the target has a different name):
   if [[ -n ${CMAKE_DRIVER_EXE_NAME+x} &&
         "${CMAKE_DRIVER_EXE_NAME}" != "driver" ]]; then
       rm -f "driver"
       ln -s "${CMAKE_DRIVER_EXE_NAME}" "driver"
   fi
fi

cd "${dir}"
