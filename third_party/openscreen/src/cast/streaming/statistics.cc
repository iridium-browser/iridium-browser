// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/statistics.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

#include "absl/strings/str_join.h"
#include "util/json/json_helpers.h"
#include "util/json/json_serialization.h"
#include "util/stringprintf.h"

namespace openscreen {
namespace cast {

namespace {

template <typename Type>
Json::Value ToJson(const Type& t) {
  return t.ToJson();
}

template <>
Json::Value ToJson(const double& t) {
  return t;
}

template <typename T, typename Type>
Json::Value ArrayToJson(
    const std::array<T, static_cast<size_t>(Type::kNumTypes)>& list,
    const EnumNameTable<Type, static_cast<size_t>(Type::kNumTypes)>& names) {
  Json::Value out;
  for (size_t i = 0; i < list.size(); ++i) {
    ErrorOr<const char*> name = GetEnumName(names, static_cast<Type>(i));
    OSP_CHECK(name);
    out[name.value()] = ToJson(list[i]);
  }
  return out;
}

}  // namespace

const EnumNameTable<StatisticType,
                    static_cast<size_t>(StatisticType::kNumTypes)>
    kStatisticTypeNames = {
        {{"kEnqueueFps", StatisticType::kEnqueueFps},
         {"kAvgCaptureLatencyMs", StatisticType::kAvgCaptureLatencyMs},
         {"kAvgEncodeTimeMs", StatisticType::kAvgEncodeTimeMs},
         {"kAvgQueueingLatencyMs", StatisticType::kAvgQueueingLatencyMs},
         {"kAvgNetworkLatencyMs", StatisticType::kAvgNetworkLatencyMs},
         {"kAvgPacketLatencyMs", StatisticType::kAvgPacketLatencyMs},
         {"kAvgFrameLatencyMs", StatisticType::kAvgFrameLatencyMs},
         {"kAvgEndToEndLatencyMs", StatisticType::kAvgEndToEndLatencyMs},
         {"kEncodeRateKbps", StatisticType::kEncodeRateKbps},
         {"kPacketTransmissionRateKbps",
          StatisticType::kPacketTransmissionRateKbps},
         {"kTimeSinceLastReceiverResponseMs",
          StatisticType::kTimeSinceLastReceiverResponseMs},
         {"kNumFramesCaptured", StatisticType::kNumFramesCaptured},
         {"kNumFramesDroppedByEncoder",
          StatisticType::kNumFramesDroppedByEncoder},
         {"kNumLateFrames", StatisticType::kNumLateFrames},
         {"kNumPacketsSent", StatisticType::kNumPacketsSent},
         {"kNumPacketsReceived", StatisticType::kNumPacketsReceived},
         {"kFirstEventTimeMs", StatisticType::kFirstEventTimeMs},
         {"kLastEventTimeMs", StatisticType::kLastEventTimeMs}}};

const EnumNameTable<HistogramType,
                    static_cast<size_t>(HistogramType::kNumTypes)>
    kHistogramTypeNames = {
        {{"kCaptureLatencyMs", HistogramType::kCaptureLatencyMs},
         {"kEncodeTimeMs", HistogramType::kEncodeTimeMs},
         {"kQueueingLatencyMs", HistogramType::kQueueingLatencyMs},
         {"kNetworkLatencyMs", HistogramType::kNetworkLatencyMs},
         {"kPacketLatencyMs", HistogramType::kPacketLatencyMs},
         {"kEndToEndLatencyMs", HistogramType::kEndToEndLatencyMs},
         {"kFrameLatenessMs", HistogramType::kFrameLatenessMs}}};

SimpleHistogram::SimpleHistogram() = default;
SimpleHistogram::SimpleHistogram(int64_t min, int64_t max, int64_t width)

    : min(min), max(max), width(width), buckets((max - min) / width + 2) {
  OSP_CHECK_GT(buckets.size(), 2u);
  OSP_CHECK_EQ(0, (max - min) % width);
}

SimpleHistogram::SimpleHistogram(int64_t min,
                                 int64_t max,
                                 int64_t width,
                                 std::vector<int> buckets)
    : min(min), max(max), width(width), buckets(buckets) {}

SimpleHistogram::SimpleHistogram(const SimpleHistogram&) = default;
SimpleHistogram::SimpleHistogram(SimpleHistogram&&) noexcept = default;
SimpleHistogram& SimpleHistogram::operator=(const SimpleHistogram&) = default;
SimpleHistogram& SimpleHistogram::operator=(SimpleHistogram&&) = default;
SimpleHistogram::~SimpleHistogram() = default;

void SimpleHistogram::Add(int64_t sample) {
  if (sample < min) {
    ++buckets.front();
  } else if (sample >= max) {
    ++buckets.back();
  } else {
    size_t index = 1 + (sample - min) / width;
    OSP_CHECK_LT(index, buckets.size());
    ++buckets[index];
  }
}

void SimpleHistogram::Reset() {
  buckets.assign(buckets.size(), 0);
}

SimpleHistogram SimpleHistogram::Copy() {
  return SimpleHistogram(min, max, width, buckets);
}

Json::Value SimpleHistogram::ToJson() const {
  Json::Value out;
  out["min"] = min;
  out["max"] = max;
  out["width"] = width;
  out["buckets"] = json::PrimitiveVectorToJson(buckets);
  return out;
}

std::string SimpleHistogram::ToString() const {
  return json::Stringify(ToJson()).value();
}

Json::Value SenderStats::ToJson() const {
  Json::Value out;
  out["audio_statistics"] = ArrayToJson(audio_statistics, kStatisticTypeNames);
  out["audio_histograms"] = ArrayToJson(audio_histograms, kHistogramTypeNames);
  out["video_statistics"] = ArrayToJson(video_statistics, kStatisticTypeNames);
  out["video_histograms"] = ArrayToJson(video_histograms, kHistogramTypeNames);
  return out;
}

std::string SenderStats::ToString() const {
  return json::Stringify(ToJson()).value();
}

SenderStatsClient::~SenderStatsClient() {}

}  // namespace cast
}  // namespace openscreen
