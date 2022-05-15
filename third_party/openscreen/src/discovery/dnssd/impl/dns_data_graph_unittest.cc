// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/impl/dns_data_graph.h"

#include <utility>

#include "discovery/mdns/testing/mdns_test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/base/ip_address.h"

namespace openscreen {
namespace discovery {
namespace {

IPAddress GetAddressV4(const DnsSdInstanceEndpoint endpoint) {
  for (const IPAddress& address : endpoint.addresses()) {
    if (address.IsV4()) {
      return address;
    }
  }
  return IPAddress{};
}

IPAddress GetAddressV6(const DnsSdInstanceEndpoint endpoint) {
  for (const IPAddress& address : endpoint.addresses()) {
    if (address.IsV6()) {
      return address;
    }
  }
  return IPAddress{};
}

}  // namespace

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

class DomainChangeImpl {
 public:
  MOCK_METHOD1(OnStartTracking, void(const DomainName&));
  MOCK_METHOD1(OnStopTracking, void(const DomainName&));
};

class DnsDataGraphTests : public testing::Test {
 public:
  DnsDataGraphTests() : graph_(DnsDataGraph::Create(network_interface_)) {
    EXPECT_CALL(callbacks_, OnStartTracking(ptr_domain_));
    StartTracking(ptr_domain_);
    testing::Mock::VerifyAndClearExpectations(&callbacks_);
    EXPECT_EQ(graph_->GetTrackedDomainCount(), size_t{1});
  }

 protected:
  void TriggerRecordCreation(MdnsRecord record,
                             Error::Code result_code = Error::Code::kNone) {
    size_t size = graph_->GetTrackedDomainCount();
    Error result =
        ApplyDataRecordChange(std::move(record), RecordChangedEvent::kCreated);
    EXPECT_EQ(result.code(), result_code)
        << "Failed with error code " << result.code();
    size_t new_size = graph_->GetTrackedDomainCount();
    EXPECT_EQ(size, new_size);
  }

  void TriggerRecordCreationWithCallback(MdnsRecord record,
                                         const DomainName& target_domain) {
    EXPECT_CALL(callbacks_, OnStartTracking(target_domain));
    size_t size = graph_->GetTrackedDomainCount();
    Error result =
        ApplyDataRecordChange(std::move(record), RecordChangedEvent::kCreated);
    EXPECT_TRUE(result.ok()) << "Failed with error code " << result.code();
    size_t new_size = graph_->GetTrackedDomainCount();
    EXPECT_EQ(size + 1, new_size);
  }

  void ExpectDomainEqual(const DnsSdInstance& instance,
                         const DomainName& name) {
    EXPECT_EQ(name.labels().size(), size_t{4});
    EXPECT_EQ(instance.instance_id(), name.labels()[0]);
    EXPECT_EQ(instance.service_id(), name.labels()[1] + "." + name.labels()[2]);
    EXPECT_EQ(instance.domain_id(), name.labels()[3]);
  }

  Error ApplyDataRecordChange(MdnsRecord record, RecordChangedEvent event) {
    return graph_->ApplyDataRecordChange(
        std::move(record), event,
        [this](const DomainName& domain) {
          callbacks_.OnStartTracking(domain);
        },
        [this](const DomainName& domain) {
          callbacks_.OnStopTracking(domain);
        });
  }

  void StartTracking(const DomainName& domain) {
    graph_->StartTracking(
        domain, [this](const DomainName& d) { callbacks_.OnStartTracking(d); });
  }

  void StopTracking(const DomainName& domain) {
    graph_->StopTracking(
        domain, [this](const DomainName& d) { callbacks_.OnStopTracking(d); });
  }

  StrictMock<DomainChangeImpl> callbacks_;
  NetworkInterfaceIndex network_interface_ = 1234;
  std::unique_ptr<DnsDataGraph> graph_;
  DomainName ptr_domain_{"_cast", "_tcp", "local"};
  DomainName primary_domain_{"test", "_cast", "_tcp", "local"};
  DomainName secondary_domain_{"test2", "_cast", "_tcp", "local"};
  DomainName tertiary_domain_{"test3", "_cast", "_tcp", "local"};
};

TEST_F(DnsDataGraphTests, CallbacksCalledForStartStopTracking) {
  EXPECT_CALL(callbacks_, OnStopTracking(ptr_domain_));
  StopTracking(ptr_domain_);

  EXPECT_EQ(graph_->GetTrackedDomainCount(), size_t{0});
}

TEST_F(DnsDataGraphTests, ApplyChangeForUntrackedDomainError) {
  Error result = ApplyDataRecordChange(GetFakeSrvRecord(primary_domain_),
                                       RecordChangedEvent::kCreated);
  EXPECT_EQ(result.code(), Error::Code::kOperationCancelled);
  EXPECT_EQ(graph_->GetTrackedDomainCount(), size_t{1});
}

TEST_F(DnsDataGraphTests, ChildrenStopTrackingWhenRootQueryStopped) {
  auto ptr = GetFakePtrRecord(primary_domain_);
  auto srv = GetFakeSrvRecord(primary_domain_, secondary_domain_);
  auto a = GetFakeARecord(secondary_domain_);

  TriggerRecordCreationWithCallback(ptr, primary_domain_);
  TriggerRecordCreationWithCallback(srv, secondary_domain_);
  TriggerRecordCreation(a);

  EXPECT_CALL(callbacks_, OnStopTracking(ptr_domain_));
  EXPECT_CALL(callbacks_, OnStopTracking(primary_domain_));
  EXPECT_CALL(callbacks_, OnStopTracking(secondary_domain_));
  StopTracking(ptr_domain_);
  testing::Mock::VerifyAndClearExpectations(&callbacks_);

  EXPECT_EQ(graph_->GetTrackedDomainCount(), size_t{0});
}

TEST_F(DnsDataGraphTests, CyclicSrvStopsTrackingWhenRootQueryStopped) {
  auto ptr = GetFakePtrRecord(primary_domain_);
  auto srv = GetFakeSrvRecord(primary_domain_);
  auto a = GetFakeARecord(primary_domain_);

  TriggerRecordCreationWithCallback(ptr, primary_domain_);
  TriggerRecordCreation(srv);
  TriggerRecordCreation(a);

  EXPECT_CALL(callbacks_, OnStopTracking(ptr_domain_));
  EXPECT_CALL(callbacks_, OnStopTracking(primary_domain_));
  StopTracking(ptr_domain_);
  testing::Mock::VerifyAndClearExpectations(&callbacks_);

  EXPECT_EQ(graph_->GetTrackedDomainCount(), size_t{0});
}

TEST_F(DnsDataGraphTests, ChildrenStopTrackingWhenParentDeleted) {
  auto ptr = GetFakePtrRecord(primary_domain_);
  auto srv = GetFakeSrvRecord(primary_domain_, secondary_domain_);
  auto a = GetFakeARecord(secondary_domain_);

  TriggerRecordCreationWithCallback(ptr, primary_domain_);
  TriggerRecordCreationWithCallback(srv, secondary_domain_);
  TriggerRecordCreation(a);

  EXPECT_CALL(callbacks_, OnStopTracking(primary_domain_));
  EXPECT_CALL(callbacks_, OnStopTracking(secondary_domain_));
  auto result = ApplyDataRecordChange(ptr, RecordChangedEvent::kExpired);
  EXPECT_TRUE(result.ok()) << "Failed with error code " << result.code();
  testing::Mock::VerifyAndClearExpectations(&callbacks_);

  EXPECT_EQ(graph_->GetTrackedDomainCount(), size_t{1});
}

TEST_F(DnsDataGraphTests, OnlyAffectedNodesChangedWhenParentDeleted) {
  auto ptr = GetFakePtrRecord(primary_domain_);
  auto srv = GetFakeSrvRecord(primary_domain_, secondary_domain_);
  auto a = GetFakeARecord(secondary_domain_);

  TriggerRecordCreationWithCallback(ptr, primary_domain_);
  TriggerRecordCreationWithCallback(srv, secondary_domain_);
  TriggerRecordCreation(a);

  EXPECT_CALL(callbacks_, OnStopTracking(secondary_domain_));
  auto result = ApplyDataRecordChange(srv, RecordChangedEvent::kExpired);
  EXPECT_TRUE(result.ok()) << "Failed with error code " << result.code();
  testing::Mock::VerifyAndClearExpectations(&callbacks_);

  EXPECT_EQ(graph_->GetTrackedDomainCount(), size_t{2});
}

TEST_F(DnsDataGraphTests, CreateFailsForExistingRecord) {
  auto ptr = GetFakePtrRecord(primary_domain_);
  auto srv = GetFakeSrvRecord(primary_domain_);

  TriggerRecordCreationWithCallback(ptr, primary_domain_);
  TriggerRecordCreation(srv);

  auto result = ApplyDataRecordChange(srv, RecordChangedEvent::kCreated);
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(graph_->GetTrackedDomainCount(), size_t{2});
}

TEST_F(DnsDataGraphTests, UpdateFailsForNonExistingRecord) {
  auto ptr = GetFakePtrRecord(primary_domain_);
  auto srv = GetFakeSrvRecord(primary_domain_);

  TriggerRecordCreationWithCallback(ptr, primary_domain_);

  auto result = ApplyDataRecordChange(srv, RecordChangedEvent::kUpdated);
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(graph_->GetTrackedDomainCount(), size_t{2});
}

TEST_F(DnsDataGraphTests, DeleteFailsForNonExistingRecord) {
  auto ptr = GetFakePtrRecord(primary_domain_);
  auto srv = GetFakeSrvRecord(primary_domain_);

  TriggerRecordCreationWithCallback(ptr, primary_domain_);

  auto result = ApplyDataRecordChange(srv, RecordChangedEvent::kExpired);
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(graph_->GetTrackedDomainCount(), size_t{2});
}

TEST_F(DnsDataGraphTests, DeleteSucceedsForRecordWithDifferentTtl) {
  const MdnsRecord ptr = GetFakePtrRecord(primary_domain_);
  TriggerRecordCreationWithCallback(ptr, primary_domain_);

  EXPECT_CALL(callbacks_, OnStopTracking(primary_domain_));
  const MdnsRecord new_ptr(ptr.name(), ptr.dns_type(), ptr.dns_class(),
                           ptr.record_type(),
                           ptr.ttl() + std::chrono::seconds(10), ptr.rdata());
  const auto result =
      ApplyDataRecordChange(new_ptr, RecordChangedEvent::kExpired);

  EXPECT_TRUE(result.ok());
  EXPECT_EQ(graph_->GetTrackedDomainCount(), size_t{1});
}

TEST_F(DnsDataGraphTests, UpdateEndpointsWorksAsExpected) {
  auto ptr = GetFakePtrRecord(primary_domain_);
  auto srv = GetFakeSrvRecord(primary_domain_, secondary_domain_);
  auto txt = GetFakeTxtRecord(primary_domain_);
  auto a = GetFakeARecord(secondary_domain_);

  TriggerRecordCreationWithCallback(ptr, primary_domain_);
  TriggerRecordCreation(txt);
  TriggerRecordCreationWithCallback(srv, secondary_domain_);
  TriggerRecordCreation(a);

  std::vector<ErrorOr<DnsSdInstanceEndpoint>> endpoints =
      graph_->CreateEndpoints(DnsDataGraph::GetDomainGroup(srv),
                              primary_domain_);
  ASSERT_EQ(endpoints.size(), size_t{1});
  ErrorOr<DnsSdInstanceEndpoint> endpoint_or_error = std::move(endpoints[0]);
  ASSERT_TRUE(endpoint_or_error.is_value());
  DnsSdInstanceEndpoint endpoint = std::move(endpoint_or_error.value());

  ARecordRdata rdata(IPAddress(192, 168, 1, 2));
  MdnsRecord new_a(secondary_domain_, DnsType::kA, DnsClass::kIN,
                   RecordType::kUnique, std::chrono::seconds(0),
                   std::move(rdata));
  auto result = ApplyDataRecordChange(new_a, RecordChangedEvent::kUpdated);

  endpoints = graph_->CreateEndpoints(DnsDataGraph::GetDomainGroup(srv),
                                      primary_domain_);
  ASSERT_EQ(endpoints.size(), size_t{1});
  endpoint_or_error = std::move(endpoints[0]);
  ASSERT_TRUE(endpoint_or_error.is_value());
  DnsSdInstanceEndpoint endpoint2 = std::move(endpoint_or_error.value());
  ASSERT_EQ(endpoint.addresses().size(), size_t{1});
  ASSERT_EQ(endpoint.addresses().size(), endpoint2.addresses().size());
  EXPECT_NE(endpoint.addresses()[0], endpoint2.addresses()[0]);
  EXPECT_EQ(endpoint.instance_id(), endpoint2.instance_id());
  EXPECT_EQ(endpoint.service_id(), endpoint2.service_id());
  EXPECT_EQ(endpoint.domain_id(), endpoint2.domain_id());
  EXPECT_EQ(endpoint.txt(), endpoint2.txt());
  EXPECT_EQ(endpoint.port(), endpoint2.port());
}

TEST_F(DnsDataGraphTests, CreateEndpointsGeneratesCorrectRecords) {
  auto ptr = GetFakePtrRecord(primary_domain_);
  auto srv = GetFakeSrvRecord(primary_domain_, secondary_domain_);
  auto txt = GetFakeTxtRecord(primary_domain_);
  auto a = GetFakeARecord(secondary_domain_);
  auto aaaa = GetFakeAAAARecord(secondary_domain_);

  TriggerRecordCreationWithCallback(ptr, primary_domain_);
  TriggerRecordCreation(txt);
  TriggerRecordCreationWithCallback(srv, secondary_domain_);

  std::vector<ErrorOr<DnsSdInstanceEndpoint>> endpoints =
      graph_->CreateEndpoints(DnsDataGraph::GetDomainGroup(srv),
                              primary_domain_);
  EXPECT_EQ(endpoints.size(), size_t{0});

  TriggerRecordCreation(a);
  endpoints = graph_->CreateEndpoints(DnsDataGraph::GetDomainGroup(srv),
                                      primary_domain_);
  ASSERT_EQ(endpoints.size(), size_t{1});
  ErrorOr<DnsSdInstanceEndpoint> endpoint_or_error = std::move(endpoints[0]);
  ASSERT_TRUE(endpoint_or_error.is_value());
  DnsSdInstanceEndpoint endpoint_a = std::move(endpoint_or_error.value());
  EXPECT_TRUE(GetAddressV4(endpoint_a));
  EXPECT_FALSE(GetAddressV6(endpoint_a));
  EXPECT_EQ(GetAddressV4(endpoint_a), kFakeARecordAddress);
  ExpectDomainEqual(endpoint_a, primary_domain_);
  EXPECT_EQ(endpoint_a.port(), kFakeSrvRecordPort);

  TriggerRecordCreation(aaaa);
  endpoints = graph_->CreateEndpoints(DnsDataGraph::GetDomainGroup(srv),
                                      primary_domain_);
  ASSERT_EQ(endpoints.size(), size_t{1});
  endpoint_or_error = std::move(endpoints[0]);
  ASSERT_TRUE(endpoint_or_error.is_value());
  DnsSdInstanceEndpoint endpoint_a_aaaa = std::move(endpoint_or_error.value());
  ASSERT_TRUE(GetAddressV4(endpoint_a_aaaa));
  ASSERT_TRUE(GetAddressV6(endpoint_a_aaaa));
  EXPECT_EQ(GetAddressV4(endpoint_a_aaaa), kFakeARecordAddress);
  EXPECT_EQ(GetAddressV6(endpoint_a_aaaa), kFakeAAAARecordAddress);
  EXPECT_EQ(static_cast<DnsSdInstance>(endpoint_a),
            static_cast<DnsSdInstance>(endpoint_a_aaaa));

  auto result = ApplyDataRecordChange(a, RecordChangedEvent::kExpired);
  EXPECT_TRUE(result.ok()) << "Failed with error code " << result.code();
  endpoints = graph_->CreateEndpoints(DnsDataGraph::GetDomainGroup(srv),
                                      primary_domain_);
  ASSERT_EQ(endpoints.size(), size_t{1});
  endpoint_or_error = std::move(endpoints[0]);
  ASSERT_TRUE(endpoint_or_error.is_value());
  DnsSdInstanceEndpoint endpoint_aaaa = std::move(endpoint_or_error.value());
  EXPECT_FALSE(GetAddressV4(endpoint_aaaa));
  ASSERT_TRUE(GetAddressV6(endpoint_aaaa));
  EXPECT_EQ(GetAddressV6(endpoint_aaaa), kFakeAAAARecordAddress);
  EXPECT_EQ(static_cast<DnsSdInstance>(endpoint_a),
            static_cast<DnsSdInstance>(endpoint_aaaa));

  result = ApplyDataRecordChange(aaaa, RecordChangedEvent::kExpired);
  EXPECT_TRUE(result.ok()) << "Failed with error code " << result.code();
  endpoints = graph_->CreateEndpoints(DnsDataGraph::GetDomainGroup(srv),
                                      primary_domain_);
  ASSERT_EQ(endpoints.size(), size_t{0});
}

TEST_F(DnsDataGraphTests, CreateEndpointsHandlesSelfLoops) {
  auto ptr = GetFakePtrRecord(primary_domain_);
  auto srv = GetFakeSrvRecord(primary_domain_, primary_domain_);
  auto txt = GetFakeTxtRecord(primary_domain_);
  auto a = GetFakeARecord(primary_domain_);
  auto aaaa = GetFakeAAAARecord(primary_domain_);

  TriggerRecordCreationWithCallback(ptr, primary_domain_);
  TriggerRecordCreation(srv);
  TriggerRecordCreation(txt);
  TriggerRecordCreation(a);
  TriggerRecordCreation(aaaa);

  auto endpoints = graph_->CreateEndpoints(DnsDataGraph::GetDomainGroup(srv),
                                           primary_domain_);
  ASSERT_EQ(endpoints.size(), size_t{1});
  ASSERT_TRUE(endpoints[0].is_value());
  DnsSdInstanceEndpoint endpoint = std::move(endpoints[0].value());

  EXPECT_EQ(GetAddressV4(endpoint), kFakeARecordAddress);
  EXPECT_EQ(GetAddressV6(endpoint), kFakeAAAARecordAddress);
  ExpectDomainEqual(endpoint, primary_domain_);
  EXPECT_EQ(endpoint.port(), kFakeSrvRecordPort);

  auto endpoints2 =
      graph_->CreateEndpoints(DnsDataGraph::GetDomainGroup(ptr), ptr_domain_);
  ASSERT_EQ(endpoints2.size(), size_t{1});
  ASSERT_TRUE(endpoints2[0].is_value());
  DnsSdInstanceEndpoint endpoint2 = std::move(endpoints2[0].value());

  EXPECT_EQ(GetAddressV4(endpoint2), kFakeARecordAddress);
  EXPECT_EQ(GetAddressV6(endpoint2), kFakeAAAARecordAddress);
  ExpectDomainEqual(endpoint2, primary_domain_);
  EXPECT_EQ(endpoint2.port(), kFakeSrvRecordPort);

  EXPECT_EQ(static_cast<DnsSdInstance>(endpoint),
            static_cast<DnsSdInstance>(endpoint2));
  EXPECT_EQ(endpoint, endpoint2);
}

TEST_F(DnsDataGraphTests, CreateEndpointsWithMultipleParents) {
  auto ptr = GetFakePtrRecord(primary_domain_);
  auto srv = GetFakeSrvRecord(primary_domain_, tertiary_domain_);
  auto txt = GetFakeTxtRecord(primary_domain_);
  auto ptr2 = GetFakePtrRecord(secondary_domain_);
  auto srv2 = GetFakeSrvRecord(secondary_domain_, tertiary_domain_);
  auto txt2 = GetFakeTxtRecord(secondary_domain_);
  auto a = GetFakeARecord(tertiary_domain_);
  auto aaaa = GetFakeAAAARecord(tertiary_domain_);

  TriggerRecordCreationWithCallback(ptr, primary_domain_);
  TriggerRecordCreationWithCallback(srv, tertiary_domain_);
  TriggerRecordCreation(txt);
  TriggerRecordCreationWithCallback(ptr2, secondary_domain_);
  TriggerRecordCreation(srv2);
  TriggerRecordCreation(txt2);
  TriggerRecordCreation(a);
  TriggerRecordCreation(aaaa);

  auto endpoints = graph_->CreateEndpoints(DnsDataGraph::GetDomainGroup(a),
                                           tertiary_domain_);
  ASSERT_EQ(endpoints.size(), size_t{2});
  ASSERT_TRUE(endpoints[0].is_value());
  ASSERT_TRUE(endpoints[1].is_value());

  DnsSdInstanceEndpoint endpoint_a = std::move(endpoints[0].value());
  DnsSdInstanceEndpoint endpoint_b = std::move(endpoints[1].value());
  DnsSdInstanceEndpoint* endpoint_1;
  DnsSdInstanceEndpoint* endpoint_2;
  if (endpoint_a.instance_id() == "test") {
    endpoint_1 = &endpoint_a;
    endpoint_2 = &endpoint_b;
  } else {
    endpoint_2 = &endpoint_a;
    endpoint_1 = &endpoint_b;
  }

  EXPECT_EQ(GetAddressV4(*endpoint_1), kFakeARecordAddress);
  EXPECT_EQ(GetAddressV6(*endpoint_1), kFakeAAAARecordAddress);
  EXPECT_EQ(endpoint_1->port(), kFakeSrvRecordPort);
  ExpectDomainEqual(*endpoint_1, primary_domain_);

  EXPECT_EQ(GetAddressV4(*endpoint_2), kFakeARecordAddress);
  EXPECT_EQ(GetAddressV6(*endpoint_2), kFakeAAAARecordAddress);
  EXPECT_EQ(endpoint_2->port(), kFakeSrvRecordPort);
  ExpectDomainEqual(*endpoint_2, secondary_domain_);
}

TEST_F(DnsDataGraphTests, FailedConversionOnlyFailsSingleEndpointCreation) {
  auto ptr = GetFakePtrRecord(primary_domain_);
  auto srv = GetFakeSrvRecord(primary_domain_, tertiary_domain_);
  auto txt = GetFakeTxtRecord(primary_domain_);
  auto ptr2 = GetFakePtrRecord(secondary_domain_);
  auto srv2 = GetFakeSrvRecord(secondary_domain_, tertiary_domain_);
  auto txt2 = MdnsRecord(secondary_domain_, DnsType::kTXT, DnsClass::kIN,
                         RecordType::kUnique, std::chrono::seconds(0),
                         MakeTxtRecord({"=bad_txt_record"}));
  auto a = GetFakeARecord(tertiary_domain_);
  auto aaaa = GetFakeAAAARecord(tertiary_domain_);

  TriggerRecordCreationWithCallback(ptr, primary_domain_);
  TriggerRecordCreationWithCallback(ptr2, secondary_domain_);
  TriggerRecordCreationWithCallback(srv, tertiary_domain_);
  TriggerRecordCreation(srv2);
  TriggerRecordCreation(txt);
  TriggerRecordCreation(txt2);
  TriggerRecordCreation(a);
  TriggerRecordCreation(aaaa);

  auto endpoints = graph_->CreateEndpoints(DnsDataGraph::GetDomainGroup(a),
                                           tertiary_domain_);
  ASSERT_EQ(endpoints.size(), size_t{2});
  ASSERT_TRUE(endpoints[0].is_error() || endpoints[1].is_error());
  ASSERT_TRUE(endpoints[0].is_value() || endpoints[1].is_value());

  DnsSdInstanceEndpoint endpoint = endpoints[0].is_value()
                                       ? std::move(endpoints[0].value())
                                       : std::move(endpoints[1].value());
  EXPECT_EQ(GetAddressV4(endpoint), kFakeARecordAddress);
  EXPECT_EQ(GetAddressV6(endpoint), kFakeAAAARecordAddress);
  EXPECT_EQ(endpoint.port(), kFakeSrvRecordPort);
  ExpectDomainEqual(endpoint, primary_domain_);
}

}  // namespace discovery
}  // namespace openscreen
