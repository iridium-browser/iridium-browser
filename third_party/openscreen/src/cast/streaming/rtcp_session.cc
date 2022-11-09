// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/rtcp_session.h"

#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

RtcpSession::RtcpSession(Ssrc sender_ssrc,
                         Ssrc receiver_ssrc,
                         Clock::time_point start_time)
    : sender_ssrc_(sender_ssrc),
      receiver_ssrc_(receiver_ssrc),
      ntp_converter_(start_time) {
  OSP_DCHECK_NE(sender_ssrc_, kNullSsrc);
  OSP_DCHECK_NE(receiver_ssrc_, kNullSsrc);
  OSP_DCHECK_NE(sender_ssrc_, receiver_ssrc_);
}

RtcpSession::~RtcpSession() = default;

}  // namespace cast
}  // namespace openscreen
