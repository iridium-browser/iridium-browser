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

if(LIBGAV1_SRC_LIBGAV1_DECODER_CMAKE_)
  return()
endif() # LIBGAV1_SRC_LIBGAV1_DECODER_CMAKE_
set(LIBGAV1_SRC_LIBGAV1_DECODER_CMAKE_ 1)

list(APPEND libgav1_decoder_sources
            "${libgav1_source}/buffer_pool.cc"
            "${libgav1_source}/buffer_pool.h"
            "${libgav1_source}/decoder_impl.cc"
            "${libgav1_source}/decoder_impl.h"
            "${libgav1_source}/decoder_state.h"
            "${libgav1_source}/tile_scratch_buffer.cc"
            "${libgav1_source}/tile_scratch_buffer.h"
            "${libgav1_source}/film_grain.cc"
            "${libgav1_source}/film_grain.h"
            "${libgav1_source}/frame_buffer.cc"
            "${libgav1_source}/frame_buffer_utils.h"
            "${libgav1_source}/frame_scratch_buffer.h"
            "${libgav1_source}/inter_intra_masks.inc"
            "${libgav1_source}/internal_frame_buffer_list.cc"
            "${libgav1_source}/internal_frame_buffer_list.h"
            "${libgav1_source}/loop_restoration_info.cc"
            "${libgav1_source}/loop_restoration_info.h"
            "${libgav1_source}/motion_vector.cc"
            "${libgav1_source}/motion_vector.h"
            "${libgav1_source}/obu_parser.cc"
            "${libgav1_source}/obu_parser.h"
            "${libgav1_source}/post_filter/cdef.cc"
            "${libgav1_source}/post_filter/deblock.cc"
            "${libgav1_source}/post_filter/deblock_thresholds.inc"
            "${libgav1_source}/post_filter/loop_restoration.cc"
            "${libgav1_source}/post_filter/post_filter.cc"
            "${libgav1_source}/post_filter/super_res.cc"
            "${libgav1_source}/post_filter.h"
            "${libgav1_source}/prediction_mask.cc"
            "${libgav1_source}/prediction_mask.h"
            "${libgav1_source}/quantizer.cc"
            "${libgav1_source}/quantizer.h"
            "${libgav1_source}/quantizer_tables.inc"
            "${libgav1_source}/reconstruction.cc"
            "${libgav1_source}/reconstruction.h"
            "${libgav1_source}/residual_buffer_pool.cc"
            "${libgav1_source}/residual_buffer_pool.h"
            "${libgav1_source}/scan_tables.inc"
            "${libgav1_source}/symbol_decoder_context.cc"
            "${libgav1_source}/symbol_decoder_context.h"
            "${libgav1_source}/symbol_decoder_context_cdfs.inc"
            "${libgav1_source}/threading_strategy.cc"
            "${libgav1_source}/threading_strategy.h"
            "${libgav1_source}/tile.h"
            "${libgav1_source}/tile/bitstream/mode_info.cc"
            "${libgav1_source}/tile/bitstream/palette.cc"
            "${libgav1_source}/tile/bitstream/partition.cc"
            "${libgav1_source}/tile/bitstream/transform_size.cc"
            "${libgav1_source}/tile/prediction.cc"
            "${libgav1_source}/tile/tile.cc"
            "${libgav1_source}/warp_prediction.cc"
            "${libgav1_source}/warp_prediction.h"
            "${libgav1_source}/yuv_buffer.cc"
            "${libgav1_source}/yuv_buffer.h")

list(APPEND libgav1_api_includes "${libgav1_source}/gav1/decoder.h"
            "${libgav1_source}/gav1/decoder_buffer.h"
            "${libgav1_source}/gav1/decoder_settings.h"
            "${libgav1_source}/gav1/frame_buffer.h"
            "${libgav1_source}/gav1/status_code.h"
            "${libgav1_source}/gav1/symbol_visibility.h"
            "${libgav1_source}/gav1/version.h")

list(APPEND libgav1_api_sources "${libgav1_source}/decoder.cc"
            "${libgav1_source}/decoder_settings.cc"
            "${libgav1_source}/status_code.cc"
            "${libgav1_source}/version.cc"
            ${libgav1_api_includes})

macro(libgav1_add_decoder_targets)
  if(BUILD_SHARED_LIBS)
    if(MSVC OR WIN32)
      # In order to produce a DLL and import library the Windows tools require
      # that the exported symbols are part of the DLL target. The unfortunate
      # side effect of this is that a single configuration cannot output both
      # the static library and the DLL: This results in an either/or situation.
      # Windows users of the libgav1 build can have a DLL and an import library,
      # or they can have a static library; they cannot have both from a single
      # configuration of the build.
      list(APPEND libgav1_shared_lib_sources ${libgav1_api_sources})
      list(APPEND libgav1_static_lib_sources ${libgav1_api_includes})
    else()
      list(APPEND libgav1_shared_lib_sources ${libgav1_api_includes})
      list(APPEND libgav1_static_lib_sources ${libgav1_api_sources})
    endif()
  else()
    list(APPEND libgav1_static_lib_sources ${libgav1_api_sources})
  endif()

  if(use_absl_threading)
    list(APPEND libgav1_absl_deps absl::base absl::synchronization)
  endif()

  libgav1_add_library(NAME libgav1_decoder TYPE OBJECT SOURCES
                      ${libgav1_decoder_sources} DEFINES ${libgav1_defines}
                      INCLUDES ${libgav1_include_paths})

  libgav1_add_library(NAME
                      libgav1_static
                      OUTPUT_NAME
                      libgav1
                      TYPE
                      STATIC
                      SOURCES
                      ${libgav1_static_lib_sources}
                      DEFINES
                      ${libgav1_defines}
                      INCLUDES
                      ${libgav1_include_paths}
                      LIB_DEPS
                      ${libgav1_absl_deps}
                      OBJLIB_DEPS
                      libgav1_dsp
                      libgav1_decoder
                      libgav1_utils
                      PUBLIC_INCLUDES
                      ${libgav1_source})

  if(BUILD_SHARED_LIBS)
    libgav1_add_library(NAME
                        libgav1_shared
                        OUTPUT_NAME
                        libgav1
                        TYPE
                        SHARED
                        SOURCES
                        ${libgav1_shared_lib_sources}
                        DEFINES
                        ${libgav1_defines}
                        INCLUDES
                        ${libgav1_include_paths}
                        LIB_DEPS
                        libgav1_static
                        PUBLIC_INCLUDES
                        ${libgav1_source})
  endif()
endmacro()
