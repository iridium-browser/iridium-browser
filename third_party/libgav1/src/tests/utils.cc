// Copyright 2020 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tests/utils.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/dsp.h"
#include "src/gav1/decoder_buffer.h"
#include "src/utils/constants.h"
#include "tests/third_party/libvpx/md5_helper.h"

namespace libgav1 {
namespace test_utils {
namespace {

int CloseFile(FILE* stream) { return fclose(stream); }

bool ReadFileToString(absl::string_view file_name, std::string* const string) {
  using FilePtr = std::unique_ptr<FILE, decltype(&CloseFile)>;
  FilePtr file(fopen(std::string(file_name).c_str(), "rb"), &CloseFile);
  if (file == nullptr) return false;

  do {
    int c = fgetc(file.get());
    if (ferror(file.get()) != 0) return false;

    if (c != EOF) {
      string->append(1, static_cast<char>(c));
    } else {
      break;
    }
  } while (true);

  return true;
}

}  // namespace

void ResetDspTable(const int bitdepth) {
  dsp::Dsp* const dsp = dsp_internal::GetWritableDspTable(bitdepth);
  ASSERT_NE(dsp, nullptr);
  memset(dsp, 0, sizeof(dsp::Dsp));
}

std::string GetMd5Sum(const void* bytes, size_t size) {
  libvpx_test::MD5 md5;
  md5.Add(static_cast<const uint8_t*>(bytes), size);
  return md5.Get();
}

template <typename Pixel>
std::string GetMd5Sum(const Pixel* block, int width, int height, int stride) {
  libvpx_test::MD5 md5;
  const Pixel* row = block;
  for (int i = 0; i < height; ++i) {
    md5.Add(reinterpret_cast<const uint8_t*>(row), width * sizeof(Pixel));
    row += stride;
  }
  return md5.Get();
}

template std::string GetMd5Sum(const int8_t* block, int width, int height,
                               int stride);
template std::string GetMd5Sum(const int16_t* block, int width, int height,
                               int stride);

std::string GetMd5Sum(const DecoderBuffer& buffer) {
  libvpx_test::MD5 md5;
  const size_t pixel_size =
      (buffer.bitdepth == 8) ? sizeof(uint8_t) : sizeof(uint16_t);
  for (int plane = kPlaneY; plane < buffer.NumPlanes(); ++plane) {
    const int height = buffer.displayed_height[plane];
    const size_t width = buffer.displayed_width[plane] * pixel_size;
    const int stride = buffer.stride[plane];
    const uint8_t* plane_buffer = buffer.plane[plane];
    for (int row = 0; row < height; ++row) {
      md5.Add(plane_buffer, width);
      plane_buffer += stride;
    }
  }
  return md5.Get();
}

void CheckMd5Digest(const char name[], const char function_name[],
                    const char expected_digest[], const void* data, size_t size,
                    absl::Duration elapsed_time) {
  const std::string digest = test_utils::GetMd5Sum(data, size);
  printf("Mode %s[%31s]: %5d us     MD5: %s\n", name, function_name,
         static_cast<int>(absl::ToInt64Microseconds(elapsed_time)),
         digest.c_str());
  EXPECT_STREQ(expected_digest, digest.c_str());
}

template <typename Pixel>
void CheckMd5Digest(const char name[], const char function_name[],
                    const char expected_digest[], const Pixel* block, int width,
                    int height, int stride, absl::Duration elapsed_time) {
  const std::string digest =
      test_utils::GetMd5Sum(block, width, height, stride);
  printf("Mode %s[%31s]: %5d us     MD5: %s\n", name, function_name,
         static_cast<int>(absl::ToInt64Microseconds(elapsed_time)),
         digest.c_str());
  EXPECT_STREQ(expected_digest, digest.c_str());
}

template void CheckMd5Digest(const char name[], const char function_name[],
                             const char expected_digest[], const int8_t* block,
                             int width, int height, int stride,
                             absl::Duration elapsed_time);
template void CheckMd5Digest(const char name[], const char function_name[],
                             const char expected_digest[], const int16_t* block,
                             int width, int height, int stride,
                             absl::Duration elapsed_time);

void CheckMd5Digest(const char name[], const char function_name[],
                    const char expected_digest[], const char actual_digest[],
                    absl::Duration elapsed_time) {
  printf("Mode %s[%31s]: %5d us     MD5: %s\n", name, function_name,
         static_cast<int>(absl::ToInt64Microseconds(elapsed_time)),
         actual_digest);
  EXPECT_STREQ(expected_digest, actual_digest);
}

namespace {

std::string GetSourceDir() {
#if defined(__ANDROID__)
  // Test files must be manually supplied. This path is frequently
  // available on development devices.
  return std::string("/data/local/tmp/tests/data");
#elif defined(LIBGAV1_FLAGS_SRCDIR)
  return std::string(LIBGAV1_FLAGS_SRCDIR) + "/tests/data";
#else
  return std::string(".");
#endif  // defined(__ANDROID__)
}

std::string GetTempDir() {
  const char* path = getenv("TMPDIR");
  if (path == nullptr || path[0] == '\0') path = getenv("TEMP");
  if (path != nullptr && path[0] != '\0') return std::string(path);

#if defined(__ANDROID__)
  return std::string("/data/local/tmp");
#elif defined(LIBGAV1_FLAGS_TMPDIR)
  return std::string(LIBGAV1_FLAGS_TMPDIR);
#else
  return std::string(".");
#endif  // defined(__ANDROID__)
}

}  // namespace

std::string GetTestInputFilePath(absl::string_view file_name) {
  const char* const path = getenv("LIBGAV1_TEST_DATA_PATH");
  if (path != nullptr && path[0] != '\0') {
    return std::string(path) + "/" + std::string(file_name);
  }
  return GetSourceDir() + "/" + std::string(file_name);
}

std::string GetTestOutputFilePath(absl::string_view file_name) {
  return GetTempDir() + "/" + std::string(file_name);
}

void GetTestData(absl::string_view file_name, const bool is_output_file,
                 std::string* const output) {
  ASSERT_NE(output, nullptr);
  const std::string absolute_file_path = is_output_file
                                             ? GetTestOutputFilePath(file_name)
                                             : GetTestInputFilePath(file_name);

  ASSERT_TRUE(ReadFileToString(absolute_file_path, output));
}

}  // namespace test_utils
}  // namespace libgav1
