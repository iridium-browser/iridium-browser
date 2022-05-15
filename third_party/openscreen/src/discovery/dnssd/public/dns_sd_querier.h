// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_PUBLIC_DNS_SD_QUERIER_H_
#define DISCOVERY_DNSSD_PUBLIC_DNS_SD_QUERIER_H_

#include "discovery/dnssd/public/dns_sd_instance_endpoint.h"

namespace openscreen {
namespace discovery {

class DnsSdQuerier {
 public:
  // TODO(rwkeane): Add support for expiring records in addition to deleting
  // them.
  class Callback {
   public:
    virtual ~Callback() = default;

    // Callback fired when a new InstanceEndpoint is created.
    // NOTE: This callback may not modify the DnsSdQuerier instance from which
    // it is called.
    virtual void OnEndpointCreated(
        const DnsSdInstanceEndpoint& new_endpoint) = 0;

    // Callback fired when an existing InstanceEndpoint is updated.
    // NOTE: This callback may not modify the DnsSdQuerier instance from which
    // it is called.
    virtual void OnEndpointUpdated(
        const DnsSdInstanceEndpoint& modified_endpoint) = 0;

    // Callback fired when an existing InstanceEndpoint is deleted.
    // NOTE: This callback may not modify the DnsSdQuerier instance from which
    // it is called.
    virtual void OnEndpointDeleted(
        const DnsSdInstanceEndpoint& old_endpoint) = 0;
  };

  virtual ~DnsSdQuerier() = default;

  // Begins a new query. The provided callback will be called whenever new
  // information about the provided (service, domain) pair becomes available.
  // The Callback provided is expected to persist until the StopQuery method is
  // called or this instance is destroyed.
  // NOTE: The provided service value is expected to be valid, as defined by the
  // IsServiceValid() method.
  // NOTE: The callback must be called on the TaskRunner thread.
  virtual void StartQuery(const std::string& service, Callback* cb) = 0;

  // Stops an already running query.
  // NOTE: The provided service value is expected to be valid, as defined by the
  // IsServiceValid() method.
  virtual void StopQuery(const std::string& service, Callback* cb) = 0;

  // Re-initializes the process of service discovery for the provided service
  // id. All ongoing queries for this domain are restarted and any previously
  // received query results are discarded.
  virtual void ReinitializeQueries(const std::string& service) = 0;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_PUBLIC_DNS_SD_QUERIER_H_
