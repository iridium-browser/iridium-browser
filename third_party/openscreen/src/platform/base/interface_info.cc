// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/interface_info.h"

#include <algorithm>
#include <utility>

namespace openscreen {

InterfaceInfo::InterfaceInfo() = default;
InterfaceInfo::InterfaceInfo(NetworkInterfaceIndex index,
                             const uint8_t hardware_address[6],
                             std::string name,
                             Type type,
                             std::vector<IPSubnet> addresses)
    : index(index),
      hardware_address{hardware_address[0], hardware_address[1],
                       hardware_address[2], hardware_address[3],
                       hardware_address[4], hardware_address[5]},
      name(std::move(name)),
      type(type),
      addresses(std::move(addresses)) {}
InterfaceInfo::~InterfaceInfo() = default;

IPSubnet::IPSubnet() = default;
IPSubnet::IPSubnet(IPAddress address, uint8_t prefix_length)
    : address(std::move(address)), prefix_length(prefix_length) {}
IPSubnet::~IPSubnet() = default;

IPAddress InterfaceInfo::GetIpAddressV4() const {
  for (const auto& address : addresses) {
    if (address.address.IsV4()) {
      return address.address;
    }
  }
  return IPAddress{};
}

IPAddress InterfaceInfo::GetIpAddressV6() const {
  for (const auto& address : addresses) {
    if (address.address.IsV6()) {
      return address.address;
    }
  }
  return IPAddress{};
}

bool InterfaceInfo::HasHardwareAddress() const {
  return std::any_of(hardware_address.begin(), hardware_address.end(),
                     [](uint8_t e) { return e != 0; });
}

std::ostream& operator<<(std::ostream& out, const IPSubnet& subnet) {
  if (subnet.address.IsV6()) {
    out << '[';
  }
  out << subnet.address;
  if (subnet.address.IsV6()) {
    out << ']';
  }
  return out << '/' << std::dec << static_cast<int>(subnet.prefix_length);
}

std::ostream& operator<<(std::ostream& out, InterfaceInfo::Type type) {
  switch (type) {
    case InterfaceInfo::Type::kEthernet:
      out << "Ethernet";
      break;
    case InterfaceInfo::Type::kWifi:
      out << "Wifi";
      break;
    case InterfaceInfo::Type::kLoopback:
      out << "Loopback";
      break;
    case InterfaceInfo::Type::kOther:
      out << "Other";
      break;
  }

  return out;
}

std::ostream& operator<<(std::ostream& out, const InterfaceInfo& info) {
  out << '{' << info.index << " (a.k.a. " << info.name
      << "); media_type=" << info.type << "; MAC=" << std::hex
      << static_cast<int>(info.hardware_address[0]);
  for (size_t i = 1; i < info.hardware_address.size(); ++i) {
    out << ':' << static_cast<int>(info.hardware_address[i]);
  }
  for (const IPSubnet& ip : info.addresses) {
    out << "; " << ip;
  }
  return out << '}';
}

}  // namespace openscreen
