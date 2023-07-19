// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_IMPL_CONSTANTS_H_
#define DISCOVERY_DNSSD_IMPL_CONSTANTS_H_

#include <string>
#include <utility>

#include "discovery/mdns/mdns_records.h"
#include "discovery/mdns/public/mdns_constants.h"

namespace openscreen {
namespace discovery {

// This is the DNS Information required to start a new query.
struct DnsQueryInfo {
  DomainName name;
  DnsType dns_type;
  DnsClass dns_class;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_IMPL_CONSTANTS_H_
