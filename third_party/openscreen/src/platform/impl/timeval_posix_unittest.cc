// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/timeval_posix.h"

#include "gtest/gtest.h"

namespace openscreen {

TEST(TimevalPosixTest, ToTimeval) {
  auto timespan = Clock::duration::zero();
  auto timeval = ToTimeval(timespan);
  EXPECT_EQ(timeval.tv_sec, 0);
  EXPECT_EQ(timeval.tv_usec, 0);

  timespan = Clock::duration(1000000);
  timeval = ToTimeval(timespan);
  EXPECT_EQ(timeval.tv_sec, 1);
  EXPECT_EQ(timeval.tv_usec, 0);

  timespan = Clock::duration(1000000 - 1);
  timeval = ToTimeval(timespan);
  EXPECT_EQ(timeval.tv_sec, 0);
  EXPECT_EQ(timeval.tv_usec, 1000000 - 1);

  timespan = Clock::duration(1);
  timeval = ToTimeval(timespan);
  EXPECT_EQ(timeval.tv_sec, 0);
  EXPECT_EQ(timeval.tv_usec, 1);

  timespan = Clock::duration(100000010);
  timeval = ToTimeval(timespan);
  EXPECT_EQ(timeval.tv_sec, 100);
  EXPECT_EQ(timeval.tv_usec, 10);
}

}  // namespace openscreen
