// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_IMPL_INSTANCE_KEY_H_
#define DISCOVERY_DNSSD_IMPL_INSTANCE_KEY_H_

#include <string>
#include <utility>

#include "absl/strings/string_view.h"
#include "discovery/dnssd/impl/service_key.h"

namespace openscreen {
namespace discovery {

class DnsSdInstance;
class DomainName;
class MdnsRecord;
class ServiceKey;

// This class is intended to be used as the key of a std::unordered_map or
// std::map  when referencing data related to a specific service instance.
class InstanceKey : public ServiceKey {
 public:
  // NOTE: The record provided must have valid instance, service, and domain
  // labels.
  explicit InstanceKey(const MdnsRecord& record);
  explicit InstanceKey(const DomainName& domain);
  explicit InstanceKey(const DnsSdInstance& instance);

  // NOTE: The provided parameters must be valid instance,  service and domain
  // ids.
  InstanceKey(absl::string_view instance,
              absl::string_view service,
              absl::string_view domain);

  InstanceKey(const InstanceKey& other);
  InstanceKey(InstanceKey&& other);

  InstanceKey& operator=(const InstanceKey& rhs);
  InstanceKey& operator=(InstanceKey&& rhs);

  DomainName GetName() const override;

  const std::string& instance_id() const { return instance_id_; }

 private:
  std::string instance_id_;

  template <typename H>
  friend H AbslHashValue(H h, const InstanceKey& key);

  friend bool operator<(const InstanceKey& lhs, const InstanceKey& rhs);
};

template <typename H>
H AbslHashValue(H h, const InstanceKey& key) {
  return H::combine(std::move(h), key.service_id(), key.domain_id(),
                    key.instance_id_);
}

inline bool operator<(const InstanceKey& lhs, const InstanceKey& rhs) {
  int comp = lhs.domain_id().compare(rhs.domain_id());
  if (comp != 0) {
    return comp < 0;
  }

  comp = lhs.service_id().compare(rhs.service_id());
  if (comp != 0) {
    return comp < 0;
  }

  return lhs.instance_id_ < rhs.instance_id_;
}

inline bool operator>(const InstanceKey& lhs, const InstanceKey& rhs) {
  return rhs < lhs;
}

inline bool operator<=(const InstanceKey& lhs, const InstanceKey& rhs) {
  return !(rhs > lhs);
}

inline bool operator>=(const InstanceKey& lhs, const InstanceKey& rhs) {
  return !(rhs < lhs);
}

inline bool operator==(const InstanceKey& lhs, const InstanceKey& rhs) {
  return lhs <= rhs && lhs >= rhs;
}

inline bool operator!=(const InstanceKey& lhs, const InstanceKey& rhs) {
  return !(lhs == rhs);
}

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_IMPL_INSTANCE_KEY_H_
