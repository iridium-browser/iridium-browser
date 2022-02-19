// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_TIMEVAL_POSIX_H_
#define PLATFORM_IMPL_TIMEVAL_POSIX_H_

#include <sys/time.h>  // timeval

#include "platform/api/time.h"

namespace openscreen {

struct timeval ToTimeval(const Clock::duration& timeout);

}  // namespace openscreen

#endif  // PLATFORM_IMPL_TIMEVAL_POSIX_H_
