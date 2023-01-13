// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <atomic>
#include <functional>
#include <map>
#include <string>

// NOTE: although we use gtest here, prefer OSP_CHECKs to
// ASSERTS due to asynchronous concerns around test failures.
// Although this causes the entire test binary to fail instead of
// just a single test, it makes debugging easier/possible.
#include "cast/common/public/receiver_info.h"
#include "discovery/common/config.h"
#include "discovery/common/reporting_client.h"
#include "discovery/public/dns_sd_service_factory.h"
#include "discovery/public/dns_sd_service_publisher.h"
#include "discovery/public/dns_sd_service_watcher.h"
#include "gtest/gtest.h"
#include "platform/api/udp_socket.h"
#include "platform/base/interface_info.h"
#include "platform/impl/network_interface.h"
#include "platform/impl/platform_client_posix.h"
#include "platform/impl/task_runner.h"
#include "testing/util/task_util.h"
#include "util/chrono_helpers.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {
namespace cast {
namespace {

// Maximum amount of time needed for a query to be received.
constexpr seconds kMaxQueryDuration{3};

// Total wait time = 4 seconds.
constexpr milliseconds kWaitLoopSleepTime(500);
constexpr int kMaxWaitLoopIterations = 8;

// Total wait time = 2.5 seconds.
// NOTE: This must be less than the above wait time.
constexpr milliseconds kCheckLoopSleepTime(100);
constexpr int kMaxCheckLoopIterations = 25;

// Publishes new service instances.
class Publisher : public discovery::DnsSdServicePublisher<ReceiverInfo> {
 public:
  explicit Publisher(discovery::DnsSdService* service)  // NOLINT
      : DnsSdServicePublisher<ReceiverInfo>(service,
                                            kCastV2ServiceId,
                                            ReceiverInfoToDnsSdInstance) {
    OSP_LOG_INFO << "Initializing Publisher...\n";
  }

  ~Publisher() override = default;

  bool IsInstanceIdClaimed(const std::string& requested_id) {
    return !Contains(instance_ids_, requested_id);
  }

 private:
  // DnsSdPublisher::Client overrides.
  void OnInstanceClaimed(const std::string& requested_id) override {
    instance_ids_.push_back(requested_id);
  }

  std::vector<std::string> instance_ids_;
};

// Receives incoming services and outputs their results to stdout.
class ServiceReceiver : public discovery::DnsSdServiceWatcher<ReceiverInfo> {
 public:
  explicit ServiceReceiver(discovery::DnsSdService* service)  // NOLINT
      : discovery::DnsSdServiceWatcher<ReceiverInfo>(
            service,
            kCastV2ServiceId,
            DnsSdInstanceEndpointToReceiverInfo,
            [this](
                std::vector<std::reference_wrapper<const ReceiverInfo>> infos) {
              ProcessResults(std::move(infos));
            }) {
    OSP_LOG_INFO << "Initializing ServiceReceiver...";
  }

  bool IsServiceFound(const ReceiverInfo& check_service) {
    return ContainsIf(
        receiver_infos_, [&check_service](const ReceiverInfo& info) {
          return info.friendly_name == check_service.friendly_name;
        });
  }

  void EraseReceivedServices() { receiver_infos_.clear(); }

 private:
  void ProcessResults(
      std::vector<std::reference_wrapper<const ReceiverInfo>> infos) {
    receiver_infos_.clear();
    for (const ReceiverInfo& info : infos) {
      receiver_infos_.push_back(info);
    }
  }

  std::vector<ReceiverInfo> receiver_infos_;
};

class FailOnErrorReporting : public discovery::ReportingClient {
  void OnFatalError(Error error) override {
    OSP_LOG_FATAL << "Fatal error received: '" << error << "'";
    OSP_NOTREACHED();
  }

  void OnRecoverableError(Error error) override {
    // Pending resolution of openscreen:105, logging recoverable errors is
    // disabled, as this will end up polluting the output with logs related to
    // mDNS messages received from non-loopback network interfaces over which
    // we have no control.
  }
};

discovery::Config GetConfigSettings() {
  // Get the loopback interface to run on.
  InterfaceInfo loopback = GetLoopbackInterfaceForTesting().value();
  OSP_LOG_INFO << "Selected network interface for testing: " << loopback;
  return discovery::Config{{std::move(loopback)}};
}

class DiscoveryE2ETest : public testing::Test {
 public:
  DiscoveryE2ETest() {
    // Sleep to let any packets clear off the network before further tests.
    std::this_thread::sleep_for(milliseconds(500));

    PlatformClientPosix::Create(milliseconds(50));
    task_runner_ = PlatformClientPosix::GetInstance()->GetTaskRunner();
  }

  ~DiscoveryE2ETest() {
    OSP_LOG_INFO << "TEST COMPLETE!";
    dnssd_service_.reset();
    PlatformClientPosix::ShutDown();
  }

 protected:
  ReceiverInfo GetInfo(int id) {
    ReceiverInfo hosted_service;
    hosted_service.port = 1234;
    hosted_service.unique_id = "id" + std::to_string(id);
    hosted_service.model_name = "openscreen-Model" + std::to_string(id);
    hosted_service.friendly_name = "Demo" + std::to_string(id);
    return hosted_service;
  }

  void SetUpService(const discovery::Config& config) {
    OSP_DCHECK(!dnssd_service_.get());
    std::atomic_bool done{false};
    task_runner_->PostTask([this, &config, &done]() {
      dnssd_service_ = discovery::CreateDnsSdService(
          task_runner_, &reporting_client_, config);
      receiver_ = std::make_unique<ServiceReceiver>(dnssd_service_.get());
      publisher_ = std::make_unique<Publisher>(dnssd_service_.get());
      done = true;
    });
    WaitForCondition([&done]() { return done.load(); }, kWaitLoopSleepTime,
                     kMaxWaitLoopIterations);
    OSP_CHECK(done);
  }

  void StartDiscovery() {
    OSP_DCHECK(dnssd_service_.get());
    task_runner_->PostTask([this]() { receiver_->StartDiscovery(); });
  }

  template <typename... RecordTypes>
  void UpdateRecords(RecordTypes... records) {
    OSP_DCHECK(dnssd_service_.get());
    OSP_DCHECK(publisher_.get());

    std::vector<ReceiverInfo> record_set{std::move(records)...};
    for (ReceiverInfo& record : record_set) {
      task_runner_->PostTask([this, r = std::move(record)]() {
        auto error = publisher_->UpdateRegistration(r);
        OSP_CHECK(error.ok()) << "\tFailed to update service instance '"
                              << r.friendly_name << "': " << error << "!";
      });
    }
  }

  template <typename... RecordTypes>
  void PublishRecords(RecordTypes... records) {
    OSP_DCHECK(dnssd_service_.get());
    OSP_DCHECK(publisher_.get());

    std::vector<ReceiverInfo> record_set{std::move(records)...};
    for (ReceiverInfo& record : record_set) {
      task_runner_->PostTask([this, r = std::move(record)]() {
        auto error = publisher_->Register(r);
        OSP_CHECK(error.ok()) << "\tFailed to publish service instance '"
                              << r.friendly_name << "': " << error << "!";
      });
    }
  }

  template <typename... AtomicBoolPtrs>
  void WaitUntilSeen(bool should_be_seen, AtomicBoolPtrs... bools) {
    OSP_DCHECK(dnssd_service_.get());
    std::vector<std::atomic_bool*> atomic_bools{bools...};

    int waiting_on = atomic_bools.size();
    for (int i = 0; i < kMaxWaitLoopIterations; i++) {
      waiting_on = atomic_bools.size();
      for (std::atomic_bool* atomic : atomic_bools) {
        if (*atomic) {
          OSP_CHECK(should_be_seen) << "Found service instance!";
          waiting_on--;
        }
      }

      if (waiting_on) {
        OSP_LOG_INFO << "\tWaiting on " << waiting_on << "...";
        std::this_thread::sleep_for(kWaitLoopSleepTime);
        continue;
      }
      return;
    }
    OSP_CHECK(!should_be_seen)
        << "Could not find " << waiting_on << " service instances!";
  }

  void CheckForClaimedIds(ReceiverInfo receiver_info,
                          std::atomic_bool* has_been_seen) {
    OSP_DCHECK(dnssd_service_.get());
    task_runner_->PostTask(
        [this, info = std::move(receiver_info), has_been_seen]() mutable {
          CheckForClaimedIds(std::move(info), has_been_seen, 0);
        });
  }

  void CheckForPublishedService(ReceiverInfo receiver_info,
                                std::atomic_bool* has_been_seen) {
    OSP_DCHECK(dnssd_service_.get());
    task_runner_->PostTask(
        [this, info = std::move(receiver_info), has_been_seen]() mutable {
          CheckForPublishedService(std::move(info), has_been_seen, 0, true);
        });
  }

  // TODO(issuetracker.google.com/159256503): Change this to use a polling
  // method to wait until the service disappears rather than immediately failing
  // if it exists, so waits throughout this file can be removed.
  void CheckNotPublishedService(ReceiverInfo receiver_info,
                                std::atomic_bool* has_been_seen) {
    OSP_DCHECK(dnssd_service_.get());
    task_runner_->PostTask(
        [this, info = std::move(receiver_info), has_been_seen]() mutable {
          CheckForPublishedService(std::move(info), has_been_seen, 0, false);
        });
  }
  TaskRunner* task_runner_;
  FailOnErrorReporting reporting_client_;
  SerialDeletePtr<discovery::DnsSdService> dnssd_service_;
  std::unique_ptr<ServiceReceiver> receiver_;
  std::unique_ptr<Publisher> publisher_;

 private:
  void CheckForClaimedIds(ReceiverInfo receiver_info,
                          std::atomic_bool* has_been_seen,
                          int attempts) {
    if (publisher_->IsInstanceIdClaimed(receiver_info.GetInstanceId())) {
      // TODO(crbug.com/openscreen/110): Log the published service instance.
      *has_been_seen = true;
      return;
    }

    OSP_CHECK_LE(attempts++, kMaxCheckLoopIterations)
        << "Service " << receiver_info.friendly_name << " publication failed.";
    task_runner_->PostTaskWithDelay(
        [this, info = std::move(receiver_info), has_been_seen,
         attempts]() mutable {
          CheckForClaimedIds(std::move(info), has_been_seen, attempts);
        },
        kCheckLoopSleepTime);
  }

  void CheckForPublishedService(ReceiverInfo receiver_info,
                                std::atomic_bool* has_been_seen,
                                int attempts,
                                bool expect_to_be_present) {
    if (!receiver_->IsServiceFound(receiver_info)) {
      if (attempts++ > kMaxCheckLoopIterations) {
        OSP_CHECK(!expect_to_be_present)
            << "Service " << receiver_info.friendly_name
            << " discovery failed.";
        return;
      }
      task_runner_->PostTaskWithDelay(
          [this, info = std::move(receiver_info), has_been_seen, attempts,
           expect_to_be_present]() mutable {
            CheckForPublishedService(std::move(info), has_been_seen, attempts,
                                     expect_to_be_present);
          },
          kCheckLoopSleepTime);
    } else if (expect_to_be_present) {
      // TODO(crbug.com/openscreen/110): Log the discovered service instance.
      *has_been_seen = true;
    } else {
      OSP_LOG_FATAL << "Found instance '" << receiver_info.friendly_name
                    << "'!";
    }
  }
};

// The below runs an E2E tests. These test requires no user interaction and is
// intended to perform a set series of actions to validate that discovery is
// functioning as intended.
//
// Known issues:
// - The ipv6 socket in discovery/mdns/service_impl.cc fails to bind to an ipv6
//   address on the loopback interface. Investigating this issue is pending
//   resolution of bug
//   https://bugs.chromium.org/p/openscreen/issues/detail?id=105.
//
// In this test, the following operations are performed:
// 1) Start up the Cast platform for a posix system.
// 2) Publish 3 CastV2 service instances to the loopback interface using mDNS,
//    with record announcement disabled.
// 3) Wait for the probing phase to successfully complete.
// 4) Query for records published over the loopback interface, and validate that
//    all 3 previously published services are discovered.
TEST_F(DiscoveryE2ETest, ValidateQueryFlow) {
  // Set up demo infra.
  auto discovery_config = GetConfigSettings();
  discovery_config.new_record_announcement_count = 0;
  SetUpService(discovery_config);

  auto instance1 = GetInfo(1);
  auto instance2 = GetInfo(2);
  auto instance3 = GetInfo(3);

  // Start discovery and publication.
  StartDiscovery();
  PublishRecords(instance1, instance2, instance3);

  // Wait until all probe phases complete and all instance ids are claimed. At
  // this point, all records should be published.
  OSP_LOG_INFO << "Service publication in progress...";
  std::atomic_bool found1{false};
  std::atomic_bool found2{false};
  std::atomic_bool found3{false};
  CheckForClaimedIds(instance1, &found1);
  CheckForClaimedIds(instance2, &found2);
  CheckForClaimedIds(instance3, &found3);
  WaitUntilSeen(true, &found1, &found2, &found3);
  OSP_LOG_INFO << "\tAll services successfully published!\n";

  // Make sure all services are found through discovery.
  OSP_LOG_INFO << "Service discovery in progress...";
  found1 = false;
  found2 = false;
  found3 = false;
  CheckForPublishedService(instance1, &found1);
  CheckForPublishedService(instance2, &found2);
  CheckForPublishedService(instance3, &found3);
  WaitUntilSeen(true, &found1, &found2, &found3);
}

// In this test, the following operations are performed:
// 1) Start up the Cast platform for a posix system.
// 2) Start service discovery and new queries, with no query messages being
//    sent.
// 3) Publish 3 CastV2 service instances to the loopback interface using mDNS,
//    with record announcement enabled.
// 4) Ensure the correct records were published over the loopback interface.
// 5) De-register all services.
// 6) Ensure that goodbye records are received for all service instances.
TEST_F(DiscoveryE2ETest, ValidateAnnouncementFlow) {
  // Set up demo infra.
  auto discovery_config = GetConfigSettings();
  discovery_config.new_query_announcement_count = 0;
  SetUpService(discovery_config);

  auto instance1 = GetInfo(1);
  auto instance2 = GetInfo(2);
  auto instance3 = GetInfo(3);

  // Start discovery and publication.
  StartDiscovery();
  PublishRecords(instance1, instance2, instance3);

  // Wait until all probe phases complete and all instance ids are claimed. At
  // this point, all records should be published.
  OSP_LOG_INFO << "Service publication in progress...";
  std::atomic_bool found1{false};
  std::atomic_bool found2{false};
  std::atomic_bool found3{false};
  CheckForClaimedIds(instance1, &found1);
  CheckForClaimedIds(instance2, &found2);
  CheckForClaimedIds(instance3, &found3);
  WaitUntilSeen(true, &found1, &found2, &found3);
  OSP_LOG_INFO << "\tAll services successfully published and announced!\n";

  // Make sure all services are found through discovery.
  OSP_LOG_INFO << "Service discovery in progress...";
  found1 = false;
  found2 = false;
  found3 = false;
  CheckForPublishedService(instance1, &found1);
  CheckForPublishedService(instance2, &found2);
  CheckForPublishedService(instance3, &found3);
  WaitUntilSeen(true, &found1, &found2, &found3);
  OSP_LOG_INFO << "\tAll services successfully discovered!\n";

  // Deregister all service instances.
  OSP_LOG_INFO << "Deregister all services...";
  task_runner_->PostTask([this]() {
    ErrorOr<int> result = publisher_->DeregisterAll();
    ASSERT_FALSE(result.is_error());
    ASSERT_EQ(result.value(), 3);
  });
  std::this_thread::sleep_for(seconds(3));
  found1 = false;
  found2 = false;
  found3 = false;
  CheckNotPublishedService(instance1, &found1);
  CheckNotPublishedService(instance2, &found2);
  CheckNotPublishedService(instance3, &found3);
  WaitUntilSeen(false, &found1, &found2, &found3);
}

// In this test, the following operations are performed:
// 1) Start up the Cast platform for a posix system.
// 2) Publish one service and ensure it is NOT received.
// 3) Start service discovery and new queries.
// 4) Ensure above published service IS received.
// 5) Stop the started query.
// 6) Update a service, and ensure that no callback is received.
// 7) Restart the query and ensure that only the expected callbacks are
// received.
TEST_F(DiscoveryE2ETest, ValidateRecordsOnlyReceivedWhenQueryRunning) {
  // Set up demo infra.
  auto discovery_config = GetConfigSettings();
  discovery_config.new_record_announcement_count = 1;
  SetUpService(discovery_config);

  auto instance = GetInfo(1);

  // Start discovery and publication.
  PublishRecords(instance);

  // Wait until all probe phases complete and all instance ids are claimed. At
  // this point, all records should be published.
  OSP_LOG_INFO << "Service publication in progress...";
  std::atomic_bool found{false};
  CheckForClaimedIds(instance, &found);
  WaitUntilSeen(true, &found);

  // And ensure stopped discovery does not find the records.
  OSP_LOG_INFO
      << "Validating no service discovery occurs when discovery stopped...";
  found = false;
  CheckNotPublishedService(instance, &found);
  WaitUntilSeen(false, &found);

  // Make sure all services are found through discovery.
  StartDiscovery();
  OSP_LOG_INFO << "Service discovery in progress...";
  found = false;
  CheckForPublishedService(instance, &found);
  WaitUntilSeen(true, &found);

  // Update discovery and ensure that the updated service is seen.
  OSP_LOG_INFO << "Updating service and waiting for discovery...";
  auto updated_instance = instance;
  updated_instance.friendly_name = "OtherName";
  found = false;
  UpdateRecords(updated_instance);
  CheckForPublishedService(updated_instance, &found);
  WaitUntilSeen(true, &found);

  // And ensure the old service has been removed.
  found = false;
  CheckNotPublishedService(instance, &found);
  WaitUntilSeen(false, &found);

  // Stop discovery.
  OSP_LOG_INFO << "Stopping discovery...";
  task_runner_->PostTask([this]() { receiver_->StopDiscovery(); });

  // Update discovery and ensure that the updated service is NOT seen.
  OSP_LOG_INFO
      << "Updating service and validating the change isn't received...";
  found = false;
  instance.friendly_name = "ThirdName";
  UpdateRecords(instance);
  CheckNotPublishedService(instance, &found);
  WaitUntilSeen(false, &found);

  StartDiscovery();
  std::this_thread::sleep_for(kMaxQueryDuration);

  OSP_LOG_INFO << "Service discovery in progress...";
  found = false;
  CheckNotPublishedService(updated_instance, &found);
  WaitUntilSeen(false, &found);

  found = false;
  CheckForPublishedService(instance, &found);
  WaitUntilSeen(true, &found);
}

// In this test, the following operations are performed:
// 1) Start up the Cast platform for a posix system.
// 2) Start service discovery and new queries.
// 3) Publish one service and ensure it is received.
// 4) Hard reset discovery
// 5) Ensure the same service is discovered
// 6) Soft reset the service, and ensure that a callback is received.
TEST_F(DiscoveryE2ETest, ValidateRefreshFlow) {
  // Set up demo infra.
  // NOTE: This configuration assumes that packets cannot be lost over the
  // loopback interface.
  auto discovery_config = GetConfigSettings();
  discovery_config.new_record_announcement_count = 0;
  discovery_config.new_query_announcement_count = 2;
  SetUpService(discovery_config);

  auto instance = GetInfo(1);

  // Start discovery and publication.
  StartDiscovery();
  PublishRecords(instance);

  // Wait until all probe phases complete and all instance ids are claimed. At
  // this point, all records should be published.
  OSP_LOG_INFO << "Service publication in progress...";
  std::atomic_bool found{false};
  CheckForClaimedIds(instance, &found);
  WaitUntilSeen(true, &found);

  // Make sure all services are found through discovery.
  OSP_LOG_INFO << "Service discovery in progress...";
  found = false;
  CheckForPublishedService(instance, &found);
  WaitUntilSeen(true, &found);

  // Force refresh discovery, then ensure that the published service is
  // re-discovered.
  OSP_LOG_INFO << "Force refresh discovery...";
  task_runner_->PostTask([this]() { receiver_->EraseReceivedServices(); });
  std::this_thread::sleep_for(kMaxQueryDuration);
  found = false;
  CheckNotPublishedService(instance, &found);
  WaitUntilSeen(false, &found);
  task_runner_->PostTask([this]() { receiver_->ForceRefresh(); });

  OSP_LOG_INFO << "Ensure that the published service is re-discovered...";
  found = false;
  CheckForPublishedService(instance, &found);
  WaitUntilSeen(true, &found);

  // Soft refresh discovery, then ensure that the published service is NOT
  // re-discovered.
  OSP_LOG_INFO << "Call DiscoverNow on discovery...";
  task_runner_->PostTask([this]() { receiver_->EraseReceivedServices(); });
  std::this_thread::sleep_for(kMaxQueryDuration);
  found = false;
  CheckNotPublishedService(instance, &found);
  WaitUntilSeen(false, &found);
  task_runner_->PostTask([this]() { receiver_->DiscoverNow(); });

  OSP_LOG_INFO << "Ensure that the published service is re-discovered...";
  found = false;
  CheckForPublishedService(instance, &found);
  WaitUntilSeen(true, &found);
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
