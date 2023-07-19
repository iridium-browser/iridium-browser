// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/str_cat.h"
#include "cast/receiver/channel/static_credentials.h"
#include "cast/standalone_receiver/cast_service.h"
#include "platform/api/time.h"
#include "platform/base/error.h"
#include "platform/base/ip_address.h"
#include "platform/impl/logging.h"
#include "platform/impl/network_interface.h"
#include "platform/impl/platform_client_posix.h"
#include "platform/impl/task_runner.h"
#include "platform/impl/text_trace_logging_platform.h"
#include "util/chrono_helpers.h"
#include "util/stringprintf.h"
#include "util/trace_logging.h"

namespace openscreen {
namespace cast {
namespace {

void LogUsage(const char* argv0) {
  constexpr char kTemplate[] = R"(
usage: %s <options> <interface>

    interface
        Specifies the network interface to bind to. The interface is
        looked up from the system interface registry.
        Mandatory, as it must be known for publishing discovery.

options:
    -p, --private-key=path-to-key: Path to OpenSSL-generated private key to be
                    used for TLS authentication. If a private key is not
                    provided, a randomly generated one will be used for this
                    session.

    -d, --developer-certificate=path-to-cert: Path to PEM file containing a
                           developer generated server root TLS certificate.
                           If a root server certificate is not provided, one
                           will be generated using a randomly generated
                           private key. Note that if a certificate path is
                           passed, the private key path is a mandatory field.

    -g, --generate-credentials: Instructs the binary to generate a private key
                                and self-signed root certificate with the CA
                                bit set to true, and then exit. The resulting
                                private key and certificate can then be used as
                                values for the -p and -s flags.

    -f, --friendly-name: Friendly name to be used for receiver discovery.

    -m, --model-name: Model name to be used for receiver discovery.

    -t, --tracing: Enable performance tracing logging.

    -v, --verbose: Enable verbose logging.

    -h, --help: Show this help message.

    -x, --disable-discovery: Disable discovery, useful for platforms like Mac OS
                             where our implementation is incompatible with
                             the native Bonjour service.

)";

  std::cerr << StringPrintf(kTemplate, argv0);
}

InterfaceInfo GetInterfaceInfoFromName(const char* name) {
  OSP_CHECK(name) << "Missing mandatory argument: <interface>";
  InterfaceInfo interface_info;
  std::vector<InterfaceInfo> network_interfaces = GetNetworkInterfaces();
  for (auto& interface : network_interfaces) {
    if (interface.name == name) {
      interface_info = std::move(interface);
      break;
    }
  }
  OSP_CHECK(!interface_info.name.empty())
      << "Invalid interface " << name << " specified.";
  return interface_info;
}

void RunCastService(TaskRunnerImpl* runner, CastService::Configuration config) {
  std::unique_ptr<CastService> service;
  runner->PostTask(
      [&] { service = std::make_unique<CastService>(std::move(config)); });

  OSP_LOG_INFO << "CastService is running. CTRL-C (SIGINT), or send a "
                  "SIGTERM to exit.";
  runner->RunUntilSignaled();

  // Spin the TaskRunner to execute destruction/shutdown tasks.
  OSP_LOG_INFO << "Shutting down...";
  runner->PostTask([&] {
    service.reset();
    runner->RequestStopSoon();
  });
  runner->RunUntilStopped();
  OSP_LOG_INFO << "Bye!";
}

int RunStandaloneReceiver(int argc, char* argv[]) {
#if !defined(CAST_ALLOW_DEVELOPER_CERTIFICATE)
  OSP_LOG_FATAL
      << "It compiled! However cast_receiver currently only supports using a "
         "passed-in certificate and private key, and must be built with "
         "cast_allow_developer_certificate=true set in the GN args to "
         "actually do anything interesting.";
  return 1;
#endif

#if !defined(CAST_STANDALONE_RECEIVER_HAVE_EXTERNAL_LIBS)
  OSP_LOG_INFO
      << "Note: compiled without external libs. The dummy player will "
         "be linked and no video decoding will occur. If this is not desired, "
         "install the required external libraries. For more information, see: "
         "[external_libraries.md](../streaming/external_libraries.md).";
#endif

  // A note about modifying command line arguments: consider uniformity
  // between all Open Screen executables. If it is a platform feature
  // being exposed, consider if it applies to the standalone receiver,
  // standalone sender, osp demo, and test_main argument options.
  const struct option kArgumentOptions[] = {
      {"private-key", required_argument, nullptr, 'p'},
      {"developer-certificate", required_argument, nullptr, 'd'},
      {"generate-credentials", no_argument, nullptr, 'g'},
      {"friendly-name", required_argument, nullptr, 'f'},
      {"model-name", required_argument, nullptr, 'm'},
      {"tracing", no_argument, nullptr, 't'},
      {"verbose", no_argument, nullptr, 'v'},
      {"help", no_argument, nullptr, 'h'},

      // Discovery is enabled by default, however there are cases where it
      // needs to be disabled, such as on Mac OS X.
      {"disable-discovery", no_argument, nullptr, 'x'},
      {nullptr, 0, nullptr, 0}};

  bool is_verbose = false;
  bool enable_discovery = true;
  std::string private_key_path;
  std::string developer_certificate_path;
  std::string friendly_name = "Cast Standalone Receiver";
  std::string model_name = "cast_standalone_receiver";
  bool should_generate_credentials = false;
  std::unique_ptr<TextTraceLoggingPlatform> trace_logger;
  int ch = -1;
  while ((ch = getopt_long(argc, argv, "p:d:f:m:grtvhx", kArgumentOptions,
                           nullptr)) != -1) {
    switch (ch) {
      case 'p':
        private_key_path = optarg;
        break;
      case 'd':
        developer_certificate_path = optarg;
        break;
      case 'f':
        friendly_name = optarg;
        break;
      case 'm':
        model_name = optarg;
        break;
      case 'g':
        should_generate_credentials = true;
        break;
      case 't':
        trace_logger = std::make_unique<TextTraceLoggingPlatform>();
        break;
      case 'v':
        is_verbose = true;
        break;
      case 'x':
        enable_discovery = false;
        break;
      case 'h':
        LogUsage(argv[0]);
        return 1;
    }
  }

  SetLogLevel(is_verbose ? LogLevel::kVerbose : LogLevel::kInfo);

  // Either -g is required, or both -p and -d.
  if (should_generate_credentials) {
    GenerateDeveloperCredentialsToFile();
    return 0;
  }
  if (private_key_path.empty() || developer_certificate_path.empty()) {
    OSP_LOG_FATAL << "You must either invoke with -g to generate credentials, "
                     "or provide both a private key path and root certificate "
                     "using -p and -d";
    return 1;
  }

  const char* interface_name = argv[optind];
  OSP_CHECK(interface_name && strlen(interface_name) > 0)
      << "No interface name provided.";

  std::string receiver_id =
      absl::StrCat("Standalone Receiver on ", interface_name);
  ErrorOr<GeneratedCredentials> creds = GenerateCredentials(
      receiver_id, private_key_path, developer_certificate_path);
  OSP_CHECK(creds.is_value()) << creds.error();

  const InterfaceInfo interface = GetInterfaceInfoFromName(interface_name);
  OSP_CHECK(interface.GetIpAddressV4() || interface.GetIpAddressV6());

  auto* const task_runner = new TaskRunnerImpl(&Clock::now);
  PlatformClientPosix::Create(milliseconds(50),
                              std::unique_ptr<TaskRunnerImpl>(task_runner));
  RunCastService(task_runner,
                 CastService::Configuration{
                     task_runner, interface, std::move(creds.value()),
                     friendly_name, model_name, enable_discovery});
  PlatformClientPosix::ShutDown();

  return 0;
}

}  // namespace
}  // namespace cast
}  // namespace openscreen

int main(int argc, char* argv[]) {
  return openscreen::cast::RunStandaloneReceiver(argc, argv);
}
