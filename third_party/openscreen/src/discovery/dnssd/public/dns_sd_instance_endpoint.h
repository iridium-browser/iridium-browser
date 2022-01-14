// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_PUBLIC_DNS_SD_INSTANCE_ENDPOINT_H_
#define DISCOVERY_DNSSD_PUBLIC_DNS_SD_INSTANCE_ENDPOINT_H_

#include <string>
#include <utility>
#include <vector>

#include "discovery/dnssd/public/dns_sd_instance.h"
#include "discovery/dnssd/public/dns_sd_txt_record.h"
#include "platform/base/interface_info.h"
#include "platform/base/ip_address.h"

namespace openscreen {
namespace discovery {

// Represents the data stored in DNS records of types SRV, TXT, A, and AAAA
class DnsSdInstanceEndpoint : public DnsSdInstance {
 public:
  using DnsSdInstance::Subtype;

  // These ctors expect valid input, and will cause a crash if they are not.
  // Additionally, these ctors expect at least one IPAddress will be provided.
  DnsSdInstanceEndpoint(DnsSdInstance record,
                        NetworkInterfaceIndex network_interface,
                        std::vector<IPEndpoint> address);

  DnsSdInstanceEndpoint(std::string instance_id,
                        std::string service_id,
                        std::string domain_id,
                        DnsSdTxtRecord txt,
                        NetworkInterfaceIndex network_interface,
                        std::vector<IPEndpoint> endpoint);
  DnsSdInstanceEndpoint(std::string instance_id,
                        std::string service_id,
                        std::string domain_id,
                        DnsSdTxtRecord txt,
                        NetworkInterfaceIndex network_interface,
                        std::vector<IPEndpoint> endpoint,
                        std::vector<Subtype> subtypes);

  // Overloads of the above ctors to allow for simpler creation. The same
  // expectations as above apply.
  template <typename... TEndpoints>
  DnsSdInstanceEndpoint(DnsSdInstance record,
                        NetworkInterfaceIndex network_interface,
                        TEndpoints... endpoints)
      : DnsSdInstanceEndpoint(
            std::move(record),
            network_interface,
            std::vector<IPEndpoint>{std::move(endpoints)...}) {}

  // NOTE: All subtypes must follow all IPEndpoints.
  template <typename... Types>
  DnsSdInstanceEndpoint(std::string instance_id,
                        std::string service_id,
                        std::string domain_id,
                        DnsSdTxtRecord txt,
                        NetworkInterfaceIndex network_interface,
                        IPEndpoint endpoint,
                        Types... types)
      : DnsSdInstanceEndpoint(
            std::move(instance_id),
            std::move(service_id),
            std::move(domain_id),
            std::move(txt),
            network_interface,
            GetVectorWithCapacity<IPEndpoint>(sizeof...(Types) + 1),
            GetVectorWithCapacity<Subtype>(sizeof...(Types)),
            std::move(endpoint),
            std::move(types)...) {}

  DnsSdInstanceEndpoint(const DnsSdInstanceEndpoint& other);
  DnsSdInstanceEndpoint(DnsSdInstanceEndpoint&& other);

  ~DnsSdInstanceEndpoint() override;

  DnsSdInstanceEndpoint& operator=(const DnsSdInstanceEndpoint& rhs);
  DnsSdInstanceEndpoint& operator=(DnsSdInstanceEndpoint&& rhs);

  // Returns the address associated with this DNS-SD record. In any valid
  // record, at least one will be set.
  const std::vector<IPAddress>& addresses() const { return addresses_; }
  const std::vector<IPEndpoint>& endpoints() const { return endpoints_; }

  // Network Interface associated with this endpoint.
  NetworkInterfaceIndex network_interface() const { return network_interface_; }

 private:
  // Pick off the first IPEndpoint then call again recursively.
  template <typename... Types>
  DnsSdInstanceEndpoint(std::string instance_id,
                        std::string service_id,
                        std::string domain_id,
                        DnsSdTxtRecord txt,
                        NetworkInterfaceIndex network_interface,
                        std::vector<IPEndpoint> endpoints,
                        std::vector<Subtype> subtypes,
                        IPEndpoint endpoint,
                        Types... types)
      : DnsSdInstanceEndpoint(std::move(instance_id),
                              std::move(service_id),
                              std::move(domain_id),
                              std::move(txt),
                              network_interface,
                              Append(std::move(endpoints), std::move(endpoint)),
                              std::move(subtypes),
                              std::move(types)...) {}

  // All following arguments must be Subtypes, so pull them all off and recurse
  // to a non-templated ctor.
  template <typename... Types>
  DnsSdInstanceEndpoint(std::string instance_id,
                        std::string service_id,
                        std::string domain_id,
                        DnsSdTxtRecord txt,
                        NetworkInterfaceIndex network_interface,
                        std::vector<IPEndpoint> endpoints,
                        std::vector<Subtype> subtypes,
                        Subtype subtype,
                        Types... types)
      : DnsSdInstanceEndpoint(std::move(instance_id),
                              std::move(service_id),
                              std::move(domain_id),
                              std::move(txt),
                              network_interface,
                              std::move(endpoints),
                              Append(std::move(subtypes),
                                     std::move(subtype),
                                     std::move(types)...)) {}

  // Lazy Initializes the |addresses_| vector.
  const std::vector<IPAddress>& CalculateAddresses() const;

  // Initialized the |endpoints_| vector after construction.
  void InitializeEndpoints();

  // NOTE: The below vector is stored in sorted order to make comparison
  // simpler.
  std::vector<IPEndpoint> endpoints_;
  std::vector<IPAddress> addresses_;

  NetworkInterfaceIndex network_interface_;

  friend bool operator<(const DnsSdInstanceEndpoint& lhs,
                        const DnsSdInstanceEndpoint& rhs);
};

bool operator<(const DnsSdInstanceEndpoint& lhs,
               const DnsSdInstanceEndpoint& rhs);

inline bool operator>(const DnsSdInstanceEndpoint& lhs,
                      const DnsSdInstanceEndpoint& rhs) {
  return rhs < lhs;
}

inline bool operator<=(const DnsSdInstanceEndpoint& lhs,
                       const DnsSdInstanceEndpoint& rhs) {
  return !(lhs > rhs);
}

inline bool operator>=(const DnsSdInstanceEndpoint& lhs,
                       const DnsSdInstanceEndpoint& rhs) {
  return !(lhs < rhs);
}

inline bool operator==(const DnsSdInstanceEndpoint& lhs,
                       const DnsSdInstanceEndpoint& rhs) {
  return lhs <= rhs && lhs >= rhs;
}

inline bool operator!=(const DnsSdInstanceEndpoint& lhs,
                       const DnsSdInstanceEndpoint& rhs) {
  return !(lhs == rhs);
}

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_PUBLIC_DNS_SD_INSTANCE_ENDPOINT_H_
