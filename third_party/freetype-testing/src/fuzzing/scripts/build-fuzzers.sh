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
cd $( dirname $( readlink -f "${0}" ) ) # go to `/fuzzing/scripts'

# We expect a bunch of flags coming in from the fuzzing framework;  in fact
# everything that is connected to compilers, sanitizers, coverage, ...  The
# only thing that we want to touch here is setting the correct fuzzer linker
# flags:  CMake has to add `-fsanitizer=fuzzer' or `-lFuzzingEngine' itself
# when building the targets:  `-fsanitizer=fuzzer' adds a `main' function
# which would cause CMake's setup process to fail.  See
# `fuzzing/src/fuzzing/CMakeLists.txt' for details.

# Each project must be listed after any project it depends on.
bash "build/zlib.sh"
bash "build/bzip2.sh"
bash "build/libarchive.sh"
bash "build/brotli.sh"
bash "build/libpng.sh"
bash "build/freetype.sh"
bash "build/libcxx.sh"
#bash "build/glog.sh"
bash "build/targets.sh"

cd "${dir}"
