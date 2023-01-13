// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_writer.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "absl/hash/hash.h"
#include "absl/strings/ascii.h"
#include "util/hashing.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace discovery {

namespace {

std::vector<uint64_t> ComputeDomainNameSubhashes(const DomainName& name) {
  const std::vector<std::string>& labels = name.labels();
  uint64_t hash_value = openscreen::kDefaultSeed;
  std::vector<uint64_t> subhashes(labels.size());
  for (size_t i = labels.size(); i-- > 0;) {
    hash_value =
        ComputeAggregateHash(hash_value, absl::AsciiStrToLower(labels[i]));
    subhashes[i] = hash_value;
  }
  return subhashes;
}

// This helper method writes the number of bytes between |begin| and |end| minus
// the size of the uint16_t into the uint16_t length field at |begin|. The
// method returns true if the number of bytes between |begin| and |end| fits in
// uint16_t type, returns false otherwise.
bool UpdateRecordLength(const uint8_t* end, uint8_t* begin) {
  OSP_DCHECK_LE(begin + sizeof(uint16_t), end);
  ptrdiff_t record_length = end - begin - sizeof(uint16_t);
  if (record_length <= std::numeric_limits<uint16_t>::max()) {
    WriteBigEndian<uint16_t>(record_length, begin);
    return true;
  }
  return false;
}

}  // namespace

bool MdnsWriter::Write(absl::string_view value) {
  if (value.length() > std::numeric_limits<uint8_t>::max()) {
    return false;
  }
  Cursor cursor(this);
  if (Write(static_cast<uint8_t>(value.length())) &&
      Write(value.data(), value.length())) {
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsWriter::Write(const std::string& value) {
  return Write(absl::string_view(value));
}

// RFC 1035: https://www.ietf.org/rfc/rfc1035.txt
// See section 4.1.4. Message compression
bool MdnsWriter::Write(const DomainName& name) {
  if (name.empty()) {
    return false;
  }

  Cursor cursor(this);
  const std::vector<uint64_t> subhashes = ComputeDomainNameSubhashes(name);
  // Tentative dictionary contains label pointer entries to be added to the
  // compression dictionary after successfully writing the domain name.
  std::unordered_map<uint64_t, uint16_t> tentative_dictionary;
  const std::vector<std::string>& labels = name.labels();
  for (size_t i = 0; i < labels.size(); ++i) {
    OSP_DCHECK(IsValidDomainLabel(labels[i]));
    // We only need to do a look up in the compression dictionary and not in the
    // tentative dictionary. The tentative dictionary cannot possibly contain a
    // valid label pointer as all the entries previously added to it are for
    // names that are longer than the currently processed sub-name.
    auto find_result = dictionary_.find(subhashes[i]);
    if (find_result != dictionary_.end()) {
      if (!Write(find_result->second)) {
        return false;
      }
      dictionary_.insert(tentative_dictionary.begin(),
                         tentative_dictionary.end());
      cursor.Commit();
      return true;
    }
    // Only add a pointer_label for compression if the offset into the buffer
    // fits into the bits available to store it.
    if (IsValidPointerLabelOffset(current() - begin())) {
      tentative_dictionary.insert(
          std::make_pair(subhashes[i], MakePointerLabel(current() - begin())));
    }
    if (!Write(MakeDirectLabel(labels[i].size())) ||
        !Write(labels[i].data(), labels[i].size())) {
      return false;
    }
  }
  if (!Write(kLabelTermination)) {
    return false;
  }
  // The probability of a collision is extremely low in this application, as the
  // number of domain names compressed is insignificant in comparison to the
  // hash function image.
  dictionary_.insert(tentative_dictionary.begin(), tentative_dictionary.end());
  cursor.Commit();
  return true;
}

bool MdnsWriter::Write(const RawRecordRdata& rdata) {
  Cursor cursor(this);
  if (Write(rdata.size()) && Write(rdata.data(), rdata.size())) {
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsWriter::Write(const SrvRecordRdata& rdata) {
  Cursor cursor(this);
  // Leave space at the beginning at |rollback_position| to write the record
  // length. Cannot write it upfront, since the exact space taken by the target
  // domain name is not known as it might be compressed.
  if (Skip(sizeof(uint16_t)) && Write(rdata.priority()) &&
      Write(rdata.weight()) && Write(rdata.port()) && Write(rdata.target()) &&
      UpdateRecordLength(current(), cursor.origin())) {
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsWriter::Write(const ARecordRdata& rdata) {
  Cursor cursor(this);
  if (Write(static_cast<uint16_t>(IPAddress::kV4Size)) &&
      Write(rdata.ipv4_address())) {
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsWriter::Write(const AAAARecordRdata& rdata) {
  Cursor cursor(this);
  if (Write(static_cast<uint16_t>(IPAddress::kV6Size)) &&
      Write(rdata.ipv6_address())) {
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsWriter::Write(const PtrRecordRdata& rdata) {
  Cursor cursor(this);
  // Leave space at the beginning at |rollback_position| to write the record
  // length. Cannot write it upfront, since the exact space taken by the target
  // domain name is not known as it might be compressed.
  if (Skip(sizeof(uint16_t)) && Write(rdata.ptr_domain()) &&
      UpdateRecordLength(current(), cursor.origin())) {
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsWriter::Write(const TxtRecordRdata& rdata) {
  Cursor cursor(this);
  // Leave space at the beginning at |rollback_position| to write the record
  // length. It's cheaper to update it at the end than precompute the length.
  if (!Skip(sizeof(uint16_t))) {
    return false;
  }
  if (rdata.texts().size() > 0) {
    if (!Write(rdata.texts())) {
      return false;
    }
  } else {
    if (!Write(kTXTEmptyRdata)) {
      return false;
    }
  }
  if (!UpdateRecordLength(current(), cursor.origin())) {
    return false;
  }
  cursor.Commit();
  return true;
}

bool MdnsWriter::Write(const NsecRecordRdata& rdata) {
  Cursor cursor(this);
  if (Skip(sizeof(uint16_t)) && Write(rdata.next_domain_name()) &&
      Write(rdata.encoded_types()) &&
      UpdateRecordLength(current(), cursor.origin())) {
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsWriter::Write(const OptRecordRdata& rdata) {
  // OPT records are currently not supported for outgoing messages.
  OSP_UNIMPLEMENTED();
  return false;
}

bool MdnsWriter::Write(const MdnsRecord& record) {
  Cursor cursor(this);
  if (Write(record.name()) && Write(static_cast<uint16_t>(record.dns_type())) &&
      Write(MakeRecordClass(record.dns_class(), record.record_type())) &&
      Write(static_cast<uint32_t>(record.ttl().count())) &&
      Write(record.rdata())) {
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsWriter::Write(const MdnsQuestion& question) {
  Cursor cursor(this);
  if (Write(question.name()) &&
      Write(static_cast<uint16_t>(question.dns_type())) &&
      Write(
          MakeQuestionClass(question.dns_class(), question.response_type()))) {
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsWriter::Write(const MdnsMessage& message) {
  Cursor cursor(this);
  Header header;
  header.id = message.id();
  header.flags = MakeFlags(message.type(), message.is_truncated());
  header.question_count = message.questions().size();
  header.answer_count = message.answers().size();
  header.authority_record_count = message.authority_records().size();
  header.additional_record_count = message.additional_records().size();
  if (Write(header) && Write(message.questions()) && Write(message.answers()) &&
      Write(message.authority_records()) &&
      Write(message.additional_records())) {
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsWriter::Write(const IPAddress& address) {
  uint8_t bytes[IPAddress::kV6Size];
  size_t size;
  if (address.IsV6()) {
    address.CopyToV6(bytes);
    size = IPAddress::kV6Size;
  } else {
    address.CopyToV4(bytes);
    size = IPAddress::kV4Size;
  }
  return Write(bytes, size);
}

bool MdnsWriter::Write(const Rdata& rdata) {
  return absl::visit([this](const auto& r) { return this->Write(r); }, rdata);
}

bool MdnsWriter::Write(const Header& header) {
  Cursor cursor(this);
  if (Write(header.id) && Write(header.flags) && Write(header.question_count) &&
      Write(header.answer_count) && Write(header.authority_record_count) &&
      Write(header.additional_record_count)) {
    cursor.Commit();
    return true;
  }
  return false;
}

}  // namespace discovery
}  // namespace openscreen
