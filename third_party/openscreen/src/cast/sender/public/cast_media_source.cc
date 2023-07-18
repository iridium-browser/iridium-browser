// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/sender/public/cast_media_source.h"

#include <algorithm>
#include <utility>

#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {
namespace cast {

// static
ErrorOr<CastMediaSource> CastMediaSource::From(const std::string& source) {
  // TODO(btolsch): Implement when we have URL parsing.
  OSP_UNIMPLEMENTED();
  return Error::Code::kUnknownError;
}

CastMediaSource::CastMediaSource(std::string source,
                                 std::vector<std::string> app_ids)
    : source_id_(std::move(source)), app_ids_(std::move(app_ids)) {}

CastMediaSource::CastMediaSource(const CastMediaSource& other) = default;
CastMediaSource::CastMediaSource(CastMediaSource&& other) = default;

CastMediaSource::~CastMediaSource() = default;

CastMediaSource& CastMediaSource::operator=(const CastMediaSource& other) =
    default;
CastMediaSource& CastMediaSource::operator=(CastMediaSource&& other) = default;

bool CastMediaSource::ContainsAppId(const std::string& app_id) const {
  return Contains(app_ids_, app_id);
}

bool CastMediaSource::ContainsAnyAppIdFrom(
    const std::vector<std::string>& app_ids) const {
  return std::find_first_of(app_ids_.begin(), app_ids_.end(), app_ids.begin(),
                            app_ids.end()) != app_ids_.end();
}

}  // namespace cast
}  // namespace openscreen
