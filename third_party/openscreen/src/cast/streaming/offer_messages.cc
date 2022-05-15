// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/offer_messages.h"

#include <inttypes.h>

#include <algorithm>
#include <limits>
#include <string>
#include <utility>

#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "cast/streaming/constants.h"
#include "platform/base/error.h"
#include "util/big_endian.h"
#include "util/enum_name_table.h"
#include "util/json/json_helpers.h"
#include "util/json/json_serialization.h"
#include "util/osp_logging.h"
#include "util/stringprintf.h"

namespace openscreen {
namespace cast {

namespace {

constexpr char kSupportedStreams[] = "supportedStreams";
constexpr char kAudioSourceType[] = "audio_source";
constexpr char kVideoSourceType[] = "video_source";
constexpr char kStreamType[] = "type";

bool CodecParameterIsValid(VideoCodec codec,
                           const std::string& codec_parameter) {
  if (codec_parameter.empty()) {
    return true;
  }
  switch (codec) {
    case VideoCodec::kVp8:
      return absl::StartsWith(codec_parameter, "vp08");
    case VideoCodec::kVp9:
      return absl::StartsWith(codec_parameter, "vp09");
    case VideoCodec::kAv1:
      return absl::StartsWith(codec_parameter, "av01");
    case VideoCodec::kHevc:
      return absl::StartsWith(codec_parameter, "hev1");
    case VideoCodec::kH264:
      return absl::StartsWith(codec_parameter, "avc1");
    case VideoCodec::kNotSpecified:
      return false;
  }
  OSP_NOTREACHED();
}

bool CodecParameterIsValid(AudioCodec codec,
                           const std::string& codec_parameter) {
  if (codec_parameter.empty()) {
    return true;
  }
  switch (codec) {
    case AudioCodec::kAac:
      return absl::StartsWith(codec_parameter, "mp4a.");

    // Opus doesn't use codec parameters.
    case AudioCodec::kOpus:  // fallthrough
    case AudioCodec::kNotSpecified:
      return false;
  }
  OSP_NOTREACHED();
}

EnumNameTable<CastMode, 2> kCastModeNames{
    {{"mirroring", CastMode::kMirroring}, {"remoting", CastMode::kRemoting}}};

bool TryParseRtpPayloadType(const Json::Value& value, RtpPayloadType* out) {
  int t;
  if (!json::TryParseInt(value, &t)) {
    return false;
  }

  uint8_t t_small = t;
  if (t_small != t || !IsRtpPayloadType(t_small)) {
    return false;
  }

  *out = static_cast<RtpPayloadType>(t_small);
  return true;
}

bool TryParseRtpTimebase(const Json::Value& value, int* out) {
  std::string raw_timebase;
  if (!json::TryParseString(value, &raw_timebase)) {
    return false;
  }

  // The spec demands a leading 1, so this isn't really a fraction.
  const auto fraction = SimpleFraction::FromString(raw_timebase);
  if (fraction.is_error() || !fraction.value().is_positive() ||
      fraction.value().numerator() != 1) {
    return false;
  }

  *out = fraction.value().denominator();
  return true;
}

// For a hex byte, the conversion is 4 bits to 1 character, e.g.
// 0b11110001 becomes F1, so 1 byte is two characters.
constexpr int kHexDigitsPerByte = 2;
constexpr int kAesBytesSize = 16;
constexpr int kAesStringLength = kAesBytesSize * kHexDigitsPerByte;
bool TryParseAesHexBytes(const Json::Value& value,
                         std::array<uint8_t, kAesBytesSize>* out) {
  std::string hex_string;
  if (!json::TryParseString(value, &hex_string)) {
    return false;
  }

  constexpr int kHexDigitsPerScanField = 16;
  constexpr int kNumScanFields = kAesStringLength / kHexDigitsPerScanField;
  uint64_t quads[kNumScanFields];
  int chars_scanned;
  if (hex_string.size() == kAesStringLength &&
      sscanf(hex_string.c_str(), "%16" SCNx64 "%16" SCNx64 "%n", &quads[0],
             &quads[1], &chars_scanned) == kNumScanFields &&
      chars_scanned == kAesStringLength &&
      std::none_of(hex_string.begin(), hex_string.end(),
                   [](char c) { return std::isspace(c); })) {
    WriteBigEndian(quads[0], out->data());
    WriteBigEndian(quads[1], out->data() + 8);
    return true;
  }

  return false;
}

absl::string_view ToString(Stream::Type type) {
  switch (type) {
    case Stream::Type::kAudioSource:
      return kAudioSourceType;
    case Stream::Type::kVideoSource:
      return kVideoSourceType;
    default: {
      OSP_NOTREACHED();
    }
  }
}

bool TryParseResolutions(const Json::Value& value,
                         std::vector<Resolution>* out) {
  out->clear();

  // Some legacy senders don't provide resolutions, so just return empty.
  if (!value.isArray() || value.empty()) {
    return false;
  }

  for (Json::ArrayIndex i = 0; i < value.size(); ++i) {
    Resolution resolution;
    if (!Resolution::TryParse(value[i], &resolution)) {
      out->clear();
      return false;
    }
    out->push_back(std::move(resolution));
  }

  return true;
}

}  // namespace

Error Stream::TryParse(const Json::Value& value,
                       Stream::Type type,
                       Stream* out) {
  out->type = type;

  if (!json::TryParseInt(value["index"], &out->index) ||
      !json::TryParseUint(value["ssrc"], &out->ssrc) ||
      !TryParseRtpPayloadType(value["rtpPayloadType"],
                              &out->rtp_payload_type) ||
      !TryParseRtpTimebase(value["timeBase"], &out->rtp_timebase)) {
    return Error(Error::Code::kJsonParseError,
                 "Offer stream has missing or invalid mandatory field");
  }

  if (!json::TryParseInt(value["channels"], &out->channels)) {
    out->channels = out->type == Stream::Type::kAudioSource
                        ? kDefaultNumAudioChannels
                        : kDefaultNumVideoChannels;
  } else if (out->channels <= 0) {
    return Error(Error::Code::kJsonParseError, "Invalid channel count");
  }

  if (!TryParseAesHexBytes(value["aesKey"], &out->aes_key) ||
      !TryParseAesHexBytes(value["aesIvMask"], &out->aes_iv_mask)) {
    return Error(Error::Code::kUnencryptedOffer,
                 "Offer stream must have both a valid aesKey and aesIvMask");
  }
  if (out->rtp_timebase <
          std::min(kDefaultAudioMinSampleRate, kRtpVideoTimebase) ||
      out->rtp_timebase > kRtpVideoTimebase) {
    return Error(Error::Code::kJsonParseError, "rtp_timebase (sample rate)");
  }

  out->target_delay = kDefaultTargetPlayoutDelay;
  int target_delay;
  if (json::TryParseInt(value["targetDelay"], &target_delay)) {
    auto d = std::chrono::milliseconds(target_delay);
    if (kMinTargetPlayoutDelay <= d && d <= kMaxTargetPlayoutDelay) {
      out->target_delay = d;
    }
  }

  json::TryParseBool(value["receiverRtcpEventLog"],
                     &out->receiver_rtcp_event_log);
  json::TryParseString(value["receiverRtcpDscp"], &out->receiver_rtcp_dscp);
  json::TryParseString(value["codecParameter"], &out->codec_parameter);

  return Error::None();
}

Json::Value Stream::ToJson() const {
  OSP_DCHECK(IsValid());

  Json::Value root;
  root["index"] = index;
  root["type"] = std::string(ToString(type));
  root["channels"] = channels;
  root["rtpPayloadType"] = static_cast<int>(rtp_payload_type);
  // rtpProfile is technically required by the spec, although it is always set
  // to cast. We set it here to be compliant with all spec implementers.
  root["rtpProfile"] = "cast";
  static_assert(sizeof(ssrc) <= sizeof(Json::UInt),
                "this code assumes Ssrc fits in a Json::UInt");
  root["ssrc"] = static_cast<Json::UInt>(ssrc);
  root["targetDelay"] = static_cast<int>(target_delay.count());
  root["aesKey"] = HexEncode(aes_key.data(), aes_key.size());
  root["aesIvMask"] = HexEncode(aes_iv_mask.data(), aes_iv_mask.size());
  root["receiverRtcpEventLog"] = receiver_rtcp_event_log;
  root["receiverRtcpDscp"] = receiver_rtcp_dscp;
  root["timeBase"] = "1/" + std::to_string(rtp_timebase);
  root["codecParameter"] = codec_parameter;
  return root;
}

bool Stream::IsValid() const {
  return channels >= 1 && index >= 0 && target_delay.count() > 0 &&
         target_delay.count() <= std::numeric_limits<int>::max() &&
         rtp_timebase >= 1;
}

Error AudioStream::TryParse(const Json::Value& value, AudioStream* out) {
  Error error =
      Stream::TryParse(value, Stream::Type::kAudioSource, &out->stream);
  if (!error.ok()) {
    return error;
  }

  std::string codec_name;
  if (!json::TryParseInt(value["bitRate"], &out->bit_rate) ||
      out->bit_rate < 0 ||
      !json::TryParseString(value[kCodecName], &codec_name)) {
    return Error(Error::Code::kJsonParseError, "Invalid audio stream field");
  }
  ErrorOr<AudioCodec> codec = StringToAudioCodec(codec_name);
  if (!codec) {
    return Error(Error::Code::kUnknownCodec,
                 "Codec is not known, can't use stream");
  }
  out->codec = codec.value();
  if (!CodecParameterIsValid(codec.value(), out->stream.codec_parameter)) {
    return Error(Error::Code::kInvalidCodecParameter,
                 StringPrintf("Invalid audio codec parameter (%s for codec %s)",
                              out->stream.codec_parameter.c_str(),
                              CodecToString(codec.value())));
  }
  return Error::None();
}

Json::Value AudioStream::ToJson() const {
  OSP_DCHECK(IsValid());

  Json::Value out = stream.ToJson();
  out[kCodecName] = CodecToString(codec);
  out["bitRate"] = bit_rate;
  return out;
}

bool AudioStream::IsValid() const {
  return bit_rate >= 0 && stream.IsValid();
}

Error VideoStream::TryParse(const Json::Value& value, VideoStream* out) {
  Error error =
      Stream::TryParse(value, Stream::Type::kVideoSource, &out->stream);
  if (!error.ok()) {
    return error;
  }

  std::string codec_name;
  if (!json::TryParseString(value[kCodecName], &codec_name)) {
    return Error(Error::Code::kJsonParseError, "Video stream missing codec");
  }
  ErrorOr<VideoCodec> codec = StringToVideoCodec(codec_name);
  if (!codec) {
    return Error(Error::Code::kUnknownCodec,
                 "Codec is not known, can't use stream");
  }
  out->codec = codec.value();
  if (!CodecParameterIsValid(codec.value(), out->stream.codec_parameter)) {
    return Error(Error::Code::kInvalidCodecParameter,
                 StringPrintf("Invalid video codec parameter (%s for codec %s)",
                              out->stream.codec_parameter.c_str(),
                              CodecToString(codec.value())));
  }

  out->max_frame_rate = SimpleFraction{kDefaultMaxFrameRate, 1};
  std::string raw_max_frame_rate;
  if (json::TryParseString(value["maxFrameRate"], &raw_max_frame_rate)) {
    auto parsed = SimpleFraction::FromString(raw_max_frame_rate);
    if (parsed.is_value() && parsed.value().is_positive()) {
      out->max_frame_rate = parsed.value();
    }
  }

  TryParseResolutions(value["resolutions"], &out->resolutions);
  json::TryParseString(value["profile"], &out->profile);
  json::TryParseString(value["protection"], &out->protection);
  json::TryParseString(value["level"], &out->level);
  json::TryParseString(value["errorRecoveryMode"], &out->error_recovery_mode);
  if (!json::TryParseInt(value["maxBitRate"], &out->max_bit_rate)) {
    out->max_bit_rate = 4 << 20;
  }

  return Error::None();
}

Json::Value VideoStream::ToJson() const {
  OSP_DCHECK(IsValid());

  Json::Value out = stream.ToJson();
  out["codecName"] = CodecToString(codec);
  out["maxFrameRate"] = max_frame_rate.ToString();
  out["maxBitRate"] = max_bit_rate;
  out["protection"] = protection;
  out["profile"] = profile;
  out["level"] = level;
  out["errorRecoveryMode"] = error_recovery_mode;

  Json::Value rs;
  for (auto resolution : resolutions) {
    rs.append(resolution.ToJson());
  }
  out["resolutions"] = std::move(rs);
  return out;
}

bool VideoStream::IsValid() const {
  return max_bit_rate > 0 && max_frame_rate.is_positive();
}

// static
Error Offer::TryParse(const Json::Value& root, Offer* out) {
  if (!root.isObject()) {
    return Error(Error::Code::kJsonParseError, "null offer");
  }
  const ErrorOr<CastMode> cast_mode =
      GetEnum(kCastModeNames, root["castMode"].asString());
  Json::Value supported_streams = root[kSupportedStreams];
  if (!supported_streams.isArray()) {
    return Error(Error::Code::kJsonParseError, "supported streams in offer");
  }

  std::vector<AudioStream> audio_streams;
  std::vector<VideoStream> video_streams;
  for (Json::ArrayIndex i = 0; i < supported_streams.size(); ++i) {
    const Json::Value& fields = supported_streams[i];
    std::string type;
    if (!json::TryParseString(fields[kStreamType], &type)) {
      return Error(Error::Code::kJsonParseError, "Missing stream type");
    }

    Error error;
    if (type == kAudioSourceType) {
      AudioStream stream;
      error = AudioStream::TryParse(fields, &stream);
      if (error.ok()) {
        audio_streams.push_back(std::move(stream));
      }
    } else if (type == kVideoSourceType) {
      VideoStream stream;
      error = VideoStream::TryParse(fields, &stream);
      if (error.ok()) {
        video_streams.push_back(std::move(stream));
      }
    }

    if (!error.ok()) {
      if (error.code() == Error::Code::kUnknownCodec) {
        OSP_VLOG << "Dropping audio stream due to unknown codec: " << error;
        continue;
      } else {
        return error;
      }
    }
  }

  *out = Offer{cast_mode.value(CastMode::kMirroring), std::move(audio_streams),
               std::move(video_streams)};
  return Error::None();
}

Json::Value Offer::ToJson() const {
  OSP_DCHECK(IsValid());
  Json::Value root;
  root["castMode"] = GetEnumName(kCastModeNames, cast_mode).value();
  Json::Value streams;
  for (auto& stream : audio_streams) {
    streams.append(stream.ToJson());
  }

  for (auto& stream : video_streams) {
    streams.append(stream.ToJson());
  }

  root[kSupportedStreams] = std::move(streams);
  return root;
}

bool Offer::IsValid() const {
  return std::all_of(audio_streams.begin(), audio_streams.end(),
                     [](const AudioStream& a) { return a.IsValid(); }) &&
         std::all_of(video_streams.begin(), video_streams.end(),
                     [](const VideoStream& v) { return v.IsValid(); });
}
}  // namespace cast
}  // namespace openscreen
