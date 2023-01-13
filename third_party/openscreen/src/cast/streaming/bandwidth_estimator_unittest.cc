// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/bandwidth_estimator.h"

#include <chrono>
#include <limits>
#include <random>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/api/time.h"
#include "util/chrono_helpers.h"

namespace openscreen {
namespace cast {
namespace {

using clock_operators::operator<<;

// BandwidthEstimator configuration common to all tests.
constexpr int kMaxPacketsPerTimeslice = 10;
constexpr Clock::duration kTimesliceDuration = milliseconds(10);
constexpr int kTimeslicesPerSecond = seconds(1) / kTimesliceDuration;

// Use a fake, fixed start time.
constexpr Clock::time_point kStartTime =
    Clock::time_point() + Clock::duration(1234567890);

// Range of "fuzz" to add to timestamps in BandwidthEstimatorTest::AddFuzz().
constexpr Clock::duration kMaxFuzzOffset = milliseconds(15);
constexpr int kFuzzLowerBoundClockTicks = (-kMaxFuzzOffset).count();
constexpr int kFuzzUpperBoundClockTicks = kMaxFuzzOffset.count();

class BandwidthEstimatorTest : public testing::Test {
 public:
  BandwidthEstimatorTest()
      : estimator_(kMaxPacketsPerTimeslice, kTimesliceDuration, kStartTime) {}

  BandwidthEstimator* estimator() { return &estimator_; }

  // Returns |t| plus or minus |kMaxFuzzOffset|.
  Clock::time_point AddFuzz(Clock::time_point t) {
    return t + Clock::duration(distribution_(rand_));
  }

 private:
  BandwidthEstimator estimator_;

  // These are used to generate random values for AddFuzz().
  static constexpr std::minstd_rand::result_type kRandSeed =
      kStartTime.time_since_epoch().count();
  std::minstd_rand rand_{kRandSeed};
  std::uniform_int_distribution<int> distribution_{kFuzzLowerBoundClockTicks,
                                                   kFuzzUpperBoundClockTicks};
};

// Tests that, without any data, there won't be any estimates.
TEST_F(BandwidthEstimatorTest, DoesNotEstimateWithoutAnyInput) {
  EXPECT_EQ(0, estimator()->ComputeNetworkBandwidth());
}

// Tests the case where packets are being sent, but the Receiver hasn't provided
// feedback (e.g., due to a network blackout).
TEST_F(BandwidthEstimatorTest, DoesNotEstimateWithoutFeedback) {
  Clock::time_point now = kStartTime;
  for (int i = 0; i < 3; ++i) {
    const Clock::time_point end = now + estimator()->history_window();
    for (; now < end; now += kTimesliceDuration) {
      estimator()->OnBurstComplete(i, now);
      EXPECT_EQ(0, estimator()->ComputeNetworkBandwidth());
    }
    now = end;
  }
}

// Tests the case where packets are being sent, and a connection to the Receiver
// has been confirmed (because RTCP packets are coming in), but the Receiver has
// not successfully received anything.
TEST_F(BandwidthEstimatorTest, DoesNotEstimateIfNothingSuccessfullyReceived) {
  const Clock::duration kRoundTripTime = milliseconds(1);

  Clock::time_point now = kStartTime;
  for (int i = 0; i < 3; ++i) {
    const Clock::time_point end = now + estimator()->history_window();
    for (; now < end; now += kTimesliceDuration) {
      estimator()->OnBurstComplete(i, now);
      estimator()->OnRtcpReceived(now + kRoundTripTime, kRoundTripTime);
      EXPECT_EQ(0, estimator()->ComputeNetworkBandwidth());
    }
    now = end;
  }
}

// Tests that, when the Receiver successfully receives the payload bytes at a
// fixed rate, the network bandwidth estimates are based on the amount of time
// the Sender spent transmitting.
TEST_F(BandwidthEstimatorTest, EstimatesAtVariousBurstSaturations) {
  // These determine how many packets to burst in the simulation below.
  constexpr int kDivisors[] = {
      1,  // Burst 100% of max possible packets.
      2,  // Burst 50% of max possible packets.
      5,  // Burst 20% of max possible packets.
  };

  const Clock::duration kRoundTripTime = milliseconds(1);

  constexpr int kReceivedBytesPerSecond = 256000;
  constexpr int kReceivedBytesPerTimeslice =
      kReceivedBytesPerSecond / kTimeslicesPerSecond;
  static_assert(kReceivedBytesPerSecond % kTimeslicesPerSecond == 0,
                "Test expectations won't account for rounding errors.");

  ASSERT_EQ(0, estimator()->ComputeNetworkBandwidth());

  // Simulate bursting at various rates, and confirm the bandwidth estimate is
  // increasing for each burst rate. The estimate should be increasing because
  // the total time spent transmitting is decreasing (while the same number of
  // bytes are being received).
  Clock::time_point now = kStartTime;
  for (int divisor : kDivisors) {
    SCOPED_TRACE(testing::Message() << "divisor=" << divisor);

    const Clock::time_point end = now + estimator()->history_window();
    for (; now < end; now += kTimesliceDuration) {
      estimator()->OnBurstComplete(kMaxPacketsPerTimeslice / divisor, now);
      const Clock::time_point rtcp_arrival_time = now + kRoundTripTime;
      estimator()->OnPayloadReceived(kReceivedBytesPerTimeslice,
                                     rtcp_arrival_time, kRoundTripTime);
      estimator()->OnRtcpReceived(rtcp_arrival_time, kRoundTripTime);
    }
    now = end;

    const int estimate = estimator()->ComputeNetworkBandwidth();
    EXPECT_EQ(divisor * kReceivedBytesPerSecond * CHAR_BIT, estimate);
  }
}

// Tests that magnitude of the network round trip times, as well as random
// variance in packet arrival times, do not have a significant effect on the
// bandwidth estimates.
TEST_F(BandwidthEstimatorTest, EstimatesIndependentOfFeedbackDelays) {
  constexpr int kFactor = 2;
  constexpr int kPacketsPerBurst = kMaxPacketsPerTimeslice / kFactor;
  static_assert(kMaxPacketsPerTimeslice % kFactor == 0, "wanted exactly half");

  constexpr milliseconds kRoundTripTimes[3] = {milliseconds(1), milliseconds(9),
                                               milliseconds(42)};

  constexpr int kReceivedBytesPerSecond = 2000000;
  constexpr int kReceivedBytesPerTimeslice =
      kReceivedBytesPerSecond / kTimeslicesPerSecond;

  // An arbitrary threshold. Sources of error include anything that would place
  // byte flows outside the history window (e.g., AddFuzz(), or the 42ms round
  // trip time).
  constexpr int kMaxErrorPercent = 3;

  Clock::time_point now = kStartTime;
  for (Clock::duration round_trip_time : kRoundTripTimes) {
    SCOPED_TRACE(testing::Message()
                 << "round_trip_time=" << round_trip_time.count());

    const Clock::time_point end = now + estimator()->history_window();
    for (; now < end; now += kTimesliceDuration) {
      estimator()->OnBurstComplete(kPacketsPerBurst, now);
      const Clock::time_point rtcp_arrival_time =
          AddFuzz(now + round_trip_time);
      estimator()->OnPayloadReceived(kReceivedBytesPerTimeslice,
                                     rtcp_arrival_time, round_trip_time);
      estimator()->OnRtcpReceived(rtcp_arrival_time, round_trip_time);
    }
    now = end;

    constexpr int kExpectedEstimate =
        kFactor * kReceivedBytesPerSecond * CHAR_BIT;
    constexpr int kMaxError = kExpectedEstimate * kMaxErrorPercent / 100;
    EXPECT_NEAR(kExpectedEstimate, estimator()->ComputeNetworkBandwidth(),
                kMaxError);
  }
}

// Tests that both the history tracking internal to BandwidthEstimator, as well
// as its computation of the bandwidth estimate, are both resistant to possible
// integer overflow cases. The internal implementation always clamps to the
// valid range of int.
TEST_F(BandwidthEstimatorTest, ClampsEstimateToMaxInt) {
  constexpr int kPacketsPerBurst = kMaxPacketsPerTimeslice / 5;
  static_assert(kMaxPacketsPerTimeslice % 5 == 0, "wanted exactly 20%");
  const Clock::duration kRoundTripTime = milliseconds(1);

  int last_estimate = estimator()->ComputeNetworkBandwidth();
  ASSERT_EQ(last_estimate, 0);

  // Simulate increasing numbers of bytes received per timeslice until it
  // reaches values near INT_MAX. Along the way, the bandwidth estimates
  // themselves should start clamping and, because of the fuzz added to RTCP
  // arrival times, individual buckets in BandwidthEstimator::FlowTracker will
  // occassionally be clamped too.
  Clock::time_point now = kStartTime;
  for (unsigned int bytes_received_per_timeslice = 1;
       // Not overflowed past INT_MAX.
       static_cast<int>(bytes_received_per_timeslice) > 0;
       bytes_received_per_timeslice *= 2) {
    int int_bytes = static_cast<int>(bytes_received_per_timeslice);
    SCOPED_TRACE(testing::Message()
                 << "bytes_received_per_timeslice=" << int_bytes);

    const Clock::time_point end = now + estimator()->history_window() / 4;
    for (; now < end; now += kTimesliceDuration) {
      estimator()->OnBurstComplete(kPacketsPerBurst, now);
      const Clock::time_point rtcp_arrival_time = AddFuzz(now + kRoundTripTime);
      estimator()->OnPayloadReceived(int_bytes, rtcp_arrival_time,
                                     kRoundTripTime);
      estimator()->OnRtcpReceived(rtcp_arrival_time, kRoundTripTime);
    }
    now = end;

    const int estimate = estimator()->ComputeNetworkBandwidth();
    EXPECT_LE(last_estimate, estimate);
    last_estimate = estimate;
  }

  // Confirm there was a loop iteration at which the estimate reached INT_MAX
  // and then stayed there for successive loop iterations.
  EXPECT_EQ(std::numeric_limits<int>::max(), last_estimate);
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
