#!/usr/bin/env bash

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

set -ex

rstcheck --ignore-language=c,cpp --report=warning morello/morello.rst
rstcheck --ignore-language=c,cpp --report=warning main/acle.rst
rstcheck --ignore-language=c,cpp --report=warning neon_intrinsics/advsimd.rst
# We have to ignore the warning reported inline 223, as it seems reporting a false positive (bug in rstcheck?).
rstcheck --ignore-messages="mve_intrinsics/mve.rst:223" --ignore-language=c,cpp --report=warning mve_intrinsics/mve.rst
