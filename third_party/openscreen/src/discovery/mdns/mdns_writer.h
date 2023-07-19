// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_MDNS_MDNS_WRITER_H_
#define DISCOVERY_MDNS_MDNS_WRITER_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "discovery/mdns/mdns_records.h"
#include "util/big_endian.h"

namespace openscreen {
namespace discovery {

class MdnsWriter : public BigEndianWriter {
 public:
  using BigEndianWriter::BigEndianWriter;
  using BigEndianWriter::Write;

  // The following methods return true if the method was able to successfully
  // write the value to the underlying buffer and advances current() to point
  // right past the written data. Returns false if the method failed to write
  // the value to the underlying buffer, current() remains unchanged.
  bool Write(absl::string_view value);
  bool Write(const std::string& value);
  bool Write(const DomainName& name);
  bool Write(const RawRecordRdata& rdata);
  bool Write(const SrvRecordRdata& rdata);
  bool Write(const ARecordRdata& rdata);
  bool Write(const AAAARecordRdata& rdata);
  bool Write(const PtrRecordRdata& rdata);
  bool Write(const TxtRecordRdata& rdata);
  bool Write(const NsecRecordRdata& rdata);
  bool Write(const OptRecordRdata& rdata);
  // Writes a DNS resource record with its RDATA.
  // The correct type of RDATA to be written is contained in the type
  // specified in the record.
  bool Write(const MdnsRecord& record);
  bool Write(const MdnsQuestion& question);
  // Writes multiple mDNS questions and records that are a part of
  // a mDNS message being read
  bool Write(const MdnsMessage& message);

 private:
  bool Write(const IPAddress& address);
  bool Write(const Rdata& rdata);
  bool Write(const Header& header);

  template <class ItemType>
  bool Write(const std::vector<ItemType>& collection) {
    Cursor cursor(this);
    for (const ItemType& entry : collection) {
      if (!Write(entry)) {
        return false;
      }
    }
    cursor.Commit();
    return true;
  }

  // Domain name compression dictionary.
  // Maps hashes of previously written domain (sub)names
  // to the label pointers of the first occurrences in the underlying buffer.
  // Compression of multiple domain names is supported on the same instance of
  // the MdnsWriter. Underlying buffer may contain other data in addition to the
  // domain names. The compression dictionary persists between calls to
  // Write.
  // Label pointer is only 16 bits in size as per RFC 1035. Only lower 14 bits
  // are allocated for storing the offset.
  std::unordered_map<uint64_t, uint16_t> dictionary_;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_MDNS_MDNS_WRITER_H_
