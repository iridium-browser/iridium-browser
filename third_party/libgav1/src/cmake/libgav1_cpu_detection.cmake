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

if(LIBGAV1_CMAKE_LIBGAV1_CPU_DETECTION_CMAKE_)
  return()
endif() # LIBGAV1_CMAKE_LIBGAV1_CPU_DETECTION_CMAKE_
set(LIBGAV1_CMAKE_LIBGAV1_CPU_DETECTION_CMAKE_ 1)

# Detect optimizations available for the current target CPU.
macro(libgav1_optimization_detect)
  if(LIBGAV1_ENABLE_OPTIMIZATIONS)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" cpu_lowercase)
    if(cpu_lowercase MATCHES "^arm|^aarch64")
      set(libgav1_have_neon ON)
    elseif(cpu_lowercase MATCHES "^x86|amd64")
      set(libgav1_have_avx2 ON)
      set(libgav1_have_sse4 ON)
    endif()
  endif()

  if(libgav1_have_avx2 AND LIBGAV1_ENABLE_AVX2)
    list(APPEND libgav1_defines "LIBGAV1_ENABLE_AVX2=1")
  else()
    list(APPEND libgav1_defines "LIBGAV1_ENABLE_AVX2=0")
    set(libgav1_have_avx2 OFF)
  endif()

  if(libgav1_have_neon AND LIBGAV1_ENABLE_NEON)
    list(APPEND libgav1_defines "LIBGAV1_ENABLE_NEON=1")
  else()
    list(APPEND libgav1_defines "LIBGAV1_ENABLE_NEON=0")
    set(libgav1_have_neon, OFF)
  endif()

  if(libgav1_have_sse4 AND LIBGAV1_ENABLE_SSE4_1)
    list(APPEND libgav1_defines "LIBGAV1_ENABLE_SSE4_1=1")
  else()
    list(APPEND libgav1_defines "LIBGAV1_ENABLE_SSE4_1=0")
    set(libgav1_have_sse4 OFF)
  endif()
endmacro()
