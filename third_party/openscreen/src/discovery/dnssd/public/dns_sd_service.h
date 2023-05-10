// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_PUBLIC_DNS_SD_SERVICE_H_
#define DISCOVERY_DNSSD_PUBLIC_DNS_SD_SERVICE_H_

#include <functional>
#include <memory>

#include "platform/base/error.h"
#include "platform/base/interface_info.h"
#include "platform/base/ip_address.h"

namespace openscreen {

struct IPEndpoint;
class TaskRunner;

namespace discovery {

struct Config;
class DnsSdPublisher;
class DnsSdQuerier;
class ReportingClient;

// This class provides a wrapper around DnsSdQuerier and DnsSdPublisher to
// allow for an embedder-overridable factory method below.
class DnsSdService {
 public:
  virtual ~DnsSdService() = default;

  // Returns the DnsSdQuerier owned by this DnsSdService. If queries are not
  // supported, returns nullptr.
  virtual DnsSdQuerier* GetQuerier() = 0;

  // Returns the DnsSdPublisher owned by this DnsSdService. If publishing is not
  // supported, returns nullptr.
  virtual DnsSdPublisher* GetPublisher() = 0;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_PUBLIC_DNS_SD_SERVICE_H_
