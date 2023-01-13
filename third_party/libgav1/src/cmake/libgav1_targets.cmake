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

if(LIBGAV1_CMAKE_GAV1_TARGETS_CMAKE_)
  return()
endif() # LIBGAV1_CMAKE_GAV1_TARGETS_CMAKE_
set(LIBGAV1_CMAKE_GAV1_TARGETS_CMAKE_ 1)

if(LIBGAV1_IDE_FOLDER)
  set(LIBGAV1_EXAMPLES_IDE_FOLDER "${LIBGAV1_IDE_FOLDER}/examples")
  set(LIBGAV1_TESTS_IDE_FOLDER "${LIBGAV1_IDE_FOLDER}/tests")
else()
  set(LIBGAV1_EXAMPLES_IDE_FOLDER "libgav1_examples")
  set(LIBGAV1_TESTS_IDE_FOLDER "libgav1_tests")
endif()

# Resets list variables used to track libgav1 targets.
macro(libgav1_reset_target_lists)
  unset(libgav1_targets)
  unset(libgav1_exe_targets)
  unset(libgav1_lib_targets)
  unset(libgav1_objlib_targets)
  unset(libgav1_sources)
  unset(libgav1_test_targets)
endmacro()

# Creates an executable target. The target name is passed as a parameter to the
# NAME argument, and the sources passed as a parameter to the SOURCES argument:
# libgav1_add_executable(NAME <name> SOURCES <sources> [optional args])
#
# Optional args:
# cmake-format: off
#   - OUTPUT_NAME: Override output file basename. Target basename defaults to
#     NAME.
#   - TEST: Flag. Presence means treat executable as a test.
#   - DEFINES: List of preprocessor macro definitions.
#   - INCLUDES: list of include directories for the target.
#   - COMPILE_FLAGS: list of compiler flags for the target.
#   - LINK_FLAGS: List of linker flags for the target.
#   - OBJLIB_DEPS: List of CMake object library target dependencies.
#   - LIB_DEPS: List of CMake library dependencies.
# cmake-format: on
#
# Sources passed to this macro are added to $libgav1_test_sources when TEST is
# specified. Otherwise sources are added to $libgav1_sources.
#
# Targets passed to this macro are always added $libgav1_targets. When TEST is
# specified targets are also added to list $libgav1_test_targets. Otherwise
# targets are added to $libgav1_exe_targets.
macro(libgav1_add_executable)
  unset(exe_TEST)
  unset(exe_TEST_DEFINES_MAIN)
  unset(exe_NAME)
  unset(exe_OUTPUT_NAME)
  unset(exe_SOURCES)
  unset(exe_DEFINES)
  unset(exe_INCLUDES)
  unset(exe_COMPILE_FLAGS)
  unset(exe_LINK_FLAGS)
  unset(exe_OBJLIB_DEPS)
  unset(exe_LIB_DEPS)
  set(optional_args TEST)
  set(single_value_args NAME OUTPUT_NAME)
  set(multi_value_args SOURCES DEFINES INCLUDES COMPILE_FLAGS LINK_FLAGS
                       OBJLIB_DEPS LIB_DEPS)

  cmake_parse_arguments(exe "${optional_args}" "${single_value_args}"
                        "${multi_value_args}" ${ARGN})

  if(LIBGAV1_VERBOSE GREATER 1)
    message("--------- libgav1_add_executable ---------\n"
            "exe_TEST=${exe_TEST}\n"
            "exe_TEST_DEFINES_MAIN=${exe_TEST_DEFINES_MAIN}\n"
            "exe_NAME=${exe_NAME}\n"
            "exe_OUTPUT_NAME=${exe_OUTPUT_NAME}\n"
            "exe_SOURCES=${exe_SOURCES}\n"
            "exe_DEFINES=${exe_DEFINES}\n"
            "exe_INCLUDES=${exe_INCLUDES}\n"
            "exe_COMPILE_FLAGS=${exe_COMPILE_FLAGS}\n"
            "exe_LINK_FLAGS=${exe_LINK_FLAGS}\n"
            "exe_OBJLIB_DEPS=${exe_OBJLIB_DEPS}\n"
            "exe_LIB_DEPS=${exe_LIB_DEPS}\n"
            "------------------------------------------\n")
  endif()

  if(NOT (exe_NAME AND exe_SOURCES))
    message(FATAL_ERROR "libgav1_add_executable: NAME and SOURCES required.")
  endif()

  list(APPEND libgav1_targets ${exe_NAME})
  if(exe_TEST)
    list(APPEND libgav1_test_targets ${exe_NAME})
    list(APPEND libgav1_test_sources ${exe_SOURCES})
  else()
    list(APPEND libgav1_exe_targets ${exe_NAME})
    list(APPEND libgav1_sources ${exe_SOURCES})
  endif()

  add_executable(${exe_NAME} ${exe_SOURCES})
  if(exe_TEST)
    add_test(NAME ${exe_NAME} COMMAND ${exe_NAME})
    set_property(TARGET ${exe_NAME} PROPERTY FOLDER ${LIBGAV1_TESTS_IDE_FOLDER})
  else()
    set_property(TARGET ${exe_NAME}
                 PROPERTY FOLDER ${LIBGAV1_EXAMPLES_IDE_FOLDER})
  endif()

  if(exe_OUTPUT_NAME)
    set_target_properties(${exe_NAME} PROPERTIES OUTPUT_NAME ${exe_OUTPUT_NAME})
  endif()

  libgav1_process_intrinsics_sources(TARGET ${exe_NAME} SOURCES ${exe_SOURCES})

  if(exe_DEFINES)
    target_compile_definitions(${exe_NAME} PRIVATE ${exe_DEFINES})
  endif()

  if(exe_INCLUDES)
    target_include_directories(${exe_NAME} PRIVATE ${exe_INCLUDES})
  endif()

  unset(exe_LIBGAV1_COMPILE_FLAGS)
  if(exe_TEST)
    list(FILTER exe_SOURCES INCLUDE REGEX "\\.c$")
    list(LENGTH exe_SOURCES exe_SOURCES_length)
    if(exe_SOURCES_length EQUAL 0)
      set(exe_LIBGAV1_COMPILE_FLAGS ${LIBGAV1_TEST_CXX_FLAGS})
    else()
      set(exe_LIBGAV1_COMPILE_FLAGS ${LIBGAV1_TEST_C_FLAGS})
    endif()
  else()
    set(exe_LIBGAV1_COMPILE_FLAGS ${LIBGAV1_CXX_FLAGS})
  endif()

  if(exe_COMPILE_FLAGS OR exe_LIBGAV1_COMPILE_FLAGS)
    target_compile_options(${exe_NAME}
                           PRIVATE ${exe_COMPILE_FLAGS}
                                   ${exe_LIBGAV1_COMPILE_FLAGS})
  endif()

  if(exe_LINK_FLAGS OR LIBGAV1_EXE_LINKER_FLAGS)
    list(APPEND exe_LINK_FLAGS "${LIBGAV1_EXE_LINKER_FLAGS}")
    if(${CMAKE_VERSION} VERSION_LESS "3.13")
      # LINK_FLAGS is managed as a string.
      libgav1_set_and_stringify(SOURCE "${exe_LINK_FLAGS}" DEST exe_LINK_FLAGS)
      set_target_properties(${exe_NAME}
                            PROPERTIES LINK_FLAGS "${exe_LINK_FLAGS}")
    else()
      target_link_options(${exe_NAME} PRIVATE ${exe_LINK_FLAGS})
    endif()
  endif()

  if(exe_OBJLIB_DEPS)
    foreach(objlib_dep ${exe_OBJLIB_DEPS})
      target_sources(${exe_NAME} PRIVATE $<TARGET_OBJECTS:${objlib_dep}>)
    endforeach()
  endif()

  if(CMAKE_THREAD_LIBS_INIT)
    list(APPEND exe_LIB_DEPS ${CMAKE_THREAD_LIBS_INIT})
  endif()

  if(BUILD_SHARED_LIBS AND (MSVC OR WIN32))
    target_compile_definitions(${exe_NAME} PRIVATE "LIBGAV1_BUILDING_DLL=0")
  endif()

  if(exe_LIB_DEPS)
    unset(exe_static)
    if("${CMAKE_EXE_LINKER_FLAGS} ${LIBGAV1_EXE_LINKER_FLAGS}" MATCHES "static")
      set(exe_static ON)
    endif()

    if(exe_static AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
      # Third party dependencies can introduce dependencies on system and test
      # libraries. Since the target created here is an executable, and CMake
      # does not provide a method of controlling order of link dependencies,
      # wrap all of the dependencies of this target in start/end group flags to
      # ensure that dependencies of third party targets can be resolved when
      # those dependencies happen to be resolved by dependencies of the current
      # target.
      list(INSERT exe_LIB_DEPS 0 -Wl,--start-group)
      list(APPEND exe_LIB_DEPS -Wl,--end-group)
    endif()
    target_link_libraries(${exe_NAME} PRIVATE ${exe_LIB_DEPS})
  endif()
endmacro()

# Creates a library target of the specified type. The target name is passed as a
# parameter to the NAME argument, the type as a parameter to the TYPE argument,
# and the sources passed as a parameter to the SOURCES argument:
# libgav1_add_library(NAME <name> TYPE <type> SOURCES <sources> [optional args])
#
# Optional args:
# cmake-format: off
#   - OUTPUT_NAME: Override output file basename. Target basename defaults to
#     NAME. OUTPUT_NAME is ignored when BUILD_SHARED_LIBS is enabled and CMake
#     is generating a build for which MSVC or WIN32 are true. This is to avoid
#     output basename collisions with DLL import libraries.
#   - TEST: Flag. Presence means treat library as a test.
#   - DEFINES: List of preprocessor macro definitions.
#   - INCLUDES: list of include directories for the target.
#   - COMPILE_FLAGS: list of compiler flags for the target.
#   - LINK_FLAGS: List of linker flags for the target.
#   - OBJLIB_DEPS: List of CMake object library target dependencies.
#   - LIB_DEPS: List of CMake library dependencies.
#   - PUBLIC_INCLUDES: List of include paths to export to dependents.
# cmake-format: on
#
# Sources passed to the macro are added to the lists tracking libgav1 sources:
# cmake-format: off
#   - When TEST is specified sources are added to $libgav1_test_sources.
#   - Otherwise sources are added to $libgav1_sources.
# cmake-format: on
#
# Targets passed to this macro are added to the lists tracking libgav1 targets:
# cmake-format: off
#   - Targets are always added to $libgav1_targets.
#   - When the TEST flag is specified, targets are added to
#     $libgav1_test_targets.
#   - When TEST is not specified:
#     - Libraries of type SHARED are added to $libgav1_dylib_targets.
#     - Libraries of type OBJECT are added to $libgav1_objlib_targets.
#     - Libraries of type STATIC are added to $libgav1_lib_targets.
# cmake-format: on
macro(libgav1_add_library)
  unset(lib_TEST)
  unset(lib_NAME)
  unset(lib_OUTPUT_NAME)
  unset(lib_TYPE)
  unset(lib_SOURCES)
  unset(lib_DEFINES)
  unset(lib_INCLUDES)
  unset(lib_COMPILE_FLAGS)
  unset(lib_LINK_FLAGS)
  unset(lib_OBJLIB_DEPS)
  unset(lib_LIB_DEPS)
  unset(lib_PUBLIC_INCLUDES)
  set(optional_args TEST)
  set(single_value_args NAME OUTPUT_NAME TYPE)
  set(multi_value_args SOURCES DEFINES INCLUDES COMPILE_FLAGS LINK_FLAGS
                       OBJLIB_DEPS LIB_DEPS PUBLIC_INCLUDES)

  cmake_parse_arguments(lib "${optional_args}" "${single_value_args}"
                        "${multi_value_args}" ${ARGN})

  if(LIBGAV1_VERBOSE GREATER 1)
    message("--------- libgav1_add_library ---------\n"
            "lib_TEST=${lib_TEST}\n"
            "lib_NAME=${lib_NAME}\n"
            "lib_OUTPUT_NAME=${lib_OUTPUT_NAME}\n"
            "lib_TYPE=${lib_TYPE}\n"
            "lib_SOURCES=${lib_SOURCES}\n"
            "lib_DEFINES=${lib_DEFINES}\n"
            "lib_INCLUDES=${lib_INCLUDES}\n"
            "lib_COMPILE_FLAGS=${lib_COMPILE_FLAGS}\n"
            "lib_LINK_FLAGS=${lib_LINK_FLAGS}\n"
            "lib_OBJLIB_DEPS=${lib_OBJLIB_DEPS}\n"
            "lib_LIB_DEPS=${lib_LIB_DEPS}\n"
            "lib_PUBLIC_INCLUDES=${lib_PUBLIC_INCLUDES}\n"
            "---------------------------------------\n")
  endif()

  if(NOT (lib_NAME AND lib_TYPE AND lib_SOURCES))
    message(FATAL_ERROR "libgav1_add_library: NAME, TYPE and SOURCES required.")
  endif()

  list(APPEND libgav1_targets ${lib_NAME})
  if(lib_TEST)
    list(APPEND libgav1_test_targets ${lib_NAME})
    list(APPEND libgav1_test_sources ${lib_SOURCES})
  else()
    list(APPEND libgav1_sources ${lib_SOURCES})
    if(lib_TYPE STREQUAL OBJECT)
      list(APPEND libgav1_objlib_targets ${lib_NAME})
    elseif(lib_TYPE STREQUAL SHARED)
      list(APPEND libgav1_dylib_targets ${lib_NAME})
    elseif(lib_TYPE STREQUAL STATIC)
      list(APPEND libgav1_lib_targets ${lib_NAME})
    else()
      message(WARNING "libgav1_add_library: Unhandled type: ${lib_TYPE}")
    endif()
  endif()

  add_library(${lib_NAME} ${lib_TYPE} ${lib_SOURCES})
  libgav1_process_intrinsics_sources(TARGET ${lib_NAME} SOURCES ${lib_SOURCES})

  if(lib_OUTPUT_NAME)
    if(NOT (BUILD_SHARED_LIBS AND (MSVC OR WIN32)))
      set_target_properties(${lib_NAME}
                            PROPERTIES OUTPUT_NAME ${lib_OUTPUT_NAME})
    endif()
  endif()

  if(lib_DEFINES)
    target_compile_definitions(${lib_NAME} PRIVATE ${lib_DEFINES})
  endif()

  if(lib_INCLUDES)
    target_include_directories(${lib_NAME} PRIVATE ${lib_INCLUDES})
  endif()

  if(lib_PUBLIC_INCLUDES)
    target_include_directories(${lib_NAME} PUBLIC ${lib_PUBLIC_INCLUDES})
  endif()

  if(lib_COMPILE_FLAGS OR LIBGAV1_CXX_FLAGS)
    target_compile_options(${lib_NAME}
                           PRIVATE ${lib_COMPILE_FLAGS} ${LIBGAV1_CXX_FLAGS})
  endif()

  if(lib_LINK_FLAGS)
    set_target_properties(${lib_NAME} PROPERTIES LINK_FLAGS ${lib_LINK_FLAGS})
  endif()

  if(lib_OBJLIB_DEPS)
    foreach(objlib_dep ${lib_OBJLIB_DEPS})
      target_sources(${lib_NAME} PRIVATE $<TARGET_OBJECTS:${objlib_dep}>)
    endforeach()
  endif()

  if(lib_LIB_DEPS)
    if(lib_TYPE STREQUAL STATIC)
      set(link_type PUBLIC)
    else()
      set(link_type PRIVATE)
      if(lib_TYPE STREQUAL SHARED AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        # The libgav1 shared object uses the static libgav1 as input to turn it
        # into a shared object. Include everything from the static library in
        # the shared object.
        if(APPLE)
          list(INSERT lib_LIB_DEPS 0 -Wl,-force_load)
        else()
          list(INSERT lib_LIB_DEPS 0 -Wl,--whole-archive)
          list(APPEND lib_LIB_DEPS -Wl,--no-whole-archive)
        endif()
      endif()
    endif()
    target_link_libraries(${lib_NAME} ${link_type} ${lib_LIB_DEPS})
  endif()

  if(NOT MSVC AND lib_NAME MATCHES "^lib")
    # Non-MSVC generators prepend lib to static lib target file names. Libgav1
    # already includes lib in its name. Avoid naming output files liblib*.
    set_target_properties(${lib_NAME} PROPERTIES PREFIX "")
  endif()

  if(lib_TYPE STREQUAL SHARED AND NOT MSVC)
    set_target_properties(${lib_NAME}
                          PROPERTIES VERSION ${LIBGAV1_SOVERSION} SOVERSION
                                     ${LIBGAV1_SOVERSION_MAJOR})
  endif()

  if(BUILD_SHARED_LIBS AND (MSVC OR WIN32))
    if(lib_TYPE STREQUAL SHARED)
      target_compile_definitions(${lib_NAME} PRIVATE "LIBGAV1_BUILDING_DLL=1")
    else()
      target_compile_definitions(${lib_NAME} PRIVATE "LIBGAV1_BUILDING_DLL=0")
    endif()
  endif()

  # Determine if $lib_NAME is a header only target.
  set(sources_list ${lib_SOURCES})
  list(FILTER sources_list INCLUDE REGEX cc$)
  if(NOT sources_list)
    if(NOT XCODE)
      # This is a header only target. Tell CMake the link language.
      set_target_properties(${lib_NAME} PROPERTIES LINKER_LANGUAGE CXX)
    else()
      # The Xcode generator ignores LINKER_LANGUAGE. Add a dummy cc file.
      libgav1_create_dummy_source_file(TARGET ${lib_NAME} BASENAME ${lib_NAME})
    endif()
  endif()

  if(lib_TEST)
    set_property(TARGET ${lib_NAME} PROPERTY FOLDER ${LIBGAV1_TESTS_IDE_FOLDER})
  else()
    set(sources_list ${lib_SOURCES})
    list(FILTER sources_list INCLUDE REGEX examples)
    if(sources_list)
      set_property(TARGET ${lib_NAME}
                   PROPERTY FOLDER ${LIBGAV1_EXAMPLES_IDE_FOLDER})
    else()
      set_property(TARGET ${lib_NAME} PROPERTY FOLDER ${LIBGAV1_IDE_FOLDER})
    endif()
  endif()
endmacro()
