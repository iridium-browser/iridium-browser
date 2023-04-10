// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/message_fields.h"

#include <array>
#include <utility>

#include "util/enum_name_table.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

constexpr EnumNameTable<AudioCodec, 3> kAudioCodecNames{
    {{"aac", AudioCodec::kAac},
     {"opus", AudioCodec::kOpus},
     {"REMOTE_AUDIO", AudioCodec::kNotSpecified}}};

constexpr EnumNameTable<VideoCodec, 6> kVideoCodecNames{
    {{"h264", VideoCodec::kH264},
     {"vp8", VideoCodec::kVp8},
     {"hevc", VideoCodec::kHevc},
     {"REMOTE_VIDEO", VideoCodec::kNotSpecified},
     {"vp9", VideoCodec::kVp9},
     {"av1", VideoCodec::kAv1}}};

}  // namespace

const char* CodecToString(AudioCodec codec) {
  return GetEnumName(kAudioCodecNames, codec).value();
}

ErrorOr<AudioCodec> StringToAudioCodec(absl::string_view name) {
  return GetEnum(kAudioCodecNames, name);
}

const char* CodecToString(VideoCodec codec) {
  return GetEnumName(kVideoCodecNames, codec).value();
}

ErrorOr<VideoCodec> StringToVideoCodec(absl::string_view name) {
  return GetEnum(kVideoCodecNames, name);
}

}  // namespace cast
}  // namespace openscreen
