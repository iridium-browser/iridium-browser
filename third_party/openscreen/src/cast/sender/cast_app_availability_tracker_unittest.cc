// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/sender/cast_app_availability_tracker.h"

#include "cast/common/public/cast_streaming_app_ids.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/test/fake_clock.h"

namespace openscreen {
namespace cast {
namespace {

using ::testing::UnorderedElementsAreArray;

MATCHER_P(CastMediaSourcesEqual, expected, "") {
  if (expected.size() != arg.size())
    return false;
  return std::equal(
      expected.begin(), expected.end(), arg.begin(),
      [](const CastMediaSource& source1, const CastMediaSource& source2) {
        return source1.source_id() == source2.source_id();
      });
}

}  // namespace

class CastAppAvailabilityTrackerTest : public ::testing::Test {
 public:
  CastAppAvailabilityTrackerTest() : clock_(Clock::now()) {}
  ~CastAppAvailabilityTrackerTest() override = default;

  Clock::time_point Now() const { return clock_.now(); }

 protected:
  FakeClock clock_;
  CastAppAvailabilityTracker tracker_;
};

TEST_F(CastAppAvailabilityTrackerTest, RegisterSource) {
  CastMediaSource source1("cast:AAA?clientId=1", {"AAA"});
  CastMediaSource source2("cast:AAA?clientId=2", {"AAA"});

  std::vector<std::string> expected_app_ids = {"AAA"};
  EXPECT_EQ(expected_app_ids, tracker_.RegisterSource(source1));

  EXPECT_EQ(std::vector<std::string>{}, tracker_.RegisterSource(source1));
  EXPECT_EQ(std::vector<std::string>{}, tracker_.RegisterSource(source2));

  tracker_.UnregisterSource(source1);
  tracker_.UnregisterSource(source2);

  EXPECT_EQ(expected_app_ids, tracker_.RegisterSource(source1));
  EXPECT_EQ(expected_app_ids, tracker_.GetRegisteredApps());
}

TEST_F(CastAppAvailabilityTrackerTest, RegisterSourceReturnsMultipleAppIds) {
  // Cast Streaming app ids.
  CastMediaSource source1("urn:x-org.chromium.media:source:tab:1",
                          GetCastStreamingAppIds());

  std::vector<std::string> expected_app_ids = GetCastStreamingAppIds();
  EXPECT_THAT(tracker_.RegisterSource(source1),
              UnorderedElementsAreArray(expected_app_ids));
  EXPECT_THAT(tracker_.GetRegisteredApps(),
              UnorderedElementsAreArray(expected_app_ids));
}

TEST_F(CastAppAvailabilityTrackerTest, MultipleAppIdsAlreadyTrackingOne) {
  // Cast Streaming audio+video app ID.
  CastMediaSource source1("cast:0F5096E8?clientId=123",
                          {GetCastStreamingAudioVideoAppId()});

  std::vector<std::string> new_app_ids = {GetCastStreamingAudioVideoAppId()};
  std::vector<std::string> registered_app_ids = {
      GetCastStreamingAudioVideoAppId()};
  EXPECT_EQ(new_app_ids, tracker_.RegisterSource(source1));
  EXPECT_EQ(registered_app_ids, tracker_.GetRegisteredApps());

  CastMediaSource source2("urn:x-org.chromium.media:source:tab:1",
                          GetCastStreamingAppIds());

  new_app_ids = {GetCastStreamingAudioOnlyAppId()};
  registered_app_ids = GetCastStreamingAppIds();

  EXPECT_EQ(new_app_ids, tracker_.RegisterSource(source2));
  EXPECT_THAT(tracker_.GetRegisteredApps(),
              UnorderedElementsAreArray(registered_app_ids));
}

TEST_F(CastAppAvailabilityTrackerTest, UpdateAppAvailability) {
  CastMediaSource source1("cast:AAA?clientId=1", {"AAA"});
  CastMediaSource source2("cast:AAA?clientId=2", {"AAA"});
  CastMediaSource source3("cast:BBB?clientId=3", {"BBB"});

  tracker_.RegisterSource(source3);

  // |source3| not affected.
  EXPECT_THAT(
      tracker_.UpdateAppAvailability(
          "receiverId1", "AAA", {AppAvailabilityResult::kAvailable, Now()}),
      CastMediaSourcesEqual(std::vector<CastMediaSource>()));

  std::vector<std::string> receivers_1 = {"receiverId1"};
  std::vector<std::string> receivers_1_2 = {"receiverId1", "receiverId2"};
  std::vector<CastMediaSource> sources_1 = {source1};
  std::vector<CastMediaSource> sources_1_2 = {source1, source2};

  // Tracker returns available receivers even though sources aren't registered.
  EXPECT_EQ(receivers_1, tracker_.GetAvailableReceivers(source1));
  EXPECT_EQ(receivers_1, tracker_.GetAvailableReceivers(source2));
  EXPECT_TRUE(tracker_.GetAvailableReceivers(source3).empty());

  tracker_.RegisterSource(source1);
  // Only |source1| is registered for this app.
  EXPECT_THAT(
      tracker_.UpdateAppAvailability(
          "receiverId2", "AAA", {AppAvailabilityResult::kAvailable, Now()}),
      CastMediaSourcesEqual(sources_1));
  EXPECT_THAT(tracker_.GetAvailableReceivers(source1),
              UnorderedElementsAreArray(receivers_1_2));
  EXPECT_THAT(tracker_.GetAvailableReceivers(source2),
              UnorderedElementsAreArray(receivers_1_2));
  EXPECT_TRUE(tracker_.GetAvailableReceivers(source3).empty());

  tracker_.RegisterSource(source2);
  EXPECT_THAT(
      tracker_.UpdateAppAvailability(
          "receiverId2", "AAA", {AppAvailabilityResult::kUnavailable, Now()}),
      CastMediaSourcesEqual(sources_1_2));
  EXPECT_EQ(receivers_1, tracker_.GetAvailableReceivers(source1));
  EXPECT_EQ(receivers_1, tracker_.GetAvailableReceivers(source2));
  EXPECT_TRUE(tracker_.GetAvailableReceivers(source3).empty());
}

TEST_F(CastAppAvailabilityTrackerTest, RemoveResultsForReceiver) {
  CastMediaSource source1("cast:AAA?clientId=1", {"AAA"});

  tracker_.UpdateAppAvailability("receiverId1", "AAA",
                                 {AppAvailabilityResult::kAvailable, Now()});
  EXPECT_EQ(AppAvailabilityResult::kAvailable,
            tracker_.GetAvailability("receiverId1", "AAA").availability);

  std::vector<std::string> expected_receiver_ids = {"receiverId1"};
  EXPECT_EQ(expected_receiver_ids, tracker_.GetAvailableReceivers(source1));

  // Unrelated receiver ID.
  tracker_.RemoveResultsForReceiver("receiverId2");
  EXPECT_EQ(AppAvailabilityResult::kAvailable,
            tracker_.GetAvailability("receiverId1", "AAA").availability);
  EXPECT_EQ(expected_receiver_ids, tracker_.GetAvailableReceivers(source1));

  tracker_.RemoveResultsForReceiver("receiverId1");
  EXPECT_EQ(AppAvailabilityResult::kUnknown,
            tracker_.GetAvailability("receiverId1", "AAA").availability);
  EXPECT_EQ(std::vector<std::string>{},
            tracker_.GetAvailableReceivers(source1));
}

}  // namespace cast
}  // namespace openscreen
