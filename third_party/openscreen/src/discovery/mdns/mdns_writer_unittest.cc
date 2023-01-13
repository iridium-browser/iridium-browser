// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_writer.h"

#include <memory>
#include <vector>

#include "discovery/mdns/testing/mdns_test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace discovery {

using testing::ElementsAreArray;

namespace {

constexpr std::chrono::seconds kTtl{120};

template <class T>
void TestWriteEntrySucceeds(const T& entry,
                            const uint8_t* expected_data,
                            size_t expected_size) {
  std::vector<uint8_t> buffer(expected_size);
  MdnsWriter writer(buffer.data(), buffer.size());
  EXPECT_TRUE(writer.Write(entry));
  EXPECT_EQ(writer.remaining(), UINT64_C(0));
  EXPECT_THAT(buffer, ElementsAreArray(expected_data, expected_size));
}

template <class T>
void TestWriteEntryInsufficientBuffer(const T& entry) {
  std::vector<uint8_t> buffer(entry.MaxWireSize() - 1);
  MdnsWriter writer(buffer.data(), buffer.size());
  EXPECT_FALSE(writer.Write(entry));
  // There should be no side effects for failing to write an entry. The
  // underlying pointer should not have changed.
  EXPECT_EQ(writer.offset(), UINT64_C(0));
}

}  // namespace

TEST(MdnsWriterTest, WriteDomainName) {
  // clang-format off
  constexpr uint8_t kExpectedResult[] = {
    0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
    0x05, 'l', 'o', 'c', 'a', 'l',
    0x00
  };
  // clang-format on
  uint8_t result[sizeof(kExpectedResult)];
  MdnsWriter writer(result, sizeof(kExpectedResult));
  ASSERT_TRUE(writer.Write(DomainName{"testing", "local"}));
  EXPECT_EQ(0UL, writer.remaining());
  EXPECT_EQ(0, memcmp(kExpectedResult, result, sizeof(result)));
}

TEST(MdnsWriterTest, WriteDomainName_CompressedMessage) {
  // clang-format off
  constexpr uint8_t kExpectedResultCompressed[] = {
    0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
    0x05, 'l', 'o', 'c', 'a', 'l',
    0x00,
    0x06, 'p', 'r', 'e', 'f', 'i', 'x',
    0xC0, 0x08,  // byte 8
    0x03, 'n', 'e', 'w',
    0xC0, 0x0F,  // byte 15
    0xC0, 0x0F,  // byte 15
  };
  // clang-format on
  uint8_t result[sizeof(kExpectedResultCompressed)];
  MdnsWriter writer(result, sizeof(kExpectedResultCompressed));
  ASSERT_TRUE(writer.Write(DomainName{"testing", "local"}));
  ASSERT_TRUE(writer.Write(DomainName{"prefix", "local"}));
  ASSERT_TRUE(writer.Write(DomainName{"new", "prefix", "local"}));
  ASSERT_TRUE(writer.Write(DomainName{"prefix", "local"}));
  EXPECT_EQ(0UL, writer.remaining());
  EXPECT_THAT(std::vector<uint8_t>(result, result + sizeof(result)),
              ElementsAreArray(kExpectedResultCompressed));
}

TEST(MdnsWriterTest, WriteDomainName_NotEnoughSpace) {
  // clang-format off
  constexpr uint8_t kExpectedResultCompressed[] = {
    0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
    0x05, 'l', 'o', 'c', 'a', 'l',
    0x00,
    0x09, 'd', 'i', 'f', 'f', 'e', 'r', 'e', 'n', 't',
    0x06, 'd', 'o', 'm', 'a', 'i', 'n',
    0x00
  };
  // clang-format on
  uint8_t result[sizeof(kExpectedResultCompressed)];
  MdnsWriter writer(result, sizeof(kExpectedResultCompressed));
  ASSERT_TRUE(writer.Write(DomainName{"testing", "local"}));
  // Not enough space to write this domain name. Failure to write it must not
  // affect correct successful write of the next domain name.
  ASSERT_FALSE(writer.Write(DomainName{"a", "different", "domain"}));
  ASSERT_TRUE(writer.Write(DomainName{"different", "domain"}));
  EXPECT_EQ(0UL, writer.remaining());
  EXPECT_THAT(std::vector<uint8_t>(result, result + sizeof(result)),
              ElementsAreArray(kExpectedResultCompressed));
}

TEST(MdnsWriterTest, WriteDomainName_Long) {
  constexpr char kLongLabel[] =
      "12345678901234567890123456789012345678901234567890";
  // clang-format off
  constexpr uint8_t kExpectedResult[] = {
      0x32, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3',
            '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6',
            '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
      0x32, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3',
            '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6',
            '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
      0x32, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3',
            '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6',
            '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
      0x32, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3',
            '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6',
            '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
      0x00,
  };
  // clang-format on
  DomainName name{kLongLabel, kLongLabel, kLongLabel, kLongLabel};
  uint8_t result[sizeof(kExpectedResult)];
  MdnsWriter writer(result, sizeof(kExpectedResult));
  ASSERT_TRUE(writer.Write(name));
  EXPECT_EQ(0UL, writer.remaining());
  EXPECT_EQ(0, memcmp(kExpectedResult, result, sizeof(result)));
}

TEST(MdnsWriterTest, WriteDomainName_Empty) {
  DomainName name;
  uint8_t result[256];
  MdnsWriter writer(result, sizeof(result));
  EXPECT_FALSE(writer.Write(name));
  // The writer should not have moved its internal pointer when it fails to
  // write. It should fail without any side effects.
  EXPECT_EQ(0u, writer.offset());
}

TEST(MdnsWriterTest, WriteDomainName_NoCompressionForBigOffsets) {
  // clang-format off
  constexpr uint8_t kExpectedResultCompressed[] = {
    0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
    0x05, 'l', 'o', 'c', 'a', 'l',
    0x00,
    0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
    0x05, 'l', 'o', 'c', 'a', 'l',
    0x00,
  };
  // clang-format on

  DomainName name{"testing", "local"};
  // Maximum supported value for label pointer offset is 0x3FFF.
  // Labels written into a buffer at greater offsets must not
  // produce compression label pointers.
  std::vector<uint8_t> buffer(0x4000 + sizeof(kExpectedResultCompressed));
  {
    MdnsWriter writer(buffer.data(), buffer.size());
    writer.Skip(0x4000);
    ASSERT_TRUE(writer.Write(name));
    ASSERT_TRUE(writer.Write(name));
    EXPECT_EQ(0UL, writer.remaining());
  }
  buffer.erase(buffer.begin(), buffer.begin() + 0x4000);
  EXPECT_THAT(buffer, ElementsAreArray(kExpectedResultCompressed));
}

TEST(MdnsWriterTest, WriteRawRecordRdata) {
  // clang-format off
  constexpr uint8_t kExpectedRdata[] = {
      0x00, 0x08,  // RDLENGTH = 8 bytes
      0x05, 'c', 'n', 'a', 'm', 'e', 0xc0, 0x00,
  };
  // clang-format on
  TestWriteEntrySucceeds(
      RawRecordRdata(kExpectedRdata + 2, sizeof(kExpectedRdata) - 2),
      kExpectedRdata, sizeof(kExpectedRdata));
}

TEST(MdnsWriterTest, WriteRawRecordRdata_InsufficientBuffer) {
  // clang-format off
  constexpr uint8_t kRawRdata[] = {
      0x05, 'c', 'n', 'a', 'm', 'e', 0xc0, 0x00,
  };
  // clang-format on
  TestWriteEntryInsufficientBuffer(
      RawRecordRdata(kRawRdata, sizeof(kRawRdata)));
}

TEST(MdnsWriterTest, WriteSrvRecordRdata) {
  constexpr uint8_t kExpectedRdata[] = {
      0x00, 0x15,  // RDLENGTH = 21
      0x00, 0x05,  // PRIORITY = 5
      0x00, 0x06,  // WEIGHT = 6
      0x1f, 0x49,  // PORT = 8009
      0x07, 't',  'e', 's', 't', 'i', 'n',  'g',
      0x05, 'l',  'o', 'c', 'a', 'l', 0x00,
  };
  TestWriteEntrySucceeds(
      SrvRecordRdata(5, 6, 8009, DomainName{"testing", "local"}),
      kExpectedRdata, sizeof(kExpectedRdata));
}

TEST(MdnsWriterTest, WriteSrvRecordRdata_InsufficientBuffer) {
  TestWriteEntryInsufficientBuffer(
      SrvRecordRdata(5, 6, 8009, DomainName{"testing", "local"}));
}

TEST(MdnsWriterTest, WriteARecordRdata) {
  constexpr uint8_t kExpectedRdata[] = {
      0x00, 0x4,               // RDLENGTH = 4
      0x08, 0x08, 0x08, 0x08,  // ADDRESS = 8.8.8.8
  };
  TestWriteEntrySucceeds(ARecordRdata(IPAddress{8, 8, 8, 8}), kExpectedRdata,
                         sizeof(kExpectedRdata));
}

TEST(MdnsWriterTest, WriteARecordRdata_InsufficientBuffer) {
  TestWriteEntryInsufficientBuffer(ARecordRdata(IPAddress{8, 8, 8, 8}));
}

TEST(MdnsWriterTest, WriteAAAARecordRdata) {
  // clang-format off
  constexpr uint8_t kExpectedRdata[] = {
      0x00, 0x10,  // RDLENGTH = 16
      // ADDRESS = FE80:0000:0000:0000:0202:B3FF:FE1E:8329
      0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x02, 0x02, 0xb3, 0xff, 0xfe, 0x1e, 0x83, 0x29,
  };
  // clang-format on
  TestWriteEntrySucceeds(
      AAAARecordRdata(IPAddress(IPAddress::Version::kV6, kExpectedRdata + 2)),
      kExpectedRdata, sizeof(kExpectedRdata));
}

TEST(MdnsWriterTest, WriteAAAARecordRdata_InsufficientBuffer) {
  // clang-format off
  constexpr uint16_t kAAAARdata[] = {
      // ADDRESS = FE80:0000:0000:0000:0202:B3FF:FE1E:8329
      0xfe80, 0x0000, 0x0000, 0x0000,
      0x0202, 0xb3ff, 0xfe1e, 0x8329,
  };
  // clang-format on
  TestWriteEntryInsufficientBuffer(AAAARecordRdata(IPAddress(kAAAARdata)));
}

TEST(MdnsWriterTest, WriteNSECRecordRdata) {
  const DomainName domain{"testing", "local"};
  NsecRecordRdata(DomainName{"mydevice", "testing", "local"}, DnsType::kA,
                  DnsType::kTXT, DnsType::kSRV, DnsType::kNSEC);

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
  TestWriteEntrySucceeds(
      NsecRecordRdata(DomainName{"mydevice", "testing", "local"}, DnsType::kA,
                      DnsType::kTXT, DnsType::kSRV, DnsType::kNSEC),
      kExpectedRdata, sizeof(kExpectedRdata));
}

TEST(MdnsWriterTest, WriteNSECRecordRdata_InsufficientBuffer) {
  TestWriteEntryInsufficientBuffer(
      NsecRecordRdata(DomainName{"mydevice", "testing", "local"}, DnsType::kA,
                      DnsType::kTXT, DnsType::kSRV, DnsType::kNSEC));
}

TEST(MdnsWriterTest, WritePtrRecordRdata) {
  // clang-format off
  constexpr uint8_t kExpectedRdata[] = {
      0x00, 0x18,  // RDLENGTH = 24
      0x08, 'm', 'y', 'd', 'e', 'v', 'i', 'c', 'e',
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a',  'l',
      0x00,
  };
  // clang-format on
  TestWriteEntrySucceeds(
      PtrRecordRdata(DomainName{"mydevice", "testing", "local"}),
      kExpectedRdata, sizeof(kExpectedRdata));
}

TEST(MdnsWriterTest, WritePtrRecordRdata_InsufficientBuffer) {
  TestWriteEntryInsufficientBuffer(
      PtrRecordRdata(DomainName{"mydevice", "testing", "local"}));
}

TEST(MdnsWriterTest, WriteTxtRecordRdata) {
  // clang-format off
  constexpr uint8_t kExpectedRdata[] = {
      0x00, 0x0C,  // RDLENGTH = 12
      0x05, 'f', 'o', 'o', '=', '1',
      0x05, 'b', 'a', 'r', '=', '2',
  };
  // clang-format on
  TestWriteEntrySucceeds(MakeTxtRecord({"foo=1", "bar=2"}), kExpectedRdata,
                         sizeof(kExpectedRdata));
}

TEST(MdnsWriterTest, WriteTxtRecordRdata_Empty) {
  constexpr uint8_t kExpectedRdata[] = {
      0x00, 0x01,  // RDLENGTH = 1
      0x00,        // empty string
  };
  TestWriteEntrySucceeds(TxtRecordRdata(), kExpectedRdata,
                         sizeof(kExpectedRdata));
}

TEST(MdnsWriterTest, WriteTxtRecordRdata_InsufficientBuffer) {
  TestWriteEntryInsufficientBuffer(MakeTxtRecord({"foo=1", "bar=2"}));
}

TEST(MdnsWriterTest, WriteMdnsRecord_ARecordRdata) {
  // clang-format off
  constexpr uint8_t kExpectedResult[] = {
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x01,              // TYPE = A (1)
      0x80, 0x01,              // CLASS = IN (1) | CACHE_FLUSH_BIT
      0x00, 0x00, 0x00, 0x78,  // TTL = 120 seconds
      0x00, 0x04,              // RDLENGTH = 4 bytes
      0xac, 0x00, 0x00, 0x01,  // 172.0.0.1
  };
  // clang-format on
  TestWriteEntrySucceeds(MdnsRecord(DomainName{"testing", "local"}, DnsType::kA,
                                    DnsClass::kIN, RecordType::kUnique, kTtl,
                                    ARecordRdata(IPAddress{172, 0, 0, 1})),
                         kExpectedResult, sizeof(kExpectedResult));
}

TEST(MdnsWriterTest, WriteMdnsRecord_PtrRecordRdata) {
  // clang-format off
  constexpr uint8_t kExpectedResult[] = {
      0x08, '_', 's', 'e', 'r', 'v', 'i', 'c', 'e',
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x0c,              // TYPE = PTR (12)
      0x00, 0x01,              // CLASS = IN (1)
      0x00, 0x00, 0x00, 0x78,  // TTL = 120 seconds
      0x00, 0x02,              // RDLENGTH = 2 bytes
      0xc0, 0x09,              // Domain name label pointer to byte
  };
  // clang-format on
  TestWriteEntrySucceeds(
      MdnsRecord(DomainName{"_service", "testing", "local"}, DnsType::kPTR,
                 DnsClass::kIN, RecordType::kShared, kTtl,
                 PtrRecordRdata(DomainName{"testing", "local"})),
      kExpectedResult, sizeof(kExpectedResult));
}

TEST(MdnsWriterTest, WriteMdnsRecord_InsufficientBuffer) {
  TestWriteEntryInsufficientBuffer(MdnsRecord(
      DomainName{"testing", "local"}, DnsType::kA, DnsClass::kIN,
      RecordType::kUnique, kTtl, ARecordRdata(IPAddress{172, 0, 0, 1})));
}

TEST(MdnsWriterTest, WriteMdnsQuestion) {
  // clang-format off
  constexpr uint8_t kExpectedResult[] = {
      0x04, 'w', 'i', 'r', 'e',
      0x06, 'f', 'o', 'r', 'm', 'a', 't',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x0c,  // TYPE = PTR (12)
      0x80, 0x01,  // CLASS = IN (1) | UNICAST_BIT
  };
  // clang-format on
  TestWriteEntrySucceeds(
      MdnsQuestion(DomainName{"wire", "format", "local"}, DnsType::kPTR,
                   DnsClass::kIN, ResponseType::kUnicast),
      kExpectedResult, sizeof(kExpectedResult));
}

TEST(MdnsWriterTest, WriteMdnsQuestion_InsufficientBuffer) {
  TestWriteEntryInsufficientBuffer(
      MdnsQuestion(DomainName{"wire", "format", "local"}, DnsType::kPTR,
                   DnsClass::kIN, ResponseType::kUnicast));
}

TEST(MdnsWriterTest, WriteMdnsMessage) {
  // clang-format off
  constexpr uint8_t kExpectedMessage[] = {
      0x00, 0x01,  // ID = 1
      0x00, 0x00,  // FLAGS = None
      0x00, 0x01,  // Question count
      0x00, 0x00,  // Answer count
      0x00, 0x01,  // Authority count
      0x00, 0x00,  // Additional count
      // Question
      0x08, 'q', 'u', 'e', 's', 't', 'i', 'o', 'n',
      0x00,
      0x00, 0x0c,  // TYPE = PTR (12)
      0x00, 0x01,  // CLASS = IN (1)
      // Authority Record
      0x04, 'a', 'u', 't', 'h',
      0x00,
      0x00, 0x10,              // TYPE = TXT (16)
      0x00, 0x01,              // CLASS = IN (1)
      0x00, 0x00, 0x00, 0x78,  // TTL = 120 seconds
      0x00, 0x0c,              // RDLENGTH = 12 bytes
      0x05, 'f', 'o', 'o', '=', '1',
      0x05, 'b', 'a', 'r', '=', '2',
  };
  // clang-format on
  MdnsQuestion question(DomainName{"question"}, DnsType::kPTR, DnsClass::kIN,
                        ResponseType::kMulticast);

  MdnsRecord auth_record(DomainName{"auth"}, DnsType::kTXT, DnsClass::kIN,
                         RecordType::kShared, kTtl,
                         MakeTxtRecord({"foo=1", "bar=2"}));

  MdnsMessage message(1, MessageType::Query);
  message.AddQuestion(question);
  message.AddAuthorityRecord(auth_record);

  std::vector<uint8_t> buffer(sizeof(kExpectedMessage));
  MdnsWriter writer(buffer.data(), buffer.size());
  EXPECT_TRUE(writer.Write(message));
  EXPECT_EQ(writer.remaining(), UINT64_C(0));
  EXPECT_THAT(buffer, ElementsAreArray(kExpectedMessage));
}

TEST(MdnsWriterTest, WriteMdnsMessage_InsufficientBuffer) {
  MdnsQuestion question(DomainName{"question"}, DnsType::kPTR, DnsClass::kIN,
                        ResponseType::kMulticast);

  MdnsRecord auth_record(DomainName{"auth"}, DnsType::kTXT, DnsClass::kIN,
                         RecordType::kShared, kTtl,
                         MakeTxtRecord({"foo=1", "bar=2"}));

  MdnsMessage message(1, MessageType::Query);
  message.AddQuestion(question);
  message.AddAuthorityRecord(auth_record);
  TestWriteEntryInsufficientBuffer(message);
}

}  // namespace discovery
}  // namespace openscreen
