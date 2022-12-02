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

if(LIBGAV1_CMAKE_LIBGAV1_HELPERS_CMAKE_)
  return()
endif() # LIBGAV1_CMAKE_LIBGAV1_HELPERS_CMAKE_
set(LIBGAV1_CMAKE_LIBGAV1_HELPERS_CMAKE_ 1)

# Kills build generation using message(FATAL_ERROR) and outputs all data passed
# to the console via use of $ARGN.
macro(libgav1_die)
  # macro parameters are not variables so a temporary is needed to work with
  # list().
  set(msg ${ARGN})
  # message(${ARGN}) will merge all list elements with no separator while
  # "${ARGN}" will output the list as a ';' delimited string.
  list(JOIN msg " " msg)
  message(FATAL_ERROR "${msg}")
endmacro()

# Converts semi-colon delimited list variable(s) to string. Output is written to
# variable supplied via the DEST parameter. Input is from an expanded variable
# referenced by SOURCE and/or variable(s) referenced by SOURCE_VARS.
macro(libgav1_set_and_stringify)
  set(optional_args)
  set(single_value_args DEST SOURCE_VAR)
  set(multi_value_args SOURCE SOURCE_VARS)
  cmake_parse_arguments(sas "${optional_args}" "${single_value_args}"
                        "${multi_value_args}" ${ARGN})

  if(NOT sas_DEST OR NOT (sas_SOURCE OR sas_SOURCE_VARS))
    libgav1_die("libgav1_set_and_stringify: DEST and at least one of SOURCE "
                "SOURCE_VARS required.")
  endif()

  unset(${sas_DEST})

  if(sas_SOURCE)
    # $sas_SOURCE is one or more expanded variables, just copy the values to
    # $sas_DEST.
    set(${sas_DEST} "${sas_SOURCE}")
  endif()

  if(sas_SOURCE_VARS)
    # $sas_SOURCE_VARS is one or more variable names. Each iteration expands a
    # variable and appends it to $sas_DEST.
    foreach(source_var ${sas_SOURCE_VARS})
      set(${sas_DEST} "${${sas_DEST}} ${${source_var}}")
    endforeach()

    # Because $sas_DEST can be empty when entering this scope leading whitespace
    # can be introduced to $sas_DEST on the first iteration of the above loop.
    # Remove it:
    string(STRIP "${${sas_DEST}}" ${sas_DEST})
  endif()

  # Lists in CMake are simply semicolon delimited strings, so stringification is
  # just a find and replace of the semicolon.
  string(REPLACE ";" " " ${sas_DEST} "${${sas_DEST}}")

  if(LIBGAV1_VERBOSE GREATER 1)
    message("libgav1_set_and_stringify: ${sas_DEST}=${${sas_DEST}}")
  endif()
endmacro()

# Creates a dummy source file in $LIBGAV1_GENERATED_SOURCES_DIRECTORY and adds
# it to the specified target. Optionally adds its path to a list variable.
#
# libgav1_create_dummy_source_file(<TARGET <target> BASENAME <basename of file>>
# [LISTVAR <list variable>])
macro(libgav1_create_dummy_source_file)
  set(optional_args)
  set(single_value_args TARGET BASENAME LISTVAR)
  set(multi_value_args)
  cmake_parse_arguments(cdsf "${optional_args}" "${single_value_args}"
                        "${multi_value_args}" ${ARGN})

  if(NOT cdsf_TARGET OR NOT cdsf_BASENAME)
    libgav1_die(
      "libgav1_create_dummy_source_file: TARGET and BASENAME required.")
  endif()

  if(NOT LIBGAV1_GENERATED_SOURCES_DIRECTORY)
    set(LIBGAV1_GENERATED_SOURCES_DIRECTORY "${libgav1_build}/gen_src")
  endif()

  set(dummy_source_dir "${LIBGAV1_GENERATED_SOURCES_DIRECTORY}")
  set(dummy_source_file
      "${dummy_source_dir}/libgav1_${cdsf_TARGET}_${cdsf_BASENAME}.cc")
  set(dummy_source_code
      "// Generated file. DO NOT EDIT!\n"
      "// C++ source file created for target ${cdsf_TARGET}.\n"
      "void libgav1_${cdsf_TARGET}_${cdsf_BASENAME}_dummy_function(void)\;\n"
      "void libgav1_${cdsf_TARGET}_${cdsf_BASENAME}_dummy_function(void) {}\n")
  file(WRITE "${dummy_source_file}" ${dummy_source_code})

  target_sources(${cdsf_TARGET} PRIVATE ${dummy_source_file})

  if(cdsf_LISTVAR)
    list(APPEND ${cdsf_LISTVAR} "${dummy_source_file}")
  endif()
endmacro()

# Loads the version components from $libgav1_source/gav1/version.h and sets the
# corresponding CMake variables:
# - LIBGAV1_MAJOR_VERSION
# - LIBGAV1_MINOR_VERSION
# - LIBGAV1_PATCH_VERSION
# - LIBGAV1_VERSION, which is:
#   - $LIBGAV1_MAJOR_VERSION.$LIBGAV1_MINOR_VERSION.$LIBGAV1_PATCH_VERSION
macro(libgav1_load_version_info)
  file(STRINGS "${libgav1_source}/gav1/version.h" version_file_strings)
  foreach(str ${version_file_strings})
    if(str MATCHES "#define LIBGAV1_")
      if(str MATCHES "#define LIBGAV1_MAJOR_VERSION ")
        string(REPLACE "#define LIBGAV1_MAJOR_VERSION " "" LIBGAV1_MAJOR_VERSION
                       "${str}")
      elseif(str MATCHES "#define LIBGAV1_MINOR_VERSION ")
        string(REPLACE "#define LIBGAV1_MINOR_VERSION " "" LIBGAV1_MINOR_VERSION
                       "${str}")
      elseif(str MATCHES "#define LIBGAV1_PATCH_VERSION ")
        string(REPLACE "#define LIBGAV1_PATCH_VERSION " "" LIBGAV1_PATCH_VERSION
                       "${str}")
      endif()
    endif()
  endforeach()
  set(LIBGAV1_VERSION "${LIBGAV1_MAJOR_VERSION}.${LIBGAV1_MINOR_VERSION}")
  set(LIBGAV1_VERSION "${LIBGAV1_VERSION}.${LIBGAV1_PATCH_VERSION}")
endmacro()
