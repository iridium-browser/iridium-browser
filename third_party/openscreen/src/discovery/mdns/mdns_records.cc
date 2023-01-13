// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_records.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <ostream>
#include <sstream>
#include <vector>

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_join.h"
#include "discovery/mdns/mdns_writer.h"

namespace openscreen {
namespace discovery {

namespace {

constexpr size_t kMaxRawRecordSize = std::numeric_limits<uint16_t>::max();

constexpr size_t kMaxMessageFieldEntryCount =
    std::numeric_limits<uint16_t>::max();

inline int CompareIgnoreCase(const std::string& x, const std::string& y) {
  size_t i = 0;
  for (; i < x.size(); i++) {
    if (i == y.size()) {
      return 1;
    }
    const char& x_char = std::tolower(x[i]);
    const char& y_char = std::tolower(y[i]);
    if (x_char < y_char) {
      return -1;
    } else if (y_char < x_char) {
      return 1;
    }
  }
  return i == y.size() ? 0 : -1;
}

template <typename RDataType>
bool IsGreaterThan(const Rdata& lhs, const Rdata& rhs) {
  const RDataType& lhs_cast = absl::get<RDataType>(lhs);
  const RDataType& rhs_cast = absl::get<RDataType>(rhs);

  // The Extra 2 in length is from the record size that Write() prepends to the
  // result.
  const size_t lhs_size = lhs_cast.MaxWireSize() + 2;
  const size_t rhs_size = rhs_cast.MaxWireSize() + 2;

  uint8_t lhs_bytes[lhs_size];  // NOLINT(runtime/arrays)
  uint8_t rhs_bytes[rhs_size];  // NOLINT(runtime/arrays)
  MdnsWriter lhs_writer(lhs_bytes, lhs_size);
  MdnsWriter rhs_writer(rhs_bytes, rhs_size);

  const bool lhs_write = lhs_writer.Write(lhs_cast);
  const bool rhs_write = rhs_writer.Write(rhs_cast);
  OSP_DCHECK(lhs_write);
  OSP_DCHECK(rhs_write);

  // Skip the size bits.
  const size_t min_size = std::min(lhs_writer.offset(), rhs_writer.offset());
  for (size_t i = 2; i < min_size; i++) {
    if (lhs_bytes[i] != rhs_bytes[i]) {
      return lhs_bytes[i] > rhs_bytes[i];
    }
  }

  return lhs_size > rhs_size;
}

bool IsGreaterThan(DnsType type, const Rdata& lhs, const Rdata& rhs) {
  switch (type) {
    case DnsType::kA:
      return IsGreaterThan<ARecordRdata>(lhs, rhs);
    case DnsType::kPTR:
      return IsGreaterThan<PtrRecordRdata>(lhs, rhs);
    case DnsType::kTXT:
      return IsGreaterThan<TxtRecordRdata>(lhs, rhs);
    case DnsType::kAAAA:
      return IsGreaterThan<AAAARecordRdata>(lhs, rhs);
    case DnsType::kSRV:
      return IsGreaterThan<SrvRecordRdata>(lhs, rhs);
    case DnsType::kNSEC:
      return IsGreaterThan<NsecRecordRdata>(lhs, rhs);
    default:
      return IsGreaterThan<RawRecordRdata>(lhs, rhs);
  }
}

}  // namespace

bool IsValidDomainLabel(absl::string_view label) {
  const size_t label_size = label.size();
  return label_size > 0 && label_size <= kMaxLabelLength;
}

DomainName::DomainName() = default;

DomainName::DomainName(std::vector<std::string> labels)
    : DomainName(labels.begin(), labels.end()) {}

DomainName::DomainName(const std::vector<absl::string_view>& labels)
    : DomainName(labels.begin(), labels.end()) {}

DomainName::DomainName(std::initializer_list<absl::string_view> labels)
    : DomainName(labels.begin(), labels.end()) {}

DomainName::DomainName(std::vector<std::string> labels, size_t max_wire_size)
    : max_wire_size_(max_wire_size), labels_(std::move(labels)) {}

DomainName::DomainName(const DomainName& other) = default;

DomainName::DomainName(DomainName&& other) noexcept = default;

DomainName& DomainName::operator=(const DomainName& rhs) = default;

DomainName& DomainName::operator=(DomainName&& rhs) = default;

bool DomainName::operator<(const DomainName& rhs) const {
  size_t i = 0;
  for (; i < labels_.size(); i++) {
    if (i == rhs.labels_.size()) {
      return false;
    } else {
      int result = CompareIgnoreCase(labels_[i], rhs.labels_[i]);
      if (result < 0) {
        return true;
      } else if (result > 0) {
        return false;
      }
    }
  }
  return i < rhs.labels_.size();
}

bool DomainName::operator<=(const DomainName& rhs) const {
  return (*this < rhs) || (*this == rhs);
}

bool DomainName::operator>(const DomainName& rhs) const {
  return !(*this < rhs) && !(*this == rhs);
}

bool DomainName::operator>=(const DomainName& rhs) const {
  return !(*this < rhs);
}

bool DomainName::operator==(const DomainName& rhs) const {
  if (labels_.size() != rhs.labels_.size()) {
    return false;
  }
  for (size_t i = 0; i < labels_.size(); i++) {
    if (CompareIgnoreCase(labels_[i], rhs.labels_[i]) != 0) {
      return false;
    }
  }
  return true;
}

bool DomainName::operator!=(const DomainName& rhs) const {
  return !(*this == rhs);
}

size_t DomainName::MaxWireSize() const {
  return max_wire_size_;
}

std::ostream& operator<<(std::ostream& os, const DomainName& domain_name) {
  return os << absl::StrJoin(domain_name.labels_, ".");
}

// static
ErrorOr<RawRecordRdata> RawRecordRdata::TryCreate(std::vector<uint8_t> rdata) {
  if (rdata.size() > kMaxRawRecordSize) {
    return Error::Code::kIndexOutOfBounds;
  } else {
    return RawRecordRdata(std::move(rdata));
  }
}

RawRecordRdata::RawRecordRdata() = default;

RawRecordRdata::RawRecordRdata(std::vector<uint8_t> rdata)
    : rdata_(std::move(rdata)) {
  // Ensure RDATA length does not exceed the maximum allowed.
  OSP_DCHECK(rdata_.size() <= kMaxRawRecordSize);
}

RawRecordRdata::RawRecordRdata(const uint8_t* begin, size_t size)
    : RawRecordRdata(std::vector<uint8_t>(begin, begin + size)) {}

RawRecordRdata::RawRecordRdata(const RawRecordRdata& other) = default;

RawRecordRdata::RawRecordRdata(RawRecordRdata&& other) noexcept = default;

RawRecordRdata& RawRecordRdata::operator=(const RawRecordRdata& rhs) = default;

RawRecordRdata& RawRecordRdata::operator=(RawRecordRdata&& rhs) = default;

bool RawRecordRdata::operator==(const RawRecordRdata& rhs) const {
  return rdata_ == rhs.rdata_;
}

bool RawRecordRdata::operator!=(const RawRecordRdata& rhs) const {
  return !(*this == rhs);
}

size_t RawRecordRdata::MaxWireSize() const {
  // max_wire_size includes uint16_t record length field.
  return sizeof(uint16_t) + rdata_.size();
}

SrvRecordRdata::SrvRecordRdata() = default;

SrvRecordRdata::SrvRecordRdata(uint16_t priority,
                               uint16_t weight,
                               uint16_t port,
                               DomainName target)
    : priority_(priority),
      weight_(weight),
      port_(port),
      target_(std::move(target)) {}

SrvRecordRdata::SrvRecordRdata(const SrvRecordRdata& other) = default;

SrvRecordRdata::SrvRecordRdata(SrvRecordRdata&& other) noexcept = default;

SrvRecordRdata& SrvRecordRdata::operator=(const SrvRecordRdata& rhs) = default;

SrvRecordRdata& SrvRecordRdata::operator=(SrvRecordRdata&& rhs) = default;

bool SrvRecordRdata::operator==(const SrvRecordRdata& rhs) const {
  return priority_ == rhs.priority_ && weight_ == rhs.weight_ &&
         port_ == rhs.port_ && target_ == rhs.target_;
}

bool SrvRecordRdata::operator!=(const SrvRecordRdata& rhs) const {
  return !(*this == rhs);
}

size_t SrvRecordRdata::MaxWireSize() const {
  // max_wire_size includes uint16_t record length field.
  return sizeof(uint16_t) + sizeof(priority_) + sizeof(weight_) +
         sizeof(port_) + target_.MaxWireSize();
}

ARecordRdata::ARecordRdata() = default;

ARecordRdata::ARecordRdata(IPAddress ipv4_address,
                           NetworkInterfaceIndex interface_index)
    : ipv4_address_(std::move(ipv4_address)),
      interface_index_(interface_index) {
  OSP_CHECK(ipv4_address_.IsV4());
}

ARecordRdata::ARecordRdata(const ARecordRdata& other) = default;

ARecordRdata::ARecordRdata(ARecordRdata&& other) noexcept = default;

ARecordRdata& ARecordRdata::operator=(const ARecordRdata& rhs) = default;

ARecordRdata& ARecordRdata::operator=(ARecordRdata&& rhs) = default;

bool ARecordRdata::operator==(const ARecordRdata& rhs) const {
  return ipv4_address_ == rhs.ipv4_address_ &&
         interface_index_ == rhs.interface_index_;
}

bool ARecordRdata::operator!=(const ARecordRdata& rhs) const {
  return !(*this == rhs);
}

size_t ARecordRdata::MaxWireSize() const {
  // max_wire_size includes uint16_t record length field.
  return sizeof(uint16_t) + IPAddress::kV4Size;
}

AAAARecordRdata::AAAARecordRdata() = default;

AAAARecordRdata::AAAARecordRdata(IPAddress ipv6_address,
                                 NetworkInterfaceIndex interface_index)
    : ipv6_address_(std::move(ipv6_address)),
      interface_index_(interface_index) {
  OSP_CHECK(ipv6_address_.IsV6());
}

AAAARecordRdata::AAAARecordRdata(const AAAARecordRdata& other) = default;

AAAARecordRdata::AAAARecordRdata(AAAARecordRdata&& other) noexcept = default;

AAAARecordRdata& AAAARecordRdata::operator=(const AAAARecordRdata& rhs) =
    default;

AAAARecordRdata& AAAARecordRdata::operator=(AAAARecordRdata&& rhs) = default;

bool AAAARecordRdata::operator==(const AAAARecordRdata& rhs) const {
  return ipv6_address_ == rhs.ipv6_address_ &&
         interface_index_ == rhs.interface_index_;
}

bool AAAARecordRdata::operator!=(const AAAARecordRdata& rhs) const {
  return !(*this == rhs);
}

size_t AAAARecordRdata::MaxWireSize() const {
  // max_wire_size includes uint16_t record length field.
  return sizeof(uint16_t) + IPAddress::kV6Size;
}

PtrRecordRdata::PtrRecordRdata() = default;

PtrRecordRdata::PtrRecordRdata(DomainName ptr_domain)
    : ptr_domain_(ptr_domain) {}

PtrRecordRdata::PtrRecordRdata(const PtrRecordRdata& other) = default;

PtrRecordRdata::PtrRecordRdata(PtrRecordRdata&& other) noexcept = default;

PtrRecordRdata& PtrRecordRdata::operator=(const PtrRecordRdata& rhs) = default;

PtrRecordRdata& PtrRecordRdata::operator=(PtrRecordRdata&& rhs) = default;

bool PtrRecordRdata::operator==(const PtrRecordRdata& rhs) const {
  return ptr_domain_ == rhs.ptr_domain_;
}

bool PtrRecordRdata::operator!=(const PtrRecordRdata& rhs) const {
  return !(*this == rhs);
}

size_t PtrRecordRdata::MaxWireSize() const {
  // max_wire_size includes uint16_t record length field.
  return sizeof(uint16_t) + ptr_domain_.MaxWireSize();
}

// static
ErrorOr<TxtRecordRdata> TxtRecordRdata::TryCreate(std::vector<Entry> texts) {
  std::vector<std::string> str_texts;
  size_t max_wire_size = 3;
  if (texts.size() > 0) {
    str_texts.reserve(texts.size());
    // max_wire_size includes uint16_t record length field.
    max_wire_size = sizeof(uint16_t);
    for (const auto& text : texts) {
      if (text.empty()) {
        return Error::Code::kParameterInvalid;
      }
      str_texts.push_back(
          std::string(reinterpret_cast<const char*>(text.data()), text.size()));
      // Include the length byte in the size calculation.
      max_wire_size += text.size() + 1;
    }
  }
  return TxtRecordRdata(std::move(str_texts), max_wire_size);
}

TxtRecordRdata::TxtRecordRdata() = default;

TxtRecordRdata::TxtRecordRdata(std::vector<Entry> texts) {
  ErrorOr<TxtRecordRdata> rdata = TxtRecordRdata::TryCreate(std::move(texts));
  *this = std::move(rdata.value());
}

TxtRecordRdata::TxtRecordRdata(std::vector<std::string> texts,
                               size_t max_wire_size)
    : max_wire_size_(max_wire_size), texts_(std::move(texts)) {}

TxtRecordRdata::TxtRecordRdata(const TxtRecordRdata& other) = default;

TxtRecordRdata::TxtRecordRdata(TxtRecordRdata&& other) noexcept = default;

TxtRecordRdata& TxtRecordRdata::operator=(const TxtRecordRdata& rhs) = default;

TxtRecordRdata& TxtRecordRdata::operator=(TxtRecordRdata&& rhs) = default;

bool TxtRecordRdata::operator==(const TxtRecordRdata& rhs) const {
  return texts_ == rhs.texts_;
}

bool TxtRecordRdata::operator!=(const TxtRecordRdata& rhs) const {
  return !(*this == rhs);
}

size_t TxtRecordRdata::MaxWireSize() const {
  return max_wire_size_;
}

NsecRecordRdata::NsecRecordRdata() = default;

NsecRecordRdata::NsecRecordRdata(DomainName next_domain_name,
                                 std::vector<DnsType> types)
    : types_(std::move(types)), next_domain_name_(std::move(next_domain_name)) {
  // Sort the types_ array for easier comparison later.
  std::sort(types_.begin(), types_.end());

  // Calculate the bitmaps as described in RFC 4034 Section 4.1.2.
  std::vector<uint8_t> block_contents;
  uint8_t current_block = 0;
  for (auto type : types_) {
    const uint16_t type_int = static_cast<uint16_t>(type);
    const uint8_t block = static_cast<uint8_t>(type_int >> 8);
    const uint8_t block_position = static_cast<uint8_t>(type_int & 0xFF);
    const uint8_t byte_bit_is_at = block_position >> 3;         // First 5 bits.
    const uint8_t byte_mask = 0x80 >> (block_position & 0x07);  // Last 3 bits.

    // If the block has changed, write the previous block's info and all of its
    // contents to the |encoded_types_| vector.
    if (block > current_block) {
      if (!block_contents.empty()) {
        encoded_types_.push_back(current_block);
        encoded_types_.push_back(static_cast<uint8_t>(block_contents.size()));
        encoded_types_.insert(encoded_types_.end(), block_contents.begin(),
                              block_contents.end());
      }
      block_contents = std::vector<uint8_t>();
      current_block = block;
    }

    // Make sure |block_contents| is large enough to hold the bit representing
    // the new type , then set it.
    if (block_contents.size() <= byte_bit_is_at) {
      block_contents.insert(block_contents.end(),
                            byte_bit_is_at - block_contents.size() + 1, 0x00);
    }

    block_contents[byte_bit_is_at] |= byte_mask;
  }

  if (!block_contents.empty()) {
    encoded_types_.push_back(current_block);
    encoded_types_.push_back(static_cast<uint8_t>(block_contents.size()));
    encoded_types_.insert(encoded_types_.end(), block_contents.begin(),
                          block_contents.end());
  }
}

NsecRecordRdata::NsecRecordRdata(const NsecRecordRdata& other) = default;

NsecRecordRdata::NsecRecordRdata(NsecRecordRdata&& other) noexcept = default;

NsecRecordRdata& NsecRecordRdata::operator=(const NsecRecordRdata& rhs) =
    default;

NsecRecordRdata& NsecRecordRdata::operator=(NsecRecordRdata&& rhs) = default;

bool NsecRecordRdata::operator==(const NsecRecordRdata& rhs) const {
  return types_ == rhs.types_ && next_domain_name_ == rhs.next_domain_name_;
}

bool NsecRecordRdata::operator!=(const NsecRecordRdata& rhs) const {
  return !(*this == rhs);
}

size_t NsecRecordRdata::MaxWireSize() const {
  return next_domain_name_.MaxWireSize() + encoded_types_.size();
}

size_t OptRecordRdata::Option::MaxWireSize() const {
  // One uint16_t for each of OPTION-LENGTH and OPTION-CODE as defined in RFC
  // 6891 section 6.1.2.
  constexpr size_t kOptionLengthAndCodeSize = 2 * sizeof(uint16_t);
  return data.size() + kOptionLengthAndCodeSize;
}

bool OptRecordRdata::Option::operator>(
    const OptRecordRdata::Option& rhs) const {
  if (code != rhs.code) {
    return code > rhs.code;
  } else if (length != rhs.length) {
    return length > rhs.length;
  } else if (data.size() != rhs.data.size()) {
    return data.size() > rhs.data.size();
  }

  for (int i = 0; i < static_cast<int>(data.size()); i++) {
    if (data[i] != rhs.data[i]) {
      return data[i] > rhs.data[i];
    }
  }

  return false;
}

bool OptRecordRdata::Option::operator<(
    const OptRecordRdata::Option& rhs) const {
  return rhs > *this;
}

bool OptRecordRdata::Option::operator>=(
    const OptRecordRdata::Option& rhs) const {
  return !(*this < rhs);
}

bool OptRecordRdata::Option::operator<=(
    const OptRecordRdata::Option& rhs) const {
  return !(*this > rhs);
}

bool OptRecordRdata::Option::operator==(
    const OptRecordRdata::Option& rhs) const {
  return *this >= rhs && *this <= rhs;
}

bool OptRecordRdata::Option::operator!=(
    const OptRecordRdata::Option& rhs) const {
  return !(*this == rhs);
}

OptRecordRdata::OptRecordRdata() = default;

OptRecordRdata::OptRecordRdata(std::vector<Option> options)
    : options_(std::move(options)) {
  for (const auto& option : options_) {
    max_wire_size_ += option.MaxWireSize();
  }
  std::sort(options_.begin(), options_.end());
}

OptRecordRdata::OptRecordRdata(const OptRecordRdata& other) = default;

OptRecordRdata::OptRecordRdata(OptRecordRdata&& other) noexcept = default;

OptRecordRdata& OptRecordRdata::operator=(const OptRecordRdata& rhs) = default;

OptRecordRdata& OptRecordRdata::operator=(OptRecordRdata&& rhs) = default;

bool OptRecordRdata::operator==(const OptRecordRdata& rhs) const {
  return options_ == rhs.options_;
}

bool OptRecordRdata::operator!=(const OptRecordRdata& rhs) const {
  return !(*this == rhs);
}

// static
ErrorOr<MdnsRecord> MdnsRecord::TryCreate(DomainName name,
                                          DnsType dns_type,
                                          DnsClass dns_class,
                                          RecordType record_type,
                                          std::chrono::seconds ttl,
                                          Rdata rdata) {
  if (!IsValidConfig(name, dns_type, ttl, rdata)) {
    return Error::Code::kParameterInvalid;
  } else {
    return MdnsRecord(std::move(name), dns_type, dns_class, record_type, ttl,
                      std::move(rdata));
  }
}

MdnsRecord::MdnsRecord() = default;

MdnsRecord::MdnsRecord(DomainName name,
                       DnsType dns_type,
                       DnsClass dns_class,
                       RecordType record_type,
                       std::chrono::seconds ttl,
                       Rdata rdata)
    : name_(std::move(name)),
      dns_type_(dns_type),
      dns_class_(dns_class),
      record_type_(record_type),
      ttl_(ttl),
      rdata_(std::move(rdata)) {
  OSP_DCHECK(IsValidConfig(name_, dns_type, ttl_, rdata_));
}

MdnsRecord::MdnsRecord(const MdnsRecord& other) = default;

MdnsRecord::MdnsRecord(MdnsRecord&& other) noexcept = default;

MdnsRecord& MdnsRecord::operator=(const MdnsRecord& rhs) = default;

MdnsRecord& MdnsRecord::operator=(MdnsRecord&& rhs) = default;

// static
bool MdnsRecord::IsValidConfig(const DomainName& name,
                               DnsType dns_type,
                               std::chrono::seconds ttl,
                               const Rdata& rdata) {
  // NOTE: Although the name_ field was initially expected to be non-empty, this
  // validation is no longer accurate for some record types (such as OPT
  // records). To ensure that future record types correctly parse into
  // RawRecordData types and do not invalidate the received message, this check
  // has been removed.
  return ttl.count() <= std::numeric_limits<uint32_t>::max() &&
         ((dns_type == DnsType::kSRV &&
           absl::holds_alternative<SrvRecordRdata>(rdata)) ||
          (dns_type == DnsType::kA &&
           absl::holds_alternative<ARecordRdata>(rdata)) ||
          (dns_type == DnsType::kAAAA &&
           absl::holds_alternative<AAAARecordRdata>(rdata)) ||
          (dns_type == DnsType::kPTR &&
           absl::holds_alternative<PtrRecordRdata>(rdata)) ||
          (dns_type == DnsType::kTXT &&
           absl::holds_alternative<TxtRecordRdata>(rdata)) ||
          (dns_type == DnsType::kNSEC &&
           absl::holds_alternative<NsecRecordRdata>(rdata)) ||
          (dns_type == DnsType::kOPT &&
           absl::holds_alternative<OptRecordRdata>(rdata)) ||
          absl::holds_alternative<RawRecordRdata>(rdata));
}

bool MdnsRecord::operator==(const MdnsRecord& rhs) const {
  return IsReannouncementOf(rhs) && ttl_ == rhs.ttl_;
}

bool MdnsRecord::operator!=(const MdnsRecord& rhs) const {
  return !(*this == rhs);
}

bool MdnsRecord::operator>(const MdnsRecord& rhs) const {
  // Returns the record which is lexicographically later. The determination of
  // "lexicographically later" is performed by first comparing the record class,
  // then the record type, then raw comparison of the binary content of the
  // rdata without regard for meaning or structure.
  // NOTE: Per RFC, the TTL is not included in this comparison.
  if (name() != rhs.name()) {
    return name() > rhs.name();
  }

  if (record_type() != rhs.record_type()) {
    return record_type() == RecordType::kUnique;
  }

  if (dns_class() != rhs.dns_class()) {
    return dns_class() > rhs.dns_class();
  }

  uint16_t this_type = static_cast<uint16_t>(dns_type()) & kClassMask;
  uint16_t other_type = static_cast<uint16_t>(rhs.dns_type()) & kClassMask;
  if (this_type != other_type) {
    return this_type > other_type;
  }

  return IsGreaterThan(dns_type(), rdata(), rhs.rdata());
}

bool MdnsRecord::operator<(const MdnsRecord& rhs) const {
  return rhs > *this;
}

bool MdnsRecord::operator<=(const MdnsRecord& rhs) const {
  return !(*this > rhs);
}

bool MdnsRecord::operator>=(const MdnsRecord& rhs) const {
  return !(*this < rhs);
}

bool MdnsRecord::IsReannouncementOf(const MdnsRecord& rhs) const {
  return dns_type_ == rhs.dns_type_ && dns_class_ == rhs.dns_class_ &&
         record_type_ == rhs.record_type_ && name_ == rhs.name_ &&
         rdata_ == rhs.rdata_;
}

size_t MdnsRecord::MaxWireSize() const {
  auto wire_size_visitor = [](auto&& arg) { return arg.MaxWireSize(); };
  // NAME size, 2-byte TYPE, 2-byte CLASS, 4-byte TTL, RDATA size
  return name_.MaxWireSize() + absl::visit(wire_size_visitor, rdata_) + 8;
}

#ifdef _DEBUG
std::ostream& operator<<(std::ostream& os, const MdnsRecord& mdns_record) {
  os << "name: " << mdns_record.name_ << "'";
  os << ", type: " << mdns_record.dns_type_;

  if (mdns_record.dns_type_ == DnsType::kPTR) {
    const DomainName& target =
        absl::get<PtrRecordRdata>(mdns_record.rdata_).ptr_domain();
    os << ", target: '" << target << "'";
  } else if (mdns_record.dns_type_ == DnsType::kSRV) {
    const DomainName& target =
        absl::get<SrvRecordRdata>(mdns_record.rdata_).target();
    os << ", target: '" << target << "'";
  } else if (mdns_record.dns_type_ == DnsType::kNSEC) {
    const auto& nsec_rdata = absl::get<NsecRecordRdata>(mdns_record.rdata_);
    std::vector<DnsType> types = nsec_rdata.types();
    os << ", representing [";
    if (!types.empty()) {
      auto it = types.begin();
      os << *it++;
      while (it != types.end()) {
        os << ", " << *it++;
      }
      os << "]";
    }
  }

  return os;
}
#endif

MdnsRecord CreateAddressRecord(DomainName name, const IPAddress& address) {
  Rdata rdata;
  DnsType type;
  std::chrono::seconds ttl;
  if (address.IsV4()) {
    type = DnsType::kA;
    rdata = ARecordRdata(address);
    ttl = kARecordTtl;
  } else {
    type = DnsType::kAAAA;
    rdata = AAAARecordRdata(address);
    ttl = kAAAARecordTtl;
  }

  return MdnsRecord(std::move(name), type, DnsClass::kIN, RecordType::kUnique,
                    ttl, std::move(rdata));
}

// static
ErrorOr<MdnsQuestion> MdnsQuestion::TryCreate(DomainName name,
                                              DnsType dns_type,
                                              DnsClass dns_class,
                                              ResponseType response_type) {
  if (name.empty()) {
    return Error::Code::kParameterInvalid;
  }

  return MdnsQuestion(std::move(name), dns_type, dns_class, response_type);
}

MdnsQuestion::MdnsQuestion(DomainName name,
                           DnsType dns_type,
                           DnsClass dns_class,
                           ResponseType response_type)
    : name_(std::move(name)),
      dns_type_(dns_type),
      dns_class_(dns_class),
      response_type_(response_type) {
  OSP_CHECK(!name_.empty());
}

bool MdnsQuestion::operator==(const MdnsQuestion& rhs) const {
  return dns_type_ == rhs.dns_type_ && dns_class_ == rhs.dns_class_ &&
         response_type_ == rhs.response_type_ && name_ == rhs.name_;
}

bool MdnsQuestion::operator!=(const MdnsQuestion& rhs) const {
  return !(*this == rhs);
}

size_t MdnsQuestion::MaxWireSize() const {
  // NAME size, 2-byte TYPE, 2-byte CLASS
  return name_.MaxWireSize() + 4;
}

// static
ErrorOr<MdnsMessage> MdnsMessage::TryCreate(
    uint16_t id,
    MessageType type,
    std::vector<MdnsQuestion> questions,
    std::vector<MdnsRecord> answers,
    std::vector<MdnsRecord> authority_records,
    std::vector<MdnsRecord> additional_records) {
  if (questions.size() >= kMaxMessageFieldEntryCount ||
      answers.size() >= kMaxMessageFieldEntryCount ||
      authority_records.size() >= kMaxMessageFieldEntryCount ||
      additional_records.size() >= kMaxMessageFieldEntryCount) {
    return Error::Code::kParameterInvalid;
  }

  return MdnsMessage(id, type, std::move(questions), std::move(answers),
                     std::move(authority_records),
                     std::move(additional_records));
}

MdnsMessage::MdnsMessage(uint16_t id, MessageType type)
    : id_(id), type_(type) {}

MdnsMessage::MdnsMessage(uint16_t id,
                         MessageType type,
                         std::vector<MdnsQuestion> questions,
                         std::vector<MdnsRecord> answers,
                         std::vector<MdnsRecord> authority_records,
                         std::vector<MdnsRecord> additional_records)
    : id_(id),
      type_(type),
      questions_(std::move(questions)),
      answers_(std::move(answers)),
      authority_records_(std::move(authority_records)),
      additional_records_(std::move(additional_records)) {
  OSP_DCHECK(questions_.size() < kMaxMessageFieldEntryCount);
  OSP_DCHECK(answers_.size() < kMaxMessageFieldEntryCount);
  OSP_DCHECK(authority_records_.size() < kMaxMessageFieldEntryCount);
  OSP_DCHECK(additional_records_.size() < kMaxMessageFieldEntryCount);

  for (const MdnsQuestion& question : questions_) {
    max_wire_size_ += question.MaxWireSize();
  }
  for (const MdnsRecord& record : answers_) {
    max_wire_size_ += record.MaxWireSize();
  }
  for (const MdnsRecord& record : authority_records_) {
    max_wire_size_ += record.MaxWireSize();
  }
  for (const MdnsRecord& record : additional_records_) {
    max_wire_size_ += record.MaxWireSize();
  }
}

bool MdnsMessage::operator==(const MdnsMessage& rhs) const {
  return id_ == rhs.id_ && type_ == rhs.type_ && questions_ == rhs.questions_ &&
         answers_ == rhs.answers_ &&
         authority_records_ == rhs.authority_records_ &&
         additional_records_ == rhs.additional_records_;
}

bool MdnsMessage::operator!=(const MdnsMessage& rhs) const {
  return !(*this == rhs);
}

bool MdnsMessage::IsProbeQuery() const {
  // A message is a probe query if it contains records in the authority section
  // which answer the question being asked.
  if (questions().empty() || authority_records().empty()) {
    return false;
  }

  for (const MdnsQuestion& question : questions_) {
    for (const MdnsRecord& record : authority_records_) {
      if (question.name() == record.name() &&
          ((question.dns_type() == record.dns_type()) ||
           (question.dns_type() == DnsType::kANY)) &&
          ((question.dns_class() == record.dns_class()) ||
           (question.dns_class() == DnsClass::kANY))) {
        return true;
      }
    }
  }

  return false;
}

size_t MdnsMessage::MaxWireSize() const {
  return max_wire_size_;
}

void MdnsMessage::AddQuestion(MdnsQuestion question) {
  OSP_DCHECK(questions_.size() < kMaxMessageFieldEntryCount);
  max_wire_size_ += question.MaxWireSize();
  questions_.emplace_back(std::move(question));
}

void MdnsMessage::AddAnswer(MdnsRecord record) {
  OSP_DCHECK(answers_.size() < kMaxMessageFieldEntryCount);
  max_wire_size_ += record.MaxWireSize();
  answers_.emplace_back(std::move(record));
}

void MdnsMessage::AddAuthorityRecord(MdnsRecord record) {
  OSP_DCHECK(authority_records_.size() < kMaxMessageFieldEntryCount);
  max_wire_size_ += record.MaxWireSize();
  authority_records_.emplace_back(std::move(record));
}

void MdnsMessage::AddAdditionalRecord(MdnsRecord record) {
  OSP_DCHECK(additional_records_.size() < kMaxMessageFieldEntryCount);
  max_wire_size_ += record.MaxWireSize();
  additional_records_.emplace_back(std::move(record));
}

bool MdnsMessage::CanAddRecord(const MdnsRecord& record) {
  return (max_wire_size_ + record.MaxWireSize()) < kMaxMulticastMessageSize;
}

uint16_t CreateMessageId() {
  static uint16_t id(0);
  return id++;
}

bool CanBePublished(DnsType type) {
  // NOTE: A 'default' switch statement has intentionally been avoided below to
  // enforce that new DnsTypes added must be added below through a compile-time
  // check.
  switch (type) {
    case DnsType::kA:
    case DnsType::kAAAA:
    case DnsType::kPTR:
    case DnsType::kTXT:
    case DnsType::kSRV:
      return true;
    case DnsType::kOPT:
    case DnsType::kNSEC:
    case DnsType::kANY:
      break;
  }

  return false;
}

bool CanBePublished(const MdnsRecord& record) {
  return CanBePublished(record.dns_type());
}

bool CanBeQueried(DnsType type) {
  // NOTE: A 'default' switch statement has intentionally been avoided below to
  // enforce that new DnsTypes added must be added below through a compile-time
  // check.
  switch (type) {
    case DnsType::kA:
    case DnsType::kAAAA:
    case DnsType::kPTR:
    case DnsType::kTXT:
    case DnsType::kSRV:
    case DnsType::kANY:
      return true;
    case DnsType::kOPT:
    case DnsType::kNSEC:
      break;
  }

  return false;
}

bool CanBeProcessed(DnsType type) {
  // NOTE: A 'default' switch statement has intentionally been avoided below to
  // enforce that new DnsTypes added must be added below through a compile-time
  // check.
  switch (type) {
    case DnsType::kA:
    case DnsType::kAAAA:
    case DnsType::kPTR:
    case DnsType::kTXT:
    case DnsType::kSRV:
    case DnsType::kNSEC:
      return true;
    case DnsType::kOPT:
    case DnsType::kANY:
      break;
  }

  return false;
}

}  // namespace discovery
}  // namespace openscreen
