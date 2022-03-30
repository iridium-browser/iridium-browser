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

if(LIBGAV1_EXAMPLES_LIBGAV1_EXAMPLES_CMAKE_)
  return()
endif() # LIBGAV1_EXAMPLES_LIBGAV1_EXAMPLES_CMAKE_
set(LIBGAV1_EXAMPLES_LIBGAV1_EXAMPLES_CMAKE_ 1)

set(libgav1_file_reader_sources "${libgav1_examples}/file_reader.cc"
                                "${libgav1_examples}/file_reader.h"
                                "${libgav1_examples}/file_reader_constants.cc"
                                "${libgav1_examples}/file_reader_constants.h"
                                "${libgav1_examples}/file_reader_factory.cc"
                                "${libgav1_examples}/file_reader_factory.h"
                                "${libgav1_examples}/file_reader_interface.h"
                                "${libgav1_examples}/ivf_parser.cc"
                                "${libgav1_examples}/ivf_parser.h"
                                "${libgav1_examples}/logging.h")

set(libgav1_file_writer_sources "${libgav1_examples}/file_writer.cc"
                                "${libgav1_examples}/file_writer.h"
                                "${libgav1_examples}/logging.h")

set(libgav1_decode_sources "${libgav1_examples}/gav1_decode.cc")

macro(libgav1_add_examples_targets)
  libgav1_add_library(NAME libgav1_file_reader TYPE OBJECT SOURCES
                      ${libgav1_file_reader_sources} DEFINES ${libgav1_defines}
                      INCLUDES ${libgav1_include_paths})

  libgav1_add_library(NAME libgav1_file_writer TYPE OBJECT SOURCES
                      ${libgav1_file_writer_sources} DEFINES ${libgav1_defines}
                      INCLUDES ${libgav1_include_paths})

  libgav1_add_executable(NAME
                         gav1_decode
                         SOURCES
                         ${libgav1_decode_sources}
                         DEFINES
                         ${libgav1_defines}
                         INCLUDES
                         ${libgav1_include_paths}
                         ${libgav1_gtest_include_paths}
                         OBJLIB_DEPS
                         libgav1_file_reader
                         libgav1_file_writer
                         LIB_DEPS
                         absl::strings
                         absl::str_format_internal
                         absl::time
                         ${libgav1_dependency})
endmacro()
