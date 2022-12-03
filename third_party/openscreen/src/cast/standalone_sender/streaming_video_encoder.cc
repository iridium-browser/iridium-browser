// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_sender/streaming_video_encoder.h"

#include <utility>

#include "util/chrono_helpers.h"

namespace openscreen {
namespace cast {

StreamingVideoEncoder::StreamingVideoEncoder(const Parameters& params,
                                             TaskRunner* task_runner,
                                             std::unique_ptr<Sender> sender)
    : params_(params),
      main_task_runner_(task_runner),
      sender_(std::move(sender)) {
  OSP_DCHECK_LE(1, params_.num_encode_threads);
  OSP_DCHECK_LE(kMinQuantizer, params_.min_quantizer);
  OSP_DCHECK_LE(params_.min_quantizer, params_.max_cpu_saver_quantizer);
  OSP_DCHECK_LE(params_.max_cpu_saver_quantizer, params_.max_quantizer);
  OSP_DCHECK_LE(params_.max_quantizer, kMaxQuantizer);
  OSP_DCHECK_LT(0.0, params_.max_time_utilization);
  OSP_DCHECK_LE(params_.max_time_utilization, 1.0);
  OSP_DCHECK(main_task_runner_);
  OSP_DCHECK(sender_);
}

StreamingVideoEncoder::~StreamingVideoEncoder() {}

void StreamingVideoEncoder::UpdateSpeedSettingForNextFrame(const Stats& stats) {
  OSP_DCHECK_EQ(std::this_thread::get_id(), encode_thread_.get_id());

  // Combine the speed setting that was used to encode the last frame, and the
  // quantizer the encoder chose into a single speed metric.
  const double speed = current_speed_setting_ +
                       kEquivalentEncodingSpeedStepPerQuantizerStep *
                           std::max(0, stats.quantizer - params_.min_quantizer);

  // Like |Stats::perfect_quantizer|, this computes a "hindsight" speed setting
  // for the last frame, one that may have potentially allowed for a
  // better-quality quantizer choice by the encoder, while also keeping CPU
  // utilization within budget.
  const double perfect_speed =
      speed * stats.time_utilization() / params_.max_time_utilization;

  // Update the ideal speed setting, to be used for the next frame. An
  // exponentially-decaying weighted average is used here to smooth-out noise.
  // The weight is based on the duration of the frame that was encoded.
  constexpr Clock::duration kDecayHalfLife = milliseconds(120);
  const double ticks = static_cast<double>(stats.frame_duration.count());
  const double weight = ticks / (ticks + kDecayHalfLife.count());
  ideal_speed_setting_ =
      weight * perfect_speed + (1.0 - weight) * ideal_speed_setting_;
  OSP_DCHECK(std::isfinite(ideal_speed_setting_));
}

}  // namespace cast
}  // namespace openscreen
