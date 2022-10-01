// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_COMMON_TESTING_MOCK_REPORTING_CLIENT_H_
#define DISCOVERY_COMMON_TESTING_MOCK_REPORTING_CLIENT_H_

#include "discovery/common/reporting_client.h"
#include "gmock/gmock.h"

namespace openscreen {
namespace discovery {

class MockReportingClient : public ReportingClient {
 public:
  MOCK_METHOD1(OnFatalError, void(Error error));
  MOCK_METHOD1(OnRecoverableError, void(Error error));
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_COMMON_TESTING_MOCK_REPORTING_CLIENT_H_
