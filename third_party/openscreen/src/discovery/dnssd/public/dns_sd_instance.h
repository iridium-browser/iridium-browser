// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_PUBLIC_DNS_SD_INSTANCE_H_
#define DISCOVERY_DNSSD_PUBLIC_DNS_SD_INSTANCE_H_

#include <string>
#include <utility>
#include <vector>

#include "discovery/dnssd/public/dns_sd_txt_record.h"
#include "platform/base/ip_address.h"
#include "util/std_util.h"

namespace openscreen {
namespace discovery {

bool IsInstanceValid(const std::string& instance);
bool IsServiceValid(const std::string& service);
bool IsDomainValid(const std::string& domain);

// Represents the data stored in DNS records of types SRV, TXT, A, and AAAA
class DnsSdInstance {
 public:
  using Subtype = std::string;

  // These ctors expect valid input, and will cause a crash if they are not.
  DnsSdInstance(std::string instance_id,
                std::string service_id,
                std::string domain_id,
                DnsSdTxtRecord txt,
                uint16_t port,
                std::vector<Subtype> subtypes);

  template <typename... T>
  DnsSdInstance(std::string instance_id,
                std::string service_id,
                std::string domain_id,
                DnsSdTxtRecord txt,
                uint16_t port,
                T... subtypes)
      : DnsSdInstance(std::move(instance_id),
                      std::move(service_id),
                      std::move(domain_id),
                      std::move(txt),
                      port,
                      std::vector<Subtype>{std::move(subtypes)...}) {}

  DnsSdInstance(const DnsSdInstance& other);
  DnsSdInstance(DnsSdInstance&& other);

  virtual ~DnsSdInstance();

  DnsSdInstance& operator=(const DnsSdInstance& rhs);
  DnsSdInstance& operator=(DnsSdInstance&& rhs);

  // Returns the instance name for this DNS-SD record.
  const std::string& instance_id() const { return instance_id_; }

  // Returns the service id for this DNS-SD record.
  const std::string& service_id() const { return service_id_; }

  // Returns the domain id for this DNS-SD record.
  const std::string& domain_id() const { return domain_id_; }

  // Returns the TXT record associated with this DNS-SD record
  const DnsSdTxtRecord& txt() const { return txt_; }

  // Returns the port associated with this instance record.
  uint16_t port() const { return port_; }

  // The set of subtypes for this instance.
  const std::vector<Subtype>& subtypes() { return subtypes_; }

 private:
  std::string instance_id_;
  std::string service_id_;
  std::string domain_id_;
  DnsSdTxtRecord txt_;
  uint16_t port_;

  // Subtypes of this instance which have been received so far.
  // NOTE: Subtypes are stored in sorted order to simplify comparison.
  // NOTE: This vector will always be empty for incoming queries and will not be
  // respected for publications. It is only present for future use.
  //
  // TODO(issuetracker.google.com/158533407): Implement use of this field.
  std::vector<Subtype> subtypes_;

  friend bool operator<(const DnsSdInstance& lhs, const DnsSdInstance& rhs);
};

bool IsSubtypeValid(const DnsSdInstance::Subtype& subtype);

bool IsValid(const DnsSdInstance::Subtype& subtype);

bool operator<(const DnsSdInstance& lhs, const DnsSdInstance& rhs);

inline bool operator>(const DnsSdInstance& lhs, const DnsSdInstance& rhs) {
  return rhs < lhs;
}

inline bool operator<=(const DnsSdInstance& lhs, const DnsSdInstance& rhs) {
  return !(lhs > rhs);
}

inline bool operator>=(const DnsSdInstance& lhs, const DnsSdInstance& rhs) {
  return !(lhs < rhs);
}

inline bool operator==(const DnsSdInstance& lhs, const DnsSdInstance& rhs) {
  return lhs <= rhs && lhs >= rhs;
}

inline bool operator!=(const DnsSdInstance& lhs, const DnsSdInstance& rhs) {
  return !(lhs == rhs);
}

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_PUBLIC_DNS_SD_INSTANCE_H_
