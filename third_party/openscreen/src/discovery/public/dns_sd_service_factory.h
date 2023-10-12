// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_PUBLIC_DNS_SD_SERVICE_FACTORY_H_
#define DISCOVERY_PUBLIC_DNS_SD_SERVICE_FACTORY_H_

#include <memory>

#include "discovery/dnssd/public/dns_sd_service.h"
#include "platform/api/task_runner_deleter.h"

namespace openscreen {

class TaskRunner;

namespace discovery {

struct Config;
class ReportingClient;

std::unique_ptr<DnsSdService, TaskRunnerDeleter> CreateDnsSdService(
    TaskRunner& task_runner,
    ReportingClient* reporting_client,
    const Config& config);

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_PUBLIC_DNS_SD_SERVICE_FACTORY_H_
