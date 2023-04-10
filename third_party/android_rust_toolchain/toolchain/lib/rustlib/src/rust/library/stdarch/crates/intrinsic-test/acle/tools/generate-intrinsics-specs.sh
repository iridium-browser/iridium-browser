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

mkdir -p tmp

function check_changes() {
# Check if the new file created from the db is different from the one
# stored in the specs folder. This is needed to make sure that changes
# to the intrinsic_db are reflected in the rst of the specs in the
# same submission.
    if ! diff  $1 $2 ; then
	echo "**** WARNING! Please check your changes in the specs."
	exit 1
    fi
}

# Function that invokes the script to generate the rst out of the db
# of the specs.
function generate_rst_specs() {
    ./tools/gen-intrinsics-specs.py --intrinsic-defs $1 \
				    --classification $2 \
				    --template $3 --outfile $4
}

# Generate specs
generate_rst_specs ./tools/intrinsic_db/advsimd.csv \
		   ./tools/intrinsic_db/advsimd_classification.csv \
		   ./neon_intrinsics/advsimd.template.rst \
		   ./tmp/advsimd.new.rst

generate_rst_specs ./tools/intrinsic_db/mve.csv \
		   ./tools/intrinsic_db/mve_classification.csv \
		   ./mve_intrinsics/mve.template.rst \
		   ./tmp/mve.new.rst

# Check changes
check_changes ./tmp/advsimd.new.rst ./neon_intrinsics/advsimd.rst
check_changes ./tmp/mve.new.rst ./mve_intrinsics/mve.rst
