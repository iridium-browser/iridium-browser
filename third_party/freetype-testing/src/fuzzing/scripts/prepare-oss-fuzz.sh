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
cd $( dirname $( readlink -f "${0}" ) ) # go to `/fuzzing/scripts'

bin_base_dir=$(      readlink -f "../build/bin"          )
corpora_base_dir=$(  readlink -f "../corpora"            )
settings_base_dir=$( readlink -f "../settings/oss-fuzz"  )

fuzzers=(
    "legacy"

    "bdf"
    "bdf-render"

    "cff"
    "cff-ftengine"
    "cff-render"
    "cff-render-ftengine"

    "cidtype1"
    "cidtype1-ftengine"
    "cidtype1-render"
    "cidtype1-render-ftengine"

    "colrv1"

    "pcf"
    "pcf-render"

    "truetype"
    "truetype-render"
    "truetype-render-i35"
    "truetype-render-i38"

    "type1"
    "type1-ftengine"
    "type1-render"
    "type1-render-ftengine"
    "type1-render-tar"
    "type1-tar"

    "type42"
    "type42-render"

    "windowsfnt"
    "windowsfnt-render"

    "glyphs-outlines"

    "glyphs-bitmaps-pcf"

    "gzip"
    "lzw"
    "bzip2"
)

# This script relies on:
#   - `$OUT':  directory to store build artifacts
#   - `$WORK': directory for storing intermediate files

cp -a "${settings_base_dir}/." "${OUT}"

for fuzzer in "${fuzzers[@]}"; do

    cp "${bin_base_dir}/${fuzzer}" "${OUT}/${fuzzer}"

    seed_dir="${WORK}/${fuzzer}_seed_corpus"
    seed_zip="${OUT}/${fuzzer}_seed_corpus.zip"

    mkdir -p "${seed_dir}"

    find "${corpora_base_dir}/${fuzzer}" \
         -type f                         \
         ! -name "README.md"             \
         -exec cp {} "${seed_dir}" \;

    zip -j "${seed_zip}" "${seed_dir}/"*

done

cd "${dir}"
