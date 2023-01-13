#
# Copyright (c) 2018, Alliance for Open Media. All rights reserved
#
# This source code is subject to the terms of the BSD 2 Clause License and the
# Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License was
# not distributed with this source code in the LICENSE file, you can obtain it
# at www.aomedia.org/license/software. If the Alliance for Open Media Patent
# License 1.0 was not distributed with this source code in the PATENTS file, you
# can obtain it at www.aomedia.org/license/patent.
#
if(AOM_BUILD_CMAKE_TOOLCHAINS_PPC_LINUX_GCC_CMAKE_)
  return()
endif() # AOM_BUILD_CMAKE_TOOLCHAINS_PPC_LINUX_GCC_CMAKE_
set(AOM_BUILD_CMAKE_TOOLCHAINS_PPC_LINUX_GCC_CMAKE_ 1)

set(CMAKE_SYSTEM_NAME "Linux")

if("${CROSS}" STREQUAL "")

  # Default the cross compiler prefix to something known to work.
  set(CROSS powerpc64le-unknown-linux-gnu-)
endif()

if(NOT CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER ${CROSS}gcc)
endif()
if(NOT CMAKE_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER ${CROSS}g++)
endif()
if(NOT AS_EXECUTABLE)
  set(AS_EXECUTABLE ${CROSS}as)
endif()
set(CMAKE_SYSTEM_PROCESSOR "ppc")

set(CONFIG_RUNTIME_CPU_DETECT 0 CACHE STRING "")
