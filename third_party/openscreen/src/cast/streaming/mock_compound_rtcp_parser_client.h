// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_MOCK_COMPOUND_RTCP_PARSER_CLIENT_H_
#define CAST_STREAMING_MOCK_COMPOUND_RTCP_PARSER_CLIENT_H_

#include "cast/streaming/compound_rtcp_parser.h"
#include "gmock/gmock.h"

namespace openscreen {
namespace cast {

class MockCompoundRtcpParserClient : public CompoundRtcpParser::Client {
 public:
  MOCK_METHOD1(OnReceiverReferenceTimeAdvanced,
               void(Clock::time_point reference_time));
  MOCK_METHOD1(OnReceiverReport, void(const RtcpReportBlock& receiver_report));
  MOCK_METHOD0(OnReceiverIndicatesPictureLoss, void());
  MOCK_METHOD2(OnReceiverCheckpoint,
               void(FrameId frame_id, std::chrono::milliseconds playout_delay));
  MOCK_METHOD1(OnReceiverHasFrames, void(std::vector<FrameId> acks));
  MOCK_METHOD1(OnReceiverIsMissingPackets, void(std::vector<PacketNack> nacks));
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_MOCK_COMPOUND_RTCP_PARSER_CLIENT_H_
