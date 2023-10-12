// Copyright 2019 The libgav1 Authors
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

#include "examples/file_reader.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <new>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif

#include "examples/file_reader_constants.h"
#include "examples/file_reader_factory.h"
#include "examples/file_reader_interface.h"
#include "examples/ivf_parser.h"
#include "examples/logging.h"

namespace libgav1 {
namespace {

FILE* SetBinaryMode(FILE* stream) {
#if defined(_WIN32)
  _setmode(_fileno(stream), _O_BINARY);
#endif
  return stream;
}

}  // namespace

bool FileReader::registered_in_factory_ =
    FileReaderFactory::RegisterReader(FileReader::Open);

FileReader::~FileReader() {
  if (owns_file_) fclose(file_);
}

std::unique_ptr<FileReaderInterface> FileReader::Open(
    const std::string& file_name, const bool error_tolerant) {
  if (file_name.empty()) return nullptr;

  FILE* raw_file_ptr;

  bool owns_file = true;
  if (file_name == "-") {
    raw_file_ptr = SetBinaryMode(stdin);
    owns_file = false;  // stdin is owned by the Standard C Library.
  } else {
    raw_file_ptr = fopen(file_name.c_str(), "rb");
  }

  if (raw_file_ptr == nullptr) {
    return nullptr;
  }

  std::unique_ptr<FileReader> file(
      new (std::nothrow) FileReader(raw_file_ptr, owns_file, error_tolerant));
  if (file == nullptr) {
    LIBGAV1_EXAMPLES_LOG_ERROR("Out of memory");
    if (owns_file) fclose(raw_file_ptr);
    return nullptr;
  }

  if (!file->ReadIvfFileHeader()) {
    LIBGAV1_EXAMPLES_LOG_ERROR("Unsupported file type");
    return nullptr;
  }

  // With C++11, to return |file|, an explicit move is required as the return
  // type differs from the local variable. Overload resolution isn't guaranteed
  // in this case, though some compilers may adopt the C++14 behavior (C++
  // Standard Core Language Issue #1579, Return by converting move
  // constructor):
  // https://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1579
  // To keep things simple we opt for the following compatible form.
  return std::unique_ptr<FileReaderInterface>(file.release());
}

// IVF Frame Header format, from https://wiki.multimedia.cx/index.php/IVF
// bytes 0-3    size of frame in bytes (not including the 12-byte header)
// bytes 4-11   64-bit presentation timestamp
// bytes 12..   frame data
bool FileReader::ReadTemporalUnit(std::vector<uint8_t>* const tu_data,
                                  int64_t* const timestamp) {
  if (tu_data == nullptr) return false;
  tu_data->clear();

  uint8_t header_buffer[kIvfFrameHeaderSize];
  const size_t num_read = fread(header_buffer, 1, kIvfFrameHeaderSize, file_);

  if (IsEndOfFile()) {
    if (num_read != 0) {
      LIBGAV1_EXAMPLES_LOG_ERROR(
          "Cannot read IVF frame header: Not enough data available");
      return false;
    }

    return true;
  }

  IvfFrameHeader ivf_frame_header;
  if (!ParseIvfFrameHeader(header_buffer, &ivf_frame_header)) {
    LIBGAV1_EXAMPLES_LOG_ERROR("Could not parse IVF frame header");
    if (error_tolerant_) {
      ivf_frame_header.frame_size =
          std::min(ivf_frame_header.frame_size, size_t{kMaxTemporalUnitSize});
    } else {
      return false;
    }
  }

  if (timestamp != nullptr) *timestamp = ivf_frame_header.timestamp;

  tu_data->resize(ivf_frame_header.frame_size);
  const size_t size_read =
      fread(tu_data->data(), 1, ivf_frame_header.frame_size, file_);
  if (size_read != ivf_frame_header.frame_size) {
    LIBGAV1_EXAMPLES_LOG_ERROR(
        "Unexpected EOF or I/O error reading frame data");
    if (error_tolerant_) {
      tu_data->resize(size_read);
    } else {
      return false;
    }
  }
  return true;
}

// Attempt to read an IVF file header. Returns true for success, and false for
// failure.
//
// IVF File Header format, from https://wiki.multimedia.cx/index.php/IVF
// bytes 0-3    signature: 'DKIF'
// bytes 4-5    version (should be 0)
// bytes 6-7    length of header in bytes
// bytes 8-11   codec FourCC (e.g., 'VP80')
// bytes 12-13  width in pixels
// bytes 14-15  height in pixels
// bytes 16-19  frame rate
// bytes 20-23  time scale
// bytes 24-27  number of frames in file
// bytes 28-31  unused
//
// Note: The rate and scale fields correspond to the numerator and denominator
// of frame rate (fps) or time base (the reciprocal of frame rate) as follows:
//
// bytes 16-19  frame rate  timebase.den  framerate.numerator
// bytes 20-23  time scale  timebase.num  framerate.denominator
bool FileReader::ReadIvfFileHeader() {
  uint8_t header_buffer[kIvfFileHeaderSize];
  const size_t num_read = fread(header_buffer, 1, kIvfFileHeaderSize, file_);
  if (num_read != kIvfFileHeaderSize) {
    LIBGAV1_EXAMPLES_LOG_ERROR(
        "Cannot read IVF header: Not enough data available");
    return false;
  }

  IvfFileHeader ivf_file_header;
  if (!ParseIvfFileHeader(header_buffer, &ivf_file_header)) {
    LIBGAV1_EXAMPLES_LOG_ERROR("Could not parse IVF file header");
    if (error_tolerant_) {
      ivf_file_header = {};
    } else {
      return false;
    }
  }

  width_ = ivf_file_header.width;
  height_ = ivf_file_header.height;
  frame_rate_ = ivf_file_header.frame_rate_numerator;
  time_scale_ = ivf_file_header.frame_rate_denominator;
  type_ = kFileTypeIvf;

  return true;
}

}  // namespace libgav1
