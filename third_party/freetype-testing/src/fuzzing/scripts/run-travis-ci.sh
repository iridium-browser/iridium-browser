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
cd $( dirname $( readlink -f "${0}" ) ) # go to `/fuzzing/scripts'

case "${TRAVIS_CI_ROW}" in
    "regression-suite")
        bash travis-ci/regression-suite.sh
        ;;
    "oss-fuzz-build")
        bash travis-ci/oss-fuzz-build.sh
        ;;
    *)
        printf "\$TRAVIS_CI_ROW has invalid value: '%s'\n" "${TRAVIS_CI_ROW}"
        exit 66
        ;;
esac

cd "${dir}"
