// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "osp/msgs/osp_messages.h"
#include "osp/public/mdns_service_listener_factory.h"
#include "osp/public/message_demuxer.h"
#include "osp/public/network_service_manager.h"
#include "osp/public/presentation/presentation_controller.h"
#include "osp/public/presentation/presentation_receiver.h"
#include "osp/public/protocol_connection_client.h"
#include "osp/public/protocol_connection_client_factory.h"
#include "osp/public/protocol_connection_server.h"
#include "osp/public/protocol_connection_server_factory.h"
#include "osp/public/service_listener.h"
#include "osp/public/service_publisher.h"
#include "osp/public/service_publisher_factory.h"
#include "platform/api/network_interface.h"
#include "platform/api/time.h"
#include "platform/impl/logging.h"
#include "platform/impl/platform_client_posix.h"
#include "platform/impl/task_runner.h"
#include "platform/impl/text_trace_logging_platform.h"
#include "platform/impl/udp_socket_reader_posix.h"
#include "third_party/tinycbor/src/src/cbor.h"
#include "util/trace_logging.h"

namespace {

const char* kReceiverLogFilename = "_recv_fifo";
const char* kControllerLogFilename = "_cntl_fifo";

bool g_done = false;
bool g_dump_services = false;

void sigusr1_dump_services(int) {
  g_dump_services = true;
}

void sigint_stop(int) {
  OSP_LOG_INFO << "caught SIGINT, exiting...";
  g_done = true;
}

void SignalThings() {
  struct sigaction usr1_sa;
  struct sigaction int_sa;
  struct sigaction unused;

  usr1_sa.sa_handler = &sigusr1_dump_services;
  sigemptyset(&usr1_sa.sa_mask);
  usr1_sa.sa_flags = 0;

  int_sa.sa_handler = &sigint_stop;
  sigemptyset(&int_sa.sa_mask);
  int_sa.sa_flags = 0;

  sigaction(SIGUSR1, &usr1_sa, &unused);
  sigaction(SIGINT, &int_sa, &unused);

  OSP_LOG_INFO << "signal handlers setup" << std::endl << "pid: " << getpid();
}

}  // namespace

namespace openscreen {
namespace osp {

class DemoListenerObserver final : public ServiceListener::Observer {
 public:
  ~DemoListenerObserver() override = default;
  void OnStarted() override { OSP_LOG_INFO << "listener started!"; }
  void OnStopped() override { OSP_LOG_INFO << "listener stopped!"; }
  void OnSuspended() override { OSP_LOG_INFO << "listener suspended!"; }
  void OnSearching() override { OSP_LOG_INFO << "listener searching!"; }

  void OnReceiverAdded(const ServiceInfo& info) override {
    OSP_LOG_INFO << "found! " << info.friendly_name;
  }
  void OnReceiverChanged(const ServiceInfo& info) override {
    OSP_LOG_INFO << "changed! " << info.friendly_name;
  }
  void OnReceiverRemoved(const ServiceInfo& info) override {
    OSP_LOG_INFO << "removed! " << info.friendly_name;
  }
  void OnAllReceiversRemoved() override { OSP_LOG_INFO << "all removed!"; }
  void OnError(ServiceListenerError) override {}
  void OnMetrics(ServiceListener::Metrics) override {}
};

std::string SanitizeServiceId(absl::string_view service_id) {
  std::string safe_service_id(service_id);
  for (auto& c : safe_service_id) {
    if (c < ' ' || c > '~') {
      c = '.';
    }
  }
  return safe_service_id;
}

class DemoReceiverObserver final : public ReceiverObserver {
 public:
  ~DemoReceiverObserver() override = default;

  void OnRequestFailed(const std::string& presentation_url,
                       const std::string& service_id) override {
    std::string safe_service_id = SanitizeServiceId(service_id);
    OSP_LOG_WARN << "request failed: (" << presentation_url << ", "
                 << safe_service_id << ")";
  }
  void OnReceiverAvailable(const std::string& presentation_url,
                           const std::string& service_id) override {
    std::string safe_service_id = SanitizeServiceId(service_id);
    safe_service_ids_.emplace(safe_service_id, service_id);
    OSP_LOG_INFO << "available! " << safe_service_id;
  }
  void OnReceiverUnavailable(const std::string& presentation_url,
                             const std::string& service_id) override {
    std::string safe_service_id = SanitizeServiceId(service_id);
    safe_service_ids_.erase(safe_service_id);
    OSP_LOG_INFO << "unavailable! " << safe_service_id;
  }

  const std::string& GetServiceId(const std::string& safe_service_id) {
    OSP_DCHECK(safe_service_ids_.find(safe_service_id) !=
               safe_service_ids_.end())
        << safe_service_id << " not found in map";
    return safe_service_ids_[safe_service_id];
  }

 private:
  std::map<std::string, std::string> safe_service_ids_;
};

class DemoPublisherObserver final : public ServicePublisher::Observer {
 public:
  ~DemoPublisherObserver() override = default;

  void OnStarted() override { OSP_LOG_INFO << "publisher started!"; }
  void OnStopped() override { OSP_LOG_INFO << "publisher stopped!"; }
  void OnSuspended() override { OSP_LOG_INFO << "publisher suspended!"; }

  void OnError(Error error) override {
    OSP_LOG_ERROR << "publisher error: " << error;
  }
  void OnMetrics(ServicePublisher::Metrics) override {}
};

class DemoConnectionClientObserver final
    : public ProtocolConnectionServiceObserver {
 public:
  ~DemoConnectionClientObserver() override = default;
  void OnRunning() override {}
  void OnStopped() override {}

  void OnMetrics(const NetworkMetrics& metrics) override {}
  void OnError(const Error& error) override {}
};

class DemoConnectionServerObserver final
    : public ProtocolConnectionServer::Observer {
 public:
  class ConnectionObserver final : public ProtocolConnection::Observer {
   public:
    explicit ConnectionObserver(DemoConnectionServerObserver* parent)
        : parent_(parent) {}
    ~ConnectionObserver() override = default;

    void OnConnectionClosed(const ProtocolConnection& connection) override {
      auto& connections = parent_->connections_;
      connections.erase(
          std::remove_if(
              connections.begin(), connections.end(),
              [this](const std::pair<std::unique_ptr<ConnectionObserver>,
                                     std::unique_ptr<ProtocolConnection>>& p) {
                return p.first.get() == this;
              }),
          connections.end());
    }

   private:
    DemoConnectionServerObserver* const parent_;
  };

  ~DemoConnectionServerObserver() override = default;

  void OnRunning() override {}
  void OnStopped() override {}
  void OnSuspended() override {}

  void OnMetrics(const NetworkMetrics& metrics) override {}
  void OnError(const Error& error) override {}

  void OnIncomingConnection(
      std::unique_ptr<ProtocolConnection> connection) override {
    auto observer = std::make_unique<ConnectionObserver>(this);
    connection->SetObserver(observer.get());
    connections_.emplace_back(std::move(observer), std::move(connection));
    connections_.back().second->CloseWriteEnd();
  }

 private:
  std::vector<std::pair<std::unique_ptr<ConnectionObserver>,
                        std::unique_ptr<ProtocolConnection>>>
      connections_;
};

class DemoRequestDelegate final : public RequestDelegate {
 public:
  DemoRequestDelegate() = default;
  ~DemoRequestDelegate() override = default;

  void OnConnection(std::unique_ptr<Connection> conn) override {
    OSP_LOG_INFO << "request successful";
    connection = std::move(conn);
  }

  void OnError(const Error& error) override {
    OSP_LOG_INFO << "on request error";
  }

  std::unique_ptr<Connection> connection;
};

class DemoConnectionDelegate final : public Connection::Delegate {
 public:
  DemoConnectionDelegate() = default;
  ~DemoConnectionDelegate() override = default;

  void OnConnected() override {
    OSP_LOG_INFO << "presentation connection connected";
  }
  void OnClosedByRemote() override {
    OSP_LOG_INFO << "presentation connection closed by remote";
  }
  void OnDiscarded() override {}
  void OnError(const absl::string_view message) override {}
  void OnTerminated() override { OSP_LOG_INFO << "presentation terminated"; }

  void OnStringMessage(absl::string_view message) override {
    OSP_LOG_INFO << "got message: " << message;
  }
  void OnBinaryMessage(const std::vector<uint8_t>& data) override {}
};

class DemoReceiverConnectionDelegate final : public Connection::Delegate {
 public:
  DemoReceiverConnectionDelegate() = default;
  ~DemoReceiverConnectionDelegate() override = default;

  void OnConnected() override {
    OSP_LOG_INFO << "presentation connection connected";
  }
  void OnClosedByRemote() override {
    OSP_LOG_INFO << "presentation connection closed by remote";
  }
  void OnDiscarded() override {}
  void OnError(const absl::string_view message) override {}
  void OnTerminated() override { OSP_LOG_INFO << "presentation terminated"; }

  void OnStringMessage(const absl::string_view message) override {
    OSP_LOG_INFO << "got message: " << message;
    connection->SendString("--echo-- " + std::string(message));
  }
  void OnBinaryMessage(const std::vector<uint8_t>& data) override {}

  Connection* connection;
};

class DemoReceiverDelegate final : public ReceiverDelegate {
 public:
  ~DemoReceiverDelegate() override = default;

  std::vector<msgs::UrlAvailability> OnUrlAvailabilityRequest(
      uint64_t client_id,
      uint64_t request_duration,
      std::vector<std::string> urls) override {
    std::vector<msgs::UrlAvailability> result;
    result.reserve(urls.size());
    for (const auto& url : urls) {
      OSP_LOG_INFO << "got availability request for: " << url;
      result.push_back(msgs::UrlAvailability::kAvailable);
    }
    return result;
  }

  bool StartPresentation(
      const Connection::PresentationInfo& info,
      uint64_t source_id,
      const std::vector<msgs::HttpHeader>& http_headers) override {
    presentation_id = info.id;
    connection = std::make_unique<Connection>(info, &cd, Receiver::Get());
    cd.connection = connection.get();
    Receiver::Get()->OnPresentationStarted(info.id, connection.get(),
                                           ResponseResult::kSuccess);
    return true;
  }

  bool ConnectToPresentation(uint64_t request_id,
                             const std::string& id,
                             uint64_t source_id) override {
    connection = std::make_unique<Connection>(
        Connection::PresentationInfo{id, connection->presentation_info().url},
        &cd, Receiver::Get());
    cd.connection = connection.get();
    Receiver::Get()->OnConnectionCreated(request_id, connection.get(),
                                         ResponseResult::kSuccess);
    return true;
  }

  void TerminatePresentation(const std::string& id,
                             TerminationReason reason) override {
    Receiver::Get()->OnPresentationTerminated(id, reason);
  }

  std::string presentation_id;
  std::unique_ptr<Connection> connection;
  DemoReceiverConnectionDelegate cd;
};

struct CommandLineSplit {
  std::string command;
  std::string argument_tail;
};

CommandLineSplit SeparateCommandFromArguments(const std::string& line) {
  size_t split_index = line.find_first_of(' ');
  // NOTE: |split_index| can be std::string::npos because not all commands
  // accept arguments.
  std::string command = line.substr(0, split_index);
  std::string argument_tail =
      split_index < line.size() ? line.substr(split_index + 1) : std::string();
  return {std::move(command), std::move(argument_tail)};
}

struct CommandWaitResult {
  bool done;
  CommandLineSplit command_line;
};

CommandWaitResult WaitForCommand(pollfd* pollfd) {
  while (poll(pollfd, 1, 10) >= 0) {
    if (g_done) {
      return {true};
    }

    if (pollfd->revents == 0) {
      continue;
    } else if (pollfd->revents & (POLLERR | POLLHUP)) {
      return {true};
    }

    std::string line;
    if (!std::getline(std::cin, line)) {
      return {true};
    }

    CommandWaitResult result;
    result.done = false;
    result.command_line = SeparateCommandFromArguments(line);
    return result;
  }
  return {true};
}

void RunControllerPollLoop(Controller* controller) {
  DemoReceiverObserver receiver_observer;
  DemoRequestDelegate request_delegate;
  DemoConnectionDelegate connection_delegate;
  Controller::ReceiverWatch watch;
  Controller::ConnectRequest connect_request;

  pollfd stdin_pollfd{STDIN_FILENO, POLLIN};
  while (true) {
    OSP_CHECK_EQ(write(STDOUT_FILENO, "$ ", 2), 2);

    CommandWaitResult command_result = WaitForCommand(&stdin_pollfd);
    if (command_result.done) {
      break;
    }

    if (command_result.command_line.command == "avail") {
      watch = controller->RegisterReceiverWatch(
          {std::string(command_result.command_line.argument_tail)},
          &receiver_observer);
    } else if (command_result.command_line.command == "start") {
      const absl::string_view& argument_tail =
          command_result.command_line.argument_tail;
      size_t next_split = argument_tail.find_first_of(' ');
      const std::string& service_id = receiver_observer.GetServiceId(
          std::string(argument_tail.substr(next_split + 1)));
      const std::string url =
          static_cast<std::string>(argument_tail.substr(0, next_split));
      connect_request = controller->StartPresentation(
          url, service_id, &request_delegate, &connection_delegate);
    } else if (command_result.command_line.command == "msg") {
      request_delegate.connection->SendString(
          command_result.command_line.argument_tail);
    } else if (command_result.command_line.command == "close") {
      request_delegate.connection->Close(Connection::CloseReason::kClosed);
    } else if (command_result.command_line.command == "reconnect") {
      connect_request = controller->ReconnectConnection(
          std::move(request_delegate.connection), &request_delegate);
    } else if (command_result.command_line.command == "term") {
      request_delegate.connection->Terminate(
          TerminationReason::kControllerTerminateCalled);
    }
  }

  watch = Controller::ReceiverWatch();
}

void ListenerDemo() {
  SignalThings();

  DemoListenerObserver listener_observer;
  MdnsServiceListenerConfig listener_config;
  auto mdns_listener = MdnsServiceListenerFactory::Create(
      listener_config, &listener_observer,
      PlatformClientPosix::GetInstance()->GetTaskRunner());

  MessageDemuxer demuxer(Clock::now, MessageDemuxer::kDefaultBufferLimit);
  DemoConnectionClientObserver client_observer;
  auto connection_client = ProtocolConnectionClientFactory::Create(
      &demuxer, &client_observer,
      PlatformClientPosix::GetInstance()->GetTaskRunner());

  auto* network_service = NetworkServiceManager::Create(
      std::move(mdns_listener), nullptr, std::move(connection_client), nullptr);
  auto controller = std::make_unique<Controller>(Clock::now);

  network_service->GetMdnsServiceListener()->Start();
  network_service->GetProtocolConnectionClient()->Start();

  RunControllerPollLoop(controller.get());

  network_service->GetMdnsServiceListener()->Stop();
  network_service->GetProtocolConnectionClient()->Stop();

  controller.reset();

  NetworkServiceManager::Dispose();
}

void HandleReceiverCommand(absl::string_view command,
                           absl::string_view argument_tail,
                           DemoReceiverDelegate& delegate,
                           NetworkServiceManager* manager) {
  if (command == "avail") {
    ServicePublisher* publisher = manager->GetServicePublisher();

    OSP_LOG_INFO << "publisher->state() == "
                 << static_cast<int>(publisher->state());

    if (publisher->state() == ServicePublisher::State::kSuspended) {
      publisher->Resume();
    } else {
      publisher->Suspend();
    }
  } else if (command == "close") {
    delegate.connection->Close(Connection::CloseReason::kClosed);
  } else if (command == "msg") {
    delegate.connection->SendString(argument_tail);
  } else if (command == "term") {
    Receiver::Get()->OnPresentationTerminated(
        delegate.presentation_id, TerminationReason::kReceiverUserTerminated);
  } else {
    OSP_LOG_FATAL << "Received unknown receiver command: " << command;
  }
}

void RunReceiverPollLoop(pollfd& file_descriptor,
                         NetworkServiceManager* manager,
                         DemoReceiverDelegate& delegate) {
  pollfd stdin_pollfd{STDIN_FILENO, POLLIN};
  while (true) {
    OSP_CHECK_EQ(write(STDOUT_FILENO, "$ ", 2), 2);

    CommandWaitResult command_result = WaitForCommand(&stdin_pollfd);
    if (command_result.done) {
      break;
    }

    HandleReceiverCommand(command_result.command_line.command,
                          command_result.command_line.argument_tail, delegate,
                          manager);
  }
}

void CleanupPublisherDemo(NetworkServiceManager* manager) {
  Receiver::Get()->SetReceiverDelegate(nullptr);
  Receiver::Get()->Deinit();
  manager->GetServicePublisher()->Stop();
  manager->GetProtocolConnectionServer()->Stop();

  NetworkServiceManager::Dispose();
}

void PublisherDemo(absl::string_view friendly_name) {
  SignalThings();

  constexpr uint16_t server_port = 6667;

  // TODO(btolsch): aggregate initialization probably better?
  ServicePublisher::Config publisher_config;
  publisher_config.friendly_name = std::string(friendly_name);
  publisher_config.hostname = "turtle-deadbeef";
  publisher_config.service_instance_name = "deadbeef";
  publisher_config.connection_server_port = server_port;

  ServerConfig server_config;
  for (const InterfaceInfo& interface : GetNetworkInterfaces()) {
    OSP_VLOG << "Found interface: " << interface;
    if (!interface.addresses.empty()) {
      server_config.connection_endpoints.push_back(
          IPEndpoint{interface.addresses[0].address, server_port});
      publisher_config.network_interfaces.push_back(interface);
    }
  }
  OSP_LOG_IF(WARN, server_config.connection_endpoints.empty())
      << "No network interfaces had usable addresses for mDNS publishing.";

  DemoPublisherObserver publisher_observer;
  auto service_publisher = ServicePublisherFactory::Create(
      publisher_config, &publisher_observer,
      PlatformClientPosix::GetInstance()->GetTaskRunner());

  MessageDemuxer demuxer(Clock::now, MessageDemuxer::kDefaultBufferLimit);
  DemoConnectionServerObserver server_observer;
  auto connection_server = ProtocolConnectionServerFactory::Create(
      server_config, &demuxer, &server_observer,
      PlatformClientPosix::GetInstance()->GetTaskRunner());

  auto* network_service =
      NetworkServiceManager::Create(nullptr, std::move(service_publisher),
                                    nullptr, std::move(connection_server));

  DemoReceiverDelegate receiver_delegate;
  Receiver::Get()->Init();
  Receiver::Get()->SetReceiverDelegate(&receiver_delegate);
  network_service->GetServicePublisher()->Start();
  network_service->GetProtocolConnectionServer()->Start();

  pollfd stdin_pollfd{STDIN_FILENO, POLLIN};

  RunReceiverPollLoop(stdin_pollfd, network_service, receiver_delegate);

  receiver_delegate.connection.reset();
  CleanupPublisherDemo(network_service);
}

}  // namespace osp
}  // namespace openscreen

struct InputArgs {
  absl::string_view friendly_server_name;
  bool is_verbose;
  bool is_help;
  bool tracing_enabled;
};

void LogUsage(const char* argv0) {
  std::cerr << R"(
usage: )" << argv0
            << R"( <options> <friendly_name>

    friendly_name
        Server name, runs the publisher demo. Omission runs the listener demo.

    -t, --tracing: Enable performance trace logging.

    -v, --verbose: Enable verbose logging.

    -h, --help: Show this help message.
  )";
}

InputArgs GetInputArgs(int argc, char** argv) {
  // A note about modifying command line arguments: consider uniformity
  // between all Open Screen executables. If it is a platform feature
  // being exposed, consider if it applies to the standalone receiver,
  // standalone sender, osp demo, and test_main argument options.
  const struct option kArgumentOptions[] = {
      {"tracing", no_argument, nullptr, 't'},
      {"verbose", no_argument, nullptr, 'v'},
      {"help", no_argument, nullptr, 'h'},
      {nullptr, 0, nullptr, 0}};

  InputArgs args = {};
  int ch = -1;
  while ((ch = getopt_long(argc, argv, "tvh", kArgumentOptions, nullptr)) !=
         -1) {
    switch (ch) {
      case 't':
        args.tracing_enabled = true;
        break;

      case 'v':
        args.is_verbose = true;
        break;

      case 'h':
        args.is_help = true;
        break;
    }
  }

  if (optind < argc) {
    args.friendly_server_name = argv[optind];
  }

  return args;
}

int main(int argc, char** argv) {
  using openscreen::Clock;
  using openscreen::LogLevel;
  using openscreen::PlatformClientPosix;

  InputArgs args = GetInputArgs(argc, argv);
  if (args.is_help) {
    LogUsage(argv[0]);
    return 1;
  }

  std::unique_ptr<openscreen::TextTraceLoggingPlatform> trace_logging_platform;
  if (args.tracing_enabled) {
    trace_logging_platform =
        std::make_unique<openscreen::TextTraceLoggingPlatform>();
  }

  const LogLevel level = args.is_verbose ? LogLevel::kVerbose : LogLevel::kInfo;
  openscreen::SetLogLevel(level);

  const bool is_receiver_demo = !args.friendly_server_name.empty();
  const char* log_filename =
      is_receiver_demo ? kReceiverLogFilename : kControllerLogFilename;
  // TODO(jophba): Mac on Mojave hangs on this command forever.
  openscreen::SetLogFifoOrDie(log_filename);

  PlatformClientPosix::Create(std::chrono::milliseconds(50));

  if (is_receiver_demo) {
    OSP_LOG_INFO << "Running publisher demo...";
    openscreen::osp::PublisherDemo(args.friendly_server_name);
  } else {
    OSP_LOG_INFO << "Running listener demo...";
    openscreen::osp::ListenerDemo();
  }

  PlatformClientPosix::ShutDown();

  return 0;
}
