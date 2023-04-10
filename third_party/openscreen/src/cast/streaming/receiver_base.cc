// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/receiver_base.h"

namespace openscreen {
namespace cast {

ReceiverBase::Consumer::~Consumer() = default;

ReceiverBase::ReceiverBase() = default;

ReceiverBase::~ReceiverBase() = default;

}  // namespace cast
}  // namespace openscreen
