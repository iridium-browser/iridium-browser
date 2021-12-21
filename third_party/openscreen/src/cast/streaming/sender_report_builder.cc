// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/sender_report_builder.h"

#include "cast/streaming/packet_util.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

SenderReportBuilder::SenderReportBuilder(RtcpSession* session)
    : session_(session) {
  OSP_DCHECK(session_);
}

SenderReportBuilder::~SenderReportBuilder() = default;

std::pair<absl::Span<uint8_t>, StatusReportId> SenderReportBuilder::BuildPacket(
    const RtcpSenderReport& sender_report,
    absl::Span<uint8_t> buffer) const {
  OSP_CHECK_GE(buffer.size(), kRequiredBufferSize);

  uint8_t* const packet_begin = buffer.data();

  RtcpCommonHeader header;
  header.packet_type = RtcpPacketType::kSenderReport;
  header.payload_size = kRtcpSenderReportSize;
  if (sender_report.report_block) {
    header.with.report_count = 1;
    header.payload_size += kRtcpReportBlockSize;
  } else {
    header.with.report_count = 0;
  }
  header.AppendFields(&buffer);

  AppendField<uint32_t>(session_->sender_ssrc(), &buffer);
  const NtpTimestamp ntp_timestamp =
      session_->ntp_converter().ToNtpTimestamp(sender_report.reference_time);
  AppendField<uint64_t>(ntp_timestamp, &buffer);
  AppendField<uint32_t>(sender_report.rtp_timestamp.lower_32_bits(), &buffer);
  AppendField<uint32_t>(sender_report.send_packet_count, &buffer);
  AppendField<uint32_t>(sender_report.send_octet_count, &buffer);
  if (sender_report.report_block) {
    sender_report.report_block->AppendFields(&buffer);
  }

  uint8_t* const packet_end = buffer.data();
  return std::make_pair(
      absl::Span<uint8_t>(packet_begin, packet_end - packet_begin),
      ToStatusReportId(ntp_timestamp));
}

Clock::time_point SenderReportBuilder::GetRecentReportTime(
    StatusReportId report_id,
    Clock::time_point on_or_before) const {
  // Assumption: The |report_id| is the middle 32 bits of a 64-bit NtpTimestamp.
  static_assert(ToStatusReportId(NtpTimestamp{0x0192a3b4c5d6e7f8}) ==
                    StatusReportId{0xa3b4c5d6},
                "FIXME: ToStatusReportId() implementation changed.");

  // Compute the maximum possible NtpTimestamp. Then, use its uppermost 16 bits
  // and the 32 bits from the report_id to produce a reconstructed NtpTimestamp.
  const NtpTimestamp max_timestamp =
      session_->ntp_converter().ToNtpTimestamp(on_or_before);
  // max_timestamp: HH......
  //     report_id:     LLLL
  //                ↓↓ ↙↙↙↙
  // reconstructed: HHLLLL00
  NtpTimestamp reconstructed = (max_timestamp & (uint64_t{0xffff} << 48)) |
                               (static_cast<uint64_t>(report_id) << 16);
  //  If the reconstructed timestamp is greater than the maximum one, rollover
  //  of the lower 48 bits occurred. Subtract one from the upper 16 bits to
  //  rectify that.
  if (reconstructed > max_timestamp) {
    reconstructed -= uint64_t{1} << 48;
  }

  return session_->ntp_converter().ToLocalTime(reconstructed);
}

}  // namespace cast
}  // namespace openscreen
