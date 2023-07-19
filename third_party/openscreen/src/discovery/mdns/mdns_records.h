// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_MDNS_MDNS_RECORDS_H_
#define DISCOVERY_MDNS_MDNS_RECORDS_H_

#include <algorithm>
#include <chrono>
#include <functional>
#include <initializer_list>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/ascii.h"
#include "absl/strings/string_view.h"
#include "absl/types/variant.h"
#include "discovery/mdns/public/mdns_constants.h"
#include "platform/base/error.h"
#include "platform/base/interface_info.h"
#include "platform/base/ip_address.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace discovery {

bool IsValidDomainLabel(absl::string_view label);

// Represents domain name as a collection of labels, ensures label length and
// domain name length requirements are met.
class DomainName {
 public:
  DomainName();

  template <typename IteratorType>
  static ErrorOr<DomainName> TryCreate(IteratorType first, IteratorType last) {
    std::vector<std::string> labels;
    size_t max_wire_size = 1;
    labels.reserve(std::distance(first, last));
    for (IteratorType entry = first; entry != last; ++entry) {
      if (!IsValidDomainLabel(*entry)) {
        return Error::Code::kParameterInvalid;
      }
      labels.emplace_back(*entry);
      // Include the length byte in the size calculation.
      max_wire_size += entry->size() + 1;
    }

    if (max_wire_size > kMaxDomainNameLength) {
      return Error::Code::kIndexOutOfBounds;
    } else {
      return DomainName(std::move(labels), max_wire_size);
    }
  }

  template <typename IteratorType>
  DomainName(IteratorType first, IteratorType last) {
    ErrorOr<DomainName> domain = TryCreate(first, last);
    *this = std::move(domain.value());
  }
  explicit DomainName(std::vector<std::string> labels);
  explicit DomainName(const std::vector<absl::string_view>& labels);
  explicit DomainName(std::initializer_list<absl::string_view> labels);
  DomainName(const DomainName& other);
  DomainName(DomainName&& other) noexcept;

  DomainName& operator=(const DomainName& rhs);
  DomainName& operator=(DomainName&& rhs);
  bool operator<(const DomainName& rhs) const;
  bool operator<=(const DomainName& rhs) const;
  bool operator>(const DomainName& rhs) const;
  bool operator>=(const DomainName& rhs) const;
  bool operator==(const DomainName& rhs) const;
  bool operator!=(const DomainName& rhs) const;

  // Returns the maximum space that the domain name could take up in its
  // on-the-wire format. This is an upper bound based on the length of the
  // labels that make up the domain name. It's possible that with domain name
  // compression the actual space taken in on-the-wire format is smaller.
  size_t MaxWireSize() const;
  bool empty() const { return labels_.empty(); }
  bool IsRoot() const { return labels_.empty(); }
  const std::vector<std::string>& labels() const { return labels_; }

  template <typename H>
  friend H AbslHashValue(H h, const DomainName& domain_name) {
    std::vector<std::string> labels_clone = domain_name.labels_;
    for (auto& label : labels_clone) {
      absl::AsciiStrToLower(&label);
    }
    return H::combine(std::move(h), std::move(labels_clone));
  }

 private:
  DomainName(std::vector<std::string> labels, size_t max_wire_size);

  // max_wire_size_ starts at 1 for the terminating character length.
  size_t max_wire_size_ = 1;
  std::vector<std::string> labels_;

  friend std::ostream& operator<<(std::ostream& os,
                                  const DomainName& domain_name);
};

// Parsed representation of the extra data in a record. Does not include
// standard DNS record data such as TTL, Name, Type, and Class. We use it to
// distinguish a raw record type that we do not know the identity of.
class RawRecordRdata {
 public:
  static ErrorOr<RawRecordRdata> TryCreate(std::vector<uint8_t> rdata);

  RawRecordRdata();
  explicit RawRecordRdata(std::vector<uint8_t> rdata);
  RawRecordRdata(const uint8_t* begin, size_t size);
  RawRecordRdata(const RawRecordRdata& other);
  RawRecordRdata(RawRecordRdata&& other) noexcept;

  RawRecordRdata& operator=(const RawRecordRdata& rhs);
  RawRecordRdata& operator=(RawRecordRdata&& rhs);
  bool operator==(const RawRecordRdata& rhs) const;
  bool operator!=(const RawRecordRdata& rhs) const;

  size_t MaxWireSize() const;
  uint16_t size() const { return rdata_.size(); }
  const uint8_t* data() const { return rdata_.data(); }

  template <typename H>
  friend H AbslHashValue(H h, const RawRecordRdata& rdata) {
    return H::combine(std::move(h), rdata.rdata_);
  }

 private:
  std::vector<uint8_t> rdata_;
};

// SRV record format (http://www.ietf.org/rfc/rfc2782.txt):
//   2 bytes network-order unsigned priority
//   2 bytes network-order unsigned weight
//   2 bytes network-order unsigned port
// target: domain name (on-the-wire representation)
class SrvRecordRdata {
 public:
  SrvRecordRdata();
  SrvRecordRdata(uint16_t priority,
                 uint16_t weight,
                 uint16_t port,
                 DomainName target);
  SrvRecordRdata(const SrvRecordRdata& other);
  SrvRecordRdata(SrvRecordRdata&& other) noexcept;

  SrvRecordRdata& operator=(const SrvRecordRdata& rhs);
  SrvRecordRdata& operator=(SrvRecordRdata&& rhs);
  bool operator==(const SrvRecordRdata& rhs) const;
  bool operator!=(const SrvRecordRdata& rhs) const;

  size_t MaxWireSize() const;
  uint16_t priority() const { return priority_; }
  uint16_t weight() const { return weight_; }
  uint16_t port() const { return port_; }
  const DomainName& target() const { return target_; }

  template <typename H>
  friend H AbslHashValue(H h, const SrvRecordRdata& rdata) {
    return H::combine(std::move(h), rdata.priority_, rdata.weight_, rdata.port_,
                      rdata.target_);
  }

 private:
  uint16_t priority_ = 0;
  uint16_t weight_ = 0;
  uint16_t port_ = 0;
  DomainName target_;
};

// A Record format (http://www.ietf.org/rfc/rfc1035.txt):
// 4 bytes for IP address.
class ARecordRdata {
 public:
  ARecordRdata();
  explicit ARecordRdata(IPAddress ipv4_address,
                        NetworkInterfaceIndex interface_index = 0);
  ARecordRdata(const ARecordRdata& other);
  ARecordRdata(ARecordRdata&& other) noexcept;

  ARecordRdata& operator=(const ARecordRdata& rhs);
  ARecordRdata& operator=(ARecordRdata&& rhs);
  bool operator==(const ARecordRdata& rhs) const;
  bool operator!=(const ARecordRdata& rhs) const;

  size_t MaxWireSize() const;
  const IPAddress& ipv4_address() const { return ipv4_address_; }
  NetworkInterfaceIndex interface_index() const { return interface_index_; }

  template <typename H>
  friend H AbslHashValue(H h, const ARecordRdata& rdata) {
    const auto& bytes = rdata.ipv4_address_.bytes();
    return H::combine_contiguous(std::move(h), bytes, 4);
  }

 private:
  IPAddress ipv4_address_{0, 0, 0, 0};
  NetworkInterfaceIndex interface_index_;
};

// AAAA Record format (http://www.ietf.org/rfc/rfc1035.txt):
// 16 bytes for IP address.
class AAAARecordRdata {
 public:
  AAAARecordRdata();
  explicit AAAARecordRdata(IPAddress ipv6_address,
                           NetworkInterfaceIndex interface_index = 0);
  AAAARecordRdata(const AAAARecordRdata& other);
  AAAARecordRdata(AAAARecordRdata&& other) noexcept;

  AAAARecordRdata& operator=(const AAAARecordRdata& rhs);
  AAAARecordRdata& operator=(AAAARecordRdata&& rhs);
  bool operator==(const AAAARecordRdata& rhs) const;
  bool operator!=(const AAAARecordRdata& rhs) const;

  size_t MaxWireSize() const;
  const IPAddress& ipv6_address() const { return ipv6_address_; }
  NetworkInterfaceIndex interface_index() const { return interface_index_; }

  template <typename H>
  friend H AbslHashValue(H h, const AAAARecordRdata& rdata) {
    const auto& bytes = rdata.ipv6_address_.bytes();
    return H::combine_contiguous(std::move(h), bytes, 16);
  }

 private:
  IPAddress ipv6_address_{0x0000, 0x0000, 0x0000, 0x0000,
                          0x0000, 0x0000, 0x0000, 0x0000};
  NetworkInterfaceIndex interface_index_;
};

// PTR record format (http://www.ietf.org/rfc/rfc1035.txt):
// domain: On the wire representation of domain name.
class PtrRecordRdata {
 public:
  PtrRecordRdata();
  explicit PtrRecordRdata(DomainName ptr_domain);
  PtrRecordRdata(const PtrRecordRdata& other);
  PtrRecordRdata(PtrRecordRdata&& other) noexcept;

  PtrRecordRdata& operator=(const PtrRecordRdata& rhs);
  PtrRecordRdata& operator=(PtrRecordRdata&& rhs);
  bool operator==(const PtrRecordRdata& rhs) const;
  bool operator!=(const PtrRecordRdata& rhs) const;

  size_t MaxWireSize() const;
  const DomainName& ptr_domain() const { return ptr_domain_; }

  template <typename H>
  friend H AbslHashValue(H h, const PtrRecordRdata& rdata) {
    return H::combine(std::move(h), rdata.ptr_domain_);
  }

 private:
  DomainName ptr_domain_;
};

// TXT record format (http://www.ietf.org/rfc/rfc1035.txt).
// texts: One or more <entries>.
// An <entry> is a length octet followed by as many data octets.
//
// DNS-SD interprets <entries> as a list of boolean keys and key=value
// attributes.  See https://tools.ietf.org/html/rfc6763#section-6 for details.
class TxtRecordRdata {
 public:
  using Entry = std::vector<uint8_t>;

  static ErrorOr<TxtRecordRdata> TryCreate(std::vector<Entry> texts);

  TxtRecordRdata();
  explicit TxtRecordRdata(std::vector<Entry> texts);
  TxtRecordRdata(const TxtRecordRdata& other);
  TxtRecordRdata(TxtRecordRdata&& other) noexcept;

  TxtRecordRdata& operator=(const TxtRecordRdata& rhs);
  TxtRecordRdata& operator=(TxtRecordRdata&& rhs);
  bool operator==(const TxtRecordRdata& rhs) const;
  bool operator!=(const TxtRecordRdata& rhs) const;

  size_t MaxWireSize() const;
  // NOTE: TXT entries are not guaranteed to be character data.
  const std::vector<std::string>& texts() const { return texts_; }

  template <typename H>
  friend H AbslHashValue(H h, const TxtRecordRdata& rdata) {
    return H::combine(std::move(h), rdata.texts_);
  }

 private:
  TxtRecordRdata(std::vector<std::string> texts, size_t max_wire_size);

  // max_wire_size_ is at least 3, uint16_t record length and at the
  // minimum a NULL byte character string is present.
  size_t max_wire_size_ = 3;
  // NOTE: For compatibility with DNS-SD usage, std::string is used for internal
  // storage.
  std::vector<std::string> texts_;
};

// NSEC record format (https://tools.ietf.org/html/rfc4034#section-4).
// In mDNS, this record type is used for representing negative responses to
// queries.
//
// next_domain_name: The next domain to process. In mDNS, this value is expected
// to match the record-level domain name in a negative response.
//
// An example of how the |types_| vector is serialized is as follows:
// When encoding the following DNS types:
// - A (value 1)
// - MX (value 15)
// - RRSIG (value 46)
// - NSEC (value 47)
// - TYPE1234 (value 1234)
// The result would be:
//          0x00 0x06 0x40 0x01 0x00 0x00 0x00 0x03
//          0x04 0x1b 0x00 0x00 0x00 0x00 0x00 0x00
//          0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
//          0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
//          0x00 0x00 0x00 0x00 0x20
class NsecRecordRdata {
 public:
  NsecRecordRdata();

  // Constructor that takes an arbitrary number of DnsType parameters.
  // NOTE: If `types...` provide a valid set of parameters for an
  // std::vector<DnsType> ctor call, this will compile. Do not use this ctor
  // except to provide multiple DnsType parameters.
  template <typename... Types>
  NsecRecordRdata(DomainName next_domain_name, Types... types)
      : NsecRecordRdata(std::move(next_domain_name),
                        std::vector<DnsType>{types...}) {}
  NsecRecordRdata(DomainName next_domain_name, std::vector<DnsType> types);
  NsecRecordRdata(const NsecRecordRdata& other);
  NsecRecordRdata(NsecRecordRdata&& other) noexcept;

  NsecRecordRdata& operator=(const NsecRecordRdata& rhs);
  NsecRecordRdata& operator=(NsecRecordRdata&& rhs);
  bool operator==(const NsecRecordRdata& rhs) const;
  bool operator!=(const NsecRecordRdata& rhs) const;

  size_t MaxWireSize() const;

  const DomainName& next_domain_name() const { return next_domain_name_; }
  const std::vector<DnsType>& types() const { return types_; }
  const std::vector<uint8_t>& encoded_types() const { return encoded_types_; }

  template <typename H>
  friend H AbslHashValue(H h, const NsecRecordRdata& rdata) {
    return H::combine(std::move(h), rdata.types_, rdata.next_domain_name_);
  }

 private:
  std::vector<uint8_t> encoded_types_;
  std::vector<DnsType> types_;
  DomainName next_domain_name_;
};

// The OPT pseudo-record / meta-record as defined by RFC6891.
class OptRecordRdata {
 public:
  // A single option as defined in RFC6891 section 6.1.2.
  struct Option {
    size_t MaxWireSize() const;

    bool operator>(const Option& rhs) const;
    bool operator<(const Option& rhs) const;
    bool operator>=(const Option& rhs) const;
    bool operator<=(const Option& rhs) const;
    bool operator==(const Option& rhs) const;
    bool operator!=(const Option& rhs) const;

    template <typename H>
    friend H AbslHashValue(H h, const Option& option) {
      return H::combine(std::move(h), option.code, option.length, option.data);
    }

    // Code assigned by the Expert Review process as defined by the DNSEXT
    // working group and the IESG, as specified in RFC6891 section 9.1. For
    // specific assignments, see:
    // https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml
    uint16_t code;

    // Size (in octets) of |data|.
    uint16_t length;

    // Bit Field with meaning varying based on |code|.
    std::vector<uint8_t> data;
  };

  OptRecordRdata();

  // Constructor that takes zero or more Option parameters.
  template <typename... Types>
  explicit OptRecordRdata(Types... types)
      : OptRecordRdata(std::vector<Option>{std::move(types)...}) {}
  explicit OptRecordRdata(std::vector<Option> options);
  OptRecordRdata(const OptRecordRdata& other);
  OptRecordRdata(OptRecordRdata&& other) noexcept;

  OptRecordRdata& operator=(const OptRecordRdata& rhs);
  OptRecordRdata& operator=(OptRecordRdata&& rhs);

  // NOTE: Only the options field is technically considered part of the rdata,
  // so only this field is considered for equality comparison. The other fields
  // are included here solely because their meaning differs for OPT pseudo-
  // records and normal record types.
  bool operator==(const OptRecordRdata& rhs) const;
  bool operator!=(const OptRecordRdata& rhs) const;

  size_t MaxWireSize() const { return max_wire_size_; }

  // Set of options stored in this OPT record.
  const std::vector<Option>& options() { return options_; }

  template <typename H>
  friend H AbslHashValue(H h, const OptRecordRdata& rdata) {
    return H::combine(std::move(h), rdata.options_);
  }

 private:
  // NOTE: The elements of |options_| are stored is sorted order to simplify the
  // comparison operators of OptRecordRdata.
  std::vector<Option> options_;

  size_t max_wire_size_ = 0;
};

using Rdata = absl::variant<RawRecordRdata,
                            SrvRecordRdata,
                            ARecordRdata,
                            AAAARecordRdata,
                            PtrRecordRdata,
                            TxtRecordRdata,
                            NsecRecordRdata,
                            OptRecordRdata>;

// Resource record top level format (http://www.ietf.org/rfc/rfc1035.txt):
// name: the name of the node to which this resource record pertains.
// type: 2 bytes network-order RR TYPE code.
// class: 2 bytes network-order RR CLASS code.
// ttl: 4 bytes network-order cache time interval.
// rdata:  RDATA describing the resource.  The format of this information varies
// according to the TYPE and CLASS of the resource record.
class MdnsRecord {
 public:
  using ConstRef = std::reference_wrapper<const MdnsRecord>;

  static ErrorOr<MdnsRecord> TryCreate(DomainName name,
                                       DnsType dns_type,
                                       DnsClass dns_class,
                                       RecordType record_type,
                                       std::chrono::seconds ttl,
                                       Rdata rdata);

  MdnsRecord();
  MdnsRecord(DomainName name,
             DnsType dns_type,
             DnsClass dns_class,
             RecordType record_type,
             std::chrono::seconds ttl,
             Rdata rdata);
  MdnsRecord(const MdnsRecord& other);
  MdnsRecord(MdnsRecord&& other) noexcept;

  MdnsRecord& operator=(const MdnsRecord& rhs);
  MdnsRecord& operator=(MdnsRecord&& rhs);
  bool operator==(const MdnsRecord& other) const;
  bool operator!=(const MdnsRecord& other) const;
  bool operator<(const MdnsRecord& other) const;
  bool operator>(const MdnsRecord& other) const;
  bool operator<=(const MdnsRecord& other) const;
  bool operator>=(const MdnsRecord& other) const;

  // While not being "equivalent", a record could be said to be an update of
  // a different record if it is the same, excepting TTL.
  bool IsReannouncementOf(const MdnsRecord& other) const;
  size_t MaxWireSize() const;
  const DomainName& name() const { return name_; }
  DnsType dns_type() const { return dns_type_; }
  DnsClass dns_class() const { return dns_class_; }
  RecordType record_type() const { return record_type_; }
  std::chrono::seconds ttl() const { return ttl_; }
  const Rdata& rdata() const { return rdata_; }

  template <typename H>
  friend H AbslHashValue(H h, const MdnsRecord& record) {
    return H::combine(std::move(h), record.name_, record.dns_type_,
                      record.dns_class_, record.record_type_,
                      record.ttl_.count(), record.rdata_);
  }

 private:
  static bool IsValidConfig(const DomainName& name,
                            DnsType dns_type,
                            std::chrono::seconds ttl,
                            const Rdata& rdata);

  DomainName name_;
  DnsType dns_type_ = static_cast<DnsType>(0);
  DnsClass dns_class_ = static_cast<DnsClass>(0);
  RecordType record_type_ = RecordType::kShared;
  std::chrono::seconds ttl_{kDefaultRecordTTLSeconds};
  // Default-constructed Rdata contains default-constructed RawRecordRdata
  // as it is the first alternative type and it is default-constructible.
  Rdata rdata_;

#ifdef _DEBUG
  friend std::ostream& operator<<(std::ostream&, const MdnsRecord& mdns_record);
#endif
};

// Creates an A or AAAA record as appropriate for the provided parameters.
MdnsRecord CreateAddressRecord(DomainName name, const IPAddress& address);

// Question top level format (http://www.ietf.org/rfc/rfc1035.txt):
// name: a domain name which identifies the target resource set.
// type: 2 bytes network-order RR TYPE code.
// class: 2 bytes network-order RR CLASS code.
class MdnsQuestion {
 public:
  static ErrorOr<MdnsQuestion> TryCreate(DomainName name,
                                         DnsType dns_type,
                                         DnsClass dns_class,
                                         ResponseType response_type);

  MdnsQuestion() = default;
  MdnsQuestion(DomainName name,
               DnsType dns_type,
               DnsClass dns_class,
               ResponseType response_type);

  bool operator==(const MdnsQuestion& other) const;
  bool operator!=(const MdnsQuestion& other) const;

  size_t MaxWireSize() const;
  const DomainName& name() const { return name_; }
  DnsType dns_type() const { return dns_type_; }
  DnsClass dns_class() const { return dns_class_; }
  ResponseType response_type() const { return response_type_; }

  template <typename H>
  friend H AbslHashValue(H h, const MdnsQuestion& record) {
    return H::combine(std::move(h), record.name_, record.dns_type_,
                      record.dns_class_, record.response_type_);
  }

 private:
  void CopyFrom(const MdnsQuestion& other);

  DomainName name_;
  DnsType dns_type_ = static_cast<DnsType>(0);
  DnsClass dns_class_ = static_cast<DnsClass>(0);
  ResponseType response_type_ = ResponseType::kMulticast;
};

// Message top level format (http://www.ietf.org/rfc/rfc1035.txt):
// id: 2 bytes network-order identifier assigned by the program that generates
// any kind of query. This identifier is copied to the corresponding reply and
// can be used by the requester to match up replies to outstanding queries.
// flags: 2 bytes network-order flags bitfield.
// questions: questions in the message.
// answers: resource records that answer the questions.
// authority_records: resource records that point toward authoritative name.
// servers additional_records: additional resource records that relate to the
// query.
class MdnsMessage {
 public:
  static ErrorOr<MdnsMessage> TryCreate(
      uint16_t id,
      MessageType type,
      std::vector<MdnsQuestion> questions,
      std::vector<MdnsRecord> answers,
      std::vector<MdnsRecord> authority_records,
      std::vector<MdnsRecord> additional_records);

  MdnsMessage() = default;
  // Constructs a message with ID, flags and empty question, answer, authority
  // and additional record collections.
  MdnsMessage(uint16_t id, MessageType type);
  MdnsMessage(uint16_t id,
              MessageType type,
              std::vector<MdnsQuestion> questions,
              std::vector<MdnsRecord> answers,
              std::vector<MdnsRecord> authority_records,
              std::vector<MdnsRecord> additional_records);

  bool operator==(const MdnsMessage& other) const;
  bool operator!=(const MdnsMessage& other) const;

  void AddQuestion(MdnsQuestion question);
  void AddAnswer(MdnsRecord record);
  void AddAuthorityRecord(MdnsRecord record);
  void AddAdditionalRecord(MdnsRecord record);

  // Returns false if adding a new record would push the size of this message
  // beyond kMaxMulticastMessageSize, and true otherwise.
  bool CanAddRecord(const MdnsRecord& record);

  // Sets the truncated bit (TC), as specified in RFC 1035 Section 4.1.1.
  void set_truncated() { is_truncated_ = true; }

  // Returns true if the provided message is an mDNS probe query as described in
  // RFC 6762 section 8.1. Specifically, it examines whether any question in
  // the 'questions' section is a query for which answers are present in the
  // 'authority records' section of the same message.
  bool IsProbeQuery() const;

  size_t MaxWireSize() const;
  uint16_t id() const { return id_; }
  MessageType type() const { return type_; }
  bool is_truncated() const { return is_truncated_; }
  const std::vector<MdnsQuestion>& questions() const { return questions_; }
  const std::vector<MdnsRecord>& answers() const { return answers_; }
  const std::vector<MdnsRecord>& authority_records() const {
    return authority_records_;
  }
  const std::vector<MdnsRecord>& additional_records() const {
    return additional_records_;
  }

  template <typename H>
  friend H AbslHashValue(H h, const MdnsMessage& message) {
    return H::combine(std::move(h), message.id_, message.type_,
                      message.questions_, message.answers_,
                      message.authority_records_, message.additional_records_);
  }

 private:
  // The mDNS header is 12 bytes long
  size_t max_wire_size_ = sizeof(Header);
  uint16_t id_ = 0;
  bool is_truncated_ = false;
  MessageType type_ = MessageType::Query;
  std::vector<MdnsQuestion> questions_;
  std::vector<MdnsRecord> answers_;
  std::vector<MdnsRecord> authority_records_;
  std::vector<MdnsRecord> additional_records_;
};

uint16_t CreateMessageId();

// Determines whether a record of the given type can be published.
bool CanBePublished(DnsType type);

// Determines whether a record of the given type can be queried for.
bool CanBeQueried(DnsType type);

// Determines whether a record of the given type received over the network
// should be processed.
bool CanBeProcessed(DnsType type);

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_MDNS_MDNS_RECORDS_H_
