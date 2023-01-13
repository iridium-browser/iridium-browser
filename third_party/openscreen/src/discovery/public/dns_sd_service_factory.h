// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_PUBLIC_DNS_SD_SERVICE_FACTORY_H_
#define DISCOVERY_PUBLIC_DNS_SD_SERVICE_FACTORY_H_

#include "discovery/dnssd/public/dns_sd_service.h"
#include "platform/api/serial_delete_ptr.h"

namespace openscreen {

class TaskRunner;

namespace discovery {

struct Config;
class ReportingClient;

SerialDeletePtr<DnsSdService> CreateDnsSdService(
    TaskRunner* task_runner,
    ReportingClient* reporting_client,
    const Config& config);

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_PUBLIC_DNS_SD_SERVICE_FACTORY_H_
