// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/impl/querier_impl.h"

#include <memory>
#include <string>
#include <utility>

#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/types/optional.h"
#include "discovery/common/testing/mock_reporting_client.h"
#include "discovery/dnssd/impl/conversion_layer.h"
#include "discovery/dnssd/testing/fake_network_interface_config.h"
#include "discovery/mdns/mdns_records.h"
#include "discovery/mdns/testing/mdns_test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {
namespace discovery {
namespace {

NetworkInterfaceIndex kNetworkInterface = 0;

class MockCallback : public DnsSdQuerier::Callback {
 public:
  MOCK_METHOD1(OnEndpointCreated, void(const DnsSdInstanceEndpoint&));
  MOCK_METHOD1(OnEndpointUpdated, void(const DnsSdInstanceEndpoint&));
  MOCK_METHOD1(OnEndpointDeleted, void(const DnsSdInstanceEndpoint&));
};

class MockMdnsService : public MdnsService {
 public:
  MOCK_METHOD4(
      StartQuery,
      void(const DomainName&, DnsType, DnsClass, MdnsRecordChangedCallback*));

  MOCK_METHOD4(
      StopQuery,
      void(const DomainName&, DnsType, DnsClass, MdnsRecordChangedCallback*));

  MOCK_METHOD1(ReinitializeQueries, void(const DomainName& name));

  // Unused.
  MOCK_METHOD3(StartProbe,
               Error(MdnsDomainConfirmedProvider*, DomainName, IPAddress));
  MOCK_METHOD1(RegisterRecord, Error(const MdnsRecord&));
  MOCK_METHOD1(UnregisterRecord, Error(const MdnsRecord&));
  MOCK_METHOD2(UpdateRegisteredRecord,
               Error(const MdnsRecord&, const MdnsRecord&));
};

class MockDnsDataGraph : public DnsDataGraph {
 public:
  MOCK_METHOD2(StartTracking,
               void(const DomainName& domain,
                    DomainChangeCallback on_start_tracking));
  MOCK_METHOD2(StopTracking,
               void(const DomainName& domain,
                    DomainChangeCallback on_start_tracking));

  MOCK_CONST_METHOD2(
      CreateEndpoints,
      std::vector<ErrorOr<DnsSdInstanceEndpoint>>(DomainGroup,
                                                  const DomainName&));

  MOCK_METHOD4(ApplyDataRecordChange,
               Error(MdnsRecord,
                     RecordChangedEvent,
                     DomainChangeCallback,
                     DomainChangeCallback));

  MOCK_CONST_METHOD0(GetTrackedDomainCount, size_t());

  MOCK_CONST_METHOD1(IsTracked, bool(const DomainName&));
};

}  // namespace

using testing::_;
using testing::ByMove;
using testing::Return;
using testing::StrictMock;

class QuerierImplTesting : public QuerierImpl {
 public:
  QuerierImplTesting()
      : QuerierImpl(&mock_service_,
                    &task_runner_,
                    &reporting_client_,
                    &network_config_),
        clock_(Clock::now()),
        task_runner_(&clock_) {}

  StrictMock<MockMdnsService>& service() { return mock_service_; }

  StrictMock<MockReportingClient>& reporting_client() {
    return reporting_client_;
  }

  // NOTE: This should only be used for testing hard-to-achieve edge cases.
  StrictMock<MockDnsDataGraph>& GetMockedGraph() {
    if (!is_graph_mocked_) {
      graph_ = std::make_unique<StrictMock<MockDnsDataGraph>>();
      is_graph_mocked_ = true;
    }

    return static_cast<StrictMock<MockDnsDataGraph>&>(*graph_);
  }

  size_t GetTrackedDomainCount() { return graph_->GetTrackedDomainCount(); }

  bool IsDomainTracked(const DomainName& domain) {
    return graph_->IsTracked(domain);
  }

  using QuerierImpl::OnRecordChanged;

 private:
  FakeClock clock_;
  FakeTaskRunner task_runner_;
  FakeNetworkInterfaceConfig network_config_;
  StrictMock<MockMdnsService> mock_service_;
  StrictMock<MockReportingClient> reporting_client_;

  bool is_graph_mocked_ = false;
};

class DnsSdQuerierImplTest : public testing::Test {
 public:
  DnsSdQuerierImplTest()
      : querier(std::make_unique<QuerierImplTesting>()),
        ptr_domain(DomainName{"_service", "_udp", domain}),
        name(DomainName{instance, "_service", "_udp", domain}),
        name2(DomainName{instance2, "_service", "_udp", domain}) {
    EXPECT_FALSE(querier->IsQueryRunning(service));

    EXPECT_CALL(querier->service(),
                StartQuery(_, DnsType::kANY, DnsClass::kANY, _))
        .Times(1);
    querier->StartQuery(service, &callback);
    EXPECT_TRUE(querier->IsQueryRunning(service));
    testing::Mock::VerifyAndClearExpectations(&querier->service());

    EXPECT_TRUE(querier->IsQueryRunning(service));
    testing::Mock::VerifyAndClearExpectations(&querier->service());
  }

 protected:
  void ValidateRecordChangeStartsQuery(
      const std::vector<PendingQueryChange>& changes,
      const DomainName& domain_name,
      size_t expected_size) {
    ValidateRecordChangeResult(changes, domain_name, expected_size,
                               PendingQueryChange::kStartQuery);
  }

  void ValidateRecordChangeStopsQuery(
      const std::vector<PendingQueryChange>& changes,
      const DomainName& domain_name,
      size_t expected_size) {
    ValidateRecordChangeResult(changes, domain_name, expected_size,
                               PendingQueryChange::kStopQuery);
  }

  void CreateServiceInstance(const DomainName& service_domain,
                             MockCallback* cb) {
    MdnsRecord ptr = GetFakePtrRecord(service_domain);
    MdnsRecord srv = GetFakeSrvRecord(service_domain);
    MdnsRecord txt = GetFakeTxtRecord(service_domain);
    MdnsRecord a = GetFakeARecord(service_domain);
    MdnsRecord aaaa = GetFakeAAAARecord(service_domain);

    auto result = querier->OnRecordChanged(ptr, RecordChangedEvent::kCreated);
    ValidateRecordChangeStartsQuery(result, service_domain, 1);

    // NOTE: This verbose iterator handling is used to avoid gcc failures.
    auto it = service_domain.labels().begin();
    it++;
    std::string service_name = *it;
    it++;
    std::string service_protocol = *it;
    std::string service_id = "";
    service_id.append(std::move(service_name))
        .append(".")
        .append(std::move(service_protocol));
    ASSERT_TRUE(querier->IsQueryRunning(service_id));

    result = querier->OnRecordChanged(srv, RecordChangedEvent::kCreated);
    EXPECT_EQ(result.size(), size_t{0});

    result = querier->OnRecordChanged(a, RecordChangedEvent::kCreated);
    EXPECT_EQ(result.size(), size_t{0});

    result = querier->OnRecordChanged(aaaa, RecordChangedEvent::kCreated);
    EXPECT_EQ(result.size(), size_t{0});

    EXPECT_CALL(*cb, OnEndpointCreated(_)).Times(1);
    result = querier->OnRecordChanged(txt, RecordChangedEvent::kCreated);
    EXPECT_EQ(result.size(), size_t{0});
    testing::Mock::VerifyAndClearExpectations(cb);
  }

  std::string instance = "instance";
  std::string instance2 = "instance2";
  std::string service = "_service._udp";
  std::string service2 = "_service2._udp";
  std::string domain = "local";
  StrictMock<MockCallback> callback;
  std::unique_ptr<QuerierImplTesting> querier;
  DomainName ptr_domain;
  DomainName name;
  DomainName name2;

 private:
  void ValidateRecordChangeResult(
      const std::vector<PendingQueryChange>& changes,
      const DomainName& domain_name,
      size_t expected_size,
      PendingQueryChange::ChangeType change_type) {
    EXPECT_EQ(changes.size(), expected_size);
    EXPECT_TRUE(ContainsIf(
        changes, [&domain_name, change_type](const PendingQueryChange& change) {
          return change.dns_type == DnsType::kANY &&
                 change.dns_class == DnsClass::kANY &&
                 change.change_type == change_type &&
                 change.name == domain_name;
        }));
  }
};

// Common Use Cases
//
// The below tests validate the common use cases for QuerierImpl, which we
// expect will be hit for reasonable actors on the network. For these tests, the
// real DnsDataGraph object will be used.

TEST_F(DnsSdQuerierImplTest, TestStartStopQueryCallsMdnsQueries) {
  DomainName other_service_id(
      DomainName{instance2, "_service2", "_udp", domain});

  StrictMock<MockCallback> callback2;
  EXPECT_FALSE(querier->IsQueryRunning(service2));

  EXPECT_CALL(querier->service(),
              StartQuery(_, DnsType::kANY, DnsClass::kANY, _))
      .Times(1);
  querier->StartQuery(service2, &callback2);
  EXPECT_TRUE(querier->IsQueryRunning(service2));

  EXPECT_CALL(querier->service(),
              StopQuery(_, DnsType::kANY, DnsClass::kANY, _))
      .Times(1);
  querier->StopQuery(service2, &callback2);
  EXPECT_FALSE(querier->IsQueryRunning(service2));
}

TEST_F(DnsSdQuerierImplTest, TestStartDuplicateQueryFiresCallbacksWhenAble) {
  StrictMock<MockCallback> callback2;
  CreateServiceInstance(name, &callback);

  EXPECT_CALL(callback2, OnEndpointCreated(_)).Times(1);
  querier->StartQuery(service, &callback2);
  testing::Mock::VerifyAndClearExpectations(&callback2);
}

TEST_F(DnsSdQuerierImplTest, TestStopQueryStopsTrackingRecords) {
  CreateServiceInstance(name, &callback);

  DomainName ptr_domain(++name.labels().begin(), name.labels().end());
  EXPECT_CALL(querier->service(),
              StopQuery(ptr_domain, DnsType::kANY, DnsClass::kANY, _))
      .Times(1);
  EXPECT_CALL(querier->service(),
              StopQuery(name, DnsType::kANY, DnsClass::kANY, _))
      .Times(1);
  querier->StopQuery(service, &callback);
  EXPECT_FALSE(querier->IsDomainTracked(ptr_domain));
  EXPECT_FALSE(querier->IsDomainTracked(name));
  EXPECT_EQ(querier->GetTrackedDomainCount(), size_t{0});
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_CALL(querier->service(),
              StartQuery(_, DnsType::kANY, DnsClass::kANY, _))
      .Times(1);
  querier->StartQuery(service, &callback);
  EXPECT_TRUE(querier->IsQueryRunning(service));
}

TEST_F(DnsSdQuerierImplTest, TestStopNonexistantQueryHasNoEffect) {
  StrictMock<MockCallback> callback2;
  querier->StopQuery(service, &callback2);
}

TEST_F(DnsSdQuerierImplTest, TestAFollowingAAAAFiresSecondCallback) {
  MdnsRecord ptr = GetFakePtrRecord(name);
  MdnsRecord srv = GetFakeSrvRecord(name);
  MdnsRecord txt = GetFakeTxtRecord(name);
  MdnsRecord a = GetFakeARecord(name);
  MdnsRecord aaaa = GetFakeAAAARecord(name);

  std::vector<DnsSdInstanceEndpoint> endpoints;
  auto changes = querier->OnRecordChanged(ptr, RecordChangedEvent::kCreated);
  ValidateRecordChangeStartsQuery(changes, name, 1);

  changes = querier->OnRecordChanged(srv, RecordChangedEvent::kCreated);
  EXPECT_EQ(changes.size(), size_t{0});
  changes = querier->OnRecordChanged(txt, RecordChangedEvent::kCreated);
  EXPECT_EQ(changes.size(), size_t{0});

  EXPECT_CALL(callback, OnEndpointCreated(_))
      .WillOnce([&endpoints](const DnsSdInstanceEndpoint& ep) mutable {
        endpoints.push_back(ep);
      });
  changes = querier->OnRecordChanged(aaaa, RecordChangedEvent::kCreated);
  EXPECT_EQ(changes.size(), size_t{0});
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_CALL(callback, OnEndpointUpdated(_))
      .WillOnce([&endpoints](const DnsSdInstanceEndpoint& ep) mutable {
        endpoints.push_back(ep);
      });
  changes = querier->OnRecordChanged(a, RecordChangedEvent::kCreated);
  EXPECT_EQ(changes.size(), size_t{0});
  testing::Mock::VerifyAndClearExpectations(&callback);

  ASSERT_EQ(endpoints.size(), size_t{2});
  DnsSdInstanceEndpoint& created = endpoints[0];
  DnsSdInstanceEndpoint& updated = endpoints[1];
  EXPECT_EQ(static_cast<DnsSdInstance>(created),
            static_cast<DnsSdInstance>(updated));

  ASSERT_EQ(created.addresses().size(), size_t{1});
  EXPECT_TRUE(created.addresses()[0].IsV6());

  ASSERT_EQ(updated.addresses().size(), size_t{2});
  EXPECT_TRUE(created.addresses()[0] == updated.addresses()[0] ||
              created.addresses()[0] == updated.addresses()[1]);
  EXPECT_TRUE(updated.addresses()[0].IsV4() || updated.addresses()[1].IsV4());
}

TEST_F(DnsSdQuerierImplTest, TestGenerateTwoRecordsCallsCallbackTwice) {
  DomainName third{"android", "local"};
  MdnsRecord ptr1 = GetFakePtrRecord(name);
  MdnsRecord srv1 = GetFakeSrvRecord(name, third);
  MdnsRecord txt1 = GetFakeTxtRecord(name);
  MdnsRecord ptr2 = GetFakePtrRecord(name2);
  MdnsRecord srv2 = GetFakeSrvRecord(name2, third);
  MdnsRecord txt2 = GetFakeTxtRecord(name2);
  MdnsRecord a = GetFakeARecord(third);

  auto changes = querier->OnRecordChanged(ptr1, RecordChangedEvent::kCreated);
  ValidateRecordChangeStartsQuery(changes, name, 1);

  changes = querier->OnRecordChanged(srv1, RecordChangedEvent::kCreated);
  ValidateRecordChangeStartsQuery(changes, third, 1);

  changes = querier->OnRecordChanged(txt1, RecordChangedEvent::kCreated);
  EXPECT_EQ(changes.size(), size_t{0});

  changes = querier->OnRecordChanged(ptr2, RecordChangedEvent::kCreated);
  ValidateRecordChangeStartsQuery(changes, name2, 1);

  changes = querier->OnRecordChanged(srv2, RecordChangedEvent::kCreated);
  EXPECT_EQ(changes.size(), size_t{0});

  changes = querier->OnRecordChanged(txt2, RecordChangedEvent::kCreated);
  EXPECT_EQ(changes.size(), size_t{0});

  EXPECT_CALL(callback, OnEndpointCreated(_)).Times(2);
  changes = querier->OnRecordChanged(a, RecordChangedEvent::kCreated);
  EXPECT_EQ(changes.size(), size_t{0});
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_CALL(callback, OnEndpointDeleted(_)).Times(2);
  changes = querier->OnRecordChanged(a, RecordChangedEvent::kExpired);
  EXPECT_EQ(changes.size(), size_t{0});
}

TEST_F(DnsSdQuerierImplTest, TestCreateDeletePtrRecordResults) {
  const auto ptr = GetFakePtrRecord(name);

  auto result = querier->OnRecordChanged(ptr, RecordChangedEvent::kCreated);
  ValidateRecordChangeStartsQuery(result, name, 1);

  result = querier->OnRecordChanged(ptr, RecordChangedEvent::kExpired);
  ValidateRecordChangeStopsQuery(result, name, 1);
}

TEST_F(DnsSdQuerierImplTest, CallbackCalledWhenPtrDeleted) {
  MdnsRecord ptr = GetFakePtrRecord(name);
  MdnsRecord srv = GetFakeSrvRecord(name, name2);
  MdnsRecord txt = GetFakeTxtRecord(name);
  MdnsRecord a = GetFakeARecord(name2);

  auto changes = querier->OnRecordChanged(ptr, RecordChangedEvent::kCreated);
  ValidateRecordChangeStartsQuery(changes, name, 1);

  changes = querier->OnRecordChanged(srv, RecordChangedEvent::kCreated);
  ValidateRecordChangeStartsQuery(changes, name2, 1);

  changes = querier->OnRecordChanged(txt, RecordChangedEvent::kCreated);
  EXPECT_EQ(changes.size(), size_t{0});

  EXPECT_CALL(callback, OnEndpointCreated(_));
  changes = querier->OnRecordChanged(a, RecordChangedEvent::kCreated);
  EXPECT_EQ(changes.size(), size_t{0});

  EXPECT_CALL(callback, OnEndpointDeleted(_));
  changes = querier->OnRecordChanged(ptr, RecordChangedEvent::kExpired);
  ValidateRecordChangeStopsQuery(changes, name, 2);
  ValidateRecordChangeStopsQuery(changes, name2, 2);
}

TEST_F(DnsSdQuerierImplTest, HardRefresh) {
  MdnsRecord ptr = GetFakePtrRecord(name);
  MdnsRecord srv = GetFakeSrvRecord(name, name2);
  MdnsRecord txt = GetFakeTxtRecord(name);
  MdnsRecord a = GetFakeARecord(name2);

  querier->OnRecordChanged(ptr, RecordChangedEvent::kCreated);
  querier->OnRecordChanged(srv, RecordChangedEvent::kCreated);
  querier->OnRecordChanged(txt, RecordChangedEvent::kCreated);

  EXPECT_CALL(callback, OnEndpointCreated(_));
  querier->OnRecordChanged(a, RecordChangedEvent::kCreated);
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_CALL(querier->service(),
              StopQuery(ptr_domain, DnsType::kANY, DnsClass::kANY, _))
      .Times(1);
  EXPECT_CALL(querier->service(),
              StopQuery(name, DnsType::kANY, DnsClass::kANY, _))
      .Times(1);
  EXPECT_CALL(querier->service(),
              StopQuery(name2, DnsType::kANY, DnsClass::kANY, _))
      .Times(1);
  EXPECT_CALL(querier->service(), ReinitializeQueries(_)).Times(1);
  EXPECT_CALL(querier->service(),
              StartQuery(ptr_domain, DnsType::kANY, DnsClass::kANY, _))
      .Times(1);
  querier->ReinitializeQueries(service);
  testing::Mock::VerifyAndClearExpectations(querier.get());
}

// Edge Cases
//
// The below tests validate against edge cases that either either difficult to
// achieve, are not expected to be possible under normal circumstances but
// should be validated against for safety, or should only occur when either a
// bad actor or a misbehaving publisher is present on the network. To simplify
// these tests, the DnsDataGraph object will be mocked.
TEST_F(DnsSdQuerierImplTest, ErrorsOnlyAfterChangesAreLogged) {
  MockDnsDataGraph& mock_graph = querier->GetMockedGraph();
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> before_changes{};
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> after_changes{};
  after_changes.emplace_back(Error::Code::kItemNotFound);
  after_changes.emplace_back(Error::Code::kItemNotFound);
  after_changes.emplace_back(Error::Code::kItemAlreadyExists);

  // Calls before and after applying record changes, then the error it logs.
  EXPECT_CALL(mock_graph, CreateEndpoints(_, _))
      .WillOnce(Return(ByMove(std::move(before_changes))))
      .WillOnce(Return(ByMove(std::move(after_changes))));
  EXPECT_CALL(querier->reporting_client(), OnRecoverableError(_)).Times(3);

  // Call to apply record changes. The specifics are unimportant.
  EXPECT_CALL(mock_graph, ApplyDataRecordChange(_, _, _, _))
      .WillOnce(Return(Error::None()));

  // Call with any record. The mocks make the specifics unimportant.
  querier->OnRecordChanged(GetFakePtrRecord(name),
                           RecordChangedEvent::kCreated);
}

TEST_F(DnsSdQuerierImplTest, ErrorsOnlyBeforeChangesNotLogged) {
  MockDnsDataGraph& mock_graph = querier->GetMockedGraph();
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> before_changes{};
  before_changes.emplace_back(Error::Code::kItemNotFound);
  before_changes.emplace_back(Error::Code::kItemNotFound);
  before_changes.emplace_back(Error::Code::kItemAlreadyExists);
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> after_changes{};

  // Calls before and after applying record changes.
  EXPECT_CALL(mock_graph, CreateEndpoints(_, _))
      .WillOnce(Return(ByMove(std::move(before_changes))))
      .WillOnce(Return(ByMove(std::move(after_changes))));

  // Call to apply record changes. The specifics are unimportant.
  EXPECT_CALL(mock_graph, ApplyDataRecordChange(_, _, _, _))
      .WillOnce(Return(Error::None()));

  // Call with any record. The mocks make the specifics unimportant.
  querier->OnRecordChanged(GetFakePtrRecord(name),
                           RecordChangedEvent::kCreated);
}

TEST_F(DnsSdQuerierImplTest, ErrorsBeforeAndAfterChangesNotLogged) {
  MockDnsDataGraph& mock_graph = querier->GetMockedGraph();
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> before_changes{};
  before_changes.emplace_back(Error::Code::kItemNotFound);
  before_changes.emplace_back(Error::Code::kItemNotFound);
  before_changes.emplace_back(Error::Code::kItemAlreadyExists);
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> after_changes{};
  after_changes.emplace_back(Error::Code::kItemNotFound);
  after_changes.emplace_back(Error::Code::kItemAlreadyExists);
  after_changes.emplace_back(Error::Code::kItemNotFound);

  // Calls before and after applying record changes.
  EXPECT_CALL(mock_graph, CreateEndpoints(_, _))
      .WillOnce(Return(ByMove(std::move(before_changes))))
      .WillOnce(Return(ByMove(std::move(after_changes))));

  // Call to apply record changes. The specifics are unimportant.
  EXPECT_CALL(mock_graph, ApplyDataRecordChange(_, _, _, _))
      .WillOnce(Return(Error::None()));

  // Call with any record. The mocks make the specifics unimportant.
  querier->OnRecordChanged(GetFakePtrRecord(name),
                           RecordChangedEvent::kCreated);
}

TEST_F(DnsSdQuerierImplTest, OrderOfErrorsDoesNotAffectResults) {
  MockDnsDataGraph& mock_graph = querier->GetMockedGraph();
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> before_changes{};
  before_changes.emplace_back(Error::Code::kIndexOutOfBounds);
  before_changes.emplace_back(Error::Code::kItemAlreadyExists);
  before_changes.emplace_back(Error::Code::kOperationCancelled);
  before_changes.emplace_back(Error::Code::kItemNotFound);
  before_changes.emplace_back(Error::Code::kOperationInProgress);
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> after_changes{};
  after_changes.emplace_back(Error::Code::kOperationInProgress);
  after_changes.emplace_back(Error::Code::kUnknownError);
  after_changes.emplace_back(Error::Code::kItemNotFound);
  after_changes.emplace_back(Error::Code::kItemAlreadyExists);
  after_changes.emplace_back(Error::Code::kOperationCancelled);

  // Calls before and after applying record changes, then the error it logs.
  EXPECT_CALL(mock_graph, CreateEndpoints(_, _))
      .WillOnce(Return(ByMove(std::move(before_changes))))
      .WillOnce(Return(ByMove(std::move(after_changes))));
  EXPECT_CALL(querier->reporting_client(), OnRecoverableError(_)).Times(1);

  // Call to apply record changes. The specifics are unimportant.
  EXPECT_CALL(mock_graph, ApplyDataRecordChange(_, _, _, _))
      .WillOnce(Return(Error::None()));

  // Call with any record. The mocks make the specifics unimportant.
  querier->OnRecordChanged(GetFakePtrRecord(name),
                           RecordChangedEvent::kCreated);
}

TEST_F(DnsSdQuerierImplTest, ResultsWithMultipleAddressRecordsHandled) {
  IPEndpoint endpointa{{192, 168, 86, 23}, 80};
  IPEndpoint endpointb{{1, 2, 3, 4, 5, 6, 7, 8}, 80};
  IPEndpoint endpointc{{192, 168, 0, 1}, 80};
  IPEndpoint endpointd{{192, 168, 0, 2}, 80};
  IPEndpoint endpointe{{192, 168, 0, 3}, 80};

  DnsSdInstanceEndpoint instance1("instance1", "_service._udp", "local", {},
                                  kNetworkInterface, {endpointa, endpointb});
  DnsSdInstanceEndpoint instance2("instance2", "_service2._udp", "local", {},
                                  kNetworkInterface, {endpointa, endpointb});
  DnsSdInstanceEndpoint instance3("instance3", "_service._udp", "local", {},
                                  kNetworkInterface, {endpointc});
  DnsSdInstanceEndpoint instance4("instance1", "_service3._udp", "local", {},
                                  kNetworkInterface, {endpointd, endpointe});
  DnsSdInstanceEndpoint instance5("instance1", "_service3._udp", "local", {},
                                  kNetworkInterface, {endpointe});

  MockDnsDataGraph& mock_graph = querier->GetMockedGraph();
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> before_changes{};
  before_changes.emplace_back(instance4);
  before_changes.emplace_back(instance2);
  before_changes.emplace_back(instance3);
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> after_changes{};
  after_changes.emplace_back(instance5);
  after_changes.emplace_back(instance3);
  after_changes.emplace_back(instance1);

  // Calls before and after applying record changes, then the error it logs.
  EXPECT_CALL(mock_graph, CreateEndpoints(_, _))
      .WillOnce(Return(ByMove(std::move(before_changes))))
      .WillOnce(Return(ByMove(std::move(after_changes))));
  EXPECT_CALL(callback, OnEndpointCreated(instance1));
  EXPECT_CALL(callback, OnEndpointUpdated(instance5));
  EXPECT_CALL(callback, OnEndpointDeleted(instance2));

  // Call to apply record changes. The specifics are unimportant.
  EXPECT_CALL(mock_graph, ApplyDataRecordChange(_, _, _, _))
      .WillOnce(Return(Error::None()));

  // Call with any record. The mocks make the specifics unimportant.
  querier->OnRecordChanged(GetFakePtrRecord(name),
                           RecordChangedEvent::kCreated);
}

TEST_F(DnsSdQuerierImplTest, MixOfErrorsAndSuccessesHandledCorrectly) {
  DnsSdInstanceEndpoint instance1("instance1", "_service._udp", "local", {},
                                  kNetworkInterface, {{{192, 168, 2, 24}, 80}});
  DnsSdInstanceEndpoint instance2("instance2", "_service2._udp", "local", {},
                                  kNetworkInterface, {{{192, 168, 17, 2}, 80}});
  DnsSdInstanceEndpoint instance3("instance3", "_service._udp", "local", {},
                                  kNetworkInterface, {{{127, 0, 0, 1}, 80}});
  DnsSdInstanceEndpoint instance4("instance1", "_service3._udp", "local", {},
                                  kNetworkInterface, {{{127, 0, 0, 1}, 80}});
  DnsSdInstanceEndpoint instance5("instance1", "_service3._udp", "local", {},
                                  kNetworkInterface,
                                  {{{127, 0, 0, 1}, 80}, {{127, 0, 0, 2}, 80}});

  MockDnsDataGraph& mock_graph = querier->GetMockedGraph();
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> before_changes{};
  before_changes.emplace_back(Error::Code::kIndexOutOfBounds);
  before_changes.emplace_back(instance2);
  before_changes.emplace_back(Error::Code::kItemAlreadyExists);
  before_changes.emplace_back(Error::Code::kOperationCancelled);
  before_changes.emplace_back(instance1);
  before_changes.emplace_back(Error::Code::kItemNotFound);
  before_changes.emplace_back(Error::Code::kOperationInProgress);
  before_changes.emplace_back(instance4);
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> after_changes{};
  after_changes.emplace_back(instance1);
  after_changes.emplace_back(Error::Code::kOperationInProgress);
  after_changes.emplace_back(Error::Code::kUnknownError);
  after_changes.emplace_back(Error::Code::kItemNotFound);
  after_changes.emplace_back(Error::Code::kItemAlreadyExists);
  after_changes.emplace_back(instance3);
  after_changes.emplace_back(instance5);
  after_changes.emplace_back(Error::Code::kOperationCancelled);

  // Calls before and after applying record changes, then the error it logs.
  EXPECT_CALL(mock_graph, CreateEndpoints(_, _))
      .WillOnce(Return(ByMove(std::move(before_changes))))
      .WillOnce(Return(ByMove(std::move(after_changes))));
  EXPECT_CALL(querier->reporting_client(), OnRecoverableError(_)).Times(1);
  EXPECT_CALL(callback, OnEndpointCreated(instance3));
  EXPECT_CALL(callback, OnEndpointUpdated(instance5));
  EXPECT_CALL(callback, OnEndpointDeleted(instance2));

  // Call to apply record changes. The specifics are unimportant.
  EXPECT_CALL(mock_graph, ApplyDataRecordChange(_, _, _, _))
      .WillOnce(Return(Error::None()));

  // Call with any record. The mocks make the specifics unimportant.
  querier->OnRecordChanged(GetFakePtrRecord(name),
                           RecordChangedEvent::kCreated);
}

}  // namespace discovery
}  // namespace openscreen
