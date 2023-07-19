// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/base64.h"

#include <stddef.h>

#include <string>
#include <utility>
#include <vector>

#include "third_party/modp_b64/modp_b64.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {
namespace base64 {

std::string Encode(absl::Span<const uint8_t> input) {
  return Encode(absl::string_view(reinterpret_cast<const char*>(input.data()),
                                  input.size()));
}

std::string Encode(absl::string_view input) {
  std::string out;
  out.resize(modp_b64_encode_len(input.size()));

  const size_t output_size =
      modp_b64_encode(data(out), input.data(), input.size());
  if (output_size == MODP_B64_ERROR) {
    return {};
  }

  // The encode_len is generally larger than needed.
  out.resize(output_size);
  return out;
}

bool Decode(absl::string_view input, std::vector<uint8_t>* output) {
  std::vector<uint8_t> out(modp_b64_decode_len(input.size()));

  const size_t output_size = modp_b64_decode(
      reinterpret_cast<char*>(out.data()), input.data(), input.size());
  if (output_size == MODP_B64_ERROR) {
    return false;
  }

  // The output size from decode_len is generally larger than needed.
  out.resize(output_size);
  *output = std::move(out);
  return true;
}

}  // namespace base64
}  // namespace openscreen
