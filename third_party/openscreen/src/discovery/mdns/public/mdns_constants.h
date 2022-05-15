// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Collection of constants defined for multicast DNS (RFC 6762) and DNS-Based
// Service Discovery (RFC 6763), along with a few constants used specifically
// for cast receiver's implementation of the mDNS spec (not 100% conformant).
//
// Some cast-specific implementation details include the maximum multicast
// message size (synced with sender SDK) and usage of site-local MDNS group
// in addition to the default link-local MDNS multicast group.

#ifndef DISCOVERY_MDNS_PUBLIC_MDNS_CONSTANTS_H_
#define DISCOVERY_MDNS_PUBLIC_MDNS_CONSTANTS_H_

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <chrono>

#include "platform/api/time.h"
#include "platform/base/ip_address.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace discovery {

// ============================================================================
// Networking
// ============================================================================

// RFC 6762: https://www.ietf.org/rfc/rfc6762.txt
// RFC 2365: https://www.ietf.org/rfc/rfc2365.txt
// RFC 5771: https://www.ietf.org/rfc/rfc5771.txt
// RFC 7346: https://www.ietf.org/rfc/rfc7346.txt

// Default multicast port used by mDNS protocol. On some systems there may be
// multiple processes binding to same port, so prefer to allow address re-use.
// See RFC 6762, Section 2
constexpr uint16_t kDefaultMulticastPort = 5353;

// IPv4 group address for sending mDNS messages, given as byte array in
// network-order. This is a link-local multicast address, so messages will not
// be forwarded outside local network. See RFC 6762, section 3.
constexpr IPAddress kDefaultMulticastGroupIPv4{224, 0, 0, 251};

// IPv6 group address for sending mDNS messages. This is a link-local
// multicast address, so messages will not be forwarded outside local network.
// See RFC 6762, section 3.
constexpr IPAddress kDefaultMulticastGroupIPv6{
    0xFF02, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00FB,
};

// The send address for multicast mDNS should be the any address (0.*) on the
// default mDNS multicast port.
constexpr IPEndpoint kMulticastSendIPv4Endpoint{kDefaultMulticastGroupIPv4,
                                                kDefaultMulticastPort};
constexpr IPEndpoint kMulticastSendIPv6Endpoint{kDefaultMulticastGroupIPv6,
                                                kDefaultMulticastPort};

// IPv4 group address for joining cast-specific site-local mDNS multicast group,
// given as byte array in network-order. This is a site-local multicast address,
// so messages can extend past the local network to the administrative area
// boundary. Local sockets will filter out messages that are not on local-
// subnet though, so it will behave the same as link-local. The difference is
// that router sometimes treat link-local and site-local differently, which
// may cause link-local to have worse reliability than site-local.
// See https://tools.ietf.org/html/draft-cai-ssdp-v1-03

// 239.X.X.X is "administratively scoped IPv4 multicast space". See RFC 2365
// and RFC 5771, Section 3. Combined with relative address of 5 for SSDP this
// gives 239.255.255.250. See
// https://www.iana.org/assignments/multicast-addresses

// NOTE: For now the group address is the same group address used for SSDP
// discovery, albeit using the MDNS port rather than SSDP port.
constexpr IPAddress kDefaultSiteLocalGroupIPv4{239, 255, 255, 250};
constexpr IPEndpoint kDefaultSiteLocalGroupIPv4Endpoint{
    kDefaultSiteLocalGroupIPv4, kDefaultMulticastPort};

// IPv6 group address for joining cast-specific site-local mDNS multicast group,
// give as byte array in network-order. See comments for IPv4 group address for
// more details on site-local vs. link-local.
// 0xFF05 is site-local. See RFC 7346.
// FF0X:0:0:0:0:0:0:C is variable scope multicast addresses for SSDP. See
// https://www.iana.org/assignments/ipv6-multicast-addresses
constexpr IPAddress kDefaultSiteLocalGroupIPv6{
    0xFF05, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x000C,
};
constexpr IPEndpoint kDefaultSiteLocalGroupIPv6Endpoint{
    kDefaultSiteLocalGroupIPv6, kDefaultMulticastPort};

// Maximum MTU size (1500) minus the UDP header size (8) and IP header size
// (20). If any packets are larger than this size, the responder or sender
// should break up the message into multiple packets and set the TC
// (Truncated) bit to 1. See RFC 6762, section 7.2.
// TODO(https://crbug.com/openscreen/47): Figure out the exact size that the
// senders are using and sync with them. We want to verify that we are using the
// same maximum packet size. The spec also suggests keeping all UDP messsages
// below 512 bytes, since that is where some fragmentation may occur. If
// possible we should measure the rate of fragmented messages and see if
// lowering the max size alleviates it.
constexpr size_t kMaxMulticastMessageSize = 1500 - 20 - 8;

// Specifies whether the site-local group described above should be enabled
// by default. When enabled, the responder will be able to receive messages from
// that group; when disabled, only the default MDNS multicast group will be
// enabled.
constexpr bool kDefaultSupportSiteLocalGroup = true;

// ============================================================================
// Message Header
// ============================================================================

// RFC 1035: https://www.ietf.org/rfc/rfc1035.txt
// RFC 2535: https://www.ietf.org/rfc/rfc2535.txt

// DNS packet consists of a fixed-size header (12 bytes) followed by
// zero or more questions and/or records.
// For the meaning of specific fields, please see RFC 1035 and 2535.
//
// Header format:
//                                  1  1  1  1  1  1
//    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                      ID                       |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |QR|   Opcode  |AA|TC|RD|RA| Z|AD|CD|   RCODE   |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    QDCOUNT                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    ANCOUNT                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    NSCOUNT                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    ARCOUNT                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

// On-the-wire header. All uint16_t are in network order.
struct Header {
  uint16_t id;
  uint16_t flags;
  uint16_t question_count;
  uint16_t answer_count;
  uint16_t authority_record_count;
  uint16_t additional_record_count;
};

static_assert(sizeof(Header) == 12, "Size of mDNS header must be 12 bytes.");

enum class MessageType {
  Query = 0,
  Response = 1,
};

constexpr uint16_t kFlagResponse = 0x8000;
constexpr uint16_t kFlagAA = 0x0400;
constexpr uint16_t kFlagTC = 0x0200;
constexpr uint16_t kOpcodeMask = 0x7800;
constexpr uint16_t kRcodeMask = 0x000F;

constexpr MessageType GetMessageType(uint16_t flags) {
  // RFC 6762 Section 18.2
  return (flags & kFlagResponse) ? MessageType::Response : MessageType::Query;
}

constexpr bool IsMessageTruncated(uint16_t flags) {
  return flags & kFlagTC;
}

constexpr uint16_t MakeFlags(MessageType type, bool is_truncated) {
  // RFC 6762 Section 18.2 and Section 18.4
  uint16_t flags =
      (type == MessageType::Response) ? (kFlagResponse | kFlagAA) : 0;
  if (is_truncated) {
    flags |= kFlagTC;
  }
  return flags;
}

constexpr bool IsValidFlagsSection(uint16_t flags) {
  // RFC 6762 Section 18.3 and Section 18.11
  return (flags & (kOpcodeMask | kRcodeMask)) == 0;
}

// ============================================================================
// Domain Name
// ============================================================================

// RFC 1035: https://www.ietf.org/rfc/rfc1035.txt

// Maximum number of octets allowed in a single domain name including the
// terminating character octet
constexpr size_t kMaxDomainNameLength = 256;
// Maximum number of octets allowed in a single domain name label.
constexpr size_t kMaxLabelLength = 63;

// To allow for message compression, domain names can fall under the following
// categories:
// - A sequence of labels ending in a zero octet (label of length zero)
// - A pointer to prior occurance of name in message
// - A sequence of labels ending with a pointer.
//
// Domain Name Label - DIRECT:
//                                  1  1  1  1  1  1
//    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  | 0  0|  LABEL LENGTH   |        CHAR 0         |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  /         CHAR 1        |        CHAR 2         /
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//
// Domain Name Label - POINTER:
//                                  1  1  1  1  1  1
//    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  | 1  1|               OFFSET                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

constexpr uint8_t kLabelTermination = 0x00;
constexpr uint8_t kLabelTypeMask = 0xC0;
constexpr uint8_t kLabelDirect = 0x00;
constexpr uint8_t kLabelPointer = 0xC0;
constexpr uint8_t kLabelLengthMask = 0x3F;
constexpr uint16_t kLabelOffsetMask = 0x3FFF;

constexpr bool IsTerminationLabel(uint8_t label) {
  return label == kLabelTermination;
}

constexpr bool IsDirectLabel(uint8_t label) {
  return (label & kLabelTypeMask) == kLabelDirect;
}

constexpr bool IsPointerLabel(uint8_t label) {
  return (label & kLabelTypeMask) == kLabelPointer;
}

constexpr uint8_t GetDirectLabelLength(uint8_t label) {
  return label & kLabelLengthMask;
}

constexpr uint16_t GetPointerLabelOffset(uint16_t label) {
  return label & kLabelOffsetMask;
}

constexpr bool IsValidPointerLabelOffset(size_t offset) {
  return offset <= kLabelOffsetMask;
}

constexpr uint16_t MakePointerLabel(uint16_t offset) {
  return (uint16_t(kLabelPointer) << 8) | (offset & kLabelOffsetMask);
}

constexpr uint8_t MakeDirectLabel(uint8_t length) {
  return kLabelDirect | (length & kLabelLengthMask);
}

// ============================================================================
// Record Fields
// ============================================================================

// RFC 1035: https://www.ietf.org/rfc/rfc1035.txt
// RFC 2535: https://www.ietf.org/rfc/rfc2535.txt

// Question format:
//                                  1  1  1  1  1  1
//    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                                               |
//  /                     QNAME                     /
//  /                                               /
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                     QTYPE                     |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                     QCLASS                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

// Answer format:
//                                  1  1  1  1  1  1
//    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                                               |
//  /                                               /
//  /                      NAME                     /
//  |                                               |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                      TYPE                     |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                     CLASS                     |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                      TTL                      |
//  |                                               |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                   RDLENGTH                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
//  /                     RDATA                     /
//  /                                               /
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

// DNS TYPE values. See http://www.iana.org/assignments/dns-parameters for full
// list. Only a sub-set is used and listed here.
enum class DnsType : uint16_t {
  kA = 1,
  kPTR = 12,
  kTXT = 16,
  kAAAA = 28,
  kSRV = 33,
  kOPT = 41,
  kNSEC = 47,
  kANY = 255,  // Only allowed for QTYPE
};

inline std::ostream& operator<<(std::ostream& output, DnsType type) {
  switch (type) {
    case DnsType::kA:
      return output << "A";
    case DnsType::kPTR:
      return output << "PTR";
    case DnsType::kTXT:
      return output << "TXT";
    case DnsType::kAAAA:
      return output << "AAAA";
    case DnsType::kSRV:
      return output << "SRV";
    case DnsType::kOPT:
      return output << "OPT";
    case DnsType::kNSEC:
      return output << "NSEC";
    case DnsType::kANY:
      return output << "ANY";
  }

  OSP_NOTREACHED();
}

constexpr std::array<DnsType, 7> kSupportedDnsTypes = {
    DnsType::kA,   DnsType::kPTR,  DnsType::kTXT, DnsType::kAAAA,
    DnsType::kSRV, DnsType::kNSEC, DnsType::kANY};

enum class DnsClass : uint16_t {
  kIN = 1,
  kANY = 255,  // Only allowed for QCLASS
};

// Unique and shared records are described in
// https://tools.ietf.org/html/rfc6762#section-2 and
// https://tools.ietf.org/html/rfc6762#section-10.2
enum class RecordType {
  kShared = 0,
  kUnique = 1,
};

// Unicast and multicast preferred response types are described in
// https://tools.ietf.org/html/rfc6762#section-5.4
enum class ResponseType {
  kMulticast = 0,
  kUnicast = 1,
};

// These are the default TTL values for supported DNS Record types  as specified
// by RFC 6762 section 10.
constexpr std::chrono::seconds kPtrRecordTtl(120);
constexpr std::chrono::seconds kSrvRecordTtl(120);
constexpr std::chrono::seconds kARecordTtl(120);
constexpr std::chrono::seconds kAAAARecordTtl(120);
constexpr std::chrono::seconds kTXTRecordTtl(120);

// DNS CLASS masks and values.
// In mDNS the most significant bit of the RRCLASS for response records is
// designated as the "cache-flush bit", as described in
// https://tools.ietf.org/html/rfc6762#section-10.2
// In mDNS the most significant bit of the RRCLASS for query records is
// designated as the "unicast-response bit", as described in
// https://tools.ietf.org/html/rfc6762#section-5.4
constexpr uint16_t kClassMask = 0x7FFF;
constexpr uint16_t kClassMsbMask = 0x8000;
constexpr uint16_t kClassMsbShift = 0xF;

constexpr DnsClass GetDnsClass(uint16_t rrclass) {
  return static_cast<DnsClass>(rrclass & kClassMask);
}

constexpr RecordType GetRecordType(uint16_t rrclass) {
  return static_cast<RecordType>((rrclass & kClassMsbMask) >> kClassMsbShift);
}

constexpr ResponseType GetResponseType(uint16_t rrclass) {
  return static_cast<ResponseType>((rrclass & kClassMsbMask) >> kClassMsbShift);
}

constexpr uint16_t MakeRecordClass(DnsClass dns_class, RecordType record_type) {
  return static_cast<uint16_t>(dns_class) |
         (static_cast<uint16_t>(record_type) << kClassMsbShift);
}

constexpr uint16_t MakeQuestionClass(DnsClass dns_class,
                                     ResponseType response_type) {
  return static_cast<uint16_t>(dns_class) |
         (static_cast<uint16_t>(response_type) << kClassMsbShift);
}

// See RFC 6762, section 11: https://tools.ietf.org/html/rfc6762#section-11
//
// The IP TTL value for the UDP packets sent to the multicast group is advised
// to be 255 in order to be compatible with older DNS queriers. This also keeps
// consistent with other mDNS solutions (jMDNS, Avahi, etc.), which use 255
// as the IP TTL as well.
//
// The default mDNS group address is in a range of link-local addresses, so
// messages should not be forwarded by routers even when TTL is greater than 1.
constexpr uint32_t kDefaultRecordTTLSeconds = 255;

// ============================================================================
// RDATA Constants
// ============================================================================

// The maximum allowed size for a single entry in a TXT resource record. The
// mDNS spec specifies that entries in TXT record should be key/value pair
// separated by '=', so the size of the key + value + '=' should not exceed
// the maximum allowed size.
// See: https://tools.ietf.org/html/rfc6763#section-6.1
constexpr size_t kTXTMaxEntrySize = 255;
// Placeholder RDATA for "empty" TXT records that contains only a single
// zero length byte. This is required since TXT records CANNOT have empty
// RDATA sections.
// See RFC: https://tools.ietf.org/html/rfc6763#section-6.1
constexpr uint8_t kTXTEmptyRdata = 0;

// ============================================================================
// Probing Constants
// ============================================================================

// RFC 6762 section 8.1 specifies that a probe should wait 250 ms between
// subsequent probe queries.
constexpr Clock::duration kDelayBetweenProbeQueries =
    Clock::to_duration(std::chrono::milliseconds(250));

// RFC 6762 section 8.1 specifies that the probing phase should send out probe
// requests 3 times before treating the probe as completed.
constexpr int kProbeIterationCountBeforeSuccess = 3;

// ============================================================================
// OPT Pseudo-Record Constants
// ============================================================================

// For OPT records, the TTL field has been re-purposed as follows:
//
//                   +0 (MSB)                            +1 (LSB)
//        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//     0: |         EXTENDED-RCODE        |            VERSION            |
//        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//     2: | DO|                           Z                               |
//        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

constexpr uint32_t kExtendedRcodeMask = 0xFF000000;
constexpr int kExtendedRcodeShift = 24;
constexpr uint32_t kVersionMask = 0x00FF0000;
constexpr int kVersionShift = 16;
constexpr uint32_t kDnssecOkBitMask = 0x00008000;
constexpr uint8_t kVersionBadvers = 0x10;

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_MDNS_PUBLIC_MDNS_CONSTANTS_H_
