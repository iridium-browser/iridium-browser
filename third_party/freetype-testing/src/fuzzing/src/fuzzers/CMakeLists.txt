# CMakeLists.txt
#
# Copyright 2019 by
# Armin Hasitzka.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

# Note: OSS-Fuzz does not allow shared objects.  Thus we have to link the fuzz
# targets as statically as possible.  Bear that in mind when linking
# additional libraries.

add_executable("fuzzer-legacy"
  "${FUZZING_SRC_DIR}/legacy/ftfuzzer.cc")

list(LENGTH FUZZ_TARGET_NAMES num_fuzz_targets)
math(EXPR num_fuzz_targets "${num_fuzz_targets} - 1")

foreach(fuzz_target_index RANGE "${num_fuzz_targets}")

  list(GET FUZZ_TARGET_NAMES        "${fuzz_target_index}" name)
  list(GET FUZZ_TARGET_CLASS_NAMES  "${fuzz_target_index}" class_name)
  list(GET FUZZ_TARGET_HEADER_PATHS "${fuzz_target_index}" header_path)

  set(exe "fuzzer-${name}")

  if(NOT "${name}" STREQUAL "legacy")
    
    add_executable("${exe}" "template.cpp")

    target_compile_definitions("${exe}" PUBLIC
      "FUZZ_TARGET_HEADER_PATH=\"${header_path}\""
      "FUZZ_TARGET_CLASS_NAME=${class_name}")

    target_link_libraries("${exe}"
      PRIVATE
      "fuzztargets")

  endif()

  target_link_libraries("${exe}"
    PRIVATE
    "${LIBARCHIVE_STATIC_LIBRARY}"
    "${FREETYPE_STATIC_LIBRARY}"
    "${BZIP2_STATIC_LIBRARY}"
    "${BROTLI_STATIC_LIBRARY}"
    "${LIBPNG_STATIC_LIBRARY}"
    "${ZLIB_STATIC_LIBRARY}"
    "${LIBCXX_STATIC_LIBRARY}")

  # `-fsanitize=fuzzer' or `-lFuzzingEngine' cannot be set earlier since CMake
  # defines a main function when testing the compiler.  Clang (obviously)
  # fails when being confronted with two main functions.

  set_target_properties("${exe}" PROPERTIES
    OUTPUT_NAME "${name}"
    LINK_FLAGS  "${FUZZING_ENGINE}")

endforeach()
