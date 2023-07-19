// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_sender/receiver_chooser.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>

#include "discovery/common/config.h"
#include "platform/api/time.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

// NOTE: the compile requires a definition as well as the declaration
// in the header.
// TODO(issuetracker.google.com/174081818): move to inline C++17 feature.
constexpr decltype(ReceiverChooser::kWaitForStragglersDelay)
    ReceiverChooser::kWaitForStragglersDelay;

ReceiverChooser::ReceiverChooser(const InterfaceInfo& interface,
                                 TaskRunner* task_runner,
                                 ResultCallback result_callback)
    : result_callback_(std::move(result_callback)),
      menu_alarm_(&Clock::now, task_runner) {
  discovery::Config config{.network_info = {interface},
                           .enable_publication = false,
                           .enable_querying = true};
  service_ =
      discovery::CreateDnsSdService(task_runner, this, std::move(config));

  watcher_ = std::make_unique<discovery::DnsSdServiceWatcher<ReceiverInfo>>(
      service_.get(), kCastV2ServiceId, DnsSdInstanceEndpointToReceiverInfo,
      [this](std::vector<std::reference_wrapper<const ReceiverInfo>> all) {
        OnDnsWatcherUpdate(std::move(all));
      });

  OSP_LOG_INFO << "Starting discovery. Note that it can take dozens of seconds "
                  "to detect anything on some networks!";
  task_runner->PostTask([this] { watcher_->StartDiscovery(); });
}

ReceiverChooser::~ReceiverChooser() = default;

void ReceiverChooser::OnFatalError(Error error) {
  OSP_LOG_FATAL << "Fatal error: " << error;
}

void ReceiverChooser::OnRecoverableError(Error error) {
  OSP_VLOG << "Recoverable error: " << error;
}

void ReceiverChooser::OnDnsWatcherUpdate(
    std::vector<std::reference_wrapper<const ReceiverInfo>> all) {
  bool added_some = false;
  for (const ReceiverInfo& info : all) {
    if (!info.IsValid() || (!info.v4_address && !info.v6_address)) {
      continue;
    }
    const std::string& instance_id = info.GetInstanceId();
    if (std::any_of(discovered_receivers_.begin(), discovered_receivers_.end(),
                    [&](const ReceiverInfo& known) {
                      return known.GetInstanceId() == instance_id;
                    })) {
      continue;
    }

    OSP_LOG_INFO << "Discovered: " << info.friendly_name
                 << " (id: " << instance_id << ')';
    discovered_receivers_.push_back(info);
    added_some = true;
  }

  if (added_some) {
    menu_alarm_.ScheduleFromNow([this] { PrintMenuAndHandleChoice(); },
                                kWaitForStragglersDelay);
  }
}

void ReceiverChooser::PrintMenuAndHandleChoice() {
  if (!result_callback_) {
    return;  // A choice has already been made.
  }

  std::cout << '\n';
  for (size_t i = 0; i < discovered_receivers_.size(); ++i) {
    const ReceiverInfo& info = discovered_receivers_[i];
    std::cout << '[' << i << "]: " << info.friendly_name << " @ ";
    if (info.v4_address) {
      std::cout << info.v4_address;
    } else {
      OSP_DCHECK(info.v6_address);
      std::cout << info.v6_address;
    }
    std::cout << ':' << info.port << '\n';
  }
  std::cout << "\nEnter choice, or 'n' to wait longer: " << std::flush;

  int menu_choice = -1;
  if (std::cin >> menu_choice || std::cin.eof()) {
    const auto callback_on_stack = std::move(result_callback_);
    if (menu_choice >= 0 &&
        menu_choice < static_cast<int>(discovered_receivers_.size())) {
      const ReceiverInfo& choice = discovered_receivers_[menu_choice];
      if (choice.v4_address) {
        callback_on_stack(IPEndpoint{choice.v4_address, choice.port});
      } else {
        callback_on_stack(IPEndpoint{choice.v6_address, choice.port});
      }
    } else {
      callback_on_stack(IPEndpoint{});  // Signal "bad choice" or EOF.
    }
    return;
  }

  // Clear bad input flag, and skip past what the user entered.
  std::cin.clear();
  std::string garbage;
  std::getline(std::cin, garbage);
}

}  // namespace cast
}  // namespace openscreen
