// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/answer_messages.h"

#include <chrono>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "util/chrono_helpers.h"
#include "util/json/json_serialization.h"

namespace openscreen {
namespace cast {

namespace {

using ::testing::ElementsAre;

// NOTE: the castMode property has been removed from the specification. We leave
// it here in the valid offer to ensure that its inclusion does not break
// parsing.
constexpr char kValidAnswerJson[] = R"({
  "castMode": "mirroring",
  "udpPort": 1234,
  "sendIndexes": [1, 3],
  "ssrcs": [1233324, 2234222],
  "constraints": {
    "audio": {
      "maxSampleRate": 96000,
      "maxChannels": 5,
      "minBitRate": 32000,
      "maxBitRate": 320000,
      "maxDelay": 5000
    },
    "video": {
      "maxPixelsPerSecond": 62208000,
      "minResolution": {
        "width": 320,
        "height": 180,
        "frameRate": 0
      },
      "maxDimensions": {
        "width": 1920,
        "height": 1080,
        "frameRate": "60"
      },
      "minBitRate": 300000,
      "maxBitRate": 10000000,
      "maxDelay": 5000
    }
  },
  "display": {
    "dimensions": {
      "width": 1920,
      "height": 1080,
      "frameRate": "60000/1001"
    },
    "aspectRatio": "64:27",
    "scaling": "sender"
  },
  "receiverRtcpEventLog": [0, 1],
  "receiverRtcpDscp": [234, 567],
  "rtpExtensions": ["adaptive_playout_delay"]
})";

const Answer kValidAnswer{
    1234,                         // udp_port
    std::vector<int>{1, 2, 3},    // send_indexes
    std::vector<Ssrc>{123, 456},  // ssrcs
    absl::optional<Constraints>(Constraints{
        AudioConstraints{
            96000,              // max_sample_rate
            7,                  // max_channels
            32000,              // min_bit_rate
            96000,              // max_bit_rate
            milliseconds(2000)  // max_delay
        },                      // audio
        VideoConstraints{
            40000.0,  // max_pixels_per_second
            absl::optional<Dimensions>(
                Dimensions{320, 480, SimpleFraction{15000, 101}}),
            Dimensions{1920, 1080, SimpleFraction{288, 2}},
            300000,             // min_bit_rate
            144000000,          // max_bit_rate
            milliseconds(3000)  // max_delay
        }                       // video
    }),                         // constraints
    absl::optional<DisplayDescription>(DisplayDescription{
        absl::optional<Dimensions>(Dimensions{640, 480, SimpleFraction{30, 1}}),
        absl::optional<AspectRatio>(AspectRatio{16, 9}),  // aspect_ratio
        absl::optional<AspectRatioConstraint>(
            AspectRatioConstraint::kFixed),  // scaling
    }),
    std::vector<int>{7, 8, 9},              // receiver_rtcp_event_log
    std::vector<int>{11, 12, 13},           // receiver_rtcp_dscp
    std::vector<std::string>{"foo", "bar"}  // rtp_extensions
};

constexpr int kValidMaxPixelsPerSecond = 1920 * 1080 * 30;
constexpr Dimensions kValidDimensions{1920, 1080, SimpleFraction{60, 1}};
static const VideoConstraints kValidVideoConstraints{
    kValidMaxPixelsPerSecond, absl::optional<Dimensions>(kValidDimensions),
    kValidDimensions,         300 * 1000,
    300 * 1000 * 1000,        milliseconds(3000)};

constexpr AudioConstraints kValidAudioConstraints{123, 456, 300, 9920,
                                                  milliseconds(123)};

void ExpectEqualsValidAnswerJson(const Answer& answer) {
  EXPECT_EQ(1234, answer.udp_port);

  EXPECT_THAT(answer.send_indexes, ElementsAre(1, 3));
  EXPECT_THAT(answer.ssrcs, ElementsAre(1233324u, 2234222u));
  ASSERT_TRUE(answer.constraints.has_value());
  const AudioConstraints& audio = answer.constraints->audio;
  EXPECT_EQ(96000, audio.max_sample_rate);
  EXPECT_EQ(5, audio.max_channels);
  EXPECT_EQ(32000, audio.min_bit_rate);
  EXPECT_EQ(320000, audio.max_bit_rate);
  EXPECT_EQ(milliseconds{5000}, audio.max_delay);

  const VideoConstraints& video = answer.constraints->video;
  EXPECT_EQ(62208000, video.max_pixels_per_second);
  ASSERT_TRUE(video.min_resolution.has_value());
  EXPECT_EQ(320, video.min_resolution->width);
  EXPECT_EQ(180, video.min_resolution->height);
  EXPECT_EQ((SimpleFraction{0, 1}), video.min_resolution->frame_rate);
  EXPECT_EQ(1920, video.max_dimensions.width);
  EXPECT_EQ(1080, video.max_dimensions.height);
  EXPECT_EQ((SimpleFraction{60, 1}), video.max_dimensions.frame_rate);
  EXPECT_EQ(300000, video.min_bit_rate);
  EXPECT_EQ(10000000, video.max_bit_rate);
  EXPECT_EQ(milliseconds{5000}, video.max_delay);

  ASSERT_TRUE(answer.display.has_value());
  const DisplayDescription& display = answer.display.value();
  ASSERT_TRUE(display.dimensions.has_value());
  EXPECT_EQ(1920, display.dimensions->width);
  EXPECT_EQ(1080, display.dimensions->height);
  EXPECT_EQ((SimpleFraction{60000, 1001}), display.dimensions->frame_rate);
  EXPECT_EQ((AspectRatio{64, 27}), display.aspect_ratio.value());
  EXPECT_EQ(AspectRatioConstraint::kFixed,
            display.aspect_ratio_constraint.value());

  EXPECT_THAT(answer.receiver_rtcp_event_log, ElementsAre(0, 1));
  EXPECT_THAT(answer.receiver_rtcp_dscp, ElementsAre(234, 567));
  EXPECT_THAT(answer.rtp_extensions, ElementsAre("adaptive_playout_delay"));
}

void ExpectFailureOnParse(absl::string_view raw_json) {
  ErrorOr<Json::Value> root = json::Parse(raw_json);
  // Must be a valid JSON object, but not a valid answer.
  ASSERT_TRUE(root.is_value());

  Answer answer;
  EXPECT_FALSE(Answer::TryParse(std::move(root.value()), &answer));
  EXPECT_FALSE(answer.IsValid());
}

// Functions that use ASSERT_* must return void, so we use an out parameter
// here instead of returning.
void ExpectSuccessOnParse(absl::string_view raw_json, Answer* out = nullptr) {
  ErrorOr<Json::Value> root = json::Parse(raw_json);
  // Must be a valid JSON object, but not a valid answer.
  ASSERT_TRUE(root.is_value());

  Answer answer;
  ASSERT_TRUE(Answer::TryParse(std::move(root.value()), &answer));
  EXPECT_TRUE(answer.IsValid());
  if (out) {
    *out = std::move(answer);
  }
}

}  // anonymous namespace

TEST(AnswerMessagesTest, ProperlyPopulatedAnswerSerializesProperly) {
  ASSERT_TRUE(kValidAnswer.IsValid());
  Json::Value root = kValidAnswer.ToJson();
  EXPECT_EQ(root["udpPort"], 1234);

  Json::Value sendIndexes = std::move(root["sendIndexes"]);
  EXPECT_EQ(sendIndexes.type(), Json::ValueType::arrayValue);
  EXPECT_EQ(sendIndexes[0], 1);
  EXPECT_EQ(sendIndexes[1], 2);
  EXPECT_EQ(sendIndexes[2], 3);

  Json::Value ssrcs = std::move(root["ssrcs"]);
  EXPECT_EQ(ssrcs.type(), Json::ValueType::arrayValue);
  EXPECT_EQ(ssrcs[0], 123u);
  EXPECT_EQ(ssrcs[1], 456u);

  Json::Value constraints = std::move(root["constraints"]);
  Json::Value audio = std::move(constraints["audio"]);
  EXPECT_EQ(audio.type(), Json::ValueType::objectValue);
  EXPECT_EQ(audio["maxSampleRate"], 96000);
  EXPECT_EQ(audio["maxChannels"], 7);
  EXPECT_EQ(audio["minBitRate"], 32000);
  EXPECT_EQ(audio["maxBitRate"], 96000);
  EXPECT_EQ(audio["maxDelay"], 2000);

  Json::Value video = std::move(constraints["video"]);
  EXPECT_EQ(video.type(), Json::ValueType::objectValue);
  EXPECT_EQ(video["maxPixelsPerSecond"], 40000.0);
  EXPECT_EQ(video["minBitRate"], 300000);
  EXPECT_EQ(video["maxBitRate"], 144000000);
  EXPECT_EQ(video["maxDelay"], 3000);

  Json::Value min_resolution = std::move(video["minResolution"]);
  EXPECT_EQ(min_resolution.type(), Json::ValueType::objectValue);
  EXPECT_EQ(min_resolution["width"], 320);
  EXPECT_EQ(min_resolution["height"], 480);
  EXPECT_EQ(min_resolution["frameRate"], "15000/101");

  Json::Value max_dimensions = std::move(video["maxDimensions"]);
  EXPECT_EQ(max_dimensions.type(), Json::ValueType::objectValue);
  EXPECT_EQ(max_dimensions["width"], 1920);
  EXPECT_EQ(max_dimensions["height"], 1080);
  EXPECT_EQ(max_dimensions["frameRate"], "288/2");

  Json::Value display = std::move(root["display"]);
  EXPECT_EQ(display.type(), Json::ValueType::objectValue);
  EXPECT_EQ(display["aspectRatio"], "16:9");
  EXPECT_EQ(display["scaling"], "sender");

  Json::Value dimensions = std::move(display["dimensions"]);
  EXPECT_EQ(dimensions.type(), Json::ValueType::objectValue);
  EXPECT_EQ(dimensions["width"], 640);
  EXPECT_EQ(dimensions["height"], 480);
  EXPECT_EQ(dimensions["frameRate"], "30");

  Json::Value receiver_rtcp_event_log = std::move(root["receiverRtcpEventLog"]);
  EXPECT_EQ(receiver_rtcp_event_log.type(), Json::ValueType::arrayValue);
  EXPECT_EQ(receiver_rtcp_event_log[0], 7);
  EXPECT_EQ(receiver_rtcp_event_log[1], 8);
  EXPECT_EQ(receiver_rtcp_event_log[2], 9);

  Json::Value receiver_rtcp_dscp = std::move(root["receiverRtcpDscp"]);
  EXPECT_EQ(receiver_rtcp_dscp.type(), Json::ValueType::arrayValue);
  EXPECT_EQ(receiver_rtcp_dscp[0], 11);
  EXPECT_EQ(receiver_rtcp_dscp[1], 12);
  EXPECT_EQ(receiver_rtcp_dscp[2], 13);

  Json::Value rtp_extensions = std::move(root["rtpExtensions"]);
  EXPECT_EQ(rtp_extensions.type(), Json::ValueType::arrayValue);
  EXPECT_EQ(rtp_extensions[0], "foo");
  EXPECT_EQ(rtp_extensions[1], "bar");
}

TEST(AnswerMessagesTest, EmptyArraysOmitted) {
  Answer missing_event_log = kValidAnswer;
  missing_event_log.receiver_rtcp_event_log.clear();
  ASSERT_TRUE(missing_event_log.IsValid());
  Json::Value root = missing_event_log.ToJson();
  EXPECT_FALSE(root["receiverRtcpEventLog"]);

  Answer missing_rtcp_dscp = kValidAnswer;
  missing_rtcp_dscp.receiver_rtcp_dscp.clear();
  ASSERT_TRUE(missing_rtcp_dscp.IsValid());
  root = missing_rtcp_dscp.ToJson();
  EXPECT_FALSE(root["receiverRtcpDscp"]);

  Answer missing_extensions = kValidAnswer;
  missing_extensions.rtp_extensions.clear();
  ASSERT_TRUE(missing_extensions.IsValid());
  root = missing_extensions.ToJson();
  EXPECT_FALSE(root["rtpExtensions"]);
}

TEST(AnswerMessagesTest, InvalidDimensionsCauseInvalid) {
  Answer invalid_dimensions = kValidAnswer;
  invalid_dimensions.display->dimensions->width = -1;
  EXPECT_FALSE(invalid_dimensions.IsValid());
}

TEST(AnswerMessagesTest, InvalidAudioConstraintsCauseError) {
  Answer invalid_audio = kValidAnswer;
  invalid_audio.constraints->audio.max_bit_rate =
      invalid_audio.constraints->audio.min_bit_rate - 1;
  EXPECT_FALSE(invalid_audio.IsValid());
}

TEST(AnswerMessagesTest, InvalidVideoConstraintsCauseError) {
  Answer invalid_video = kValidAnswer;
  invalid_video.constraints->video.max_pixels_per_second = -1.0;
  EXPECT_FALSE(invalid_video.IsValid());
}

TEST(AnswerMessagesTest, InvalidDisplayDescriptionsCauseError) {
  Answer invalid_display = kValidAnswer;
  invalid_display.display->aspect_ratio = {0, 0};
  EXPECT_FALSE(invalid_display.IsValid());
}

TEST(AnswerMessagesTest, InvalidUdpPortsCauseError) {
  Answer invalid_port = kValidAnswer;
  invalid_port.udp_port = 65536;
  EXPECT_FALSE(invalid_port.IsValid());
}

TEST(AnswerMessagesTest, CanParseValidAnswerJson) {
  Answer answer;
  ExpectSuccessOnParse(kValidAnswerJson, &answer);
  ExpectEqualsValidAnswerJson(answer);
}

// In practice, the rtpExtensions, receiverRtcpDscp, and receiverRtcpEventLog
// fields may be missing from some receivers. We handle this case by treating
// them as empty.
TEST(AnswerMessagesTest, SucceedsWithMissingRtpFields) {
  ExpectSuccessOnParse(R"({
  "udpPort": 1234,
  "sendIndexes": [1, 3],
  "ssrcs": [1233324, 2234222]
  })");
}

TEST(AnswerMessagesTest, ErrorOnEmptyAnswer) {
  ExpectFailureOnParse("{}");
}

TEST(AnswerMessagesTest, ErrorOnMissingUdpPort) {
  ExpectFailureOnParse(R"({
    "sendIndexes": [1, 3],
    "ssrcs": [1233324, 2234222]
  })");
}

TEST(AnswerMessagesTest, ErrorOnMissingSsrcs) {
  ExpectFailureOnParse(R"({
    "udpPort": 1234,
    "sendIndexes": [1, 3]
  })");
}

TEST(AnswerMessagesTest, ErrorOnMissingSendIndexes) {
  ExpectFailureOnParse(R"({
    "udpPort": 1234,
    "ssrcs": [1233324, 2234222]
  })");
}

TEST(AnswerMessagesTest, AllowsReceiverSideScaling) {
  Answer answer;
  ExpectSuccessOnParse(R"({
  "udpPort": 1234,
  "sendIndexes": [1, 3],
  "ssrcs": [1233324, 2234222],
  "display": {
    "dimensions": {
      "width": 1920,
      "height": 1080,
      "frameRate": "60000/1001"
    },
    "aspectRatio": "64:27",
    "scaling": "receiver"
    }
  })",
                       &answer);
  ASSERT_TRUE(answer.display.has_value());
  EXPECT_EQ(answer.display->aspect_ratio_constraint.value(),
            AspectRatioConstraint::kVariable);
}

TEST(AnswerMessagesTest, AssumesMinBitRateIfOmitted) {
  Answer answer;
  ExpectSuccessOnParse(R"({
    "udpPort": 1234,
    "sendIndexes": [1, 3],
    "ssrcs": [1233324, 2234222],
    "constraints": {
      "audio": {
        "maxSampleRate": 96000,
        "maxChannels": 5,
        "maxBitRate": 320000,
        "maxDelay": 5000
      },
      "video": {
        "maxPixelsPerSecond": 62208000,
        "maxDimensions": {
          "width": 1920,
          "height": 1080,
          "frameRate": "60"
        },
        "maxBitRate": 10000000,
        "maxDelay": 5000
      }
    }
  })",
                       &answer);

  EXPECT_EQ(32000, answer.constraints->audio.min_bit_rate);
  EXPECT_EQ(300000, answer.constraints->video.min_bit_rate);
}

// Instead of testing all possible json parsing options for validity, we
// can instead directly test the IsValid() methods.
TEST(AnswerMessagesTest, AudioConstraintsIsValid) {
  constexpr AudioConstraints kInvalidSampleRate{0, 456, 300, 9920,
                                                milliseconds(123)};
  constexpr AudioConstraints kInvalidMaxChannels{123, 0, 300, 9920,
                                                 milliseconds(123)};
  constexpr AudioConstraints kInvalidMinBitRate{123, 456, 0, 9920,
                                                milliseconds(123)};
  constexpr AudioConstraints kInvalidMaxBitRate{123, 456, 300, 0,
                                                milliseconds(123)};
  constexpr AudioConstraints kInvalidMaxDelay{123, 456, 300, 0,
                                              milliseconds(0)};

  EXPECT_TRUE(kValidAudioConstraints.IsValid());
  EXPECT_FALSE(kInvalidSampleRate.IsValid());
  EXPECT_FALSE(kInvalidMaxChannels.IsValid());
  EXPECT_FALSE(kInvalidMinBitRate.IsValid());
  EXPECT_FALSE(kInvalidMaxBitRate.IsValid());
  EXPECT_FALSE(kInvalidMaxDelay.IsValid());
}

TEST(AnswerMessagesTest, DimensionsIsValid) {
  // NOTE: in some cases (such as min dimensions) a frame rate of zero is valid.
  constexpr Dimensions kValidZeroFrameRate{1920, 1080, SimpleFraction{0, 60}};
  constexpr Dimensions kInvalidWidth{0, 1080, SimpleFraction{60, 1}};
  constexpr Dimensions kInvalidHeight{1920, 0, SimpleFraction{60, 1}};
  constexpr Dimensions kInvalidFrameRateZeroDenominator{1920, 1080,
                                                        SimpleFraction{60, 0}};
  constexpr Dimensions kInvalidFrameRateNegativeNumerator{
      1920, 1080, SimpleFraction{-1, 30}};
  constexpr Dimensions kInvalidFrameRateNegativeDenominator{
      1920, 1080, SimpleFraction{30, -1}};

  EXPECT_TRUE(kValidDimensions.IsValid());
  EXPECT_TRUE(kValidZeroFrameRate.IsValid());
  EXPECT_FALSE(kInvalidWidth.IsValid());
  EXPECT_FALSE(kInvalidHeight.IsValid());
  EXPECT_FALSE(kInvalidFrameRateZeroDenominator.IsValid());
  EXPECT_FALSE(kInvalidFrameRateNegativeNumerator.IsValid());
  EXPECT_FALSE(kInvalidFrameRateNegativeDenominator.IsValid());
}

TEST(AnswerMessagesTest, VideoConstraintsIsValid) {
  VideoConstraints invalid_max_pixels_per_second = kValidVideoConstraints;
  invalid_max_pixels_per_second.max_pixels_per_second = 0;

  VideoConstraints invalid_min_resolution = kValidVideoConstraints;
  invalid_min_resolution.min_resolution->width = 0;

  VideoConstraints invalid_max_dimensions = kValidVideoConstraints;
  invalid_max_dimensions.max_dimensions.height = 0;

  VideoConstraints invalid_min_bit_rate = kValidVideoConstraints;
  invalid_min_bit_rate.min_bit_rate = 0;

  VideoConstraints invalid_max_bit_rate = kValidVideoConstraints;
  invalid_max_bit_rate.max_bit_rate = invalid_max_bit_rate.min_bit_rate - 1;

  VideoConstraints invalid_max_delay = kValidVideoConstraints;
  invalid_max_delay.max_delay = milliseconds(0);

  EXPECT_TRUE(kValidVideoConstraints.IsValid());
  EXPECT_FALSE(invalid_max_pixels_per_second.IsValid());
  EXPECT_FALSE(invalid_min_resolution.IsValid());
  EXPECT_FALSE(invalid_max_dimensions.IsValid());
  EXPECT_FALSE(invalid_min_bit_rate.IsValid());
  EXPECT_FALSE(invalid_max_bit_rate.IsValid());
  EXPECT_FALSE(invalid_max_delay.IsValid());
}

TEST(AnswerMessagesTest, ConstraintsIsValid) {
  VideoConstraints invalid_video_constraints = kValidVideoConstraints;
  invalid_video_constraints.max_pixels_per_second = 0;

  AudioConstraints invalid_audio_constraints = kValidAudioConstraints;
  invalid_audio_constraints.max_bit_rate = 0;

  const Constraints valid{kValidAudioConstraints, kValidVideoConstraints};
  const Constraints invalid_audio{kValidAudioConstraints,
                                  invalid_video_constraints};
  const Constraints invalid_video{invalid_audio_constraints,
                                  kValidVideoConstraints};

  EXPECT_TRUE(valid.IsValid());
  EXPECT_FALSE(invalid_audio.IsValid());
  EXPECT_FALSE(invalid_video.IsValid());
}

TEST(AnswerMessagesTest, AspectRatioIsValid) {
  constexpr AspectRatio kValid{16, 9};
  constexpr AspectRatio kInvalidWidth{0, 9};
  constexpr AspectRatio kInvalidHeight{16, 0};

  EXPECT_TRUE(kValid.IsValid());
  EXPECT_FALSE(kInvalidWidth.IsValid());
  EXPECT_FALSE(kInvalidHeight.IsValid());
}

TEST(AnswerMessagesTest, AspectRatioTryParse) {
  const Json::Value kValid = "16:9";
  const Json::Value kWrongDelimiter = "16-9";
  const Json::Value kTooManyFields = "16:9:3";
  const Json::Value kTooFewFields = "1:";
  const Json::Value kNoDelimiter = "12345";
  const Json::Value kNegativeWidth = "-123:2345";
  const Json::Value kNegativeHeight = "22:-7";
  const Json::Value kNegativeBoth = "22:-7";
  const Json::Value kNonNumberWidth = "twenty2#:9";
  const Json::Value kNonNumberHeight = "2:thirty";
  const Json::Value kZeroWidth = "0:9";
  const Json::Value kZeroHeight = "16:0";

  AspectRatio out;
  EXPECT_TRUE(AspectRatio::TryParse(kValid, &out));
  EXPECT_EQ(out.width, 16);
  EXPECT_EQ(out.height, 9);
  EXPECT_FALSE(AspectRatio::TryParse(kWrongDelimiter, &out));
  EXPECT_FALSE(AspectRatio::TryParse(kTooManyFields, &out));
  EXPECT_FALSE(AspectRatio::TryParse(kTooFewFields, &out));
  EXPECT_FALSE(AspectRatio::TryParse(kWrongDelimiter, &out));
  EXPECT_FALSE(AspectRatio::TryParse(kNoDelimiter, &out));
  EXPECT_FALSE(AspectRatio::TryParse(kNegativeWidth, &out));
  EXPECT_FALSE(AspectRatio::TryParse(kNegativeHeight, &out));
  EXPECT_FALSE(AspectRatio::TryParse(kNegativeBoth, &out));
  EXPECT_FALSE(AspectRatio::TryParse(kNonNumberWidth, &out));
  EXPECT_FALSE(AspectRatio::TryParse(kNonNumberHeight, &out));
  EXPECT_FALSE(AspectRatio::TryParse(kZeroWidth, &out));
  EXPECT_FALSE(AspectRatio::TryParse(kZeroHeight, &out));
}

TEST(AnswerMessagesTest, DisplayDescriptionTryParse) {
  Json::Value valid_scaling;
  valid_scaling["scaling"] = "receiver";
  Json::Value invalid_scaling;
  invalid_scaling["scaling"] = "embedder";
  Json::Value invalid_scaling_valid_ratio;
  invalid_scaling_valid_ratio["scaling"] = "embedder";
  invalid_scaling_valid_ratio["aspectRatio"] = "16:9";

  Json::Value dimensions;
  dimensions["width"] = 1920;
  dimensions["height"] = 1080;
  dimensions["frameRate"] = "30";
  Json::Value valid_dimensions;
  valid_dimensions["dimensions"] = dimensions;

  Json::Value dimensions_invalid = dimensions;
  dimensions_invalid["frameRate"] = "infinity";
  Json::Value invalid_dimensions;
  invalid_dimensions["dimensions"] = dimensions_invalid;

  Json::Value aspect_ratio_and_constraint;
  aspect_ratio_and_constraint["scaling"] = "sender";
  aspect_ratio_and_constraint["aspectRatio"] = "4:3";

  DisplayDescription out;
  ASSERT_TRUE(DisplayDescription::TryParse(valid_scaling, &out));
  ASSERT_TRUE(out.aspect_ratio_constraint.has_value());
  EXPECT_EQ(out.aspect_ratio_constraint.value(),
            AspectRatioConstraint::kVariable);

  EXPECT_FALSE(DisplayDescription::TryParse(invalid_scaling, &out));
  EXPECT_TRUE(DisplayDescription::TryParse(invalid_scaling_valid_ratio, &out));

  ASSERT_TRUE(DisplayDescription::TryParse(valid_dimensions, &out));
  ASSERT_TRUE(out.dimensions.has_value());
  EXPECT_EQ(1920, out.dimensions->width);
  EXPECT_EQ(1080, out.dimensions->height);
  EXPECT_EQ((SimpleFraction{30, 1}), out.dimensions->frame_rate);

  EXPECT_FALSE(DisplayDescription::TryParse(invalid_dimensions, &out));

  ASSERT_TRUE(DisplayDescription::TryParse(aspect_ratio_and_constraint, &out));
  EXPECT_EQ(AspectRatioConstraint::kFixed, out.aspect_ratio_constraint.value());
}

TEST(AnswerMessagesTest, DisplayDescriptionIsValid) {
  const DisplayDescription kInvalidEmptyDescription{
      absl::optional<Dimensions>{}, absl::optional<AspectRatio>{},
      absl::optional<AspectRatioConstraint>{}};

  DisplayDescription has_valid_dimensions = kInvalidEmptyDescription;
  has_valid_dimensions.dimensions =
      absl::optional<Dimensions>(kValidDimensions);

  DisplayDescription has_invalid_dimensions = kInvalidEmptyDescription;
  has_invalid_dimensions.dimensions =
      absl::optional<Dimensions>(kValidDimensions);
  has_invalid_dimensions.dimensions->width = 0;

  DisplayDescription has_aspect_ratio = kInvalidEmptyDescription;
  has_aspect_ratio.aspect_ratio =
      absl::optional<AspectRatio>{AspectRatio{16, 9}};

  DisplayDescription has_invalid_aspect_ratio = kInvalidEmptyDescription;
  has_invalid_aspect_ratio.aspect_ratio =
      absl::optional<AspectRatio>{AspectRatio{0, 20}};

  DisplayDescription has_aspect_ratio_constraint = kInvalidEmptyDescription;
  has_aspect_ratio_constraint.aspect_ratio_constraint =
      absl::optional<AspectRatioConstraint>(AspectRatioConstraint::kFixed);

  DisplayDescription has_constraint_and_dimensions =
      has_aspect_ratio_constraint;
  has_constraint_and_dimensions.dimensions =
      absl::optional<Dimensions>(kValidDimensions);

  DisplayDescription has_constraint_and_ratio = has_aspect_ratio_constraint;
  has_constraint_and_ratio.aspect_ratio = AspectRatio{4, 3};

  EXPECT_FALSE(kInvalidEmptyDescription.IsValid());
  EXPECT_TRUE(has_valid_dimensions.IsValid());
  EXPECT_FALSE(has_invalid_dimensions.IsValid());
  EXPECT_TRUE(has_aspect_ratio.IsValid());
  EXPECT_FALSE(has_invalid_aspect_ratio.IsValid());
  EXPECT_FALSE(has_aspect_ratio_constraint.IsValid());
  EXPECT_TRUE(has_constraint_and_dimensions.IsValid());
}

// Instead of being tested here, Answer's IsValid is checked in all other
// relevant tests.

}  // namespace cast
}  // namespace openscreen
