// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/answer_messages.h"

#include <utility>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "platform/base/error.h"
#include "util/enum_name_table.h"
#include "util/json/json_helpers.h"
#include "util/osp_logging.h"
namespace openscreen {
namespace cast {

namespace {

/// Constraint properties.
// Audio constraints. See properties below.
static constexpr char kAudio[] = "audio";
// Video constraints. See properties below.
static constexpr char kVideo[] = "video";

// An optional field representing the minimum bits per second. If not specified
// by the receiver, the sender will use kDefaultAudioMinBitRate and
// kDefaultVideoMinBitRate, which represent the true operational minimum.
static constexpr char kMinBitRate[] = "minBitRate";
// 32kbps is sender default for audio minimum bit rate.
static constexpr int kDefaultAudioMinBitRate = 32 * 1000;
// 300kbps is sender default for video minimum bit rate.
static constexpr int kDefaultVideoMinBitRate = 300 * 1000;

// Maximum encoded bits per second. This is the lower of (1) the max capability
// of the decoder, or (2) the max data transfer rate.
static constexpr char kMaxBitRate[] = "maxBitRate";
// Maximum supported end-to-end latency, in milliseconds. Proportional to the
// size of the data buffers in the receiver.
static constexpr char kMaxDelay[] = "maxDelay";

/// Video constraint properties.
// Maximum pixel rate (width * height * framerate). Is often less than
// multiplying the fields in maxDimensions. This field is used to set the
// maximum processing rate.
static constexpr char kMaxPixelsPerSecond[] = "maxPixelsPerSecond";
// Minimum dimensions. If omitted, the sender will assume a reasonable minimum
// with the same aspect ratio as maxDimensions, as close to 320*180 as possible.
// Should reflect the true operational minimum.
static constexpr char kMinResolution[] = "minResolution";
// Maximum dimensions, not necessarily ideal dimensions.
static constexpr char kMaxDimensions[] = "maxDimensions";

/// Audio constraint properties.
// Maximum supported sampling frequency (not necessarily ideal).
static constexpr char kMaxSampleRate[] = "maxSampleRate";
// Maximum number of audio channels (1 is mono, 2 is stereo, etc.).
static constexpr char kMaxChannels[] = "maxChannels";

/// Display description properties
// If this optional field is included in the ANSWER message, the receiver is
// attached to a fixed display that has the given dimensions and frame rate
// configuration. These may exceed, be the same, or be less than the values in
// constraints. If undefined, we assume the display is not fixed (e.g. a Google
// Hangouts UI panel).
static constexpr char kDimensions[] = "dimensions";
// An optional field. When missing and dimensions are specified, the sender
// will assume square pixels and the dimensions imply the aspect ratio of the
// fixed display. WHen present and dimensions are also specified, implies the
// pixels are not square.
static constexpr char kAspectRatio[] = "aspectRatio";
// The delimeter used for the aspect ratio format ("A:B").
static constexpr char kAspectRatioDelimiter[] = ":";
// Sets the aspect ratio constraints. Value must be either "sender" or
// "receiver", see kScalingSender and kScalingReceiver below.
static constexpr char kScaling[] = "scaling";
// scaling = "sender" means that the sender must provide video frames of a fixed
// aspect ratio. In this case, the dimensions object must be passed or an error
// case will occur.
static constexpr char kScalingSender[] = "sender";
// scaling = "receiver" means that the sender may send arbitrarily sized frames,
// and the receiver will handle scaling and letterboxing as necessary.
static constexpr char kScalingReceiver[] = "receiver";

/// Answer properties.
// A number specifying the UDP port used for all streams in this session.
// Must have a value between kUdpPortMin and kUdpPortMax.
static constexpr char kUdpPort[] = "udpPort";
static constexpr int kUdpPortMin = 1;
static constexpr int kUdpPortMax = 65535;
// Numbers specifying the indexes chosen from the offer message.
static constexpr char kSendIndexes[] = "sendIndexes";
// uint32_t values specifying the RTP SSRC values used to send the RTCP feedback
// of the stream indicated in kSendIndexes.
static constexpr char kSsrcs[] = "ssrcs";
// Provides detailed maximum and minimum capabilities of the receiver for
// processing the selected streams. The sender may alter video resolution and
// frame rate throughout the session, and the constraints here determine how
// much data volume is allowed.
static constexpr char kConstraints[] = "constraints";
// Provides details about the display on the receiver.
static constexpr char kDisplay[] = "display";
// absl::optional array of numbers specifying the indexes of streams that will
// send event logs through RTCP.
static constexpr char kReceiverRtcpEventLog[] = "receiverRtcpEventLog";
// OPtional array of numbers specifying the indexes of streams that will use
// DSCP values specified in the OFFER message for RTCP packets.
static constexpr char kReceiverRtcpDscp[] = "receiverRtcpDscp";
// If this optional field is present the receiver supports the specific
// RTP extensions (such as adaptive playout delay).
static constexpr char kRtpExtensions[] = "rtpExtensions";

EnumNameTable<AspectRatioConstraint, 2> kAspectRatioConstraintNames{
    {{kScalingReceiver, AspectRatioConstraint::kVariable},
     {kScalingSender, AspectRatioConstraint::kFixed}}};

Json::Value AspectRatioConstraintToJson(AspectRatioConstraint aspect_ratio) {
  return Json::Value(GetEnumName(kAspectRatioConstraintNames, aspect_ratio)
                         .value(kScalingSender));
}

bool TryParseAspectRatioConstraint(const Json::Value& value,
                                   AspectRatioConstraint* out) {
  std::string aspect_ratio;
  if (!json::TryParseString(value, &aspect_ratio)) {
    return false;
  }

  ErrorOr<AspectRatioConstraint> constraint =
      GetEnum(kAspectRatioConstraintNames, aspect_ratio);
  if (constraint.is_error()) {
    return false;
  }
  *out = constraint.value();
  return true;
}

template <typename T>
Json::Value PrimitiveVectorToJson(const std::vector<T>& vec) {
  Json::Value array(Json::ValueType::arrayValue);
  array.resize(vec.size());

  for (Json::Value::ArrayIndex i = 0; i < vec.size(); ++i) {
    array[i] = Json::Value(vec[i]);
  }

  return array;
}

template <typename T>
bool ParseOptional(const Json::Value& value, absl::optional<T>* out) {
  // It's fine if the value is empty.
  if (!value) {
    return true;
  }
  T tentative_out;
  if (!T::TryParse(value, &tentative_out)) {
    return false;
  }
  *out = tentative_out;
  return true;
}

}  // namespace

// static
bool AspectRatio::TryParse(const Json::Value& value, AspectRatio* out) {
  std::string parsed_value;
  if (!json::TryParseString(value, &parsed_value)) {
    return false;
  }

  std::vector<absl::string_view> fields =
      absl::StrSplit(parsed_value, kAspectRatioDelimiter);
  if (fields.size() != 2) {
    return false;
  }

  if (!absl::SimpleAtoi(fields[0], &out->width) ||
      !absl::SimpleAtoi(fields[1], &out->height)) {
    return false;
  }
  return out->IsValid();
}

bool AspectRatio::IsValid() const {
  return width > 0 && height > 0;
}

// static
bool AudioConstraints::TryParse(const Json::Value& root,
                                AudioConstraints* out) {
  if (!json::TryParseInt(root[kMaxSampleRate], &(out->max_sample_rate)) ||
      !json::TryParseInt(root[kMaxChannels], &(out->max_channels)) ||
      !json::TryParseInt(root[kMaxBitRate], &(out->max_bit_rate))) {
    return false;
  }

  std::chrono::milliseconds max_delay;
  if (json::TryParseMilliseconds(root[kMaxDelay], &max_delay)) {
    out->max_delay = max_delay;
  }

  if (!json::TryParseInt(root[kMinBitRate], &(out->min_bit_rate))) {
    out->min_bit_rate = kDefaultAudioMinBitRate;
  }
  return out->IsValid();
}

Json::Value AudioConstraints::ToJson() const {
  OSP_DCHECK(IsValid());
  Json::Value root;
  root[kMaxSampleRate] = max_sample_rate;
  root[kMaxChannels] = max_channels;
  root[kMinBitRate] = min_bit_rate;
  root[kMaxBitRate] = max_bit_rate;
  if (max_delay.has_value()) {
    root[kMaxDelay] = Json::Value::Int64(max_delay->count());
  }
  return root;
}

bool AudioConstraints::IsValid() const {
  return max_sample_rate > 0 && max_channels > 0 && min_bit_rate > 0 &&
         max_bit_rate >= min_bit_rate;
}

// static
bool VideoConstraints::TryParse(const Json::Value& root,
                                VideoConstraints* out) {
  if (!Dimensions::TryParse(root[kMaxDimensions], &(out->max_dimensions)) ||
      !json::TryParseInt(root[kMaxBitRate], &(out->max_bit_rate)) ||
      !ParseOptional<Dimensions>(root[kMinResolution],
                                 &(out->min_resolution))) {
    return false;
  }

  std::chrono::milliseconds max_delay;
  if (json::TryParseMilliseconds(root[kMaxDelay], &max_delay)) {
    out->max_delay = max_delay;
  }

  double max_pixels_per_second;
  if (json::TryParseDouble(root[kMaxPixelsPerSecond], &max_pixels_per_second)) {
    out->max_pixels_per_second = max_pixels_per_second;
  }

  if (!json::TryParseInt(root[kMinBitRate], &(out->min_bit_rate))) {
    out->min_bit_rate = kDefaultVideoMinBitRate;
  }
  return out->IsValid();
}

bool VideoConstraints::IsValid() const {
  return max_pixels_per_second > 0 && min_bit_rate > 0 &&
         max_bit_rate > min_bit_rate &&
         (!max_delay.has_value() || max_delay->count() > 0) &&
         max_dimensions.IsValid() &&
         (!min_resolution.has_value() || min_resolution->IsValid()) &&
         max_dimensions.frame_rate.numerator() > 0;
}

Json::Value VideoConstraints::ToJson() const {
  OSP_DCHECK(IsValid());
  Json::Value root;
  root[kMaxDimensions] = max_dimensions.ToJson();
  root[kMinBitRate] = min_bit_rate;
  root[kMaxBitRate] = max_bit_rate;
  if (max_pixels_per_second.has_value()) {
    root[kMaxPixelsPerSecond] = max_pixels_per_second.value();
  }

  if (min_resolution.has_value()) {
    root[kMinResolution] = min_resolution->ToJson();
  }

  if (max_delay.has_value()) {
    root[kMaxDelay] = Json::Value::Int64(max_delay->count());
  }
  return root;
}

// static
bool Constraints::TryParse(const Json::Value& root, Constraints* out) {
  if (!AudioConstraints::TryParse(root[kAudio], &(out->audio)) ||
      !VideoConstraints::TryParse(root[kVideo], &(out->video))) {
    return false;
  }
  return out->IsValid();
}

bool Constraints::IsValid() const {
  return audio.IsValid() && video.IsValid();
}

Json::Value Constraints::ToJson() const {
  OSP_DCHECK(IsValid());
  Json::Value root;
  root[kAudio] = audio.ToJson();
  root[kVideo] = video.ToJson();
  return root;
}

// static
bool DisplayDescription::TryParse(const Json::Value& root,
                                  DisplayDescription* out) {
  if (!ParseOptional<Dimensions>(root[kDimensions], &(out->dimensions)) ||
      !ParseOptional<AspectRatio>(root[kAspectRatio], &(out->aspect_ratio))) {
    return false;
  }

  AspectRatioConstraint constraint;
  if (TryParseAspectRatioConstraint(root[kScaling], &constraint)) {
    out->aspect_ratio_constraint =
        absl::optional<AspectRatioConstraint>(std::move(constraint));
  } else {
    out->aspect_ratio_constraint = absl::nullopt;
  }

  return out->IsValid();
}

bool DisplayDescription::IsValid() const {
  // At least one of the properties must be set, and if a property is set
  // it must be valid.
  if (aspect_ratio.has_value() && !aspect_ratio->IsValid()) {
    return false;
  }

  if (dimensions.has_value() && !dimensions->IsValid()) {
    return false;
  }

  // Sender behavior is undefined if the aspect ratio is fixed but no
  // dimensions or aspect ratio are provided.
  if (aspect_ratio_constraint.has_value() &&
      (aspect_ratio_constraint.value() == AspectRatioConstraint::kFixed) &&
      !dimensions.has_value() && !aspect_ratio.has_value()) {
    return false;
  }
  return aspect_ratio.has_value() || dimensions.has_value() ||
         aspect_ratio_constraint.has_value();
}

Json::Value DisplayDescription::ToJson() const {
  OSP_DCHECK(IsValid());
  Json::Value root;
  if (aspect_ratio.has_value()) {
    root[kAspectRatio] = absl::StrCat(
        aspect_ratio->width, kAspectRatioDelimiter, aspect_ratio->height);
  }
  if (dimensions.has_value()) {
    root[kDimensions] = dimensions->ToJson();
  }
  if (aspect_ratio_constraint.has_value()) {
    root[kScaling] =
        AspectRatioConstraintToJson(aspect_ratio_constraint.value());
  }
  return root;
}

bool Answer::TryParse(const Json::Value& root, Answer* out) {
  if (!json::TryParseInt(root[kUdpPort], &(out->udp_port)) ||
      !json::TryParseIntArray(root[kSendIndexes], &(out->send_indexes)) ||
      !json::TryParseUintArray(root[kSsrcs], &(out->ssrcs)) ||
      !ParseOptional<Constraints>(root[kConstraints], &(out->constraints)) ||
      !ParseOptional<DisplayDescription>(root[kDisplay], &(out->display))) {
    return false;
  }

  // These function set to empty array if not present, so we can ignore
  // the return value for optional values.
  json::TryParseIntArray(root[kReceiverRtcpEventLog],
                         &(out->receiver_rtcp_event_log));
  json::TryParseIntArray(root[kReceiverRtcpDscp], &(out->receiver_rtcp_dscp));
  json::TryParseStringArray(root[kRtpExtensions], &(out->rtp_extensions));

  return out->IsValid();
}

bool Answer::IsValid() const {
  if (ssrcs.empty() || send_indexes.empty()) {
    return false;
  }

  // We don't know what the indexes used in the offer were here, so we just
  // sanity check.
  for (const int index : send_indexes) {
    if (index < 0) {
      return false;
    }
  }
  if (constraints.has_value() && !constraints->IsValid()) {
    return false;
  }
  if (display.has_value() && !display->IsValid()) {
    return false;
  }
  return kUdpPortMin <= udp_port && udp_port <= kUdpPortMax;
}

Json::Value Answer::ToJson() const {
  OSP_DCHECK(IsValid());
  Json::Value root;
  if (constraints.has_value()) {
    root[kConstraints] = constraints->ToJson();
  }
  if (display.has_value()) {
    root[kDisplay] = display->ToJson();
  }
  root[kUdpPort] = udp_port;
  root[kSendIndexes] = PrimitiveVectorToJson(send_indexes);
  root[kSsrcs] = PrimitiveVectorToJson(ssrcs);
  // Some sender do not handle empty array properly, so we omit these fields
  // if they are empty.
  if (!receiver_rtcp_event_log.empty()) {
    root[kReceiverRtcpEventLog] =
        PrimitiveVectorToJson(receiver_rtcp_event_log);
  }
  if (!receiver_rtcp_dscp.empty()) {
    root[kReceiverRtcpDscp] = PrimitiveVectorToJson(receiver_rtcp_dscp);
  }
  if (!rtp_extensions.empty()) {
    root[kRtpExtensions] = PrimitiveVectorToJson(rtp_extensions);
  }
  return root;
}

}  // namespace cast
}  // namespace openscreen
