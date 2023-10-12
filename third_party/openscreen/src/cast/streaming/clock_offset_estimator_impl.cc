// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/clock_offset_estimator_impl.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "platform/base/trivial_clock_traits.h"

namespace openscreen {
namespace cast {
namespace {

// The lower this is, the faster we adjust to clock drift (but with more
// jitter). Each successful call to BoundCalculator::UpdateBound() uses this as
// the weight of the bound upte.
constexpr size_t kBoundUpdateWeight = 500;

// This should be large enough so that we can collect all 3 events before
// the entry gets removed from the map.
constexpr size_t kMaxEventTimesMapSize = 500;

// Bitwise merging of values to produce an ordered key for entries in the
// BoundCalculator::events_ map. Since std::map is sorted by key value, we
// ensure that the Packet ID is first (since the RTP timestamp may roll over
// eventually).
//
//  0         1         2         3         4         5         6
//  0 2 4 6 8 0 2 4 6 8 0 2 4 6 8 0 2 4 6 8 0 2 4 6 8 0 2 4 6 8 0 2 4
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |   Packet ID   |               RTP Timestamp                 |*| (is_audio)
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
uint64_t MakeEventKey(RtpTimeTicks rtp, uint16_t packet_id, bool audio) {
  return (static_cast<uint64_t>(packet_id) << 48) |
         (static_cast<uint64_t>(rtp.lower_32_bits()) << 1) |
         static_cast<uint64_t>(audio ? 1 : 0);
}

}  // namespace

std::unique_ptr<ClockOffsetEstimator> ClockOffsetEstimator::Create() {
  return std::make_unique<ClockOffsetEstimatorImpl>();
}

ClockOffsetEstimatorImpl::ClockOffsetEstimatorImpl() = default;
ClockOffsetEstimatorImpl::ClockOffsetEstimatorImpl(
    ClockOffsetEstimatorImpl&&) noexcept = default;
ClockOffsetEstimatorImpl& ClockOffsetEstimatorImpl::operator=(
    ClockOffsetEstimatorImpl&&) = default;
ClockOffsetEstimatorImpl::~ClockOffsetEstimatorImpl() = default;

void ClockOffsetEstimatorImpl::OnFrameEvent(const FrameEvent& frame_event) {
  switch (frame_event.type) {
    case StatisticsEventType::kFrameAckSent:
      lower_bound_.SetSent(
          frame_event.rtp_timestamp, 0,
          frame_event.media_type == StatisticsEventMediaType::kAudio,
          frame_event.timestamp);
      break;
    case StatisticsEventType::kFrameAckReceived:
      lower_bound_.SetReceived(
          frame_event.rtp_timestamp, 0,
          frame_event.media_type == StatisticsEventMediaType::kAudio,
          frame_event.timestamp);
      break;
    default:
      // Ignored
      break;
  }
}

void ClockOffsetEstimatorImpl::OnPacketEvent(const PacketEvent& packet_event) {
  switch (packet_event.type) {
    case StatisticsEventType::kPacketSentToNetwork:
      upper_bound_.SetSent(
          packet_event.rtp_timestamp, packet_event.packet_id,
          packet_event.media_type == StatisticsEventMediaType::kAudio,
          packet_event.timestamp);
      break;
    case StatisticsEventType::kPacketReceived:
      upper_bound_.SetReceived(
          packet_event.rtp_timestamp, packet_event.packet_id,
          packet_event.media_type == StatisticsEventMediaType::kAudio,
          packet_event.timestamp);
      break;
    default:
      // Ignored
      break;
  }
}

bool ClockOffsetEstimatorImpl::GetReceiverOffsetBounds(
    Clock::duration& lower_bound,
    Clock::duration& upper_bound) const {
  if (!lower_bound_.has_bound() || !upper_bound_.has_bound()) {
    return false;
  }

  lower_bound = -lower_bound_.bound();
  upper_bound = upper_bound_.bound();

  // Sanitize the output, we don't want the upper bound to be
  // lower than the lower bound, make them the same.
  if (upper_bound < lower_bound) {
    lower_bound += (lower_bound - upper_bound) / 2;
    upper_bound = lower_bound;
  }
  return true;
}

absl::optional<Clock::duration> ClockOffsetEstimatorImpl::GetEstimatedOffset()
    const {
  Clock::duration lower_bound;
  Clock::duration upper_bound;
  if (!GetReceiverOffsetBounds(lower_bound, upper_bound)) {
    return {};
  }
  return (upper_bound + lower_bound) / 2;
}

ClockOffsetEstimatorImpl::BoundCalculator::BoundCalculator() = default;
ClockOffsetEstimatorImpl::BoundCalculator::BoundCalculator(
    BoundCalculator&&) noexcept = default;
ClockOffsetEstimatorImpl::BoundCalculator&
ClockOffsetEstimatorImpl::BoundCalculator::operator=(BoundCalculator&&) =
    default;
ClockOffsetEstimatorImpl::BoundCalculator::~BoundCalculator() = default;

void ClockOffsetEstimatorImpl::BoundCalculator::SetSent(RtpTimeTicks rtp,
                                                        uint16_t packet_id,
                                                        bool audio,
                                                        Clock::time_point t) {
  const uint64_t key = MakeEventKey(rtp, packet_id, audio);
  events_[key].first = t;
  CheckUpdate(key);
}

void ClockOffsetEstimatorImpl::BoundCalculator::SetReceived(
    RtpTimeTicks rtp,
    uint16_t packet_id,
    bool audio,
    Clock::time_point t) {
  const uint64_t key = MakeEventKey(rtp, packet_id, audio);
  events_[key].second = t;
  CheckUpdate(key);
}

void ClockOffsetEstimatorImpl::BoundCalculator::UpdateBound(
    Clock::time_point sent,
    Clock::time_point received) {
  Clock::duration delta = received - sent;
  if (has_bound_) {
    if (delta < bound_) {
      bound_ = delta;
    } else {
      bound_ += (delta - bound_) / kBoundUpdateWeight;
    }
  } else {
    bound_ = delta;
  }
  has_bound_ = true;
}

void ClockOffsetEstimatorImpl::BoundCalculator::CheckUpdate(uint64_t key) {
  const TimeTickPair& ticks = events_[key];
  if (ticks.first && ticks.second) {
    UpdateBound(ticks.first.value(), ticks.second.value());
    events_.erase(key);
    return;
  }

  if (events_.size() > kMaxEventTimesMapSize) {
    // We can make use of the fact that std::map sorts by key and just erase
    // the first entry.
    events_.erase(events_.begin());
  }
}

}  // namespace cast
}  // namespace openscreen
