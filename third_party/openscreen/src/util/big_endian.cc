// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/big_endian.h"

namespace openscreen {

BigEndianReader::BigEndianReader(const uint8_t* buffer, size_t length)
    : BigEndianBuffer(buffer, length) {}

bool BigEndianReader::Read(size_t length, void* out) {
  const uint8_t* read_position = current();
  if (Skip(length)) {
    memcpy(out, read_position, length);
    return true;
  }
  return false;
}

BigEndianWriter::BigEndianWriter(uint8_t* buffer, size_t length)
    : BigEndianBuffer(buffer, length) {}

bool BigEndianWriter::Write(const void* buffer, size_t length) {
  uint8_t* write_position = current();
  if (Skip(length)) {
    memcpy(write_position, buffer, length);
    return true;
  }
  return false;
}

}  // namespace openscreen
