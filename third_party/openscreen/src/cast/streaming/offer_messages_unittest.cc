// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/offer_messages.h"

#include <limits>
#include <utility>

#include "cast/streaming/rtp_defines.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "util/json/json_serialization.h"

using ::testing::ElementsAre;

namespace openscreen {
namespace cast {

namespace {

constexpr char kValidOffer[] = R"({
  "castMode": "mirroring",
  "supportedStreams": [
    {
      "index": 0,
      "type": "video_source",
      "codecName": "h264",
      "rtpProfile": "cast",
      "rtpPayloadType": 101,
      "ssrc": 19088743,
      "maxFrameRate": "60000/1000",
      "timeBase": "1/90000",
      "maxBitRate": 5000000,
      "profile": "main",
      "level": "4",
      "targetDelay": 200,
      "aesKey": "040d756791711fd3adb939066e6d8690",
      "aesIvMask": "9ff0f022a959150e70a2d05a6c184aed",
      "resolutions": [
        {
          "width": 1280,
          "height": 720
        },
        {
          "width": 640,
          "height": 360
        },
        {
          "width": 640,
          "height": 480
        }
      ]
    },
    {
      "index": 1,
      "type": "video_source",
      "codecName": "vp8",
      "rtpProfile": "cast",
      "rtpPayloadType": 100,
      "ssrc": 19088744,
      "maxFrameRate": "30000/1001",
      "targetDelay": 1000,
      "timeBase": "1/90000",
      "maxBitRate": 5000000,
      "profile": "main",
      "level": "5",
      "aesKey": "bbf109bf84513b456b13a184453b66ce",
      "aesIvMask": "edaf9e4536e2b66191f560d9c04b2a69"
    },
    {
      "index": 2,
      "type": "audio_source",
      "codecName": "opus",
      "targetDelay": 300,
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "ssrc": 4294967295,
      "bitRate": 124000,
      "timeBase": "1/48000",
      "channels": 2,
      "aesKey": "51027e4e2347cbcb49d57ef10177aebc",
      "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
    },
    {
      "index": 3,
      "type": "video_source",
      "codecName": "av1",
      "rtpProfile": "cast",
      "rtpPayloadType": 104,
      "ssrc": 19088744,
      "maxFrameRate": "30000/1001",
      "targetDelay": 1000,
      "timeBase": "1/90000",
      "maxBitRate": 5000000,
      "profile": "main",
      "level": "5",
      "aesKey": "bbf109bf84513b456b13a184453b66ce",
      "aesIvMask": "edaf9e4536e2b66191f560d9c04b2a69"
    }
  ]
})";

void ExpectFailureOnParse(
    absl::string_view body,
    absl::optional<Error::Code> expected = absl::nullopt) {
  ErrorOr<Json::Value> root = json::Parse(body);
  ASSERT_TRUE(root.is_value()) << root.error();

  Offer offer;
  Error error = Offer::TryParse(std::move(root.value()), &offer);
  EXPECT_FALSE(error.ok());
  if (expected) {
    EXPECT_EQ(expected, error.code());
  }
}

void ExpectEqualsValidOffer(const Offer& offer) {
  EXPECT_EQ(CastMode::kMirroring, offer.cast_mode);

  // Verify list of video streams.
  EXPECT_EQ(3u, offer.video_streams.size());
  const auto& video_streams = offer.video_streams;

  const bool flipped = video_streams[0].stream.index != 0;
  const VideoStream& vs_one = flipped ? video_streams[2] : video_streams[0];
  const VideoStream& vs_two = video_streams[1];
  const VideoStream& vs_three = flipped ? video_streams[0] : video_streams[2];

  EXPECT_EQ(0, vs_one.stream.index);
  EXPECT_EQ(1, vs_one.stream.channels);
  EXPECT_EQ(Stream::Type::kVideoSource, vs_one.stream.type);
  EXPECT_EQ(VideoCodec::kH264, vs_one.codec);
  EXPECT_EQ(RtpPayloadType::kVideoH264, vs_one.stream.rtp_payload_type);
  EXPECT_EQ(19088743u, vs_one.stream.ssrc);
  EXPECT_EQ((SimpleFraction{60000, 1000}), vs_one.max_frame_rate);
  EXPECT_EQ(90000, vs_one.stream.rtp_timebase);
  EXPECT_EQ(5000000, vs_one.max_bit_rate);
  EXPECT_EQ("main", vs_one.profile);
  EXPECT_EQ("4", vs_one.level);
  EXPECT_THAT(vs_one.stream.aes_key,
              ElementsAre(0x04, 0x0d, 0x75, 0x67, 0x91, 0x71, 0x1f, 0xd3, 0xad,
                          0xb9, 0x39, 0x06, 0x6e, 0x6d, 0x86, 0x90));
  EXPECT_THAT(vs_one.stream.aes_iv_mask,
              ElementsAre(0x9f, 0xf0, 0xf0, 0x22, 0xa9, 0x59, 0x15, 0x0e, 0x70,
                          0xa2, 0xd0, 0x5a, 0x6c, 0x18, 0x4a, 0xed));

  const auto& resolutions = vs_one.resolutions;
  EXPECT_EQ(3u, resolutions.size());
  const Resolution& r_one = resolutions[0];
  EXPECT_EQ(1280, r_one.width);
  EXPECT_EQ(720, r_one.height);

  const Resolution& r_two = resolutions[1];
  EXPECT_EQ(640, r_two.width);
  EXPECT_EQ(360, r_two.height);

  const Resolution& r_three = resolutions[2];
  EXPECT_EQ(640, r_three.width);
  EXPECT_EQ(480, r_three.height);

  EXPECT_EQ(1, vs_two.stream.index);
  EXPECT_EQ(1, vs_two.stream.channels);
  EXPECT_EQ(Stream::Type::kVideoSource, vs_two.stream.type);
  EXPECT_EQ(VideoCodec::kVp8, vs_two.codec);
  EXPECT_EQ(RtpPayloadType::kVideoVp8, vs_two.stream.rtp_payload_type);
  EXPECT_EQ(19088744u, vs_two.stream.ssrc);
  EXPECT_EQ((SimpleFraction{30000, 1001}), vs_two.max_frame_rate);
  EXPECT_EQ(90000, vs_two.stream.rtp_timebase);
  EXPECT_EQ(5000000, vs_two.max_bit_rate);
  EXPECT_EQ("main", vs_two.profile);
  EXPECT_EQ("5", vs_two.level);
  EXPECT_THAT(vs_two.stream.aes_key,
              ElementsAre(0xbb, 0xf1, 0x09, 0xbf, 0x84, 0x51, 0x3b, 0x45, 0x6b,
                          0x13, 0xa1, 0x84, 0x45, 0x3b, 0x66, 0xce));
  EXPECT_THAT(vs_two.stream.aes_iv_mask,
              ElementsAre(0xed, 0xaf, 0x9e, 0x45, 0x36, 0xe2, 0xb6, 0x61, 0x91,
                          0xf5, 0x60, 0xd9, 0xc0, 0x4b, 0x2a, 0x69));

  const auto& resolutions_two = vs_two.resolutions;
  EXPECT_EQ(0u, resolutions_two.size());

  EXPECT_EQ(3, vs_three.stream.index);
  EXPECT_EQ(1, vs_three.stream.channels);
  EXPECT_EQ(Stream::Type::kVideoSource, vs_three.stream.type);
  EXPECT_EQ(VideoCodec::kAv1, vs_three.codec);
  EXPECT_EQ(RtpPayloadType::kVideoAv1, vs_three.stream.rtp_payload_type);
  EXPECT_EQ(19088744u, vs_three.stream.ssrc);
  EXPECT_EQ((SimpleFraction{30000, 1001}), vs_three.max_frame_rate);
  EXPECT_EQ(90000, vs_three.stream.rtp_timebase);
  EXPECT_EQ(5000000, vs_three.max_bit_rate);
  EXPECT_EQ("main", vs_three.profile);
  EXPECT_EQ("5", vs_three.level);
  EXPECT_THAT(vs_three.stream.aes_key,
              ElementsAre(0xbb, 0xf1, 0x09, 0xbf, 0x84, 0x51, 0x3b, 0x45, 0x6b,
                          0x13, 0xa1, 0x84, 0x45, 0x3b, 0x66, 0xce));
  EXPECT_THAT(vs_three.stream.aes_iv_mask,
              ElementsAre(0xed, 0xaf, 0x9e, 0x45, 0x36, 0xe2, 0xb6, 0x61, 0x91,
                          0xf5, 0x60, 0xd9, 0xc0, 0x4b, 0x2a, 0x69));

  const auto& resolutions_three = vs_three.resolutions;
  EXPECT_EQ(0u, resolutions_three.size());

  // Verify list of audio streams.
  EXPECT_EQ(1u, offer.audio_streams.size());
  const AudioStream& as = offer.audio_streams[0];
  EXPECT_EQ(2, as.stream.index);
  EXPECT_EQ(Stream::Type::kAudioSource, as.stream.type);
  EXPECT_EQ(AudioCodec::kOpus, as.codec);
  EXPECT_EQ(RtpPayloadType::kAudioOpus, as.stream.rtp_payload_type);
  EXPECT_EQ(std::numeric_limits<Ssrc>::max(), as.stream.ssrc);
  EXPECT_EQ(124000, as.bit_rate);
  EXPECT_EQ(2, as.stream.channels);

  EXPECT_THAT(as.stream.aes_key,
              ElementsAre(0x51, 0x02, 0x7e, 0x4e, 0x23, 0x47, 0xcb, 0xcb, 0x49,
                          0xd5, 0x7e, 0xf1, 0x01, 0x77, 0xae, 0xbc));
  EXPECT_THAT(as.stream.aes_iv_mask,
              ElementsAre(0x7f, 0x12, 0xa1, 0x9b, 0xe6, 0x2a, 0x36, 0xc0, 0x4a,
                          0xe4, 0x11, 0x6c, 0xaa, 0xef, 0xf6, 0xd1));
}

}  // namespace

TEST(OfferTest, ErrorOnEmptyOffer) {
  ExpectFailureOnParse("{}");
}

TEST(OfferTest, ErrorOnMissingMandatoryFields) {
  // It's okay if castMode is omitted, but if supportedStreams is omitted we
  // should fail here.
  ExpectFailureOnParse(R"({
    "castMode": "mirroring"
  })");
}

TEST(OfferTest, CanParseValidButStreamlessOffer) {
  ErrorOr<Json::Value> root = json::Parse(R"({
    "castMode": "mirroring",
    "supportedStreams": []
  })");
  ASSERT_TRUE(root.is_value()) << root.error();

  Offer offer;
  EXPECT_TRUE(Offer::TryParse(std::move(root.value()), &offer).ok());
}

TEST(OfferTest, ErrorOnMissingAudioStreamMandatoryField) {
  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "codecName": "opus",
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "ssrc": 19088743,
      "bitRate": 124000,
      "timeBase": "1/48000",
      "channels": 2
    }]})");

  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "audio_source",
      "codecName": "opus",
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "bitRate": 124000,
      "timeBase": "1/48000",
      "channels": 2
    }]})");
}

TEST(OfferTest, CanParseValidButMinimalAudioOffer) {
  ErrorOr<Json::Value> root = json::Parse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "audio_source",
      "codecName": "opus",
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "ssrc": 19088743,
      "bitRate": 124000,
      "timeBase": "1/48000",
      "channels": 2,
      "aesKey": "51027e4e2347cbcb49d57ef10177aebc",
      "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
    }]
  })");
  ASSERT_TRUE(root.is_value());
  Offer offer;
  EXPECT_TRUE(Offer::TryParse(std::move(root.value()), &offer).ok());
}

TEST(OfferTest, CanParseValidZeroBitRateAudioOffer) {
  ErrorOr<Json::Value> root = json::Parse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "audio_source",
      "codecName": "opus",
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "ssrc": 19088743,
      "bitRate": 0,
      "timeBase": "1/48000",
      "channels": 5,
      "aesKey": "51029e4e2347cbcb49d57ef10177aebd",
      "aesIvMask": "7f12a19be62a36c04ae4116caaeff5d2"
    }]
  })");
  ASSERT_TRUE(root.is_value()) << root.error();
  Offer offer;
  EXPECT_TRUE(Offer::TryParse(std::move(root.value()), &offer).ok());
}

TEST(OfferTest, ErrorOnInvalidRtpTimebase) {
  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "audio_source",
      "codecName": "opus",
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "ssrc": 19088743,
      "bitRate": 124000,
      "timeBase": "1/10000000",
      "channels": 2,
      "aesKey": "51027e4e2347cbcb49d57ef10177aebc",
      "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
    }]
  })");

  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "audio_source",
      "codecName": "opus",
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "ssrc": 19088743,
      "bitRate": 124000,
      "timeBase": "0",
      "channels": 2,
      "aesKey": "51027e4e2347cbcb49d57ef10177aebc",
      "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
    }]
  })");

  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "audio_source",
      "codecName": "opus",
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "ssrc": 19088743,
      "bitRate": 124000,
      "timeBase": "1/1",
      "channels": 2,
      "aesKey": "51027e4e2347cbcb49d57ef10177aebc",
      "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
    }]
  })");

  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "audio_source",
      "codecName": "opus",
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "ssrc": 19088743,
      "bitRate": 124000,
      "timeBase": "really fast plz, kthx",
      "channels": 2,
      "aesKey": "51027e4e2347cbcb49d57ef10177aebc",
      "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
    }]
  })");
}

TEST(OfferTest, ErrorOnMissingVideoStreamMandatoryField) {
  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "codecName": "video_source",
      "rtpProfile": "h264",
      "rtpPayloadType": 101,
      "ssrc": 19088743,
      "bitRate": 124000,
      "timeBase": "1/48000"
    }]
  })");

  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "video_source",
      "codecName": "h264",
      "rtpProfile": "cast",
      "rtpPayloadType": 101,
      "bitRate": 124000,
      "timeBase": "1/48000",
       "maxBitRate": 10000
    }]
  })");

  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "video_source",
      "codecName": "vp8",
      "rtpProfile": "cast",
      "rtpPayloadType": 100,
      "ssrc": 19088743,
      "timeBase": "1/48000",
       "resolutions": [],
       "maxBitRate": 10000
    }]
  })");

  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "video_source",
      "codecName": "vp8",
      "rtpProfile": "cast",
      "rtpPayloadType": 100,
      "ssrc": 19088743,
      "timeBase": "1/48000",
       "resolutions": [],
       "maxBitRate": 10000,
       "aesKey": "51027e4e2347cbcb49d57ef10177aebc"
    }]
  })");

  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "video_source",
      "codecName": "vp8",
      "rtpProfile": "cast",
      "rtpPayloadType": 100,
      "ssrc": 19088743,
      "timeBase": "1/48000",
       "resolutions": [],
       "maxBitRate": 10000,
       "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
    }]
  })");
}

TEST(OfferTest, ValidatesCodecParameterFormat) {
  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "audio_source",
      "codecName": "aac",
      "codecParameter": "vp08.123.332",
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "ssrc": 19088743,
      "bitRate": 124000,
      "timeBase": "1/10000000",
      "channels": 2,
      "aesKey": "51027e4e2347cbcb49d57ef10177aebc",
      "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
    }]
  })");

  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "video_source",
      "codecName": "vp8",
      "codecParameter": "vp09.11.23",
      "rtpProfile": "cast",
      "rtpPayloadType": 100,
      "ssrc": 19088743,
      "timeBase": "1/48000",
       "resolutions": [],
       "maxBitRate": 10000,
       "aesKey": "51027e4e2347cbcb49d57ef10177aebc"
    }]
  })");

  const ErrorOr<Json::Value> audio_root = json::Parse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "audio_source",
      "codecName": "aac",
      "codecParameter": "mp4a.12",
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "ssrc": 19088743,
      "bitRate": 124000,
      "timeBase": "1/10000000",
      "channels": 2,
      "aesKey": "51027e4e2347cbcb49d57ef10177aebc",
      "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
    }]
  })");
  ASSERT_TRUE(audio_root.is_value()) << audio_root.error();

  const ErrorOr<Json::Value> video_root = json::Parse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "video_source",
      "codecName": "vp9",
      "codecParameter": "vp09.11.23",
      "rtpProfile": "cast",
      "rtpPayloadType": 100,
      "ssrc": 19088743,
      "timeBase": "1/48000",
       "resolutions": [],
       "maxBitRate": 10000,
       "aesKey": "51027e4e2347cbcb49d57ef10177aebc"
    }]
  })");
  ASSERT_TRUE(video_root.is_value()) << video_root.error();
}

TEST(OfferTest, CanParseValidButMinimalVideoOffer) {
  ErrorOr<Json::Value> root = json::Parse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "video_source",
      "codecName": "vp8",
      "rtpProfile": "cast",
      "rtpPayloadType": 100,
      "ssrc": 19088743,
      "timeBase": "1/48000",
       "resolutions": [],
       "maxBitRate": 10000,
       "aesKey": "51027e4e2347cbcb49d57ef10177aebc",
       "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
    }]
  })");

  ASSERT_TRUE(root.is_value());
  Offer offer;
  EXPECT_TRUE(Offer::TryParse(std::move(root.value()), &offer).ok());
}

TEST(OfferTest, CanParseValidOffer) {
  ErrorOr<Json::Value> root = json::Parse(kValidOffer);
  ASSERT_TRUE(root.is_value());
  Offer offer;
  EXPECT_TRUE(Offer::TryParse(std::move(root.value()), &offer).ok());

  ExpectEqualsValidOffer(offer);
}

TEST(OfferTest, ParseAndToJsonResultsInSameOffer) {
  ErrorOr<Json::Value> root = json::Parse(kValidOffer);
  ASSERT_TRUE(root.is_value());
  Offer offer;
  EXPECT_TRUE(Offer::TryParse(std::move(root.value()), &offer).ok());
  ExpectEqualsValidOffer(offer);

  Offer reparsed_offer;
  EXPECT_TRUE(Offer::TryParse(std::move(root.value()), &reparsed_offer).ok());
  ExpectEqualsValidOffer(reparsed_offer);
}

// We don't want to enforce that a given offer must have both audio and
// video, so we don't assert on either.
TEST(OfferTest, IsValidWithMissingStreams) {
  ErrorOr<Json::Value> root = json::Parse(kValidOffer);
  ASSERT_TRUE(root.is_value());
  Offer offer;
  EXPECT_TRUE(Offer::TryParse(std::move(root.value()), &offer).ok());
  ExpectEqualsValidOffer(offer);
  const Offer valid_offer = std::move(offer);

  Offer missing_audio_streams = valid_offer;
  missing_audio_streams.audio_streams.clear();
  EXPECT_TRUE(missing_audio_streams.IsValid());

  Offer missing_video_streams = valid_offer;
  missing_video_streams.audio_streams.clear();
  EXPECT_TRUE(missing_video_streams.IsValid());
}

TEST(OfferTest, InvalidIfInvalidStreams) {
  ErrorOr<Json::Value> root = json::Parse(kValidOffer);
  ASSERT_TRUE(root.is_value());
  Offer offer;
  EXPECT_TRUE(Offer::TryParse(std::move(root.value()), &offer).ok());
  ExpectEqualsValidOffer(offer);

  Offer video_stream_invalid = offer;
  video_stream_invalid.video_streams[0].max_frame_rate = SimpleFraction{1, 0};
  EXPECT_FALSE(video_stream_invalid.IsValid());

  Offer audio_stream_invalid = offer;
  video_stream_invalid.audio_streams[0].bit_rate = 0;
  EXPECT_FALSE(video_stream_invalid.IsValid());
}

TEST(OfferTest, FailsIfUnencrypted) {
  // Video stream missing AES fields.
  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "video_source",
      "codecName": "vp8",
      "rtpProfile": "cast",
      "rtpPayloadType": 100,
      "ssrc": 19088743,
      "timeBase": "1/48000",
       "resolutions": [],
       "maxBitRate": 10000,
       "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
    }]
  })",
                       Error::Code::kUnencryptedOffer);

  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "video_source",
      "codecName": "vp8",
      "rtpProfile": "cast",
      "rtpPayloadType": 100,
      "ssrc": 19088743,
      "timeBase": "1/48000",
       "resolutions": [],
       "maxBitRate": 10000,
       "aesKey": "51027e4e2347cbcb49d57ef10177aebc"
    }]
  })",
                       Error::Code::kUnencryptedOffer);

  // Audio stream missing AES fields.
  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "audio_source",
      "codecName": "opus",
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "ssrc": 19088743,
      "bitRate": 124000,
      "timeBase": "1/48000",
      "channels": 2,
      "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
    }]
  })",
                       Error::Code::kUnencryptedOffer);

  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "audio_source",
      "codecName": "opus",
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "ssrc": 19088743,
      "bitRate": 124000,
      "timeBase": "1/48000",
      "channels": 2,
      "aesKey": "51027e4e2347cbcb49d57ef10177aebc"
    }]
  })",
                       Error::Code::kUnencryptedOffer);

  // And finally, fields provided but not properly formatted.
  ExpectFailureOnParse(R"({
    "castMode": "mirroring",
    "supportedStreams": [{
      "index": 2,
      "type": "audio_source",
      "codecName": "opus",
      "rtpProfile": "cast",
      "rtpPayloadType": 96,
      "ssrc": 19088743,
      "bitRate": 124000,
      "timeBase": "1/48000",
      "channels": 2,
      "aesKey": "51027e4e2347$bcb49d57ef10177aebc",
      "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
    }]
  })",
                       Error::Code::kUnencryptedOffer);
}

}  // namespace cast
}  // namespace openscreen
