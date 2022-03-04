// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/location.h"

#include <sstream>

#include "platform/base/macros.h"

namespace openscreen {

Location::Location() = default;
Location::Location(const Location&) = default;
Location::Location(Location&&) noexcept = default;

Location::Location(const void* program_counter)
    : program_counter_(program_counter) {}

Location& Location::operator=(const Location& other) = default;
Location& Location::operator=(Location&& other) = default;

std::string Location::ToString() const {
  if (program_counter_ == nullptr) {
    return "pc:NULL";
  }

  std::ostringstream oss;
  oss << "pc:" << program_counter_;
  return oss.str();
}

#if defined(__GNUC__)
#define RETURN_ADDRESS() \
  __builtin_extract_return_addr(__builtin_return_address(0))
#else
#define RETURN_ADDRESS() nullptr
#endif

// static
OSP_NOINLINE Location Location::CreateFromHere() {
  return Location(RETURN_ADDRESS());
}

// static
OSP_NOINLINE const void* GetProgramCounter() {
  return RETURN_ADDRESS();
}

}  // namespace openscreen
