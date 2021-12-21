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

if(LIBGAV1_CMAKE_LIBGAV1_FLAGS_CMAKE_)
  return()
endif() # LIBGAV1_CMAKE_LIBGAV1_FLAGS_CMAKE_
set(LIBGAV1_CMAKE_LIBGAV1_FLAGS_CMAKE_ 1)

include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

# Adds compiler flags specified by FLAGS to the sources specified by SOURCES:
#
# libgav1_set_compiler_flags_for_sources(SOURCES <sources> FLAGS <flags>)
macro(libgav1_set_compiler_flags_for_sources)
  unset(compiler_SOURCES)
  unset(compiler_FLAGS)
  unset(optional_args)
  unset(single_value_args)
  set(multi_value_args SOURCES FLAGS)
  cmake_parse_arguments(compiler "${optional_args}" "${single_value_args}"
                        "${multi_value_args}" ${ARGN})

  if(NOT (compiler_SOURCES AND compiler_FLAGS))
    libgav1_die("libgav1_set_compiler_flags_for_sources: SOURCES and "
                "FLAGS required.")
  endif()

  set_source_files_properties(${compiler_SOURCES} PROPERTIES COMPILE_FLAGS
                              ${compiler_FLAGS})

  if(LIBGAV1_VERBOSE GREATER 1)
    foreach(source ${compiler_SOURCES})
      foreach(flag ${compiler_FLAGS})
        message("libgav1_set_compiler_flags_for_sources: source:${source} "
                "flag:${flag}")
      endforeach()
    endforeach()
  endif()
endmacro()

# Tests compiler flags stored in list(s) specified by FLAG_LIST_VAR_NAMES, adds
# flags to $LIBGAV1_CXX_FLAGS when tests pass. Terminates configuration if
# FLAG_REQUIRED is specified and any flag check fails.
#
# ~~~
# libgav1_test_cxx_flag(<FLAG_LIST_VAR_NAMES <flag list variable(s)>>
#                       [FLAG_REQUIRED])
# ~~~
macro(libgav1_test_cxx_flag)
  unset(cxx_test_FLAG_LIST_VAR_NAMES)
  unset(cxx_test_FLAG_REQUIRED)
  unset(single_value_args)
  set(optional_args FLAG_REQUIRED)
  set(multi_value_args FLAG_LIST_VAR_NAMES)
  cmake_parse_arguments(cxx_test "${optional_args}" "${single_value_args}"
                        "${multi_value_args}" ${ARGN})

  if(NOT cxx_test_FLAG_LIST_VAR_NAMES)
    libgav1_die("libgav1_test_cxx_flag: FLAG_LIST_VAR_NAMES required")
  endif()

  unset(cxx_flags)
  foreach(list_var ${cxx_test_FLAG_LIST_VAR_NAMES})
    if(LIBGAV1_VERBOSE)
      message("libgav1_test_cxx_flag: adding ${list_var} to cxx_flags")
    endif()
    list(APPEND cxx_flags ${${list_var}})
  endforeach()

  if(LIBGAV1_VERBOSE)
    message("CXX test: all flags: ${cxx_flags}")
  endif()

  unset(all_cxx_flags)
  list(APPEND all_cxx_flags ${LIBGAV1_CXX_FLAGS} ${cxx_flags})

  # Turn off output from check_cxx_source_compiles. Print status directly
  # instead since the logging messages from check_cxx_source_compiles can be
  # quite confusing.
  set(CMAKE_REQUIRED_QUIET TRUE)

  # Run the actual compile test.
  unset(libgav1_all_cxx_flags_pass CACHE)
  message("--- Running combined CXX flags test, flags: ${all_cxx_flags}")
  check_cxx_compiler_flag("${all_cxx_flags}" libgav1_all_cxx_flags_pass)

  if(cxx_test_FLAG_REQUIRED AND NOT libgav1_all_cxx_flags_pass)
    libgav1_die("Flag test failed for required flag(s): "
                "${all_cxx_flags} and FLAG_REQUIRED specified.")
  endif()

  if(libgav1_all_cxx_flags_pass)
    # Test passed: update the global flag list used by the libgav1 target
    # creation wrappers.
    set(LIBGAV1_CXX_FLAGS ${cxx_flags})
    list(REMOVE_DUPLICATES LIBGAV1_CXX_FLAGS)

    if(LIBGAV1_VERBOSE)
      message("LIBGAV1_CXX_FLAGS=${LIBGAV1_CXX_FLAGS}")
    endif()

    message("--- Passed combined CXX flags test")
  else()
    message("--- Failed combined CXX flags test, testing flags individually.")

    if(cxx_flags)
      message("--- Testing flags from $cxx_flags: " "${cxx_flags}")
      foreach(cxx_flag ${cxx_flags})
        # Between 3.17.0 and 3.18.2 check_cxx_compiler_flag() sets a normal
        # variable at parent scope while check_cxx_source_compiles() continues
        # to set an internal cache variable, so we unset both to avoid the
        # failure / success state persisting between checks. See
        # https://gitlab.kitware.com/cmake/cmake/-/issues/21207.
        unset(cxx_flag_test_passed)
        unset(cxx_flag_test_passed CACHE)
        message("--- Testing flag: ${cxx_flag}")
        check_cxx_compiler_flag("${cxx_flag}" cxx_flag_test_passed)

        if(cxx_flag_test_passed)
          message("--- Passed test for ${cxx_flag}")
        else()
          list(REMOVE_ITEM cxx_flags ${cxx_flag})
          message("--- Failed test for ${cxx_flag}, flag removed.")
        endif()
      endforeach()

      set(LIBGAV1_CXX_FLAGS ${cxx_flags})
    endif()
  endif()

  if(LIBGAV1_CXX_FLAGS)
    list(REMOVE_DUPLICATES LIBGAV1_CXX_FLAGS)
  endif()
endmacro()

# Tests executable linker flags stored in list specified by FLAG_LIST_VAR_NAME,
# adds flags to $LIBGAV1_EXE_LINKER_FLAGS when test passes. Terminates
# configuration when flag check fails. libgav1_set_cxx_flags() must be called
# before calling this macro because it assumes $LIBGAV1_CXX_FLAGS contains only
# valid CXX flags.
#
# libgav1_test_exe_linker_flag(<FLAG_LIST_VAR_NAME <flag list variable)>)
macro(libgav1_test_exe_linker_flag)
  unset(link_FLAG_LIST_VAR_NAME)
  unset(optional_args)
  unset(multi_value_args)
  set(single_value_args FLAG_LIST_VAR_NAME)
  cmake_parse_arguments(link "${optional_args}" "${single_value_args}"
                        "${multi_value_args}" ${ARGN})

  if(NOT link_FLAG_LIST_VAR_NAME)
    libgav1_die("libgav1_test_link_flag: FLAG_LIST_VAR_NAME required")
  endif()

  libgav1_set_and_stringify(DEST linker_flags SOURCE_VARS
                            ${link_FLAG_LIST_VAR_NAME})

  if(LIBGAV1_VERBOSE)
    message("EXE LINKER test: all flags: ${linker_flags}")
  endif()

  # Tests of $LIBGAV1_CXX_FLAGS have already passed. Include them with the
  # linker test.
  libgav1_set_and_stringify(DEST CMAKE_REQUIRED_FLAGS SOURCE_VARS
                            LIBGAV1_CXX_FLAGS)

  # Cache the global exe linker flags.
  if(CMAKE_EXE_LINKER_FLAGS)
    set(cached_CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
    libgav1_set_and_stringify(DEST CMAKE_EXE_LINKER_FLAGS SOURCE
                              ${linker_flags})
  endif()

  libgav1_set_and_stringify(DEST CMAKE_EXE_LINKER_FLAGS SOURCE ${linker_flags}
                            ${CMAKE_EXE_LINKER_FLAGS})

  # Turn off output from check_cxx_source_compiles. Print status directly
  # instead since the logging messages from check_cxx_source_compiles can be
  # quite confusing.
  set(CMAKE_REQUIRED_QUIET TRUE)

  message("--- Running EXE LINKER test for flags: ${linker_flags}")

  unset(linker_flag_test_passed CACHE)
  set(libgav1_cxx_main "\nint main() { return 0; }")
  check_cxx_source_compiles("${libgav1_cxx_main}" linker_flag_test_passed)

  if(NOT linker_flag_test_passed)
    libgav1_die("EXE LINKER test failed.")
  endif()

  message("--- Passed EXE LINKER flag test.")

  # Restore cached global exe linker flags.
  if(cached_CMAKE_EXE_LINKER_FLAGS)
    set(CMAKE_EXE_LINKER_FLAGS ${cached_CMAKE_EXE_LINKER_FLAGS})
  else()
    unset(CMAKE_EXE_LINKER_FLAGS)
  endif()
endmacro()

# Runs the libgav1 compiler tests. This macro builds up the list of list var(s)
# that is passed to libgav1_test_cxx_flag().
#
# Note: libgav1_set_build_definitions() must be called before this macro.
macro(libgav1_set_cxx_flags)
  unset(cxx_flag_lists)

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    list(APPEND cxx_flag_lists libgav1_base_cxx_flags)
  endif()

  # Append clang flags after the base set to allow -Wno* overrides to take
  # effect. Some of the base flags may enable a large set of warnings, e.g.,
  # -Wall.
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND cxx_flag_lists libgav1_clang_cxx_flags)
  endif()

  if(MSVC)
    list(APPEND cxx_flag_lists libgav1_msvc_cxx_flags)
  endif()

  if(LIBGAV1_VERBOSE)
    if(cxx_flag_lists)
      libgav1_set_and_stringify(DEST cxx_flags SOURCE_VARS ${cxx_flag_lists})
      message("libgav1_set_cxx_flags: internal CXX flags: ${cxx_flags}")
    endif()
  endif()

  if(LIBGAV1_CXX_FLAGS)
    list(APPEND cxx_flag_lists LIBGAV1_CXX_FLAGS)
    if(LIBGAV1_VERBOSE)
      message("libgav1_set_cxx_flags: user CXX flags: ${LIBGAV1_CXX_FLAGS}")
    endif()
  endif()

  libgav1_test_cxx_flag(FLAG_LIST_VAR_NAMES ${cxx_flag_lists})
endmacro()

# Sets LIBGAV1_TEST_C_FLAGS and LIBGAV1_TEST_CXX_FLAGS.
#
# Note: libgav1_set_cxx_flags() must be called before this macro. Furthermore,
# the call to this macro should be made after all additions to LIBGAV1_CXX_FLAGS
# are complete.
macro(libgav1_set_test_flags)
  if(LIBGAV1_ENABLE_TESTS)
    set(LIBGAV1_TEST_CXX_FLAGS ${LIBGAV1_CXX_FLAGS})
    list(FILTER LIBGAV1_TEST_CXX_FLAGS EXCLUDE REGEX "-Wframe-larger-than")

    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL CMAKE_C_COMPILER_ID)
      message(
        FATAL_ERROR
          "C/CXX compiler mismatch (${CMAKE_C_COMPILER_ID} vs"
          " ${CMAKE_CXX_COMPILER_ID})! Compiler flags are only tested using"
          " CMAKE_CXX_COMPILER, rerun cmake with CMAKE_C_COMPILER set to the"
          " C compiler from the same package as CMAKE_CXX_COMPILER to ensure"
          " the build completes successfully.")
    endif()
    set(LIBGAV1_TEST_C_FLAGS ${LIBGAV1_TEST_CXX_FLAGS})
    list(FILTER LIBGAV1_TEST_C_FLAGS EXCLUDE REGEX
         "-fvisibility-inlines-hidden")
  endif()
endmacro()
