// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_COMMON_REPORTING_CLIENT_H_
#define DISCOVERY_COMMON_REPORTING_CLIENT_H_

#include "platform/base/error.h"

namespace openscreen {
namespace discovery {

// This class is implemented by the embedder who wishes to use discovery. The
// discovery implementation will use this API to report back errors and metrics.
// NOTE: All methods in the reporting client will be called from the task runner
// thread.
// TODO(rwkeane): Report state changes back to the caller.
class ReportingClient {
 public:
  virtual ~ReportingClient() = default;

  // This method is called when an error is detected by the underlying
  // infrastructure from which recovery cannot be initiated. For example, an
  // error binding a multicast socket.
  virtual void OnFatalError(Error error) = 0;

  // This method is called when an error is detected by the underlying
  // infrastructure which does not prevent further functionality of the runtime.
  // For example, a conversion failure between DnsSdInstanceRecord and the
  // externally supplied class.
  virtual void OnRecoverableError(Error error) = 0;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_COMMON_REPORTING_CLIENT_H_
