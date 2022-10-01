// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_PUBLIC_DNS_SD_PUBLISHER_H_
#define DISCOVERY_DNSSD_PUBLIC_DNS_SD_PUBLISHER_H_

#include <string>

#include "discovery/dnssd/public/dns_sd_instance.h"
#include "platform/base/error.h"

namespace openscreen {
namespace discovery {

class DnsSdInstanceEndpoint;

class DnsSdPublisher {
 public:
  class Client {
   public:

    // Callback called when an endpoint is successfully claimed and published
    // via the Register() method. These values are expected to only differ in
    // the DnsSdInstance::instance_id() field, or to be equal. This callback is
    // purely for informational purposes and the caller is not required to act
    // on it.
    virtual void OnEndpointClaimed(
        const DnsSdInstance& requested_instance,
        const DnsSdInstanceEndpoint& claimed_endpoint) = 0;

   protected:
    virtual ~Client() = default;
  };

  virtual ~DnsSdPublisher() = default;

  // Publishes the PTR, SRV, TXT, A, and AAAA records provided in the
  // DnsSdInstance. If the record already exists, an error with code
  // kItemAlreadyExists is returned. On success, Error::None is returned.
  // NOTE: Some embedders may return errors on other conditions (for instance,
  // android will return an error if the resulting TXT record has values not
  // encodable with UTF8).
  virtual Error Register(const DnsSdInstance& instance, Client* client) = 0;

  // Updates the TXT, A, and AAAA records associated with the provided record,
  // if any changes have occurred. The instance and domain names must match
  // those of a previously published record. If either this is not true, no
  // changes have occurred, or additional embedder-specific requirements have
  // been violated, an error is returned. Else, Error::None is returned.
  virtual Error UpdateRegistration(const DnsSdInstance& instance) = 0;

  // Unpublishes any PTR, SRV, TXT, A, and AAAA records associated with this
  // service id, where the service id is the second part of the
  // <instance>.<service>.<domain> domain name as described in RFC 6763. If no
  // such records are published, this operation will be a no-op. Returns the
  // number of records which were removed, or an error code on error.
  virtual ErrorOr<int> DeregisterAll(const std::string& service) = 0;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_PUBLIC_DNS_SD_PUBLISHER_H_
