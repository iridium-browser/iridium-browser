// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_SENDER_TESTING_TEST_HELPERS_H_
#define CAST_SENDER_TESTING_TEST_HELPERS_H_

#include <cstdint>
#include <string>

#include "cast/sender/channel/message_util.h"

namespace cast {
namespace channel {
class CastMessage;
}  // namespace channel
}  // namespace cast

namespace openscreen {
namespace cast {

void VerifyAppAvailabilityRequest(const ::cast::channel::CastMessage& message,
                                  const std::string& expected_app_id,
                                  int* request_id_out,
                                  std::string* sender_id_out);
void VerifyAppAvailabilityRequest(const ::cast::channel::CastMessage& message,
                                  std::string* app_id_out,
                                  int* request_id_out,
                                  std::string* sender_id_out);

::cast::channel::CastMessage CreateAppAvailableResponseChecked(
    int request_id,
    const std::string& sender_id,
    const std::string& app_id);
::cast::channel::CastMessage CreateAppUnavailableResponseChecked(
    int request_id,
    const std::string& sender_id,
    const std::string& app_id);

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_SENDER_TESTING_TEST_HELPERS_H_
