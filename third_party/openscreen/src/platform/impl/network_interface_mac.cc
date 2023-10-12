// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

// net/if.h must be included before this.
#include <ifaddrs.h>

#include <algorithm>
#include <string>
#include <vector>

#include "platform/api/network_interface.h"
#include "platform/base/ip_address.h"
#include "platform/impl/network_interface.h"
#include "platform/impl/scoped_pipe.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {

namespace {

// Assuming |netmask| consists of 0 to N*8 leftmost bits set followed by all
// unset bits, return the number of leftmost bits set. This also sanity-checks
// that there are no "holes" in the bit pattern, returning 0 if that check
// fails.
template <size_t N>
uint8_t ToPrefixLength(const uint8_t (&netmask)[N]) {
  uint8_t result = 0;
  size_t i = 0;

  // Ensure all of the leftmost bits are set.
  while (i < N && netmask[i] == UINT8_C(0xff)) {
    result += 8;
    ++i;
  }

  // Check the intermediate byte, the first that is not 0xFF,
  // e.g. 0b11100000 or 0x00
  if (i < N && netmask[i] != UINT8_C(0x00)) {
    uint8_t last_byte = netmask[i];
    // Check the left most bit, bitshifting as we go.
    while (last_byte & UINT8_C(0x80)) {
      ++result;
      last_byte <<= 1;
    }
    OSP_CHECK(last_byte == UINT8_C(0x00));
    ++i;
  }

  // Ensure the rest of the bytes are zeroed out.
  while (i < N) {
    OSP_CHECK(netmask[i] == UINT8_C(0x00));
    ++i;
  }

  return result;
}

std::vector<InterfaceInfo> ProcessInterfacesList(ifaddrs* interfaces) {
  // Socket used for querying interface media types.
  const ScopedFd ioctl_socket(socket(AF_INET6, SOCK_DGRAM, 0));

  // Walk the |interfaces| linked list, creating the hierarchical structure.
  std::vector<InterfaceInfo> results;
  for (ifaddrs* cur = interfaces; cur; cur = cur->ifa_next) {
    // Skip: 1) interfaces that are down, 2) interfaces with no address
    // configured.
    if (!(IFF_RUNNING & cur->ifa_flags) || !cur->ifa_addr) {
      continue;
    }

    // Look-up the InterfaceInfo entry by name. Auto-create a new one if none by
    // the current name exists in |results|.
    const std::string name = cur->ifa_name;
    const auto it = std::find_if(
        results.begin(), results.end(),
        [&name](const InterfaceInfo& info) { return info.name == name; });
    InterfaceInfo* interface;
    if (it == results.end()) {
      InterfaceInfo::Type type = InterfaceInfo::Type::kOther;
      // Query for the interface media type and status. If not valid/active,
      // skip further processing. Note that "active" here means the media is
      // connected to the interface, which is different than the interface being
      // up/down.
      ifmediareq ifmr;
      memset(&ifmr, 0, sizeof(ifmr));
      // Note: Because of the memset(), memcpy() can be used to copy the
      // ifmr.ifm_name string, and it will always be NUL terminated.
      memcpy(ifmr.ifm_name, name.data(),
             std::min(name.size(), sizeof(ifmr.ifm_name) - 1));
      if (ioctl(ioctl_socket.get(), SIOCGIFMEDIA, &ifmr) >= 0) {
        if (!((ifmr.ifm_status & IFM_AVALID) &&
              (ifmr.ifm_status & IFM_ACTIVE))) {
          continue;  // Skip this interface since it's not valid or active.
        }
        if (ifmr.ifm_current & IFM_IEEE80211) {
          type = InterfaceInfo::Type::kWifi;
        } else if (ifmr.ifm_current & IFM_ETHER) {
          type = InterfaceInfo::Type::kEthernet;
        }
      } else if (cur->ifa_flags & IFF_LOOPBACK) {
        type = InterfaceInfo::Type::kLoopback;
      } else {
        continue;
      }

      // Start with an unknown hardware ethernet address, which should be
      // updated as the linked list is walked.
      const uint8_t kUnknownHardwareAddress[6] = {0, 0, 0, 0, 0, 0};
      results.emplace_back(if_nametoindex(cur->ifa_name),
                           kUnknownHardwareAddress, name, type,
                           // IPSubnets to be filled-in later.
                           std::vector<IPSubnet>());
      interface = &(results.back());
    } else {
      interface = &(*it);
    }

    // Add another address to the list of addresses for the current interface.
    if (cur->ifa_addr->sa_family == AF_LINK) {  // Hardware ethernet address.
      auto* const addr_dl = reinterpret_cast<const sockaddr_dl*>(cur->ifa_addr);
      const caddr_t lladdr = LLADDR(addr_dl);
      static_assert(sizeof(lladdr) >= sizeof(interface->hardware_address),
                    "Platform defines too-small link addresses?");
      memcpy(&interface->hardware_address[0], &lladdr[0],
             sizeof(interface->hardware_address));
    } else if (cur->ifa_addr->sa_family == AF_INET6) {  // Ipv6 address.
      struct in6_ifreq ifr = {};
      // Reject network interfaces that have a deprecated flag set.
      strncpy(ifr.ifr_name, cur->ifa_name, sizeof(ifr.ifr_name) - 1);
      memcpy(&ifr.ifr_ifru.ifru_addr, cur->ifa_addr, cur->ifa_addr->sa_len);
      if (ioctl(ioctl_socket.get(), SIOCGIFAFLAG_IN6, &ifr) != 0 ||
          ifr.ifr_ifru.ifru_flags & IN6_IFF_DEPRECATED) {
        continue;
      }

      auto* const addr_in6 =
          reinterpret_cast<const sockaddr_in6*>(cur->ifa_addr);
      uint8_t tmp[sizeof(addr_in6->sin6_addr.s6_addr)];
      memcpy(tmp, &(addr_in6->sin6_addr.s6_addr), sizeof(tmp));
      const IPAddress ip(IPAddress::Version::kV6, tmp);
      memset(tmp, 0, sizeof(tmp));
      if (cur->ifa_netmask && cur->ifa_netmask->sa_family == AF_INET6) {
        memcpy(tmp,
               &(reinterpret_cast<const sockaddr_in6*>(cur->ifa_netmask)
                     ->sin6_addr.s6_addr),
               sizeof(tmp));
      }
      interface->addresses.emplace_back(ip, ToPrefixLength(tmp));
    } else if (cur->ifa_addr->sa_family == AF_INET) {  // Ipv4 address.
      auto* const addr_in = reinterpret_cast<const sockaddr_in*>(cur->ifa_addr);
      uint8_t tmp[sizeof(addr_in->sin_addr.s_addr)];
      memcpy(tmp, &(addr_in->sin_addr.s_addr), sizeof(tmp));
      IPAddress ip(IPAddress::Version::kV4, tmp);
      memset(tmp, 0, sizeof(tmp));
      if (cur->ifa_netmask && cur->ifa_netmask->sa_family == AF_INET) {
        memcpy(tmp,
               &(reinterpret_cast<const sockaddr_in*>(cur->ifa_netmask)
                     ->sin_addr.s_addr),
               sizeof(tmp));
      }
      interface->addresses.emplace_back(ip, ToPrefixLength(tmp));
    }
  }

  return results;
}

}  // namespace

std::vector<InterfaceInfo> GetNetworkInterfaces() {
  std::vector<InterfaceInfo> results;
  ifaddrs* interfaces;
  if (getifaddrs(&interfaces) == 0) {
    results = ProcessInterfacesList(interfaces);
    freeifaddrs(interfaces);
  }
  return results;
}

}  // namespace openscreen
