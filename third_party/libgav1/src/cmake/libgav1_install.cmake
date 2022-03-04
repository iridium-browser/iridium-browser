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

if(LIBGAV1_CMAKE_LIBGAV1_INSTALL_CMAKE_)
  return()
endif() # LIBGAV1_CMAKE_LIBGAV1_INSTALL_CMAKE_
set(LIBGAV1_CMAKE_LIBGAV1_INSTALL_CMAKE_ 1)

# Sets up the Libgav1 install targets. Must be called after the static library
# target is created.
macro(libgav1_setup_install_target)
  if(NOT (MSVC OR XCODE))
    include(GNUInstallDirs)

    # pkg-config: libgav1.pc
    set(prefix "${CMAKE_INSTALL_PREFIX}")
    set(exec_prefix "\${prefix}")
    set(libdir "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
    set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
    set(libgav1_lib_name "libgav1")

    configure_file("${libgav1_root}/cmake/libgav1.pc.template"
                   "${libgav1_build}/libgav1.pc" @ONLY NEWLINE_STYLE UNIX)
    install(FILES "${libgav1_build}/libgav1.pc"
            DESTINATION "${prefix}/${CMAKE_INSTALL_LIBDIR}/pkgconfig")

    # CMake config: libgav1-config.cmake
    set(LIBGAV1_INCLUDE_DIRS "${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
    configure_file("${libgav1_root}/cmake/libgav1-config.cmake.template"
                   "${libgav1_build}/libgav1-config.cmake" @ONLY
                   NEWLINE_STYLE UNIX)
    install(
      FILES "${libgav1_build}/libgav1-config.cmake"
      DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/cmake")

    install(
      FILES ${libgav1_api_includes}
      DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/gav1")

    install(TARGETS gav1_decode DESTINATION
                    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
    install(TARGETS libgav1_static DESTINATION
                    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    if(BUILD_SHARED_LIBS)
      install(TARGETS libgav1_shared DESTINATION
                      "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    endif()
  endif()
endmacro()
