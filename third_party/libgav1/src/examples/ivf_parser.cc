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

#include "examples/ivf_parser.h"

#include <cstdio>
#include <cstring>

#include "examples/file_reader_constants.h"
#include "examples/logging.h"

namespace libgav1 {
namespace {

size_t ReadLittleEndian16(const uint8_t* const buffer) {
  size_t value = buffer[1] << 8;
  value |= buffer[0];
  return value;
}

size_t ReadLittleEndian32(const uint8_t* const buffer) {
  size_t value = buffer[3] << 24;
  value |= buffer[2] << 16;
  value |= buffer[1] << 8;
  value |= buffer[0];
  return value;
}

}  // namespace

bool ParseIvfFileHeader(const uint8_t* const header_buffer,
                        IvfFileHeader* const ivf_file_header) {
  if (header_buffer == nullptr || ivf_file_header == nullptr) return false;

  if (memcmp(kIvfSignature, header_buffer, 4) != 0) {
    return false;
  }

  // Verify header version and length.
  const size_t ivf_header_version = ReadLittleEndian16(&header_buffer[4]);
  if (ivf_header_version != kIvfHeaderVersion) {
    LIBGAV1_EXAMPLES_LOG_ERROR("Unexpected IVF version");
  }

  const size_t ivf_header_size = ReadLittleEndian16(&header_buffer[6]);
  if (ivf_header_size != kIvfFileHeaderSize) {
    LIBGAV1_EXAMPLES_LOG_ERROR("Invalid IVF file header size");
    return false;
  }

  if (memcmp(kAv1FourCcLower, &header_buffer[8], 4) != 0 &&
      memcmp(kAv1FourCcUpper, &header_buffer[8], 4) != 0) {
    LIBGAV1_EXAMPLES_LOG_ERROR("Unsupported codec 4CC");
    return false;
  }

  ivf_file_header->width = ReadLittleEndian16(&header_buffer[12]);
  ivf_file_header->height = ReadLittleEndian16(&header_buffer[14]);
  ivf_file_header->frame_rate_numerator =
      ReadLittleEndian32(&header_buffer[16]);
  ivf_file_header->frame_rate_denominator =
      ReadLittleEndian32(&header_buffer[20]);

  return true;
}

bool ParseIvfFrameHeader(const uint8_t* const header_buffer,
                         IvfFrameHeader* const ivf_frame_header) {
  if (header_buffer == nullptr || ivf_frame_header == nullptr) return false;

  ivf_frame_header->frame_size = ReadLittleEndian32(header_buffer);
  if (ivf_frame_header->frame_size > kMaxTemporalUnitSize) {
    LIBGAV1_EXAMPLES_LOG_ERROR("Temporal Unit size exceeds maximum");
    return false;
  }

  ivf_frame_header->timestamp = ReadLittleEndian32(&header_buffer[4]);
  const uint64_t timestamp_hi =
      static_cast<uint64_t>(ReadLittleEndian32(&header_buffer[8])) << 32;
  ivf_frame_header->timestamp |= timestamp_hi;

  return true;
}

}  // namespace libgav1
