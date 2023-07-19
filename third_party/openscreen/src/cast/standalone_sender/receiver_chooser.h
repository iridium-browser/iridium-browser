// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_SENDER_RECEIVER_CHOOSER_H_
#define CAST_STANDALONE_SENDER_RECEIVER_CHOOSER_H_

#include <functional>
#include <memory>
#include <vector>

#include "cast/common/public/receiver_info.h"
#include "discovery/common/reporting_client.h"
#include "discovery/public/dns_sd_service_factory.h"
#include "discovery/public/dns_sd_service_watcher.h"
#include "platform/api/network_interface.h"
#include "platform/api/serial_delete_ptr.h"
#include "platform/api/task_runner.h"
#include "platform/base/ip_address.h"
#include "util/alarm.h"
#include "util/chrono_helpers.h"

namespace openscreen {
namespace cast {

// Discovers Cast Receivers on the LAN for a given network interface, and
// provides a console menu interface for the user to choose one.
class ReceiverChooser final : public discovery::ReportingClient {
 public:
  using ResultCallback = std::function<void(IPEndpoint)>;

  ReceiverChooser(const InterfaceInfo& interface,
                  TaskRunner* task_runner,
                  ResultCallback result_callback);

  ~ReceiverChooser() final;

 private:
  // discovery::ReportingClient implementation.
  void OnFatalError(Error error) final;
  void OnRecoverableError(Error error) final;

  // Called from the DnsWatcher with |all| ReceiverInfos any time there is a
  // change in the set of discovered devices.
  void OnDnsWatcherUpdate(
      std::vector<std::reference_wrapper<const ReceiverInfo>> all);

  // Called from |menu_alarm_| when it is a good time for the user to choose
  // from the discovered-so-far set of Cast Receivers.
  void PrintMenuAndHandleChoice();

  ResultCallback result_callback_;
  SerialDeletePtr<discovery::DnsSdService> service_;
  std::unique_ptr<discovery::DnsSdServiceWatcher<ReceiverInfo>> watcher_;
  std::vector<ReceiverInfo> discovered_receivers_;
  Alarm menu_alarm_;

  // After there is another Cast Receiver discovered, ready to show to the user
  // via the console menu, how long should the ReceiverChooser wait for
  // additional receivers to be discovered and be included in the menu too?
  static constexpr auto kWaitForStragglersDelay = seconds(5);
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_SENDER_RECEIVER_CHOOSER_H_
