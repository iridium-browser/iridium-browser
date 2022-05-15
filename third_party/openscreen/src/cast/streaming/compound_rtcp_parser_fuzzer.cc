// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "cast/streaming/compound_rtcp_parser.h"
#include "cast/streaming/frame_id.h"
#include "cast/streaming/rtcp_session.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  using openscreen::cast::CompoundRtcpParser;
  using openscreen::cast::FrameId;
  using openscreen::cast::RtcpSession;
  using openscreen::cast::Ssrc;

  constexpr Ssrc kSenderSsrcInSeedCorpus = 1;
  constexpr Ssrc kReceiverSsrcInSeedCorpus = 2;

  class ClientThatIgnoresEverything : public CompoundRtcpParser::Client {
   public:
    ClientThatIgnoresEverything() = default;
    ~ClientThatIgnoresEverything() override = default;
  };
  // Allocate the RtcpSession and CompoundRtcpParser statically (i.e., one-time
  // init) to improve the fuzzer's execution rate. This is because RtcpSession
  // also contains a NtpTimeConverter, which samples the system clock at
  // construction time. There is no reason to re-construct these objects for
  // each fuzzer test input.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
  static RtcpSession session(kSenderSsrcInSeedCorpus, kReceiverSsrcInSeedCorpus,
                             openscreen::Clock::time_point{});
  static ClientThatIgnoresEverything client_that_ignores_everything;
  static CompoundRtcpParser parser(&session, &client_that_ignores_everything);
#pragma clang diagnostic pop

  const auto max_feedback_frame_id = FrameId::first() + 100;
  parser.Parse(absl::Span<const uint8_t>(data, size), max_feedback_frame_id);

  return 0;
}

#if defined(NEEDS_MAIN_TO_CALL_FUZZER_DRIVER)

// Forward declarations of Clang's built-in libFuzzer driver.
namespace fuzzer {
using TestOneInputCallback = int (*)(const uint8_t* data, size_t size);
int FuzzerDriver(int* argc, char*** argv, TestOneInputCallback callback);
}  // namespace fuzzer

int main(int argc, char* argv[]) {
  return fuzzer::FuzzerDriver(&argc, &argv, LLVMFuzzerTestOneInput);
}

#endif  // defined(NEEDS_MAIN_TO_CALL_FUZZER_DRIVER)
