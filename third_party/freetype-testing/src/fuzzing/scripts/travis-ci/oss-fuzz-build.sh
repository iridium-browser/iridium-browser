#!/bin/bash
set -exo pipefail

# Copyright 2018 by
# Armin Hasitzka.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

dir="${PWD}"
cd $( dirname $( readlink -f "${0}" ) ) # go to `/fuzzing/scripts/travis-ci'

if [[ ! -v "TRAVIS_BUILD_DIR" ]]; then
    exit 66
fi

cd "${TRAVIS_BUILD_DIR}"

git clone "https://github.com/google/oss-fuzz.git"
cd "oss-fuzz"

python infra/helper.py build_image   --pull freetype2
python infra/helper.py build_fuzzers        freetype2

cd "${dir}"
