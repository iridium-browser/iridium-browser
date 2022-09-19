// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/rust_tool.h"
#include "gn/rust_values.h"
#include "gn/target.h"

RustValues::RustValues() : crate_type_(RustValues::CRATE_AUTO) {}

RustValues::~RustValues() = default;

// static
RustValues::CrateType RustValues::InferredCrateType(const Target* target) {
  // TODO: Consider removing crate_type. It allows for things like
  //
  // executable("foo") {
  //   crate_type = "rlib"
  // }
  //
  // Which doesn't make sense.
  if (!target->source_types_used().RustSourceUsed()) {
    return CRATE_AUTO;
  }
  if (!target->has_rust_values())
    return CRATE_AUTO;

  CrateType crate_type = target->rust_values().crate_type();
  if (crate_type != CRATE_AUTO) {
      return crate_type;
  }

  switch (target->output_type()) {
    case Target::EXECUTABLE:
      return CRATE_BIN;
    case Target::SHARED_LIBRARY:
      return CRATE_DYLIB;
    case Target::STATIC_LIBRARY:
      return CRATE_STATICLIB;
    case Target::RUST_LIBRARY:
      return CRATE_RLIB;
    case Target::RUST_PROC_MACRO:
      return CRATE_PROC_MACRO;
    default:
      return CRATE_AUTO;
  }
}

// static
bool RustValues::IsRustLibrary(const Target* target) {
  return target->output_type() == Target::RUST_LIBRARY ||
     InferredCrateType(target) == CRATE_DYLIB ||
     InferredCrateType(target) == CRATE_PROC_MACRO;
}
