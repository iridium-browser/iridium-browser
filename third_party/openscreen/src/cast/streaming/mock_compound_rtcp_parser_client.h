// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_MOCK_COMPOUND_RTCP_PARSER_CLIENT_H_
#define CAST_STREAMING_MOCK_COMPOUND_RTCP_PARSER_CLIENT_H_

#include <vector>

#include "cast/streaming/compound_rtcp_parser.h"
#include "gmock/gmock.h"

namespace openscreen {
namespace cast {

class MockCompoundRtcpParserClient : public CompoundRtcpParser::Client {
 public:
  MOCK_METHOD(void,
              OnReceiverReferenceTimeAdvanced,
              (Clock::time_point reference_time),
              (override));
  MOCK_METHOD(void,
              OnReceiverReport,
              (const RtcpReportBlock& receiver_report),
              (override));
  MOCK_METHOD(void,
              OnCastReceiverFrameLogMessages,
              (std::vector<RtcpReceiverFrameLogMessage> messages),
              (override));
  MOCK_METHOD(void, OnReceiverIndicatesPictureLoss, (), (override));
  MOCK_METHOD(void,
              OnReceiverCheckpoint,
              (FrameId frame_id, std::chrono::milliseconds playout_delay),
              (override));
  MOCK_METHOD(void,
              OnReceiverHasFrames,
              (std::vector<FrameId> acks),
              (override));
  MOCK_METHOD(void,
              OnReceiverIsMissingPackets,
              (std::vector<PacketNack> nacks),
              (override));
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_MOCK_COMPOUND_RTCP_PARSER_CLIENT_H_
