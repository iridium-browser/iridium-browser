// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_reader.h"

#include <memory>
#include <sstream>

#include "discovery/common/config.h"
#include "discovery/mdns/testing/mdns_test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace discovery {

namespace {

constexpr std::chrono::seconds kTtl{120};

template <class T>
void TestReadEntrySucceeds(const uint8_t* data,
                           size_t size,
                           const T& expected) {
  Config config;
  MdnsReader reader(config, data, size);
  T entry;
  EXPECT_TRUE(reader.Read(&entry));
  EXPECT_EQ(entry, expected);
  EXPECT_EQ(reader.remaining(), UINT64_C(0));
}

template <>
void TestReadEntrySucceeds<MdnsMessage>(const uint8_t* data,
                                        size_t size,
                                        const MdnsMessage& expected) {
  MdnsReader reader(Config{}, data, size);
  const ErrorOr<MdnsMessage> message = reader.Read();
  EXPECT_TRUE(message.is_value());
  EXPECT_EQ(message.value(), expected);
  EXPECT_EQ(reader.remaining(), UINT64_C(0));
}

template <class T>
void TestReadEntryFails(const uint8_t* data, size_t size) {
  Config config;
  MdnsReader reader(config, data, size);
  T entry;
  EXPECT_FALSE(reader.Read(&entry));
  // There should be no side effects for failing to read an entry. The
  // underlying pointer should not have changed.
  EXPECT_EQ(reader.offset(), UINT64_C(0));
}

template <>
void TestReadEntryFails<MdnsMessage>(const uint8_t* data, size_t size) {
  Config config;
  MdnsReader reader(config, data, size);
  const ErrorOr<MdnsMessage> message = reader.Read();
  EXPECT_TRUE(message.is_error());

  // There should be no side effects for failing to read an entry. The
  // underlying pointer should not have changed.
  EXPECT_EQ(reader.offset(), UINT64_C(0));
}
}  // namespace

TEST(MdnsReaderTest, ReadDomainName) {
  constexpr uint8_t kMessage[] = {
      // First name
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',  // Byte: 0
      0x05, 'l', 'o', 'c', 'a', 'l',            // Byte: 8
      0x00,                                     // Byte: 14
      // Second name
      0x07, 's', 'e', 'r', 'v', 'i', 'c', 'e',  // Byte: 15
      0xc0, 0x00,                               // Byte: 23
      // Third name
      0x06, 'd', 'e', 'v', 'i', 'c', 'e',  // Byte: 25
      0xc0, 0x0f,                          // Byte: 32
      // Fourth name
      0xc0, 0x20,  // Byte: 34
  };
  Config config;
  MdnsReader reader(config, kMessage, sizeof(kMessage));
  EXPECT_EQ(reader.begin(), kMessage);
  EXPECT_EQ(reader.length(), sizeof(kMessage));
  EXPECT_EQ(reader.offset(), 0u);
  DomainName name;
  std::stringstream name_stream;
  EXPECT_TRUE(reader.Read(&name));
  name_stream.str("");
  name_stream << name;
  EXPECT_EQ(name_stream.str(), "testing.local");
  EXPECT_TRUE(reader.Read(&name));
  name_stream.str("");
  name_stream << name;
  EXPECT_EQ(name_stream.str(), "service.testing.local");
  EXPECT_TRUE(reader.Read(&name));
  name_stream.str("");
  name_stream << name;
  EXPECT_EQ(name_stream.str(), "device.service.testing.local");
  EXPECT_TRUE(reader.Read(&name));
  name_stream.str("");
  name_stream << name;
  EXPECT_EQ(name_stream.str(), "service.testing.local");
  EXPECT_EQ(reader.offset(), sizeof(kMessage));
  EXPECT_EQ(0UL, reader.remaining());
  EXPECT_FALSE(reader.Read(&name));
}

TEST(MdnsReaderTest, ReadDomainName_Empty) {
  constexpr uint8_t kDomainName[] = {0x00};
  TestReadEntrySucceeds(kDomainName, sizeof(kDomainName), DomainName());
}

TEST(MdnsReaderTest, ReadDomainName_TooShort) {
  // Length 0x03 is longer than available data for the domain name
  constexpr uint8_t kDomainName[] = {0x03, 'a', 'b'};
  TestReadEntryFails<DomainName>(kDomainName, sizeof(kDomainName));
}

TEST(MdnsReaderTest, ReadDomainName_TooLong) {
  std::vector<uint8_t> kDomainName;
  for (uint8_t letter = 'a'; letter <= 'z'; ++letter) {
    constexpr uint8_t repetitions = 10;
    kDomainName.push_back(repetitions);
    kDomainName.insert(kDomainName.end(), repetitions, letter);
  }
  kDomainName.push_back(0);
  TestReadEntryFails<DomainName>(kDomainName.data(), kDomainName.size());
}

TEST(MdnsReaderTest, ReadDomainName_LabelPointerOutOfBounds) {
  constexpr uint8_t kDomainName[] = {0xc0, 0x02};
  TestReadEntryFails<DomainName>(kDomainName, sizeof(kDomainName));
}

TEST(MdnsReaderTest, ReadDomainName_InvalidLabel) {
  constexpr uint8_t kDomainName[] = {0x80};
  TestReadEntryFails<DomainName>(kDomainName, sizeof(kDomainName));
}

TEST(MdnsReaderTest, ReadDomainName_CircularCompression) {
  // clang-format off
  constexpr uint8_t kDomainName[] = {
      // NOTE: Circular label pointer at end of name.
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',  // Byte: 0
      0x05, 'l', 'o', 'c', 'a', 'l',            // Byte: 8
      0xc0, 0x00,                               // Byte: 14
  };
  // clang-format on
  TestReadEntryFails<DomainName>(kDomainName, sizeof(kDomainName));
}

TEST(MdnsReaderTest, ReadRawRecordRdata) {
  // clang-format off
  constexpr uint8_t kRawRecordRdata[] = {
      0x00, 0x08,  // RDLENGTH = 8 bytes
      0x05, 'c', 'n', 'a', 'm', 'e', 0xc0, 0x00,
  };
  // clang-format on
  TestReadEntrySucceeds(
      kRawRecordRdata, sizeof(kRawRecordRdata),
      RawRecordRdata(kRawRecordRdata + 2, sizeof(kRawRecordRdata) - 2));
}

TEST(MdnsReaderTest, ReadRawRecordRdata_TooLong) {
  // clang-format off
  constexpr uint8_t kRawRecordRdata[] = {
      0x00, 0x08,  // RDLENGTH = 8 bytes
      0x05, 'c', 'n', 'a', 'm', 'e', 0xc0, 0x00,
  };
  // clang-format on

  Config config;
  config.maximum_valid_rdata_size = 1;
  MdnsReader reader(config, kRawRecordRdata, sizeof(kRawRecordRdata));
  RawRecordRdata entry;
  EXPECT_FALSE(reader.Read(&entry));
  // There should be no side effects for failing to read an entry. The
  // underlying pointer should not have changed.
  EXPECT_EQ(reader.offset(), UINT64_C(0));
}

TEST(MdnsReaderTest, ReadRawRecord_Empty) {
  // clang-format off
  constexpr uint8_t kRawRecordRdata[] = {
      0x00, 0x00,  // RDLENGTH = 0 bytes
  };
  // clang-format on
  TestReadEntrySucceeds(kRawRecordRdata, sizeof(kRawRecordRdata),
                        RawRecordRdata());
}

TEST(MdnsReaderTest, ReadRawRecordRdata_TooShort) {
  // clang-format off
  constexpr uint8_t kRawRecordRdata[] = {
      0x00, 0x05,  // RDLENGTH = 5 bytes is longer that available data
      0x03, 'f', 'o', 'o'
  };
  // clang-format on
  TestReadEntryFails<RawRecordRdata>(kRawRecordRdata, sizeof(kRawRecordRdata));
}

TEST(MdnsReaderTest, ReadSrvRecordRdata) {
  // clang-format off
  constexpr uint8_t kSrvRecordRdata[] = {
      0x00, 0x15,  // RDLENGTH = 21
      0x00, 0x05,  // PRIORITY = 5
      0x00, 0x06,  // WEIGHT = 6
      0x1f, 0x49,  // PORT = 8009
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l', 0x00,
  };
  // clang-format on
  TestReadEntrySucceeds(
      kSrvRecordRdata, sizeof(kSrvRecordRdata),
      SrvRecordRdata(5, 6, 8009, DomainName{"testing", "local"}));
}

TEST(MdnsReaderTest, ReadSrvRecordRdata_TooShort) {
  // clang-format off
  constexpr uint8_t kSrvRecordRdata[] = {
      0x00, 0x14,  // RDLENGTH = 21
      0x00, 0x05,  // PRIORITY = 5
      0x00, 0x06,  // WEIGHT = 6
                   // Missing bytes
  };
  // clang-format on
  TestReadEntryFails<SrvRecordRdata>(kSrvRecordRdata, sizeof(kSrvRecordRdata));
}

TEST(MdnsReaderTest, ReadSrvRecordRdata_WrongLength) {
  // clang-format off
  constexpr uint8_t kSrvRecordRdata[] = {
      0x00, 0x14,  // RDLENGTH = 20
      0x00, 0x05,  // PRIORITY = 5
      0x00, 0x06,  // WEIGHT = 6
      0x1f, 0x49,  // PORT = 8009
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l', 0x00,
  };
  // clang-format on
  TestReadEntryFails<SrvRecordRdata>(kSrvRecordRdata, sizeof(kSrvRecordRdata));
}

TEST(MdnsReaderTest, ReadARecordRdata) {
  constexpr uint8_t kARecordRdata[] = {
      0x00, 0x4,               // RDLENGTH = 4
      0x08, 0x08, 0x08, 0x08,  // ADDRESS = 8.8.8.8
  };
  TestReadEntrySucceeds(kARecordRdata, sizeof(kARecordRdata),
                        ARecordRdata(IPAddress{8, 8, 8, 8}));
}

TEST(MdnsReaderTest, ReadARecordRdata_TooShort) {
  constexpr uint8_t kARecordRdata[] = {
      0x00, 0x4,   // RDLENGTH = 4
      0x08, 0x08,  // Address missing bytes
  };
  TestReadEntryFails<ARecordRdata>(kARecordRdata, sizeof(kARecordRdata));
}

TEST(MdnsReaderTest, ReadARecordRdata_WrongLength) {
  constexpr uint8_t kARecordRdata[] = {
      0x00, 0x5,               // RDLENGTH = 5
      0x08, 0x08, 0x08, 0x08,  // ADDRESS = 8.8.8.8
  };
  TestReadEntryFails<ARecordRdata>(kARecordRdata, sizeof(kARecordRdata));
}

TEST(MdnsReaderTest, ReadAAAARecordRdata) {
  // clang-format off
  constexpr uint8_t kAAAARecordRdata[] = {
      0x00, 0x10,  // RDLENGTH = 16
      // ADDRESS = FE80:0000:0000:0000:0202:B3FF:FE1E:8329
      0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x02, 0x02, 0xb3, 0xff, 0xfe, 0x1e, 0x83, 0x29,
  };
  // clang-format on
  TestReadEntrySucceeds(kAAAARecordRdata, sizeof(kAAAARecordRdata),
                        AAAARecordRdata(IPAddress(IPAddress::Version::kV6,
                                                  kAAAARecordRdata + 2)));
}

TEST(MdnsReaderTest, ReadAAAARecordRdata_TooShort) {
  // clang-format off
  constexpr uint8_t kAAAARecordRdata[] = {
      0x00, 0x10,  // RDLENGTH = 16
      // ADDRESS missing bytes
      0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  // clang-format on
  TestReadEntryFails<AAAARecordRdata>(kAAAARecordRdata,
                                      sizeof(kAAAARecordRdata));
}

TEST(MdnsReaderTest, ReadAAAARecordRdata_WrongLength) {
  // clang-format off
  constexpr uint8_t kAAAARecordRdata[] = {
      0x00, 0x11,  // RDLENGTH = 17
      // ADDRESS = FE80:0000:0000:0000:0202:B3FF:FE1E:8329
      0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x02, 0x02, 0xb3, 0xff, 0xfe, 0x1e, 0x83, 0x29,
  };
  // clang-format on
  TestReadEntryFails<AAAARecordRdata>(kAAAARecordRdata,
                                      sizeof(kAAAARecordRdata));
}

TEST(MdnsReaderTest, ReadPtrRecordRdata) {
  // clang-format off
  constexpr uint8_t kPtrRecordRdata[] = {
      0x00, 0x18,  // RDLENGTH = 24
      0x08, 'm', 'y', 'd', 'e', 'v', 'i', 'c', 'e',
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a',  'l',
      0x00,
  };
  // clang-format on
  TestReadEntrySucceeds(
      kPtrRecordRdata, sizeof(kPtrRecordRdata),
      PtrRecordRdata(DomainName{"mydevice", "testing", "local"}));
}

TEST(MdnsReaderTest, ReadPtrRecordRdata_TooShort) {
  // clang-format off
  constexpr uint8_t kPtrRecordRdata[] = {
      0x00, 0x18,  // RDLENGTH = 24
      0x08, 'm', 'y', 'd', 'e', 'v', 'i', 'c', 'e',
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c'
      // Missing bytes
  };
  // clang-format on
  TestReadEntryFails<PtrRecordRdata>(kPtrRecordRdata, sizeof(kPtrRecordRdata));
}

TEST(MdnsReaderTest, ReadPtrRecordRdata_WrongLength) {
  // clang-format off
  constexpr uint8_t kPtrRecordRdata[] = {
      0x00, 0x18,  // RDLENGTH = 24
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a',  'l',
      0x00,
  };
  // clang-format on
  TestReadEntryFails<PtrRecordRdata>(kPtrRecordRdata, sizeof(kPtrRecordRdata));
}

TEST(MdnsReaderTest, ReadTxtRecordRdata) {
  // clang-format off
  constexpr uint8_t kTxtRecordRdata[] = {
      0x00, 0x0C,  // RDLENGTH = 12
      0x05, 'f', 'o', 'o', '=', '1',
      0x05, 'b', 'a', 'r', '=', '2',
  };
  // clang-format on
  TestReadEntrySucceeds(kTxtRecordRdata, sizeof(kTxtRecordRdata),
                        MakeTxtRecord({"foo=1", "bar=2"}));
}

TEST(MdnsReaderTest, ReadTxtRecordRdata_Empty) {
  constexpr uint8_t kTxtRecordRdata[] = {
      0x00, 0x01,  // RDLENGTH = 1
      0x00,        // empty string
  };
  TestReadEntrySucceeds(kTxtRecordRdata, sizeof(kTxtRecordRdata),
                        TxtRecordRdata());
}

TEST(MdnsReaderTest, ReadTxtRecordRdata_WithNullInTheMiddle) {
  // clang-format off
  constexpr uint8_t kTxtRecordRdata[] = {
      0x00, 0x10,  // RDLENGTH = 16
      0x09, 'w', 'i', 't', 'h', '\0', 'N', 'U', 'L', 'L',
      0x05, 'o', 't', 'h', 'e', 'r',
  };
  // clang-format on
  TestReadEntrySucceeds(
      kTxtRecordRdata, sizeof(kTxtRecordRdata),
      MakeTxtRecord({absl::string_view("with\0NULL", 9), "other"}));
}

TEST(MdnsReaderTest, ReadTxtRecordRdata_EmptyEntries) {
  // clang-format off
  constexpr uint8_t kTxtRecordRdata[] = {
      0x00, 0x0F,  // RDLENGTH = 12
      0x05, 'f', 'o', 'o', '=', '1',
      0x00,
      0x00,
      0x05, 'b', 'a', 'r', '=', '2',
      0x00,
  };
  // clang-format on
  TestReadEntrySucceeds(kTxtRecordRdata, sizeof(kTxtRecordRdata),
                        MakeTxtRecord({"foo=1", "bar=2"}));
}

TEST(MdnsReaderTest, ReadTxtRecordRdata_TooLong) {
  // clang-format off
  constexpr uint8_t kTxtRecordRdata[] = {
      0x00, 0x0C,  // RDLENGTH = 12
      0x05, 'f', 'o', 'o', '=', '1',
      0x05, 'b', 'a', 'r', '=', '2',
  };
  // clang-format on

  Config config;
  config.maximum_valid_rdata_size = 1;
  MdnsReader reader(config, kTxtRecordRdata, sizeof(kTxtRecordRdata));
  TxtRecordRdata entry;
  EXPECT_FALSE(reader.Read(&entry));
  // There should be no side effects for failing to read an entry. The
  // underlying pointer should not have changed.
  EXPECT_EQ(reader.offset(), UINT64_C(0));
}

TEST(MdnsReaderTest, ReadNsecRecordRdata) {
  // clang-format off
  constexpr uint8_t kExpectedRdata[] = {
    0x00, 0x20,  // RDLENGTH = 32
    0x08, 'm', 'y', 'd', 'e', 'v', 'i', 'c', 'e',
    0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
    0x05, 'l', 'o', 'c', 'a',  'l',
    0x00,
    // It takes 8 bytes to encode the kA and kSRV records because:
    // - Both record types have value less than 256, so they are both in window
    //   block 1.
    // - The bitmap length for this block is always a single byte
    // - DnsTypes have the following values:
    //   - kA = 1 (encoded in byte 1)
    //     kTXT = 16 (encoded in byte 3)
    //   - kSRV = 33 (encoded in byte 5)
    //   - kNSEC = 47 (encoded in 6 bytes)
    // - The largest of these is 47, so 6 bytes are needed to encode this data.
    // So the full encoded version is:
    //   00000000 00000110 01000000 00000000 10000000 00000000 0100000  00000001
    //   |window| | size | | 0-7  | | 8-15 | |16-23 | |24-31 | |32-39 | |40-47 |
          0x00,    0x06,    0x40,    0x00,    0x80,    0x00,    0x40,    0x01
  };
  // clang-format on
  TestReadEntrySucceeds(
      kExpectedRdata, sizeof(kExpectedRdata),
      NsecRecordRdata(DomainName{"mydevice", "testing", "local"}, DnsType::kA,
                      DnsType::kTXT, DnsType::kSRV, DnsType::kNSEC));
}

TEST(MdnsReaderTest, ReadNsecRecordRdata_TooShort) {
  // clang-format off
  constexpr uint8_t kNsecRecordRdata[] = {
    0x00, 0x20,  // RDLENGTH = 32
    0x08, 'm', 'y', 'd', 'e', 'v', 'i', 'c', 'e',
    0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
    0x05, 'l', 'o', 'c', 'a',  'l',
    0x00,
    0x00, 0x06, 0x40, 0x00
  };
  // clang-format on
  TestReadEntryFails<NsecRecordRdata>(kNsecRecordRdata,
                                      sizeof(kNsecRecordRdata));
}

TEST(MdnsReaderTest, ReadNsecRecordRdata_WrongLength) {
  // clang-format off
  constexpr uint8_t kNsecRecordRdata[] = {
    0x00, 0x21,  // RDLENGTH = 33
    0x08, 'm', 'y', 'd', 'e', 'v', 'i', 'c', 'e',
    0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
    0x05, 'l', 'o', 'c', 'a',  'l',
    0x00,
    0x00, 0x06, 0x40, 0x00, 0x80, 0x00, 0x40, 0x01
  };
  // clang-format on
  TestReadEntryFails<NsecRecordRdata>(kNsecRecordRdata,
                                      sizeof(kNsecRecordRdata));
}

TEST(MdnsReaderTest, ReadMdnsRecord_ARecordRdata) {
  // clang-format off
  constexpr uint8_t kTestRecord[] = {
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x01,              // TYPE = A (1)
      0x80, 0x01,              // CLASS = IN (1) | CACHE_FLUSH_BIT
      0x00, 0x00, 0x00, 0x78,  // TTL = 120 seconds
      0x00, 0x04,              // RDLENGTH = 4 bytes
      0x08, 0x08, 0x08, 0x08,  // RDATA = 8.8.8.8
  };
  // clang-format on
  TestReadEntrySucceeds(kTestRecord, sizeof(kTestRecord),
                        MdnsRecord(DomainName{"testing", "local"}, DnsType::kA,
                                   DnsClass::kIN, RecordType::kUnique, kTtl,
                                   ARecordRdata(IPAddress{8, 8, 8, 8})));
}

TEST(MdnsReaderTest, ReadMdnsRecord_UnknownRecordType) {
  // clang-format off
  constexpr uint8_t kTestRecord[] = {
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x05,              // TYPE = CNAME (5)
      0x80, 0x01,              // CLASS = IN (1) | CACHE_FLUSH_BIT
      0x00, 0x00, 0x00, 0x78,  // TTL = 120 seconds
      0x00, 0x08,              // RDLENGTH = 8 bytes
      0x05, 'c', 'n', 'a', 'm', 'e', 0xc0, 0x00,
  };
  constexpr uint8_t kCnameRdata[] = {
      0x05, 'c', 'n', 'a', 'm', 'e', 0xc0, 0x00,
  };
  // clang-format on
  TestReadEntrySucceeds(
      kTestRecord, sizeof(kTestRecord),
      MdnsRecord(DomainName{"testing", "local"},
                 static_cast<DnsType>(5) /*CNAME class*/, DnsClass::kIN,
                 RecordType::kUnique, kTtl,
                 RawRecordRdata(kCnameRdata, sizeof(kCnameRdata))));
}

TEST(MdnsReaderTest, ReadMdnsRecord_CompressedNames) {
  // clang-format off
  constexpr uint8_t kTestRecord[] = {
      // First message
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x0c,              // TYPE = PTR (12)
      0x00, 0x01,              // CLASS = IN (1)
      0x00, 0x00, 0x00, 0x78,  // TTL = 120 seconds
      0x00, 0x06,              // RDLENGTH = 6 bytes
      0x03, 'p',  't',  'r',
      0xc0, 0x00,              // Domain name label pointer to byte 0
      // Second message
      0x03, 'o', 'n', 'e',
      0x03, 't', 'w', 'o',
      0xc0, 0x00,              // Domain name label pointer to byte 0
      0x00, 0x01,              // TYPE = A (1)
      0x80, 0x01,              // CLASS = IN (1) | CACHE_FLUSH_BIT
      0x00, 0x00, 0x00, 0x78,  // TTL = 120 seconds
      0x00, 0x04,              // RDLENGTH = 4 bytes
      0x08, 0x08, 0x08, 0x08,  // RDATA = 8.8.8.8
  };
  // clang-format on
  Config config;
  MdnsReader reader(config, kTestRecord, sizeof(kTestRecord));

  MdnsRecord record;
  EXPECT_TRUE(reader.Read(&record));
  EXPECT_EQ(record,
            MdnsRecord(DomainName{"testing", "local"}, DnsType::kPTR,
                       DnsClass::kIN, RecordType::kShared, kTtl,
                       PtrRecordRdata(DomainName{"ptr", "testing", "local"})));
  EXPECT_TRUE(reader.Read(&record));
  EXPECT_EQ(record, MdnsRecord(DomainName{"one", "two", "testing", "local"},
                               DnsType::kA, DnsClass::kIN, RecordType::kUnique,
                               kTtl, ARecordRdata(IPAddress{8, 8, 8, 8})));
}

TEST(MdnsReaderTest, ReadMdnsRecord_MissingRdata) {
  // clang-format off
  constexpr uint8_t kTestRecord[] = {
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x01,              // TYPE = A (1)
      0x00, 0x01,              // CLASS = IN (1)
      0x00, 0x00, 0x00, 0x78,  // TTL = 120 seconds
      0x00, 0x04,              // RDLENGTH = 4 bytes
                               // Missing RDATA
  };
  // clang-format on
  TestReadEntryFails<MdnsRecord>(kTestRecord, sizeof(kTestRecord));
}

TEST(MdnsReaderTest, ReadMdnsRecord_InvalidHostName) {
  // clang-format off
  constexpr uint8_t kTestRecord[] = {
      // Invalid NAME: length byte too short
      0x03, 'i', 'n', 'v', 'a', 'l', 'i', 'd',
      0x00,
      0x00, 0x01,              // TYPE = A (1)
      0x00, 0x01,              // CLASS = IN (1)
      0x00, 0x00, 0x00, 0x78,  // TTL = 120 seconds
      0x00, 0x04,              // RDLENGTH = 4 bytes
      0x08, 0x08, 0x08, 0x08,  // RDATA = 8.8.8.8
  };
  // clang-format on
  TestReadEntryFails<MdnsRecord>(kTestRecord, sizeof(kTestRecord));
}

TEST(MdnsReaderTest, ReadMdnsQuestion) {
  // clang-format off
  constexpr uint8_t kTestQuestion[] = {
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x01,  // TYPE = A (1)
      0x80, 0x01,  // CLASS = IN (1) | UNICAST_BIT
  };
  // clang-format on
  TestReadEntrySucceeds(
      kTestQuestion, sizeof(kTestQuestion),
      MdnsQuestion(DomainName{"testing", "local"}, DnsType::kA, DnsClass::kIN,
                   ResponseType::kUnicast));
}

TEST(MdnsReaderTest, ReadMdnsQuestion_CompressedNames) {
  // clang-format off
  constexpr uint8_t kTestQuestions[] = {
      // First Question
      0x05, 'f', 'i', 'r', 's', 't',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x01,  // TYPE = A (1)
      0x80, 0x01,  // CLASS = IN (1) | UNICAST_BIT
      // Second Question
      0x06, 's', 'e', 'c', 'o', 'n', 'd',
      0xc0, 0x06,  // Domain name label pointer
      0x00, 0x0c,  // TYPE = PTR (12)
      0x00, 0x01,  // CLASS = IN (1)
  };
  // clang-format on
  Config config;
  MdnsReader reader(config, kTestQuestions, sizeof(kTestQuestions));
  MdnsQuestion question;
  EXPECT_TRUE(reader.Read(&question));
  EXPECT_EQ(question, MdnsQuestion(DomainName{"first", "local"}, DnsType::kA,
                                   DnsClass::kIN, ResponseType::kUnicast));
  EXPECT_TRUE(reader.Read(&question));
  EXPECT_EQ(question, MdnsQuestion(DomainName{"second", "local"}, DnsType::kPTR,
                                   DnsClass::kIN, ResponseType::kMulticast));
  EXPECT_EQ(reader.remaining(), UINT64_C(0));
}

TEST(MdnsReaderTest, ReadMdnsQuestion_InvalidHostName) {
  // clang-format off
  constexpr uint8_t kTestQuestion[] = {
      // Invalid NAME: length byte too short
      0x03, 'i', 'n', 'v', 'a', 'l', 'i', 'd',
      0x00,
      0x00, 0x01,  // TYPE = A (1)
      0x00, 0x01,  // CLASS = IN (1)
  };
  // clang-format on
  TestReadEntryFails<MdnsQuestion>(kTestQuestion, sizeof(kTestQuestion));
}

TEST(MdnsReaderTest, ReadMdnsMessage) {
  // clang-format off
  constexpr uint8_t kTestMessage[] = {
      // Header
      0x00, 0x01,  // ID = 1
      0x84, 0x00,  // FLAGS = AA | RESPONSE
      0x00, 0x00,  // Questions = 0
      0x00, 0x01,  // Answers = 1
      0x00, 0x00,  // Authority = 0
      0x00, 0x01,  // Additional = 1
      // Record 1
      0x07, 'r', 'e', 'c', 'o', 'r', 'd', '1',
      0x00,
      0x00, 0x0c,              // TYPE = PTR (12)
      0x00, 0x01,              // CLASS = IN (1)
      0x00, 0x00, 0x00, 0x78,  // TTL = 120 seconds
      0x00, 0x0f,              // RDLENGTH = 15 bytes
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      // Record 2
      0x07, 'r', 'e', 'c', 'o', 'r', 'd', '2',
      0x00,
      0x00, 0x01,              // TYPE = A (1)
      0x00, 0x01,              // CLASS = IN (1)
      0x00, 0x00, 0x00, 0x78,  // TTL = 120 seconds
      0x00, 0x04,              // RDLENGTH = 4 bytes
      0xac, 0x00, 0x00, 0x01,  // 172.0.0.1
  };
  // clang-format on

  MdnsRecord record1(DomainName{"record1"}, DnsType::kPTR, DnsClass::kIN,
                     RecordType::kShared, kTtl,
                     PtrRecordRdata(DomainName{"testing", "local"}));
  MdnsRecord record2(DomainName{"record2"}, DnsType::kA, DnsClass::kIN,
                     RecordType::kShared, kTtl,
                     ARecordRdata(IPAddress{172, 0, 0, 1}));
  MdnsMessage message(1, MessageType::Response, std::vector<MdnsQuestion>{},
                      std::vector<MdnsRecord>{record1},
                      std::vector<MdnsRecord>{},
                      std::vector<MdnsRecord>{record2});
  TestReadEntrySucceeds(kTestMessage, sizeof(kTestMessage), message);
}

TEST(MdnsReaderTest, ReadMdnsMessage_MissingAnswerRecord) {
  // clang-format off
  constexpr uint8_t kInvalidMessage[] = {
      0x00, 0x00,  // ID = 0
      0x00, 0x00,  // FLAGS = 0
      0x00, 0x01,  // Questions = 1
      0x00, 0x01,  // Answers = 1
      0x00, 0x00,  // Authority = 0
      0x00, 0x00,  // Additional = 0
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x0c,  // TYPE = PTR (12)
      0x00, 0x01,  // CLASS = IN (1)
      // NOTE: Missing answer record
  };
  // clang-format on
  TestReadEntryFails<MdnsMessage>(kInvalidMessage, sizeof(kInvalidMessage));
}

TEST(MdnsReaderTest, ReadMdnsMessage_MissingAdditionalRecord) {
  // clang-format off
  constexpr uint8_t kInvalidMessage[] = {
      0x00, 0x00,  // ID = 0
      0x00, 0x00,  // FLAGS = 0
      0x00, 0x00,  // Questions = 0
      0x00, 0x00,  // Answers = 0
      0x00, 0x00,  // Authority = 0
      0x00, 0x02,  // Additional = 2
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x0c,              // TYPE = PTR (12)
      0x00, 0x01,              // CLASS = IN (1)
      0x00, 0x00, 0x00, 0x78,  // TTL = 120 seconds
      0x00, 0x00,              // RDLENGTH = 0
      // NOTE: Only 1 answer record is given.
  };
  // clang-format on
  TestReadEntryFails<MdnsMessage>(kInvalidMessage, sizeof(kInvalidMessage));
}

}  // namespace discovery
}  // namespace openscreen
