# Copyright 2019 The libgav1 Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if(LIBGAV1_UTILS_LIBGAV1_UTILS_CMAKE_)
  return()
endif() # LIBGAV1_UTILS_LIBGAV1_UTILS_CMAKE_
set(LIBGAV1_UTILS_LIBGAV1_UTILS_CMAKE_ 1)

list(APPEND libgav1_utils_sources
            "${libgav1_source}/utils/array_2d.h"
            "${libgav1_source}/utils/bit_mask_set.h"
            "${libgav1_source}/utils/bit_reader.cc"
            "${libgav1_source}/utils/bit_reader.h"
            "${libgav1_source}/utils/block_parameters_holder.cc"
            "${libgav1_source}/utils/block_parameters_holder.h"
            "${libgav1_source}/utils/blocking_counter.h"
            "${libgav1_source}/utils/common.h"
            "${libgav1_source}/utils/compiler_attributes.h"
            "${libgav1_source}/utils/constants.cc"
            "${libgav1_source}/utils/constants.h"
            "${libgav1_source}/utils/cpu.cc"
            "${libgav1_source}/utils/cpu.h"
            "${libgav1_source}/utils/dynamic_buffer.h"
            "${libgav1_source}/utils/entropy_decoder.cc"
            "${libgav1_source}/utils/entropy_decoder.h"
            "${libgav1_source}/utils/executor.cc"
            "${libgav1_source}/utils/executor.h"
            "${libgav1_source}/utils/logging.cc"
            "${libgav1_source}/utils/logging.h"
            "${libgav1_source}/utils/memory.h"
            "${libgav1_source}/utils/queue.h"
            "${libgav1_source}/utils/raw_bit_reader.cc"
            "${libgav1_source}/utils/raw_bit_reader.h"
            "${libgav1_source}/utils/reference_info.h"
            "${libgav1_source}/utils/segmentation.cc"
            "${libgav1_source}/utils/segmentation.h"
            "${libgav1_source}/utils/segmentation_map.cc"
            "${libgav1_source}/utils/segmentation_map.h"
            "${libgav1_source}/utils/stack.h"
            "${libgav1_source}/utils/threadpool.cc"
            "${libgav1_source}/utils/threadpool.h"
            "${libgav1_source}/utils/types.h"
            "${libgav1_source}/utils/unbounded_queue.h"
            "${libgav1_source}/utils/vector.h")

macro(libgav1_add_utils_targets)
  libgav1_add_library(NAME
                      libgav1_utils
                      TYPE
                      OBJECT
                      SOURCES
                      ${libgav1_utils_sources}
                      DEFINES
                      ${libgav1_defines}
                      INCLUDES
                      ${libgav1_include_paths}
                      ${libgav1_gtest_include_paths})

endmacro()
