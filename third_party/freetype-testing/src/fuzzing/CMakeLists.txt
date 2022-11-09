# CMakeLists.txt
#
# This script listens to a few environment variables:
#
#   CMAKE_DRIVER_EXE_NAME
#     (optional) Override the default name "driver".
#
#   CMAKE_FUZZING_ENGINE
#     If set:  set up everything to build the fuzzers.  The value is expected
#              to be a linkable library that contains the desired
#              implementation of the fuzzer.
#              For example: `-fsanitize=fuzzer'.
#     If not set:  set up everything to build the driver.
#
#   CMAKE_USE_LOGGER_GLOG
#     (optional) If set and `1', glog will be linked.
#
# Copyright 2018-2019 by
# Armin Hasitzka.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

cmake_minimum_required(VERSION 3.0.2)

project(fuzzing)

# ----------------------------------------------------------------------------
# defines:

set(FUZZING_BASE_DIR    "${CMAKE_CURRENT_SOURCE_DIR}")
set(FUZZING_SRC_DIR     "${FUZZING_BASE_DIR}/src")
set(FUZZING_CORPORA_DIR "${FUZZING_BASE_DIR}/corpora")

# `FUZZ_TARGET_NAMES', `FUZZ_TARGET_CLASS_NAMES', and
# `FUZZ_TARGET_HEADER_PATHS' are filled by `add_fuzz_target()'.

set(FUZZ_TARGET_NAMES        "legacy")
set(FUZZ_TARGET_CLASS_NAMES  "x")
set(FUZZ_TARGET_HEADER_PATHS "x")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(SUBMODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external")

set(FREETYPE_BASE_DIR       "${SUBMODULES_DIR}/freetype2")
set(FREETYPE_SRC_DIR        "${FREETYPE_BASE_DIR}/src")
set(FREETYPE_STATIC_LIBRARY "${FREETYPE_BASE_DIR}/objs/.libs/libfreetype.a")

set(GLOG_BASE_DIR       "${SUBMODULES_DIR}/glog")
set(GLOG_SRC_DIR        "${GLOG_BASE_DIR}/src")
set(GLOG_BUILD_DIR      "${GLOG_BASE_DIR}/build")
set(GLOG_STATIC_LIBRARY "${GLOG_BUILD_DIR}/libglog.a")

set(LIBARCHIVE_BASE_DIR       "${SUBMODULES_DIR}/libarchive")
set(LIBARCHIVE_SRC_DIR        "${LIBARCHIVE_BASE_DIR}/src")
set(LIBARCHIVE_STATIC_LIBRARY "${LIBARCHIVE_BASE_DIR}/.libs/libarchive.a")

set(BZIP2_BASE_DIR       "${SUBMODULES_DIR}/bzip2")
set(BZIP2_STATIC_LIBRARY "${BZIP2_BASE_DIR}/libbz2.a")

set(BROTLI_BASE_DIR  "${SUBMODULES_DIR}/brotli")
set(BROTLI_BUILD_DIR "${BROTLI_BASE_DIR}/build")
set(BROTLI_STATIC_LIBRARY
  "${BROTLI_BUILD_DIR}/libbrotlidec-static.a"
  "${BROTLI_BUILD_DIR}/libbrotlicommon-static.a")

set(ZLIB_BASE_DIR  "${SUBMODULES_DIR}/zlib")
set(ZLIB_STATIC_LIBRARY "${ZLIB_BASE_DIR}/usr/lib-asan/libz.a")

set(LIBPNG_BASE_DIR  "${SUBMODULES_DIR}/libpng")
set(LIBPNG_STATIC_LIBRARY "${LIBPNG_BASE_DIR}/usr/lib-asan/libpng.a")

set(LIBCXX_BASE_DIR  "${SUBMODULES_DIR}/llvm-project")
set(LIBCXX_BUILD_DIR "${LIBCXX_BASE_DIR}/build")
set(LIBCXX_STATIC_LIBRARY
  "${LIBCXX_BUILD_DIR}/lib/libc++.a"
  "${LIBCXX_BUILD_DIR}/lib/libc++abi.a")
add_compile_options(
  "-I${LIBCXX_BUILD_DIR}/include"
  "-I${LIBCXX_BUILD_DIR}/include/c++/v1"
  "-nostdinc++")
add_link_options("-nostdlib++")

# ----------------------------------------------------------------------------
# functions:

function(add_fuzz_target
    name
    class_name
    header_path)

  set(FUZZ_TARGET_NAMES
    ${FUZZ_TARGET_NAMES}
    "${name}"
    PARENT_SCOPE)

  set(FUZZ_TARGET_CLASS_NAMES
    ${FUZZ_TARGET_CLASS_NAMES}
    "${class_name}"
    PARENT_SCOPE)

  set(FUZZ_TARGET_HEADER_PATHS
    ${FUZZ_TARGET_HEADER_PATHS}
    "${header_path}"
    PARENT_SCOPE)

endfunction()

# ----------------------------------------------------------------------------
# fuzz targets:

add_fuzz_target(
  "bdf"
  "freetype::BdfFuzzTarget"
  "targets/font-drivers/bdf.h")
add_fuzz_target(
  "bdf-render"
  "freetype::BdfRenderFuzzTarget"
  "targets/font-drivers/bdf-render.h")

add_fuzz_target(
  "cff"
  "freetype::CffFuzzTarget"
  "targets/font-drivers/cff.h")
add_fuzz_target(
  "cff-ftengine"
  "freetype::CffFtEngineFuzzTarget"
  "targets/font-drivers/cff-ftengine.h")
add_fuzz_target(
  "cff-render"
  "freetype::CffRenderFuzzTarget"
  "targets/font-drivers/cff-render.h")
add_fuzz_target(
  "cff-render-ftengine"
  "freetype::CffRenderFtEngineFuzzTarget"
  "targets/font-drivers/cff-render-ftengine.h")

add_fuzz_target(
  "cidtype1"
  "freetype::CidType1FuzzTarget"
  "targets/font-drivers/cidtype1.h")
add_fuzz_target(
  "cidtype1-ftengine"
  "freetype::CidType1FtEngineFuzzTarget"
  "targets/font-drivers/cidtype1-ftengine.h")
add_fuzz_target(
  "cidtype1-render"
  "freetype::CidType1RenderFuzzTarget"
  "targets/font-drivers/cidtype1-render.h")
add_fuzz_target(
  "cidtype1-render-ftengine"
  "freetype::CidType1RenderFtEngineFuzzTarget"
  "targets/font-drivers/cidtype1-render-ftengine.h")

add_fuzz_target(
  "pcf"
  "freetype::PcfFuzzTarget"
  "targets/font-drivers/pcf.h")
add_fuzz_target(
  "pcf-render"
  "freetype::PcfRenderFuzzTarget"
  "targets/font-drivers/pcf-render.h")

add_fuzz_target(
  "truetype"
  "freetype::TrueTypeFuzzTarget"
  "targets/font-drivers/truetype.h")
add_fuzz_target(
  "truetype-render"
  "freetype::TrueTypeRenderFuzzTarget"
  "targets/font-drivers/truetype-render.h")
add_fuzz_target(
  "truetype-render-i35"
  "freetype::TrueTypeRenderI35FuzzTarget"
  "targets/font-drivers/truetype-render-i35.h")
add_fuzz_target(
  "truetype-render-i38"
  "freetype::TrueTypeRenderI38FuzzTarget"
  "targets/font-drivers/truetype-render-i38.h")

add_fuzz_target(
  "type1"
  "freetype::Type1FuzzTarget"
  "targets/font-drivers/type1.h")
add_fuzz_target(
  "type1-ftengine"
  "freetype::Type1FtEngineFuzzTarget"
  "targets/font-drivers/type1-ftengine.h")
add_fuzz_target(
  "type1-render"
  "freetype::Type1RenderFuzzTarget"
  "targets/font-drivers/type1-render.h")
add_fuzz_target(
  "type1-render-ftengine"
  "freetype::Type1RenderFtEngineFuzzTarget"
  "targets/font-drivers/type1-render-ftengine.h")
add_fuzz_target(
  "type1-render-tar"
  "freetype::Type1RenderTarFuzzTarget"
  "targets/font-drivers/type1-render-tar.h")
add_fuzz_target(
  "type1-tar"
  "freetype::Type1TarFuzzTarget"
  "targets/font-drivers/type1-tar.h")

add_fuzz_target(
  "type42"
  "freetype::Type42FuzzTarget"
  "targets/font-drivers/type42.h")
add_fuzz_target(
  "type42-render"
  "freetype::Type42RenderFuzzTarget"
  "targets/font-drivers/type42-render.h")

add_fuzz_target(
  "windowsfnt"
  "freetype::WindowsFntFuzzTarget"
  "targets/font-drivers/windowsfnt.h")
add_fuzz_target(
  "windowsfnt-render"
  "freetype::WindowsFntRenderFuzzTarget"
  "targets/font-drivers/windowsfnt-render.h")

add_fuzz_target(
  "glyphs-bitmaps-pcf"
  "freetype::GlyphsBitmapsPcfFuzzTarget"
  "targets/glyphs/bitmaps-pcf.h")
add_fuzz_target(
  "glyphs-outlines"
  "freetype::GlyphsOutlinesFuzzTarget"
  "targets/glyphs/outlines.h")

add_fuzz_target(
  "gzip"
  "freetype::GzipFuzzTarget"
  "targets/support/GzipFuzzTarget.h")
add_fuzz_target(
  "lzw"
  "freetype::LzwFuzzTarget"
  "targets/support/LzwFuzzTarget.h")
add_fuzz_target(
  "bzip2"
  "freetype::Bzip2FuzzTarget"
  "targets/support/Bzip2FuzzTarget.h")

add_fuzz_target(
  "colrv1"
  "freetype::ColrV1FuzzTarget"
  "targets/font-drivers/colrv1.h")

# ----------------------------------------------------------------------------
# environment options:

set(FUZZING_ENGINE "$ENV{CMAKE_FUZZING_ENGINE}")

set(DRIVER_EXE_NAME "driver")
if(NOT "$ENV{CMAKE_DRIVER_EXE_NAME}" STREQUAL "")
  set(DRIVER_EXE_NAME "$ENV{CMAKE_DRIVER_EXE_NAME}")
endif()

if("$ENV{CMAKE_USE_LOGGER_GLOG}")
  set(LOGGER_NAME "LOGGER_GLOG")
endif()

# ----------------------------------------------------------------------------
# build logic:

include_directories(
  "${FREETYPE_BASE_DIR}/include"
  "${FUZZING_SRC_DIR}"
  "${LIBARCHIVE_BASE_DIR}/libarchive")

if("${LOGGER_NAME}" STREQUAL "LOGGER_GLOG")
  include_directories(
    "${GLOG_BUILD_DIR}"
    "${GLOG_SRC_DIR}")
endif()

enable_testing()

add_subdirectory("${FUZZING_SRC_DIR}")
