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

base_path=$( readlink -f ../.. )

cd "${base_path}"
git reset --hard
git clean -dfqx
git submodule deinit --all -f

cd "${dir}"
