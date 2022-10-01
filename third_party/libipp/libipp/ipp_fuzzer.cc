// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp.h"

#include <cstdint>
#include <limits>
#include <vector>

#include "base/test/fuzzed_data_provider.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  base::FuzzedDataProvider fuzz_data(data, size);
  const bool is_client = fuzz_data.ConsumeBool();
  const ipp::Operation oper_id =
      static_cast<ipp::Operation>(fuzz_data.ConsumeUint16());
  const std::string packet_str = fuzz_data.ConsumeRemainingBytes();
  const std::vector<uint8_t> packet_bytes(packet_str.begin(), packet_str.end());

  if (is_client) {
    ipp::Client client;
    client.ReadResponseFrameFrom(packet_bytes);
    auto response = ipp::Response::NewResponse(oper_id);
    if (response == nullptr)
      response = std::make_unique<ipp::Response>(oper_id);
    client.ParseResponseAndSaveTo(response.get());
  } else {
    ipp::Server server;
    server.ReadRequestFrameFrom(packet_bytes);
    auto request = ipp::Request::NewRequest(oper_id);
    if (request == nullptr)
      request = std::make_unique<ipp::Request>(oper_id);
    server.ParseRequestAndSaveTo(request.get());
  }

  return 0;
}
