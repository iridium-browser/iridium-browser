// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_records.h"

#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "discovery/mdns/mdns_reader.h"
#include "discovery/mdns/mdns_writer.h"
#include "discovery/mdns/testing/hash_test_util.h"
#include "discovery/mdns/testing/mdns_test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/api/network_interface.h"

namespace openscreen {
namespace discovery {

using testing::ElementsAreArray;

namespace {

constexpr std::chrono::seconds kTtl{120};

template <class T>
void TestCopyAndMove(const T& value) {
  T value_copy_constuct(value);
  EXPECT_EQ(value_copy_constuct, value);
  T value_copy_assign = value;
  EXPECT_EQ(value_copy_assign, value);
  T value_move_constuct(std::move(value_copy_constuct));
  EXPECT_EQ(value_move_constuct, value);
  T value_move_assign = std::move(value_copy_assign);
  EXPECT_EQ(value_move_assign, value);
}

}  // namespace

TEST(MdnsDomainNameTest, Construct) {
  DomainName name1;
  EXPECT_TRUE(name1.empty());
  EXPECT_EQ(name1.MaxWireSize(), UINT64_C(1));
  EXPECT_EQ(name1.labels().size(), UINT64_C(0));

  DomainName name2{"MyDevice", "_mYSERvice", "local"};
  EXPECT_FALSE(name2.empty());
  EXPECT_EQ(name2.MaxWireSize(), UINT64_C(27));
  ASSERT_EQ(name2.labels().size(), UINT64_C(3));
  EXPECT_EQ(name2.labels()[0], "MyDevice");
  EXPECT_EQ(name2.labels()[1], "_mYSERvice");
  EXPECT_EQ(name2.labels()[2], "local");
  std::stringstream name2_stream;
  name2_stream << name2;
  EXPECT_EQ(name2_stream.str(), "MyDevice._mYSERvice.local");

  std::vector<absl::string_view> labels{"OtherDevice", "_MYservice", "LOcal"};
  DomainName name3(labels);
  EXPECT_FALSE(name3.empty());
  EXPECT_EQ(name3.MaxWireSize(), UINT64_C(30));
  ASSERT_EQ(name3.labels().size(), UINT64_C(3));
  EXPECT_EQ(name3.labels()[0], "OtherDevice");
  EXPECT_EQ(name3.labels()[1], "_MYservice");
  EXPECT_EQ(name3.labels()[2], "LOcal");
  std::stringstream name3_stream;
  name3_stream << name3;
  EXPECT_EQ(name3_stream.str(), "OtherDevice._MYservice.LOcal");
}

TEST(MdnsDomainNameTest, Compare) {
  DomainName first{"testing", "local"};
  DomainName second{"TeStInG", "LOCAL"};
  DomainName third{"testing"};
  DomainName fourth{"testing.local"};
  DomainName fifth{"Testing.Local"};

  EXPECT_EQ(first, second);
  EXPECT_TRUE(first >= second);
  EXPECT_TRUE(second >= first);
  EXPECT_TRUE(first <= second);
  EXPECT_TRUE(second <= first);
  EXPECT_EQ(fourth, fifth);
  EXPECT_NE(first, third);
  EXPECT_NE(first, fourth);

  EXPECT_FALSE(first < second);
  EXPECT_FALSE(second < first);
  EXPECT_FALSE(first < third);
  EXPECT_TRUE(third < first);
  EXPECT_TRUE(third <= first);
  EXPECT_FALSE(third > first);
  EXPECT_TRUE(first < fourth);
  EXPECT_TRUE(fourth > first);
  EXPECT_TRUE(fourth >= first);
  EXPECT_TRUE(first < fifth);
  EXPECT_FALSE(fifth < first);

  EXPECT_FALSE(second < third);
  EXPECT_TRUE(third < second);
  EXPECT_TRUE(second < fourth);
  EXPECT_FALSE(fourth < second);
  EXPECT_TRUE(second < fifth);
  EXPECT_FALSE(fifth < second);

  EXPECT_TRUE(third < fourth);
  EXPECT_FALSE(fourth < third);
  EXPECT_TRUE(third < fifth);
  EXPECT_FALSE(fifth < third);

  EXPECT_FALSE(fourth < fifth);
  EXPECT_FALSE(fifth < fourth);

  EXPECT_TRUE(VerifyTypeImplementsAbslHashCorrectly(
      {first, second, third, fourth, fifth}));
}

TEST(MdnsDomainNameTest, CopyAndMove) {
  TestCopyAndMove(DomainName{"testing", "local"});
}

TEST(MdnsRawRecordRdataTest, Construct) {
  constexpr uint8_t kRawRdata[] = {
      0x05, 'c', 'n', 'a', 'm', 'e', 0xc0, 0x00,
  };

  RawRecordRdata rdata1;
  EXPECT_EQ(rdata1.MaxWireSize(), UINT64_C(2));
  EXPECT_EQ(rdata1.size(), UINT16_C(0));

  RawRecordRdata rdata2(kRawRdata, sizeof(kRawRdata));
  EXPECT_EQ(rdata2.MaxWireSize(), UINT64_C(10));
  EXPECT_EQ(rdata2.size(), UINT16_C(8));
  EXPECT_THAT(
      std::vector<uint8_t>(rdata2.data(), rdata2.data() + rdata2.size()),
      ElementsAreArray(kRawRdata));

  RawRecordRdata rdata3(
      std::vector<uint8_t>(kRawRdata, kRawRdata + sizeof(kRawRdata)));
  EXPECT_EQ(rdata3.MaxWireSize(), UINT64_C(10));
  EXPECT_EQ(rdata3.size(), UINT16_C(8));
  EXPECT_THAT(
      std::vector<uint8_t>(rdata3.data(), rdata3.data() + rdata3.size()),
      ElementsAreArray(kRawRdata));
}

TEST(MdnsRawRecordRdataTest, Compare) {
  constexpr uint8_t kRawRdata1[] = {
      0x05, 'c', 'n', 'a', 'm', 'e', 0xc0, 0x00,
  };
  constexpr uint8_t kRawRdata2[] = {
      0x05, 'r', 'd', 'a', 't', 'a',
  };

  RawRecordRdata rdata1(kRawRdata1, sizeof(kRawRdata1));
  RawRecordRdata rdata2(kRawRdata1, sizeof(kRawRdata1));
  RawRecordRdata rdata3(kRawRdata2, sizeof(kRawRdata2));

  EXPECT_EQ(rdata1, rdata2);
  EXPECT_NE(rdata1, rdata3);

  EXPECT_TRUE(VerifyTypeImplementsAbslHashCorrectly({rdata1, rdata2, rdata3}));
}

TEST(MdnsRawRecordRdataTest, CopyAndMove) {
  constexpr uint8_t kRawRdata[] = {
      0x05, 'c', 'n', 'a', 'm', 'e', 0xc0, 0x00,
  };
  TestCopyAndMove(RawRecordRdata(kRawRdata, sizeof(kRawRdata)));
}

TEST(MdnsSrvRecordRdataTest, Construct) {
  SrvRecordRdata rdata1;
  EXPECT_EQ(rdata1.MaxWireSize(), UINT64_C(9));
  EXPECT_EQ(rdata1.priority(), UINT16_C(0));
  EXPECT_EQ(rdata1.weight(), UINT16_C(0));
  EXPECT_EQ(rdata1.port(), UINT16_C(0));
  EXPECT_EQ(rdata1.target(), DomainName());

  SrvRecordRdata rdata2(1, 2, 3, DomainName{"testing", "local"});
  EXPECT_EQ(rdata2.MaxWireSize(), UINT64_C(23));
  EXPECT_EQ(rdata2.priority(), UINT16_C(1));
  EXPECT_EQ(rdata2.weight(), UINT16_C(2));
  EXPECT_EQ(rdata2.port(), UINT16_C(3));
  EXPECT_EQ(rdata2.target(), (DomainName{"testing", "local"}));
}

TEST(MdnsSrvRecordRdataTest, Compare) {
  SrvRecordRdata rdata1(1, 2, 3, DomainName{"testing", "local"});
  SrvRecordRdata rdata2(1, 2, 3, DomainName{"testing", "local"});
  SrvRecordRdata rdata3(4, 2, 3, DomainName{"testing", "local"});
  SrvRecordRdata rdata4(1, 5, 3, DomainName{"testing", "local"});
  SrvRecordRdata rdata5(1, 2, 6, DomainName{"testing", "local"});
  SrvRecordRdata rdata6(1, 2, 3, DomainName{"device", "local"});

  EXPECT_EQ(rdata1, rdata2);
  EXPECT_NE(rdata1, rdata3);
  EXPECT_NE(rdata1, rdata4);
  EXPECT_NE(rdata1, rdata5);
  EXPECT_NE(rdata1, rdata6);

  EXPECT_TRUE(VerifyTypeImplementsAbslHashCorrectly(
      {rdata1, rdata2, rdata3, rdata4, rdata5, rdata6}));
}

TEST(MdnsSrvRecordRdataTest, CopyAndMove) {
  TestCopyAndMove(SrvRecordRdata(1, 2, 3, DomainName{"testing", "local"}));
}

TEST(MdnsARecordRdataTest, Construct) {
  ARecordRdata rdata1;
  EXPECT_EQ(rdata1.MaxWireSize(), UINT64_C(6));
  EXPECT_EQ(rdata1.ipv4_address(), (IPAddress{0, 0, 0, 0}));

  ARecordRdata rdata2(IPAddress{8, 8, 8, 8});
  EXPECT_EQ(rdata2.MaxWireSize(), UINT64_C(6));
  EXPECT_EQ(rdata2.ipv4_address(), (IPAddress{8, 8, 8, 8}));
}

TEST(MdnsARecordRdataTest, Compare) {
  ARecordRdata rdata1(IPAddress{8, 8, 8, 8});
  ARecordRdata rdata2(IPAddress{8, 8, 8, 8});
  ARecordRdata rdata3(IPAddress{1, 2, 3, 4});

  EXPECT_EQ(rdata1, rdata2);
  EXPECT_NE(rdata1, rdata3);

  EXPECT_TRUE(VerifyTypeImplementsAbslHashCorrectly({rdata1, rdata2, rdata3}));
}

TEST(MdnsARecordRdataTest, CopyAndMove) {
  TestCopyAndMove(ARecordRdata(IPAddress{8, 8, 8, 8}));
}

TEST(MdnsAAAARecordRdataTest, Construct) {
  constexpr uint16_t kIPv6AddressHextets1[] = {
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  };
  constexpr uint16_t kIPv6AddressHextets2[] = {
      0xfe80, 0x0000, 0x0000, 0x0000, 0x0202, 0xb3ff, 0xfe1e, 0x8329,
  };

  IPAddress address1(kIPv6AddressHextets1);
  AAAARecordRdata rdata1;
  EXPECT_EQ(rdata1.MaxWireSize(), UINT64_C(18));
  EXPECT_EQ(rdata1.ipv6_address(), address1);

  IPAddress address2(kIPv6AddressHextets2);
  AAAARecordRdata rdata2(address2);
  EXPECT_EQ(rdata2.MaxWireSize(), UINT64_C(18));
  EXPECT_EQ(rdata2.ipv6_address(), address2);
}

TEST(MdnsAAAARecordRdataTest, Compare) {
  constexpr uint16_t kIPv6AddressHextets1[] = {
      0x0001, 0x0203, 0x0405, 0x0607, 0x0809, 0x0A0B, 0x0C0D, 0x0E0F,
  };
  constexpr uint16_t kIPv6AddressHextets2[] = {
      0xfe80, 0x0000, 0x0000, 0x0000, 0x0202, 0xb3ff, 0xfe1e, 0x8329,
  };

  IPAddress address1(kIPv6AddressHextets1);
  IPAddress address2(kIPv6AddressHextets2);
  AAAARecordRdata rdata1(address1);
  AAAARecordRdata rdata2(address1);
  AAAARecordRdata rdata3(address2);

  EXPECT_EQ(rdata1, rdata2);
  EXPECT_NE(rdata1, rdata3);

  EXPECT_TRUE(VerifyTypeImplementsAbslHashCorrectly({rdata1, rdata2, rdata3}));
}

TEST(MdnsAAAARecordRdataTest, CopyAndMove) {
  constexpr uint16_t kIPv6AddressHextets[] = {
      0xfe80, 0x0000, 0x0000, 0x0000, 0x0202, 0xb3ff, 0xfe1e, 0x8329,
  };
  TestCopyAndMove(AAAARecordRdata(IPAddress(kIPv6AddressHextets)));
}

TEST(MdnsPtrRecordRdataTest, Construct) {
  PtrRecordRdata rdata1;
  EXPECT_EQ(rdata1.MaxWireSize(), UINT64_C(3));
  EXPECT_EQ(rdata1.ptr_domain(), DomainName());

  PtrRecordRdata rdata2(DomainName{"testing", "local"});
  EXPECT_EQ(rdata2.MaxWireSize(), UINT64_C(17));
  EXPECT_EQ(rdata2.ptr_domain(), (DomainName{"testing", "local"}));
}

TEST(MdnsPtrRecordRdataTest, Compare) {
  PtrRecordRdata rdata1(DomainName{"testing", "local"});
  PtrRecordRdata rdata2(DomainName{"testing", "local"});
  PtrRecordRdata rdata3(DomainName{"device", "local"});

  EXPECT_EQ(rdata1, rdata2);
  EXPECT_NE(rdata1, rdata3);

  EXPECT_TRUE(VerifyTypeImplementsAbslHashCorrectly({rdata1, rdata2, rdata3}));
}

TEST(MdnsPtrRecordRdataTest, CopyAndMove) {
  TestCopyAndMove(PtrRecordRdata(DomainName{"testing", "local"}));
}

TEST(MdnsTxtRecordRdataTest, Construct) {
  TxtRecordRdata rdata1;
  EXPECT_EQ(rdata1.MaxWireSize(), UINT64_C(3));
  EXPECT_EQ(rdata1.texts(), std::vector<std::string>());

  TxtRecordRdata rdata2 = MakeTxtRecord({"foo=1", "bar=2"});
  EXPECT_EQ(rdata2.MaxWireSize(), UINT64_C(14));
  EXPECT_EQ(rdata2.texts(), (std::vector<std::string>{"foo=1", "bar=2"}));
}

TEST(MdnsTxtRecordRdataTest, Compare) {
  TxtRecordRdata rdata1 = MakeTxtRecord({"foo=1", "bar=2"});
  TxtRecordRdata rdata2 = MakeTxtRecord({"foo=1", "bar=2"});
  TxtRecordRdata rdata3 = MakeTxtRecord({"foo=1"});
  TxtRecordRdata rdata4 = MakeTxtRecord({"E=mc^2", "F=ma"});

  EXPECT_EQ(rdata1, rdata2);
  EXPECT_NE(rdata1, rdata3);
  EXPECT_NE(rdata1, rdata4);

  EXPECT_TRUE(
      VerifyTypeImplementsAbslHashCorrectly({rdata1, rdata2, rdata3, rdata4}));
}

TEST(MdnsTxtRecordRdataTest, CopyAndMove) {
  TestCopyAndMove(MakeTxtRecord({"foo=1", "bar=2"}));
}

TEST(MdnsNsecRecordRdataTest, Construct) {
  const DomainName domain{"testing", "local"};
  NsecRecordRdata rdata(domain);
  EXPECT_EQ(rdata.MaxWireSize(), domain.MaxWireSize());
  EXPECT_EQ(rdata.next_domain_name(), domain);

  rdata = NsecRecordRdata(domain, DnsType::kA);
  // It takes 3 bytes to encode the kA and kSRV records because:
  // - Both record types have value less than 256, so they are both in window
  //   block 1.
  // - The bitmap length for this block is always a single byte
  // - DnsType kA = 1 (encoded in byte 1)
  // So the full encoded version is:
  //   00000000 00000001 01000000
  //   |window| | size | | 0-7  |
  // For a total of 3 bytes.
  EXPECT_EQ(rdata.encoded_types(), (std::vector<uint8_t>{0x00, 0x01, 0x40}));
  EXPECT_EQ(rdata.MaxWireSize(), domain.MaxWireSize() + 3);
  EXPECT_EQ(rdata.next_domain_name(), domain);

  // It takes 8 bytes to encode the kA and kSRV records because:
  // - Both record types have value less than 256, so they are both in window
  //   block 1.
  // - The bitmap length for this block is always a single byte
  // - DnsTypes  kTXT = 16 (encoded in byte 3)
  // So the full encoded version is:
  //   00000000 00000011 00000000 00000000 10000000
  //   |window| | size | | 0-7  | | 8-15 | |16-23 |
  // For a total of 5 bytes.
  rdata = NsecRecordRdata(domain, DnsType::kTXT);
  EXPECT_EQ(rdata.encoded_types(),
            (std::vector<uint8_t>{0x00, 0x03, 0x00, 0x00, 0x80}));
  EXPECT_EQ(rdata.MaxWireSize(), domain.MaxWireSize() + 5);
  EXPECT_EQ(rdata.next_domain_name(), domain);

  // It takes 8 bytes to encode the kA and kSRV records because:
  // - Both record types have value less than 256, so they are both in window
  //   block 1.
  // - The bitmap length for this block is always a single byte
  // - DnsTypes kSRV = 33 (encoded in byte 5)
  // So the full encoded version is:
  //   00000000 00000101 00000000 00000000 00000000 00000000 01000000
  //   |window| | size | | 0-7  | | 8-15 | |16-23 | |24-31 | |32-39 |
  // For a total of 7 bytes.
  rdata = NsecRecordRdata(domain, DnsType::kSRV);
  EXPECT_EQ(rdata.encoded_types(),
            (std::vector<uint8_t>{0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x40}));
  EXPECT_EQ(rdata.MaxWireSize(), domain.MaxWireSize() + 7);
  EXPECT_EQ(rdata.next_domain_name(), domain);

  // It takes 8 bytes to encode the kA and kSRV records because:
  // - Both record types have value less than 256, so they are both in window
  //   block 1.
  // - The bitmap length for this block is always a single byte
  // - DnsTypes kNSEC = 47
  // So the full encoded version is:
  //   00000000 00000110 00000000 00000000 00000000 00000000 0000000  00000001
  //   |window| | size | | 0-7  | | 8-15 | |16-23 | |24-31 | |32-39 | |40-47 |
  // For a total of 8 bytes.
  rdata = NsecRecordRdata(domain, DnsType::kNSEC);
  EXPECT_EQ(
      rdata.encoded_types(),
      (std::vector<uint8_t>{0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}));
  EXPECT_EQ(rdata.MaxWireSize(), domain.MaxWireSize() + 8);
  EXPECT_EQ(rdata.next_domain_name(), domain);

  // It takes 8 bytes to encode the kA and kSRV records because:
  // - Both record types have value less than 256, so they are both in window
  //   block 1.
  // - The bitmap length for this block is always a single byte
  // - DnsTypes kNSEC = 255
  // So 32 bits are required for the bitfield, for a total of 34 bits.
  rdata = NsecRecordRdata(domain, DnsType::kANY);
  std::vector<uint8_t> results{0x00, 32};
  for (int i = 1; i < 32; i++) {
    results.push_back(0x00);
  }
  results.push_back(0x01);
  EXPECT_EQ(rdata.encoded_types(), results);
  EXPECT_EQ(rdata.MaxWireSize(), domain.MaxWireSize() + 34);
  EXPECT_EQ(rdata.next_domain_name(), domain);

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
  // For a total of 8 bytes.
  rdata = NsecRecordRdata(domain, DnsType::kA, DnsType::kTXT, DnsType::kSRV,
                          DnsType::kNSEC);
  EXPECT_EQ(
      rdata.encoded_types(),
      (std::vector<uint8_t>{0x00, 0x06, 0x40, 0x00, 0x80, 0x00, 0x40, 0x01}));
  EXPECT_EQ(rdata.MaxWireSize(), domain.MaxWireSize() + 8);
  EXPECT_EQ(rdata.next_domain_name(), domain);
}

TEST(MdnsNsecRecordRdataTest, Compare) {
  const DomainName domain{"testing", "local"};
  const NsecRecordRdata rdata1(domain, DnsType::kA, DnsType::kSRV);
  const NsecRecordRdata rdata2(domain, DnsType::kSRV, DnsType::kA);
  const NsecRecordRdata rdata3(domain, DnsType::kSRV, DnsType::kA,
                               DnsType::kAAAA);
  const NsecRecordRdata rdata4(domain, DnsType::kSRV, DnsType::kAAAA);

  // Ensure equal Rdata values are evaluated as equal.
  EXPECT_EQ(rdata1, rdata1);
  EXPECT_EQ(rdata1, rdata2);
  EXPECT_EQ(rdata2, rdata1);

  // Ensure different Rdata values are not.
  EXPECT_NE(rdata1, rdata3);
  EXPECT_NE(rdata1, rdata4);
  EXPECT_NE(rdata3, rdata4);

  EXPECT_TRUE(
      VerifyTypeImplementsAbslHashCorrectly({rdata1, rdata2, rdata3, rdata4}));
}

TEST(MdnsNsecRecordRdataTest, CopyAndMove) {
  TestCopyAndMove(NsecRecordRdata(DomainName{"testing", "local"}, DnsType::kA,
                                  DnsType::kSRV));
}

TEST(MdnsOptRecordRdataTest, Construct) {
  OptRecordRdata rdata1;
  EXPECT_EQ(rdata1.MaxWireSize(), size_t{0});
  EXPECT_EQ(rdata1.options().size(), size_t{0});

  OptRecordRdata::Option opt1{12, 34, {0x12, 0x34}};
  OptRecordRdata::Option opt2{12, 34, {0x12, 0x34}};
  OptRecordRdata::Option opt3{12, 34, {0x12, 0x34, 0x56}};
  OptRecordRdata::Option opt4{34, 12, {0x00}};
  OptRecordRdata::Option opt5{12, 12, {0x12, 0x34}};
  rdata1 = OptRecordRdata(opt1, opt2, opt3, opt4, opt5);
  EXPECT_EQ(rdata1.MaxWireSize(), size_t{30});

  ASSERT_EQ(rdata1.options().size(), size_t{5});
  EXPECT_EQ(rdata1.options()[0], opt5);
  EXPECT_EQ(rdata1.options()[1], opt1);
  EXPECT_EQ(rdata1.options()[2], opt2);
  EXPECT_EQ(rdata1.options()[3], opt3);
  EXPECT_EQ(rdata1.options()[4], opt4);
}

TEST(MdnsOptRecordRdataTest, Compare) {
  OptRecordRdata::Option opt1{12, 34, {0x12, 0x34}};
  OptRecordRdata::Option opt2{12, 34, {0x12, 0x34}};
  OptRecordRdata::Option opt3{12, 34, {0x12, 0x56}};
  OptRecordRdata rdata1(opt1);
  OptRecordRdata rdata2(opt2);
  OptRecordRdata rdata3(opt3);
  OptRecordRdata rdata4;

  EXPECT_EQ(rdata1, rdata1);
  EXPECT_EQ(rdata2, rdata2);
  EXPECT_EQ(rdata3, rdata3);
  EXPECT_EQ(rdata4, rdata4);

  EXPECT_EQ(rdata1, rdata2);
  EXPECT_NE(rdata1, rdata3);
  EXPECT_NE(rdata1, rdata4);
  EXPECT_NE(rdata2, rdata3);
  EXPECT_NE(rdata2, rdata4);
  EXPECT_NE(rdata3, rdata4);

  EXPECT_TRUE(
      VerifyTypeImplementsAbslHashCorrectly({rdata1, rdata2, rdata3, rdata4}));
}

TEST(MdnsOptRecordRdataTest, CopyAndMove) {
  OptRecordRdata::Option opt1{12, 34, {0x12, 0x34}};
  TestCopyAndMove(OptRecordRdata(opt1));
}

TEST(MdnsRecordTest, Construct) {
  MdnsRecord record1;
  EXPECT_EQ(record1.MaxWireSize(), UINT64_C(11));
  EXPECT_EQ(record1.name(), DomainName());
  EXPECT_EQ(record1.dns_type(), static_cast<DnsType>(0));
  EXPECT_EQ(record1.dns_class(), static_cast<DnsClass>(0));
  EXPECT_EQ(record1.record_type(), RecordType::kShared);
  EXPECT_EQ(record1.ttl(),
            std::chrono::seconds(255));  // 255 is kDefaultRecordTTLSeconds
  EXPECT_EQ(record1.rdata(), Rdata(RawRecordRdata()));

  MdnsRecord record2(DomainName{"hostname", "local"}, DnsType::kPTR,
                     DnsClass::kIN, RecordType::kUnique, kTtl,
                     PtrRecordRdata(DomainName{"testing", "local"}));
  EXPECT_EQ(record2.MaxWireSize(), UINT64_C(41));
  EXPECT_EQ(record2.name(), (DomainName{"hostname", "local"}));
  EXPECT_EQ(record2.dns_type(), DnsType::kPTR);
  EXPECT_EQ(record2.dns_class(), DnsClass::kIN);
  EXPECT_EQ(record2.record_type(), RecordType::kUnique);
  EXPECT_EQ(record2.ttl(), kTtl);
  EXPECT_EQ(record2.rdata(),
            Rdata(PtrRecordRdata(DomainName{"testing", "local"})));
}

TEST(MdnsRecordTest, Compare) {
  const MdnsRecord record1(DomainName{"hostname", "local"}, DnsType::kPTR,
                           DnsClass::kIN, RecordType::kShared, kTtl,
                           PtrRecordRdata(DomainName{"testing", "local"}));
  const MdnsRecord record2(DomainName{"hostname", "local"}, DnsType::kPTR,
                           DnsClass::kIN, RecordType::kShared, kTtl,
                           PtrRecordRdata(DomainName{"testing", "local"}));
  const MdnsRecord record3(DomainName{"othername", "local"}, DnsType::kPTR,
                           DnsClass::kIN, RecordType::kShared, kTtl,
                           PtrRecordRdata(DomainName{"testing", "local"}));
  const MdnsRecord record4(DomainName{"hostname", "local"}, DnsType::kA,
                           DnsClass::kIN, RecordType::kShared, kTtl,
                           ARecordRdata(IPAddress{8, 8, 8, 8}));
  const MdnsRecord record5(DomainName{"hostname", "local"}, DnsType::kPTR,
                           DnsClass::kIN, RecordType::kUnique, kTtl,
                           PtrRecordRdata(DomainName{"testing", "local"}));
  const MdnsRecord record6(DomainName{"hostname", "local"}, DnsType::kPTR,
                           DnsClass::kIN, RecordType::kShared,
                           std::chrono::seconds(200),
                           PtrRecordRdata(DomainName{"testing", "local"}));
  const MdnsRecord record7(DomainName{"hostname", "local"}, DnsType::kPTR,
                           DnsClass::kIN, RecordType::kShared, kTtl,
                           PtrRecordRdata(DomainName{"device", "local"}));
  const MdnsRecord record8(
      DomainName{"testing", "local"}, DnsType::kNSEC, DnsClass::kIN,
      RecordType::kUnique, std::chrono::seconds(120),
      NsecRecordRdata(DomainName{"testing", "local"}, DnsType::kA));
  const MdnsRecord record9(
      DomainName{"testing", "local"}, DnsType::kNSEC, DnsClass::kIN,
      RecordType::kUnique, std::chrono::seconds(120),
      NsecRecordRdata(DomainName{"testing", "local"}, DnsType::kAAAA));

  EXPECT_EQ(record1, record2);

  // Account for intentional differences between > / < and = / !=. This is
  // unfortunate but required difference for > / < per RFC.
  EXPECT_NE(record1, record6);
  ASSERT_FALSE(record1 > record6);
  ASSERT_FALSE(record6 > record1);

  std::vector<const MdnsRecord*> records_sorted{
      &record4, &record7, &record1, &record5, &record3, &record8, &record9};
  for (size_t i = 0; i < records_sorted.size(); i++) {
    for (size_t j = i + 1; j < records_sorted.size(); j++) {
      EXPECT_NE(*records_sorted[i], *records_sorted[j])
          << "failure for i=" << i << " , j=" << j;
      EXPECT_GT(*records_sorted[j], *records_sorted[i])
          << "failure for i=" << i << " , j=" << j;
      EXPECT_LT(*records_sorted[i], *records_sorted[j])
          << "failure for i=" << i << " , j=" << j;
    }
  }

  EXPECT_TRUE(VerifyTypeImplementsAbslHashCorrectly(
      {record1, record2, record3, record4, record5, record6, record7, record8,
       record9}));
}

TEST(MdnsRecordTest, CopyAndMove) {
  MdnsRecord record(DomainName{"hostname", "local"}, DnsType::kPTR,
                    DnsClass::kIN, RecordType::kUnique, kTtl,
                    PtrRecordRdata(DomainName{"testing", "local"}));
  TestCopyAndMove(record);
}

TEST(MdnsQuestionTest, Construct) {
  MdnsQuestion question1;
  EXPECT_EQ(question1.MaxWireSize(), UINT64_C(5));
  EXPECT_EQ(question1.name(), DomainName());
  EXPECT_EQ(question1.dns_type(), static_cast<DnsType>(0));
  EXPECT_EQ(question1.dns_class(), static_cast<DnsClass>(0));
  EXPECT_EQ(question1.response_type(), ResponseType::kMulticast);

  MdnsQuestion question2(DomainName{"testing", "local"}, DnsType::kPTR,
                         DnsClass::kIN, ResponseType::kUnicast);
  EXPECT_EQ(question2.MaxWireSize(), UINT64_C(19));
  EXPECT_EQ(question2.name(), (DomainName{"testing", "local"}));
  EXPECT_EQ(question2.dns_type(), DnsType::kPTR);
  EXPECT_EQ(question2.dns_class(), DnsClass::kIN);
  EXPECT_EQ(question2.response_type(), ResponseType::kUnicast);
}

TEST(MdnsQuestionTest, Compare) {
  MdnsQuestion question1(DomainName{"testing", "local"}, DnsType::kPTR,
                         DnsClass::kIN, ResponseType::kMulticast);
  MdnsQuestion question2(DomainName{"testing", "local"}, DnsType::kPTR,
                         DnsClass::kIN, ResponseType::kMulticast);
  MdnsQuestion question3(DomainName{"hostname", "local"}, DnsType::kPTR,
                         DnsClass::kIN, ResponseType::kMulticast);
  MdnsQuestion question4(DomainName{"testing", "local"}, DnsType::kA,
                         DnsClass::kIN, ResponseType::kMulticast);
  MdnsQuestion question5(DomainName{"hostname", "local"}, DnsType::kPTR,
                         DnsClass::kIN, ResponseType::kUnicast);

  EXPECT_EQ(question1, question2);
  EXPECT_NE(question1, question3);
  EXPECT_NE(question1, question4);
  EXPECT_NE(question1, question5);

  EXPECT_TRUE(VerifyTypeImplementsAbslHashCorrectly(
      {question1, question2, question3, question4, question5}));
}

TEST(MdnsQuestionTest, CopyAndMove) {
  MdnsQuestion question(DomainName{"testing", "local"}, DnsType::kPTR,
                        DnsClass::kIN, ResponseType::kUnicast);
  TestCopyAndMove(question);
}

TEST(MdnsMessageTest, Construct) {
  MdnsMessage message1;
  EXPECT_EQ(message1.MaxWireSize(), UINT64_C(12));
  EXPECT_EQ(message1.id(), UINT16_C(0));
  EXPECT_EQ(message1.type(), MessageType::Query);
  EXPECT_EQ(message1.questions().size(), UINT64_C(0));
  EXPECT_EQ(message1.answers().size(), UINT64_C(0));
  EXPECT_EQ(message1.authority_records().size(), UINT64_C(0));
  EXPECT_EQ(message1.additional_records().size(), UINT64_C(0));

  MdnsQuestion question(DomainName{"testing", "local"}, DnsType::kPTR,
                        DnsClass::kIN, ResponseType::kUnicast);
  MdnsRecord record1(DomainName{"record1"}, DnsType::kA, DnsClass::kIN,
                     RecordType::kShared, kTtl,
                     ARecordRdata(IPAddress{172, 0, 0, 1}));
  MdnsRecord record2(DomainName{"record2"}, DnsType::kTXT, DnsClass::kIN,
                     RecordType::kShared, kTtl,
                     MakeTxtRecord({"foo=1", "bar=2"}));
  MdnsRecord record3(DomainName{"record3"}, DnsType::kPTR, DnsClass::kIN,
                     RecordType::kShared, kTtl,
                     PtrRecordRdata(DomainName{"device", "local"}));

  MdnsMessage message2(123, MessageType::Response);
  EXPECT_EQ(message2.MaxWireSize(), UINT64_C(12));
  EXPECT_EQ(message2.id(), UINT16_C(123));
  EXPECT_EQ(message2.type(), MessageType::Response);
  EXPECT_EQ(message2.questions().size(), UINT64_C(0));
  EXPECT_EQ(message2.answers().size(), UINT64_C(0));
  EXPECT_EQ(message2.authority_records().size(), UINT64_C(0));
  EXPECT_EQ(message2.additional_records().size(), UINT64_C(0));

  message2.AddQuestion(question);
  message2.AddAnswer(record1);
  message2.AddAuthorityRecord(record2);
  message2.AddAdditionalRecord(record3);

  EXPECT_EQ(message2.MaxWireSize(), UINT64_C(118));
  ASSERT_EQ(message2.questions().size(), UINT64_C(1));
  ASSERT_EQ(message2.answers().size(), UINT64_C(1));
  ASSERT_EQ(message2.authority_records().size(), UINT64_C(1));
  ASSERT_EQ(message2.additional_records().size(), UINT64_C(1));

  EXPECT_EQ(message2.questions()[0], question);
  EXPECT_EQ(message2.answers()[0], record1);
  EXPECT_EQ(message2.authority_records()[0], record2);
  EXPECT_EQ(message2.additional_records()[0], record3);

  MdnsMessage message3(
      123, MessageType::Response, std::vector<MdnsQuestion>{question},
      std::vector<MdnsRecord>{record1}, std::vector<MdnsRecord>{record2},
      std::vector<MdnsRecord>{record3});

  EXPECT_EQ(message3.MaxWireSize(), UINT64_C(118));
  ASSERT_EQ(message3.questions().size(), UINT64_C(1));
  ASSERT_EQ(message3.answers().size(), UINT64_C(1));
  ASSERT_EQ(message3.authority_records().size(), UINT64_C(1));
  ASSERT_EQ(message3.additional_records().size(), UINT64_C(1));

  EXPECT_EQ(message3.questions()[0], question);
  EXPECT_EQ(message3.answers()[0], record1);
  EXPECT_EQ(message3.authority_records()[0], record2);
  EXPECT_EQ(message3.additional_records()[0], record3);
}

TEST(MdnsMessageTest, Compare) {
  MdnsQuestion question(DomainName{"testing", "local"}, DnsType::kPTR,
                        DnsClass::kIN, ResponseType::kUnicast);
  MdnsRecord record1(DomainName{"record1"}, DnsType::kA, DnsClass::kIN,
                     RecordType::kShared, kTtl,
                     ARecordRdata(IPAddress{172, 0, 0, 1}));
  MdnsRecord record2(DomainName{"record2"}, DnsType::kTXT, DnsClass::kIN,
                     RecordType::kShared, kTtl,
                     MakeTxtRecord({"foo=1", "bar=2"}));
  MdnsRecord record3(DomainName{"record3"}, DnsType::kPTR, DnsClass::kIN,
                     RecordType::kShared, kTtl,
                     PtrRecordRdata(DomainName{"device", "local"}));

  MdnsMessage message1(
      123, MessageType::Response, std::vector<MdnsQuestion>{question},
      std::vector<MdnsRecord>{record1}, std::vector<MdnsRecord>{record2},
      std::vector<MdnsRecord>{record3});
  MdnsMessage message2(
      123, MessageType::Response, std::vector<MdnsQuestion>{question},
      std::vector<MdnsRecord>{record1}, std::vector<MdnsRecord>{record2},
      std::vector<MdnsRecord>{record3});
  MdnsMessage message3(
      456, MessageType::Response, std::vector<MdnsQuestion>{question},
      std::vector<MdnsRecord>{record1}, std::vector<MdnsRecord>{record2},
      std::vector<MdnsRecord>{record3});
  MdnsMessage message4(
      123, MessageType::Query, std::vector<MdnsQuestion>{question},
      std::vector<MdnsRecord>{record1}, std::vector<MdnsRecord>{record2},
      std::vector<MdnsRecord>{record3});
  MdnsMessage message5(123, MessageType::Response, std::vector<MdnsQuestion>{},
                       std::vector<MdnsRecord>{record1},
                       std::vector<MdnsRecord>{record2},
                       std::vector<MdnsRecord>{record3});
  MdnsMessage message6(
      123, MessageType::Response, std::vector<MdnsQuestion>{question},
      std::vector<MdnsRecord>{}, std::vector<MdnsRecord>{record2},
      std::vector<MdnsRecord>{record3});
  MdnsMessage message7(
      123, MessageType::Response, std::vector<MdnsQuestion>{question},
      std::vector<MdnsRecord>{record1}, std::vector<MdnsRecord>{},
      std::vector<MdnsRecord>{record3});
  MdnsMessage message8(
      123, MessageType::Response, std::vector<MdnsQuestion>{question},
      std::vector<MdnsRecord>{record1}, std::vector<MdnsRecord>{record2},
      std::vector<MdnsRecord>{});

  EXPECT_EQ(message1, message2);
  EXPECT_NE(message1, message3);
  EXPECT_NE(message1, message4);
  EXPECT_NE(message1, message5);
  EXPECT_NE(message1, message6);
  EXPECT_NE(message1, message7);
  EXPECT_NE(message1, message8);

  EXPECT_TRUE(VerifyTypeImplementsAbslHashCorrectly(
      {message1, message2, message3, message4, message5, message6, message7,
       message8}));
}

TEST(MdnsMessageTest, CopyAndMove) {
  MdnsQuestion question(DomainName{"testing", "local"}, DnsType::kPTR,
                        DnsClass::kIN, ResponseType::kUnicast);
  MdnsRecord record1(DomainName{"record1"}, DnsType::kA, DnsClass::kIN,
                     RecordType::kShared, kTtl,
                     ARecordRdata(IPAddress{172, 0, 0, 1}));
  MdnsRecord record2(DomainName{"record2"}, DnsType::kTXT, DnsClass::kIN,
                     RecordType::kShared, kTtl,
                     MakeTxtRecord({"foo=1", "bar=2"}));
  MdnsRecord record3(DomainName{"record3"}, DnsType::kPTR, DnsClass::kIN,
                     RecordType::kShared, kTtl,
                     PtrRecordRdata(DomainName{"device", "local"}));
  MdnsMessage message(
      123, MessageType::Response, std::vector<MdnsQuestion>{question},
      std::vector<MdnsRecord>{record1}, std::vector<MdnsRecord>{record2},
      std::vector<MdnsRecord>{record3});
  TestCopyAndMove(message);
}

TEST(MdnsRecordOperations, CanBeProcessed) {
  EXPECT_FALSE(CanBeProcessed(static_cast<DnsType>(1234)));
  EXPECT_FALSE(CanBeProcessed(static_cast<DnsType>(222)));
  EXPECT_FALSE(CanBeProcessed(static_cast<DnsType>(8973)));
}

}  // namespace discovery
}  // namespace openscreen
