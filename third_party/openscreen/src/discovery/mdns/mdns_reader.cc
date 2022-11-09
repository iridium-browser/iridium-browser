// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_reader.h"

#include <algorithm>
#include <utility>

#include "discovery/common/config.h"
#include "discovery/mdns/public/mdns_constants.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {
namespace discovery {
namespace {

bool TryParseDnsType(uint16_t to_parse, DnsType* type) {
  auto it = std::find(kSupportedDnsTypes.begin(), kSupportedDnsTypes.end(),
                      static_cast<DnsType>(to_parse));
  if (it == kSupportedDnsTypes.end()) {
    return false;
  }

  *type = *it;
  return true;
}

}  // namespace

MdnsReader::MdnsReader(const Config& config,
                       const uint8_t* buffer,
                       size_t length)
    : BigEndianReader(buffer, length),
      kMaximumAllowedRdataSize(
          static_cast<size_t>(config.maximum_valid_rdata_size)) {
  // TODO(rwkeane): Validate |maximum_valid_rdata_size| > MaxWireSize() for
  // rdata types A, AAAA, SRV, PTR.
  OSP_DCHECK_GT(config.maximum_valid_rdata_size, 0);
}

bool MdnsReader::Read(TxtRecordRdata::Entry* out) {
  Cursor cursor(this);
  uint8_t entry_length;
  if (!Read(&entry_length)) {
    return false;
  }
  const uint8_t* entry_begin = current();
  if (!Skip(entry_length)) {
    return false;
  }
  out->reserve(entry_length);
  out->insert(out->end(), entry_begin, entry_begin + entry_length);
  cursor.Commit();
  return true;
}

// RFC 1035: https://www.ietf.org/rfc/rfc1035.txt
// See section 4.1.4. Message compression.
bool MdnsReader::Read(DomainName* out) {
  OSP_DCHECK(out);
  const uint8_t* position = current();
  // The number of bytes consumed reading from the starting position to either
  // the first label pointer or the final termination byte, including the
  // pointer or the termination byte. This is equal to the actual wire size of
  // the DomainName accounting for compression.
  size_t bytes_consumed = 0;
  // The number of bytes that was processed when reading the DomainName,
  // including all label pointers and direct labels. It is used to detect
  // circular compression. The number of processed bytes cannot be possibly
  // greater than the length of the buffer.
  size_t bytes_processed = 0;
  size_t domain_name_length = 0;
  std::vector<absl::string_view> labels;
  // If we are pointing before the beginning or past the end of the buffer, we
  // hit a malformed pointer. If we have processed more bytes than there are in
  // the buffer, we are in a circular compression loop.
  while (position >= begin() && position < end() &&
         bytes_processed <= length()) {
    const uint8_t label_type = ReadBigEndian<uint8_t>(position);
    if (IsTerminationLabel(label_type)) {
      ErrorOr<DomainName> domain =
          DomainName::TryCreate(labels.begin(), labels.end());
      if (domain.is_error()) {
        return false;
      }
      *out = std::move(domain.value());
      if (!bytes_consumed) {
        bytes_consumed = position + sizeof(uint8_t) - current();
      }
      return Skip(bytes_consumed);
    } else if (IsPointerLabel(label_type)) {
      if (position + sizeof(uint16_t) > end()) {
        return false;
      }
      const uint16_t label_offset =
          GetPointerLabelOffset(ReadBigEndian<uint16_t>(position));
      if (!bytes_consumed) {
        bytes_consumed = position + sizeof(uint16_t) - current();
      }
      bytes_processed += sizeof(uint16_t);
      position = begin() + label_offset;
    } else if (IsDirectLabel(label_type)) {
      const uint8_t label_length = GetDirectLabelLength(label_type);
      OSP_DCHECK_GT(label_length, 0);
      bytes_processed += sizeof(uint8_t);
      position += sizeof(uint8_t);
      if (position + label_length >= end()) {
        return false;
      }
      const absl::string_view label(reinterpret_cast<const char*>(position),
                                    label_length);
      domain_name_length += label_length + 1;  // including the length byte
      if (!IsValidDomainLabel(label) ||
          domain_name_length > kMaxDomainNameLength) {
        return false;
      }
      labels.push_back(label);
      bytes_processed += label_length;
      position += label_length;
    } else {
      return false;
    }
  }
  return false;
}

bool MdnsReader::Read(RawRecordRdata* out) {
  OSP_DCHECK(out);
  Cursor cursor(this);
  uint16_t record_length;
  if (Read(&record_length)) {
    if (record_length > kMaximumAllowedRdataSize) {
      return false;
    }

    std::vector<uint8_t> buffer(record_length);
    if (Read(buffer.size(), buffer.data())) {
      ErrorOr<RawRecordRdata> rdata =
          RawRecordRdata::TryCreate(std::move(buffer));
      if (rdata.is_error()) {
        return false;
      }
      *out = std::move(rdata.value());
      cursor.Commit();
      return true;
    }
  }
  return false;
}

bool MdnsReader::Read(SrvRecordRdata* out) {
  OSP_DCHECK(out);
  Cursor cursor(this);
  uint16_t record_length;
  uint16_t priority;
  uint16_t weight;
  uint16_t port;
  DomainName target;
  if (Read(&record_length) && Read(&priority) && Read(&weight) && Read(&port) &&
      Read(&target) &&
      (cursor.delta() == sizeof(record_length) + record_length)) {
    *out = SrvRecordRdata(priority, weight, port, std::move(target));
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsReader::Read(ARecordRdata* out) {
  OSP_DCHECK(out);
  Cursor cursor(this);
  uint16_t record_length;
  IPAddress address;
  if (Read(&record_length) && (record_length == IPAddress::kV4Size) &&
      Read(IPAddress::Version::kV4, &address)) {
    *out = ARecordRdata(address);
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsReader::Read(AAAARecordRdata* out) {
  OSP_DCHECK(out);
  Cursor cursor(this);
  uint16_t record_length;
  IPAddress address;
  if (Read(&record_length) && (record_length == IPAddress::kV6Size) &&
      Read(IPAddress::Version::kV6, &address)) {
    *out = AAAARecordRdata(address);
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsReader::Read(PtrRecordRdata* out) {
  OSP_DCHECK(out);
  Cursor cursor(this);
  uint16_t record_length;
  DomainName ptr_domain;
  if (Read(&record_length) && Read(&ptr_domain) &&
      (cursor.delta() == sizeof(record_length) + record_length)) {
    *out = PtrRecordRdata(std::move(ptr_domain));
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsReader::Read(TxtRecordRdata* out) {
  OSP_DCHECK(out);
  Cursor cursor(this);
  uint16_t record_length;
  if (!Read(&record_length)) {
    return false;
  }
  if (record_length > kMaximumAllowedRdataSize) {
    return false;
  }
  std::vector<TxtRecordRdata::Entry> texts;
  while (cursor.delta() < sizeof(record_length) + record_length) {
    TxtRecordRdata::Entry entry;
    if (!Read(&entry)) {
      return false;
    }
    OSP_DCHECK(entry.size() <= kTXTMaxEntrySize);
    if (!entry.empty()) {
      texts.emplace_back(entry);
    }
  }
  if (cursor.delta() != sizeof(record_length) + record_length) {
    return false;
  }
  ErrorOr<TxtRecordRdata> rdata = TxtRecordRdata::TryCreate(std::move(texts));
  if (rdata.is_error()) {
    return false;
  }
  *out = std::move(rdata.value());
  cursor.Commit();
  return true;
}

bool MdnsReader::Read(NsecRecordRdata* out) {
  OSP_DCHECK(out);
  Cursor cursor(this);

  const uint8_t* start_position = current();
  uint16_t record_length;
  DomainName next_record_name;
  if (!Read(&record_length) || !Read(&next_record_name)) {
    return false;
  }
  if (record_length > kMaximumAllowedRdataSize) {
    return false;
  }

  // Calculate the next record name length. This may not be equal to the length
  // of |next_record_name| due to domain name compression.
  const int encoded_next_name_length =
      current() - start_position - sizeof(record_length);
  const int remaining_length = record_length - encoded_next_name_length;
  if (remaining_length <= 0) {
    // This means either the length is invalid or the NSEC record has no
    // associated types.
    return false;
  }

  std::vector<DnsType> types;
  if (Read(&types, remaining_length)) {
    *out = NsecRecordRdata(std::move(next_record_name), std::move(types));
    cursor.Commit();
    return true;
  }

  return false;
}

bool MdnsReader::Read(MdnsRecord* out) {
  OSP_DCHECK(out);
  Cursor cursor(this);
  DomainName name;
  uint16_t type;
  uint16_t rrclass;
  uint32_t ttl;
  Rdata rdata;
  if (Read(&name) && Read(&type) && Read(&rrclass) && Read(&ttl) &&
      Read(static_cast<DnsType>(type), &rdata)) {
    ErrorOr<MdnsRecord> record = MdnsRecord::TryCreate(
        std::move(name), static_cast<DnsType>(type), GetDnsClass(rrclass),
        GetRecordType(rrclass), std::chrono::seconds(ttl), std::move(rdata));
    if (record.is_error()) {
      return false;
    }
    *out = std::move(record.value());

    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsReader::Read(MdnsQuestion* out) {
  OSP_DCHECK(out);
  Cursor cursor(this);
  DomainName name;
  uint16_t type;
  uint16_t rrclass;
  if (Read(&name) && Read(&type) && Read(&rrclass)) {
    ErrorOr<MdnsQuestion> question =
        MdnsQuestion::TryCreate(std::move(name), static_cast<DnsType>(type),
                                GetDnsClass(rrclass), GetResponseType(rrclass));
    if (question.is_error()) {
      return false;
    }
    *out = std::move(question.value());

    cursor.Commit();
    return true;
  }
  return false;
}

ErrorOr<MdnsMessage> MdnsReader::Read() {
  MdnsMessage out;
  Cursor cursor(this);
  Header header;
  std::vector<MdnsQuestion> questions;
  std::vector<MdnsRecord> answers;
  std::vector<MdnsRecord> authority_records;
  std::vector<MdnsRecord> additional_records;
  if (Read(&header) && Read(header.question_count, &questions) &&
      Read(header.answer_count, &answers) &&
      Read(header.authority_record_count, &authority_records) &&
      Read(header.additional_record_count, &additional_records)) {
    if (!IsValidFlagsSection(header.flags)) {
      return Error::Code::kMdnsNonConformingFailure;
    }

    ErrorOr<MdnsMessage> message = MdnsMessage::TryCreate(
        header.id, GetMessageType(header.flags), questions, answers,
        authority_records, additional_records);
    if (message.is_error()) {
      return std::move(message.error());
    }
    out = std::move(message.value());

    if (IsMessageTruncated(header.flags)) {
      out.set_truncated();
    }

    cursor.Commit();
    return out;
  }
  return Error::Code::kMdnsReadFailure;
}

bool MdnsReader::Read(IPAddress::Version version, IPAddress* out) {
  OSP_DCHECK(out);
  size_t ipaddress_size = (version == IPAddress::Version::kV6)
                              ? IPAddress::kV6Size
                              : IPAddress::kV4Size;
  const uint8_t* const address_bytes = current();
  if (Skip(ipaddress_size)) {
    *out = IPAddress(version, address_bytes);
    return true;
  }
  return false;
}

bool MdnsReader::Read(DnsType type, Rdata* out) {
  OSP_DCHECK(out);
  switch (type) {
    case DnsType::kSRV:
      return Read<SrvRecordRdata>(out);
    case DnsType::kA:
      return Read<ARecordRdata>(out);
    case DnsType::kAAAA:
      return Read<AAAARecordRdata>(out);
    case DnsType::kPTR:
      return Read<PtrRecordRdata>(out);
    case DnsType::kTXT:
      return Read<TxtRecordRdata>(out);
    case DnsType::kNSEC:
      return Read<NsecRecordRdata>(out);
    default:
      OSP_DCHECK(!Contains(kSupportedDnsTypes, type));
      return Read<RawRecordRdata>(out);
  }
}

bool MdnsReader::Read(Header* out) {
  OSP_DCHECK(out);
  Cursor cursor(this);
  if (Read(&out->id) && Read(&out->flags) && Read(&out->question_count) &&
      Read(&out->answer_count) && Read(&out->authority_record_count) &&
      Read(&out->additional_record_count)) {
    cursor.Commit();
    return true;
  }
  return false;
}

bool MdnsReader::Read(std::vector<DnsType>* out, int remaining_size) {
  OSP_DCHECK(out);
  Cursor cursor(this);

  // Continue reading bitmaps until the entire input is read. If we have gone
  // past the end of the record, it's malformed input so fail.
  *out = std::vector<DnsType>();
  int processed_bytes = 0;
  while (processed_bytes < remaining_size) {
    NsecBitMapField bitmap;
    if (!Read(&bitmap)) {
      return false;
    }

    processed_bytes += bitmap.bitmap_length + 2;
    if (processed_bytes > remaining_size) {
      return false;
    }

    // The ith bit of the bitmap represents DnsType with value i, shifted
    // a multiple of 0x100 according to the window.
    for (int32_t i = 0; i < bitmap.bitmap_length * 8; i++) {
      int current_byte = i / 8;
      uint8_t bitmask = 0x80 >> i % 8;

      // If this bit flag represents a type we support, add it to the vector.
      // Else, we won't be able to use it later on in the code anyway, so drop
      // it.
      DnsType type;
      uint16_t type_index = i | (bitmap.window_block << 8);
      if ((bitmap.bitmap[current_byte] & bitmask) &&
          TryParseDnsType(type_index, &type)) {
        out->push_back(type);
      }
    }
  }

  cursor.Commit();
  return true;
}

bool MdnsReader::Read(NsecBitMapField* out) {
  OSP_DCHECK(out);
  Cursor cursor(this);

  // Read the window and bitmap length, then one byte for each byte called out
  // by the length.
  if (Read(&out->window_block) && Read(&out->bitmap_length)) {
    if (out->bitmap_length == 0 || out->bitmap_length > 32) {
      return false;
    }

    out->bitmap = current();
    if (!Skip(out->bitmap_length)) {
      return false;
    }
    cursor.Commit();
    return true;
  }

  return false;
}

}  // namespace discovery
}  // namespace openscreen
