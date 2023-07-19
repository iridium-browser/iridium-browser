// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_IMPL_SERVICE_KEY_H_
#define DISCOVERY_DNSSD_IMPL_SERVICE_KEY_H_

#include <string>
#include <utility>

#include "absl/strings/string_view.h"
#include "platform/base/error.h"

namespace openscreen {
namespace discovery {

class DomainName;
class MdnsRecord;

// This class is intended to be used as the key of a std::unordered_map or a
// std::map when referencing data related to a service type
class ServiceKey {
 public:
  // NOTE: The record provided must have valid service domain labels.
  explicit ServiceKey(const MdnsRecord& record);
  explicit ServiceKey(const DomainName& domain);
  virtual ~ServiceKey() = default;

  // NOTE: The provided service and domain labels must be valid.
  ServiceKey(absl::string_view service, absl::string_view domain);
  ServiceKey(const ServiceKey& other);
  ServiceKey(ServiceKey&& other);

  ServiceKey& operator=(const ServiceKey& rhs);
  ServiceKey& operator=(ServiceKey&& rhs);

  virtual DomainName GetName() const;

  const std::string& service_id() const { return service_id_; }
  const std::string& domain_id() const { return domain_id_; }

 private:
  static ErrorOr<ServiceKey> TryCreate(const MdnsRecord& record);
  static ErrorOr<ServiceKey> TryCreate(const DomainName& domain);

  std::string service_id_;
  std::string domain_id_;

  template <typename H>
  friend H AbslHashValue(H h, const ServiceKey& key);

  friend bool operator<(const ServiceKey& lhs, const ServiceKey& rhs);

  // Validation method which needs the same code as CreateFromRecord(). Use a
  // friend declaration to avoid duplicating this code while still keeping the
  // factory private.
  friend bool HasValidDnsRecordAddress(const MdnsRecord& record);
  friend bool HasValidDnsRecordAddress(const DomainName& domain);
};

// Hashing functions to allow for using with absl::Hash<...>.
template <typename H>
H AbslHashValue(H h, const ServiceKey& key) {
  return H::combine(std::move(h), key.service_id_, key.domain_id_);
}

// Comparison operators for using above keys with an std::map
inline bool operator<(const ServiceKey& lhs, const ServiceKey& rhs) {
  int comp = lhs.domain_id_.compare(rhs.domain_id_);
  if (comp != 0) {
    return comp < 0;
  }
  return lhs.service_id_ < rhs.service_id_;
}

inline bool operator>(const ServiceKey& lhs, const ServiceKey& rhs) {
  return rhs < lhs;
}

inline bool operator<=(const ServiceKey& lhs, const ServiceKey& rhs) {
  return !(rhs > lhs);
}

inline bool operator>=(const ServiceKey& lhs, const ServiceKey& rhs) {
  return !(rhs < lhs);
}

inline bool operator==(const ServiceKey& lhs, const ServiceKey& rhs) {
  return lhs <= rhs && lhs >= rhs;
}

inline bool operator!=(const ServiceKey& lhs, const ServiceKey& rhs) {
  return !(lhs == rhs);
}

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_IMPL_SERVICE_KEY_H_
