// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_MDNS_MDNS_READER_H_
#define DISCOVERY_MDNS_MDNS_READER_H_

#include <utility>
#include <vector>

#include "discovery/mdns/mdns_records.h"
#include "platform/base/error.h"
#include "util/big_endian.h"

namespace openscreen {
namespace discovery {

struct Config;

class MdnsReader : public BigEndianReader {
 public:
  MdnsReader(const Config& config, const uint8_t* buffer, size_t length);

  using BigEndianReader::Read;

  // The following methods return true if the method was able to successfully
  // read the value to |out| and advances current() to point right past the read
  // data. Returns false if the method failed to read the value to |out|,
  // current() remains unchanged.
  bool Read(TxtRecordRdata::Entry* out);
  bool Read(DomainName* out);
  bool Read(RawRecordRdata* out);
  bool Read(SrvRecordRdata* out);
  bool Read(ARecordRdata* out);
  bool Read(AAAARecordRdata* out);
  bool Read(PtrRecordRdata* out);
  bool Read(TxtRecordRdata* out);
  bool Read(NsecRecordRdata* out);

  // Reads a DNS resource record with its RDATA.
  // The correct type of RDATA to be read is determined by the type
  // specified in the record.
  bool Read(MdnsRecord* out);
  bool Read(MdnsQuestion* out);

  // Reads multiple mDNS questions and records that are a part of
  // a mDNS message being read.
  ErrorOr<MdnsMessage> Read();

 private:
  struct NsecBitMapField {
    uint8_t window_block;
    uint8_t bitmap_length;
    const uint8_t* bitmap;
  };

  bool Read(IPAddress::Version version, IPAddress* out);
  bool Read(DnsType type, Rdata* out);
  bool Read(Header* out);
  bool Read(std::vector<DnsType>* types, int remaining_length);
  bool Read(NsecBitMapField* out);

  template <class ItemType>
  bool Read(uint16_t count, std::vector<ItemType>* out) {
    Cursor cursor(this);
    out->reserve(count);
    for (uint16_t i = 0; i < count; ++i) {
      ItemType entry;
      if (!Read(&entry)) {
        return false;
      }
      out->emplace_back(std::move(entry));
    }
    cursor.Commit();
    return true;
  }

  template <class RdataType>
  bool Read(Rdata* out) {
    RdataType rdata;
    if (Read(&rdata)) {
      *out = std::move(rdata);
      return true;
    }
    return false;
  }

  // Maximum allowed size for the rdata in any received record.
  const size_t kMaximumAllowedRdataSize;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_MDNS_MDNS_READER_H_
