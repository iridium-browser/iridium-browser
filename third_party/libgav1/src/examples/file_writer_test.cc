// Copyright 2021 The libgav1 Authors
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

#include "examples/file_writer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include "absl/memory/memory.h"
#include "gav1/decoder_buffer.h"
#include "gtest/gtest.h"
#include "tests/utils.h"

namespace libgav1 {
namespace {

const char kExpectedY4mHeader8bit[] = "YUV4MPEG2 W352 H288 F30:1 Ip C420jpeg\n";
const char kExpectedY4mHeader10bit[] = "YUV4MPEG2 W352 H288 F30:1 Ip C420p10\n";
const char kExpectedY4mHeader8bitMonochrome[] =
    "YUV4MPEG2 W352 H288 F30:1 Ip Cmono\n";
const char kExpectedY4mHeader10bitMonochrome[] =
    "YUV4MPEG2 W352 H288 F30:1 Ip Cmono10\n";

// Note: These are non-const because DecoderBuffer.plane is non-const.
char fake_plane0[] = "PLANE0\n";
char fake_plane1[] = "PLANE1\n";
char fake_plane2[] = "PLANE2\n";

constexpr size_t kExpectedRawDataBufferCount = 3;
const char* kExpectedRawData[kExpectedRawDataBufferCount] = {
    fake_plane0, fake_plane1, fake_plane2};

const char* const kExpectedRawDataMonochrome = fake_plane0;

constexpr size_t kExpectedY4mDataBufferCount = 5;
const char* const kExpectedY4mFileData8bit[kExpectedY4mDataBufferCount] = {
    kExpectedY4mHeader8bit, "FRAME\n", fake_plane0, fake_plane1, fake_plane2};
const char* const kExpectedY4mFileData10bit[kExpectedY4mDataBufferCount] = {
    kExpectedY4mHeader10bit, "FRAME\n", fake_plane0, fake_plane1, fake_plane2};

constexpr size_t kExpectedY4mDataBufferCountMonochrome = 3;
const char* const
    kExpectedY4mFileData8bitMonochrome[kExpectedY4mDataBufferCountMonochrome] =
        {kExpectedY4mHeader8bitMonochrome, "FRAME\n", fake_plane0};
const char* const
    kExpectedY4mFileData10bitMonochrome[kExpectedY4mDataBufferCountMonochrome] =
        {kExpectedY4mHeader10bitMonochrome, "FRAME\n", fake_plane0};

// TODO(tomfinegan): Add a bitdepth arg, and test writing 10 bit frame buffers.
std::unique_ptr<DecoderBuffer> GetFakeDecoderBuffer(ImageFormat image_format) {
  auto buffer = absl::make_unique<DecoderBuffer>();
  if (buffer == nullptr) return nullptr;
  buffer->chroma_sample_position = kChromaSamplePositionUnknown;
  buffer->image_format = image_format;
  buffer->bitdepth = 8;
  buffer->displayed_width[0] = static_cast<int>(strlen(fake_plane0));
  buffer->displayed_width[1] = static_cast<int>(strlen(fake_plane1));
  buffer->displayed_width[2] = static_cast<int>(strlen(fake_plane2));
  buffer->displayed_height[0] = 1;
  buffer->displayed_height[1] = 1;
  buffer->displayed_height[2] = 1;
  buffer->stride[0] = static_cast<int>(strlen(fake_plane0));
  buffer->stride[1] = static_cast<int>(strlen(fake_plane1));
  buffer->stride[2] = static_cast<int>(strlen(fake_plane2));
  buffer->plane[0] = reinterpret_cast<uint8_t*>(fake_plane0);
  buffer->plane[1] = reinterpret_cast<uint8_t*>(fake_plane1);
  buffer->plane[2] = reinterpret_cast<uint8_t*>(fake_plane2);
  buffer->user_private_data = 0;
  buffer->buffer_private_data = nullptr;
  return buffer;
}

TEST(FileWriterTest, FailOpen) {
  EXPECT_EQ(FileWriter::Open(test_utils::GetTestOutputFilePath("fail_open"),
                             static_cast<FileWriter::FileType>(3), nullptr),
            nullptr);
  EXPECT_EQ(FileWriter::Open(test_utils::GetTestOutputFilePath("fail_open"),
                             FileWriter::kFileTypeY4m, nullptr),
            nullptr);
}

struct FileWriterY4mHeaderTestParameters {
  FileWriterY4mHeaderTestParameters() = default;
  FileWriterY4mHeaderTestParameters(const FileWriterY4mHeaderTestParameters&) =
      default;
  FileWriterY4mHeaderTestParameters& operator=(
      const FileWriterY4mHeaderTestParameters&) = default;
  FileWriterY4mHeaderTestParameters(FileWriterY4mHeaderTestParameters&&) =
      default;
  FileWriterY4mHeaderTestParameters& operator=(
      FileWriterY4mHeaderTestParameters&&) = default;
  ~FileWriterY4mHeaderTestParameters() = default;

  FileWriterY4mHeaderTestParameters(std::string file_name,
                                    ChromaSamplePosition chroma_sample_position,
                                    ImageFormat image_format, int bitdepth,
                                    const char* expected_header_string)
      : file_name(std::move(file_name)),
        chroma_sample_position(chroma_sample_position),
        image_format(image_format),
        bitdepth(bitdepth),
        expected_header_string(expected_header_string) {}
  std::string file_name;
  ChromaSamplePosition chroma_sample_position = kChromaSamplePositionUnknown;
  ImageFormat image_format = kImageFormatMonochrome400;
  int bitdepth = 8;
  const char* expected_header_string = nullptr;
};

std::ostream& operator<<(std::ostream& stream,
                         const FileWriterY4mHeaderTestParameters& parameters) {
  stream << "file_name=" << parameters.file_name << "\n"
         << "chroma_sample_position=" << parameters.chroma_sample_position
         << "\n"
         << "image_format=" << parameters.image_format << "\n"
         << "bitdepth=" << parameters.bitdepth << "\n"
         << "expected_header_string=" << parameters.expected_header_string
         << "\n";
  return stream;
}

class FileWriterY4mHeaderTest
    : public testing::TestWithParam<FileWriterY4mHeaderTestParameters> {
 public:
  FileWriterY4mHeaderTest() {
    test_parameters_ = GetParam();
    y4m_parameters_.width = 352;
    y4m_parameters_.height = 288;
    y4m_parameters_.frame_rate_numerator = 30;
    y4m_parameters_.frame_rate_denominator = 1;
    y4m_parameters_.chroma_sample_position =
        test_parameters_.chroma_sample_position;
    y4m_parameters_.image_format = test_parameters_.image_format;
    y4m_parameters_.bitdepth = test_parameters_.bitdepth;
  }
  FileWriterY4mHeaderTest(const FileWriterY4mHeaderTest&) = delete;
  FileWriterY4mHeaderTest& operator=(const FileWriterY4mHeaderTest&) = delete;
  ~FileWriterY4mHeaderTest() override = default;

 protected:
  FileWriterY4mHeaderTestParameters test_parameters_;
  FileWriter::Y4mParameters y4m_parameters_;
};

TEST_P(FileWriterY4mHeaderTest, WriteY4mHeader) {
  const std::string file_name =
      test_utils::GetTestOutputFilePath(test_parameters_.file_name);
  EXPECT_NE(
      FileWriter::Open(file_name, FileWriter::kFileTypeY4m, &y4m_parameters_),
      nullptr);
  std::string y4m_header_string;
  test_utils::GetTestData(test_parameters_.file_name, true, &y4m_header_string);
  EXPECT_STREQ(y4m_header_string.c_str(),
               test_parameters_.expected_header_string);
}

INSTANTIATE_TEST_SUITE_P(
    WriteY4mHeader, FileWriterY4mHeaderTest,
    testing::Values(
        FileWriterY4mHeaderTestParameters(
            "y4m_header_8bit", kChromaSamplePositionUnknown, kImageFormatYuv420,
            /*bitdepth=*/8, kExpectedY4mHeader8bit),
        FileWriterY4mHeaderTestParameters("y4m_header_10bit",
                                          kChromaSamplePositionUnknown,
                                          kImageFormatYuv420, /*bitdepth=*/10,
                                          kExpectedY4mHeader10bit),
        FileWriterY4mHeaderTestParameters("y4m_header_8bit_monochrome",
                                          kChromaSamplePositionUnknown,
                                          kImageFormatMonochrome400,
                                          /*bitdepth=*/8,
                                          kExpectedY4mHeader8bitMonochrome),
        FileWriterY4mHeaderTestParameters("y4m_header_10bit_monochrome",
                                          kChromaSamplePositionUnknown,
                                          kImageFormatMonochrome400,
                                          /*bitdepth=*/10,
                                          kExpectedY4mHeader10bitMonochrome)));

struct FileWriterTestParameters {
  FileWriterTestParameters() = default;
  FileWriterTestParameters(const FileWriterTestParameters&) = default;
  FileWriterTestParameters& operator=(const FileWriterTestParameters&) =
      default;
  FileWriterTestParameters(FileWriterTestParameters&&) = default;
  FileWriterTestParameters& operator=(FileWriterTestParameters&&) = default;
  ~FileWriterTestParameters() = default;

  FileWriterTestParameters(std::string file_name,
                           FileWriter::FileType file_type,
                           const FileWriter::Y4mParameters* y4m_parameters,
                           size_t num_frames)
      : file_name(std::move(file_name)),
        file_type(file_type),
        y4m_parameters(y4m_parameters),
        num_frames(num_frames) {}
  std::string file_name;
  FileWriter::FileType file_type = FileWriter::kFileTypeRaw;
  const FileWriter::Y4mParameters* y4m_parameters = nullptr;
  size_t num_frames = 1;
};

std::ostream& operator<<(std::ostream& stream,
                         const ChromaSamplePosition& position) {
  switch (position) {
    case kChromaSamplePositionUnknown:
      stream << "kCromaSamplePositionUnknown";
      break;
    case kChromaSamplePositionVertical:
      stream << "kChromaSamplePositionVertical";
      break;
    case kChromaSamplePositionColocated:
      stream << "kChromaSamplePositionColocated";
      break;
    case kChromaSamplePositionReserved:
      stream << "kChromaSamplePositionReserved";
      break;
  }
  return stream;
}

std::ostream& operator<<(std::ostream& stream,
                         const ImageFormat& image_format) {
  switch (image_format) {
    case kImageFormatMonochrome400:
      stream << "kImageFormatMonochrome400";
      break;
    case kImageFormatYuv420:
      stream << "kImageFormatYuv420";
      break;
    case kImageFormatYuv422:
      stream << "kImageFormatYuv422";
      break;
    case kImageFormatYuv444:
      stream << "kImageFormatYuv444";
      break;
  }
  return stream;
}

std::ostream& operator<<(std::ostream& stream,
                         const FileWriter::Y4mParameters& parameters) {
  stream << "y4m_parameters:\n"
         << "  width=" << parameters.width << "\n"
         << "  height=" << parameters.height << "\n"
         << "  frame_rate_numerator=" << parameters.frame_rate_numerator << "\n"
         << "  frame_rate_denominator=" << parameters.frame_rate_denominator
         << "\n"
         << "  chroma_sample_position=" << parameters.chroma_sample_position
         << "\n"
         << "  image_format=" << parameters.image_format << "\n"
         << "  bitdepth=" << parameters.bitdepth << "\n";

  return stream;
}

std::ostream& operator<<(std::ostream& stream,
                         const FileWriterTestParameters& parameters) {
  stream << "file_name=" << parameters.file_name << "\n"
         << "file_type="
         << (parameters.file_type == FileWriter::kFileTypeRaw ? "kFileTypeRaw"
                                                              : "kFileTypeY4m")
         << "\n";
  if (parameters.y4m_parameters != nullptr) {
    stream << *parameters.y4m_parameters;
  } else {
    stream << "y4m_parameters: <nullptr>\n";
  }
  stream << "num_frames=" << parameters.num_frames << "\n";
  return stream;
}

class FileWriterTestBase
    : public testing::TestWithParam<FileWriterTestParameters> {
 public:
  FileWriterTestBase() = default;
  FileWriterTestBase(const FileWriterTestBase&) = delete;
  FileWriterTestBase& operator=(const FileWriterTestBase&) = delete;
  ~FileWriterTestBase() override = default;

 protected:
  void SetUp() override { OpenWriter(GetParam()); }

  void OpenWriter(const FileWriterTestParameters& parameters) {
    parameters_ = parameters;
    parameters_.file_name = parameters.file_name;
    file_writer_ = FileWriter::Open(
        test_utils::GetTestOutputFilePath(parameters.file_name),
        parameters_.file_type, parameters_.y4m_parameters);
    ASSERT_NE(file_writer_, nullptr);
  }

  void WriteFramesAndCloseFile() {
    if (parameters_.y4m_parameters != nullptr) {
      image_format_ = parameters_.y4m_parameters->image_format;
    }
    decoder_buffer_ = GetFakeDecoderBuffer(image_format_);
    for (size_t frame_num = 0; frame_num < parameters_.num_frames;
         ++frame_num) {
      ASSERT_TRUE(file_writer_->WriteFrame(*decoder_buffer_));
    }
    file_writer_ = nullptr;
  }

  ImageFormat image_format_ = kImageFormatYuv420;
  FileWriterTestParameters parameters_;
  std::unique_ptr<FileWriter> file_writer_;
  std::unique_ptr<DecoderBuffer> decoder_buffer_;
};

class FileWriterTestRaw : public FileWriterTestBase {
 public:
  FileWriterTestRaw() = default;
  FileWriterTestRaw(const FileWriterTestRaw&) = delete;
  FileWriterTestRaw& operator=(const FileWriterTestRaw&) = delete;
  ~FileWriterTestRaw() override = default;

 protected:
  void SetUp() override { FileWriterTestBase::SetUp(); }
};

class FileWriterTestY4m : public FileWriterTestBase {
 public:
  FileWriterTestY4m() = default;
  FileWriterTestY4m(const FileWriterTestY4m&) = delete;
  FileWriterTestY4m& operator=(const FileWriterTestY4m&) = delete;
  ~FileWriterTestY4m() override = default;

 protected:
  void SetUp() override { FileWriterTestBase::SetUp(); }
};

TEST_P(FileWriterTestRaw, WriteRawFrames) {
  WriteFramesAndCloseFile();

  std::string actual_file_data;
  test_utils::GetTestData(parameters_.file_name, true, &actual_file_data);

  std::string expected_file_data;
  for (size_t frame_num = 0; frame_num < parameters_.num_frames; ++frame_num) {
    if (image_format_ == kImageFormatMonochrome400) {
      expected_file_data += kExpectedRawDataMonochrome;
    } else {
      for (const auto& buffer : kExpectedRawData) {
        expected_file_data += buffer;
      }
    }
  }

  ASSERT_EQ(actual_file_data, expected_file_data);
}

TEST_P(FileWriterTestY4m, WriteY4mFrames) {
  WriteFramesAndCloseFile();

  std::string actual_file_data;
  test_utils::GetTestData(parameters_.file_name, true, &actual_file_data);

  std::string expected_file_data;
  for (size_t frame_num = 0; frame_num < parameters_.num_frames; ++frame_num) {
    if (image_format_ == kImageFormatMonochrome400) {
      const char* const* expected_data_planes =
          (parameters_.y4m_parameters->bitdepth == 8)
              ? kExpectedY4mFileData8bitMonochrome
              : kExpectedY4mFileData10bitMonochrome;
      // Skip the Y4M file header "plane" after frame 0.
      for (size_t buffer_num = (frame_num == 0) ? 0 : 1;
           buffer_num < kExpectedY4mDataBufferCountMonochrome; ++buffer_num) {
        expected_file_data += expected_data_planes[buffer_num];
      }
    } else {
      const char* const* expected_data_planes =
          (parameters_.y4m_parameters->bitdepth == 8)
              ? kExpectedY4mFileData8bit
              : kExpectedY4mFileData10bit;

      // Skip the Y4M file header "plane" after frame 0.
      for (size_t buffer_num = (frame_num == 0) ? 0 : 1;
           buffer_num < kExpectedY4mDataBufferCount; ++buffer_num) {
        expected_file_data += expected_data_planes[buffer_num];
      }
    }
  }

  ASSERT_EQ(actual_file_data, expected_file_data);
}

INSTANTIATE_TEST_SUITE_P(
    WriteRawFrames, FileWriterTestRaw,
    testing::Values(
        FileWriterTestParameters("raw_frames_test_1frame",
                                 FileWriter::kFileTypeRaw,
                                 /*y4m_parameters=*/nullptr,
                                 /*num_frames=*/1),
        FileWriterTestParameters("raw_frames_test_5frames",
                                 FileWriter::kFileTypeRaw,
                                 /*y4m_parameters=*/nullptr,
                                 /*num_frames=*/5),
        FileWriterTestParameters("raw_frames_test_1frame_monochrome",
                                 FileWriter::kFileTypeRaw,
                                 /*y4m_parameters=*/nullptr,
                                 /*num_frames=*/1),
        FileWriterTestParameters("raw_frames_test_5frames_monochrome",
                                 FileWriter::kFileTypeRaw,
                                 /*y4m_parameters=*/nullptr,
                                 /*num_frames=*/5)));

const FileWriter::Y4mParameters kY4mParameters8Bit = {
    352,  // width
    288,  // height
    30,   // frame_rate_numerator
    1,    // frame_rate_denominator
    kChromaSamplePositionUnknown,
    kImageFormatYuv420,
    8  // bitdepth
};

const FileWriter::Y4mParameters kY4mParameters10Bit = {
    352,  // width
    288,  // height
    30,   // frame_rate_numerator
    1,    // frame_rate_denominator
    kChromaSamplePositionUnknown,
    kImageFormatYuv420,
    10  // bitdepth
};

const FileWriter::Y4mParameters kY4mParameters8BitMonochrome = {
    352,  // width
    288,  // height
    30,   // frame_rate_numerator
    1,    // frame_rate_denominator
    kChromaSamplePositionUnknown,
    kImageFormatMonochrome400,
    8  // bitdepth
};

const FileWriter::Y4mParameters kY4mParameters10BitMonochrome = {
    352,  // width
    288,  // height
    30,   // frame_rate_numerator
    1,    // frame_rate_denominator
    kChromaSamplePositionUnknown,
    kImageFormatMonochrome400,
    10  // bitdepth
};

INSTANTIATE_TEST_SUITE_P(
    WriteY4mFrames, FileWriterTestY4m,
    testing::Values(
        FileWriterTestParameters("y4m_frames_test_8bit_1frame",
                                 FileWriter::kFileTypeY4m, &kY4mParameters8Bit,
                                 /*num_frames=*/1),
        FileWriterTestParameters("y4m_frames_test_8bit_5frames",
                                 FileWriter::kFileTypeY4m, &kY4mParameters8Bit,
                                 /*num_frames=*/5),
        FileWriterTestParameters("y4m_frames_test_10bit_1frame",
                                 FileWriter::kFileTypeY4m, &kY4mParameters10Bit,
                                 /*num_frames=*/1),
        FileWriterTestParameters("y4m_frames_test_10bit_5frames",
                                 FileWriter::kFileTypeY4m, &kY4mParameters10Bit,
                                 /*num_frames=*/5),
        FileWriterTestParameters("y4m_frames_test_8bit_1frame_monochrome",
                                 FileWriter::kFileTypeY4m,
                                 &kY4mParameters8BitMonochrome,
                                 /*num_frames=*/1),
        FileWriterTestParameters("y4m_frames_test_8bit_5frames_monochrome",
                                 FileWriter::kFileTypeY4m,
                                 &kY4mParameters8BitMonochrome,
                                 /*num_frames=*/5),
        FileWriterTestParameters("y4m_frames_test_10bit_1frame_monochrome",
                                 FileWriter::kFileTypeY4m,
                                 &kY4mParameters10BitMonochrome,
                                 /*num_frames=*/1),
        FileWriterTestParameters("y4m_frames_test_10bit_5frames_monochrome",
                                 FileWriter::kFileTypeY4m,
                                 &kY4mParameters10BitMonochrome,
                                 /*num_frames=*/5)));

}  // namespace
}  // namespace libgav1
