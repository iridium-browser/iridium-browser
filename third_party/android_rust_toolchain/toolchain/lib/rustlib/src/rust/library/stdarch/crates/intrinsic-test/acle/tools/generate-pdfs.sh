#!/usr/bin/env bash
set -ex

# Copyright (c) 2021, Arm Limited
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

mkdir -p pdfs

rst2pdf morello/morello.rst         \
	-s tools/rst2pdf-acle.style \
        --repeat-table-rows         \
        --default-dpi=110           \
        -o pdfs/morello.pdf

# the option`--inline-footnotes` is used to print the footnotes off
# the references "in place" in the `References` section.
rst2pdf main/acle.rst         \
	--inline-footnotes \
	-s tools/rst2pdf-acle.style \
        --repeat-table-rows         \
        --default-dpi=110           \
        -o pdfs/acle.pdf

rst2pdf neon_intrinsics/advsimd.rst         \
	-s tools/rst2pdf-acle-intrinsics.style \
        --repeat-table-rows         \
        --default-dpi=110           \
        -o pdfs/advsimd.pdf

rst2pdf mve_intrinsics/mve.rst         \
	-s tools/rst2pdf-acle-intrinsics.style \
        --repeat-table-rows         \
        --default-dpi=110           \
        -o pdfs/mve.pdf
