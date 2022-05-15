// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_switch_event_latency_recorder.h"
#include "base/test/metrics/histogram_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

class TabSwitchEventLatencyRecorderTest : public testing::Test {
 public:
  ~TabSwitchEventLatencyRecorderTest() override {}

  void SetUp() override {
    EXPECT_EQ(histogram_tester
                  .GetAllSamples("Browser.Tabs.InputEventToSelectionTime.Mouse")
                  .size(),
              0ULL);
    EXPECT_EQ(
        histogram_tester
            .GetAllSamples("Browser.Tabs.InputEventToSelectionTime.Keyboard")
            .size(),
        0ULL);
    EXPECT_EQ(histogram_tester
                  .GetAllSamples("Browser.Tabs.InputEventToSelectionTime.Touch")
                  .size(),
              0ULL);
    EXPECT_EQ(histogram_tester
                  .GetAllSamples("Browser.Tabs.InputEventToSelectionTime.Wheel")
                  .size(),
              0ULL);
  }

 protected:
  size_t GetHistogramSampleSize(
      TabSwitchEventLatencyRecorder::EventType event) {
    switch (event) {
      case TabSwitchEventLatencyRecorder::EventType::kMouse:
        return histogram_tester
            .GetAllSamples("Browser.Tabs.InputEventToSelectionTime.Mouse")
            .size();
      case TabSwitchEventLatencyRecorder::EventType::kKeyboard:
        return histogram_tester
            .GetAllSamples("Browser.Tabs.InputEventToSelectionTime.Keyboard")
            .size();
      case TabSwitchEventLatencyRecorder::EventType::kTouch:
        return histogram_tester
            .GetAllSamples("Browser.Tabs.InputEventToSelectionTime.Touch")
            .size();
      case TabSwitchEventLatencyRecorder::EventType::kWheel:
        return histogram_tester
            .GetAllSamples("Browser.Tabs.InputEventToSelectionTime.Wheel")
            .size();
      default:
        return 0;
    }
  }

  TabSwitchEventLatencyRecorder tab_switch_event_latency_recorder_;
  base::HistogramTester histogram_tester;
};

// Mouse input event latency is recorded to histogram
TEST_F(TabSwitchEventLatencyRecorderTest, MouseInputLatency) {
  const auto now = base::TimeTicks::Now();

  tab_switch_event_latency_recorder_.BeginLatencyTiming(
      now, TabSwitchEventLatencyRecorder::EventType::kMouse);
  tab_switch_event_latency_recorder_.OnWillChangeActiveTab(
      base::TimeTicks::Now());
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kMouse),
      1ULL);
  EXPECT_EQ(GetHistogramSampleSize(
                TabSwitchEventLatencyRecorder::EventType::kKeyboard),
            0ULL);
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kTouch),
      0ULL);
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kWheel),
      0ULL);
}

// Keyboard input event latency is recorded to histogram
TEST_F(TabSwitchEventLatencyRecorderTest, KeyboardInputLatency) {
  const auto now = base::TimeTicks::Now();

  tab_switch_event_latency_recorder_.BeginLatencyTiming(
      now, TabSwitchEventLatencyRecorder::EventType::kKeyboard);
  tab_switch_event_latency_recorder_.OnWillChangeActiveTab(
      base::TimeTicks::Now());
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kMouse),
      0ULL);
  EXPECT_EQ(GetHistogramSampleSize(
                TabSwitchEventLatencyRecorder::EventType::kKeyboard),
            1ULL);
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kTouch),
      0ULL);
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kWheel),
      0ULL);
}

// Touch input event latency is recorded to histogram
TEST_F(TabSwitchEventLatencyRecorderTest, TouchInputLatency) {
  const auto now = base::TimeTicks::Now();

  tab_switch_event_latency_recorder_.BeginLatencyTiming(
      now, TabSwitchEventLatencyRecorder::EventType::kTouch);
  tab_switch_event_latency_recorder_.OnWillChangeActiveTab(
      base::TimeTicks::Now());
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kMouse),
      0ULL);
  EXPECT_EQ(GetHistogramSampleSize(
                TabSwitchEventLatencyRecorder::EventType::kKeyboard),
            0ULL);
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kTouch),
      1ULL);
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kWheel),
      0ULL);
}

// Scroll wheel input event latency is recorded to histogram
TEST_F(TabSwitchEventLatencyRecorderTest, WheelInputLatency) {
  const auto now = base::TimeTicks::Now();

  tab_switch_event_latency_recorder_.BeginLatencyTiming(
      now, TabSwitchEventLatencyRecorder::EventType::kWheel);
  tab_switch_event_latency_recorder_.OnWillChangeActiveTab(
      base::TimeTicks::Now());
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kMouse),
      0ULL);
  EXPECT_EQ(GetHistogramSampleSize(
                TabSwitchEventLatencyRecorder::EventType::kKeyboard),
            0ULL);
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kTouch),
      0ULL);
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kWheel),
      1ULL);
}

// Other input event type is not recorded to histogram
TEST_F(TabSwitchEventLatencyRecorderTest, OtherInputLatency) {
  const auto now = base::TimeTicks::Now();

  tab_switch_event_latency_recorder_.BeginLatencyTiming(
      now, TabSwitchEventLatencyRecorder::EventType::kOther);
  tab_switch_event_latency_recorder_.OnWillChangeActiveTab(
      base::TimeTicks::Now());
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kMouse),
      0ULL);
  EXPECT_EQ(GetHistogramSampleSize(
                TabSwitchEventLatencyRecorder::EventType::kKeyboard),
            0ULL);
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kTouch),
      0ULL);
  EXPECT_EQ(
      GetHistogramSampleSize(TabSwitchEventLatencyRecorder::EventType::kWheel),
      0ULL);
}
