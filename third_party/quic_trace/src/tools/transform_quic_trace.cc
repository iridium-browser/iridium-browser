// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// A helper tool to perform various transformations on traces, notably
// protobuf-to-JSON conversion and trace truncation.

#include <cstdint>
#include <iostream>
#include <istream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/util/json_util.h"
#include "quic_trace/quic_trace.pb.h"

ABSL_FLAG(bool, whitespace, true, "Add whitespace to the JSON output");
ABSL_FLAG(std::string,
          input_format,
          "protobuf",
          "Can be 'protobuf' or 'json'.");
ABSL_FLAG(std::string,
          output_format,
          "protobuf",
          "Can be 'protobuf' or 'json'.");
ABSL_FLAG(int64_t,
          truncate_after_time_us,
          -1,
          "If positive, removes all events after the specified point in "
          "time (us) from the trace.");

namespace {

using google::protobuf::util::JsonOptions;
using google::protobuf::util::JsonStringToMessage;
using google::protobuf::util::MessageToJsonString;

bool IsValidFormatString(absl::string_view format) {
  return format == "protobuf" || format == "json";
}

bool InputTrace(quic_trace::Trace* trace) {
  if (absl::GetFlag(FLAGS_input_format) == "protobuf") {
    return trace->ParseFromIstream(&std::cin);
  }

  std::istreambuf_iterator<char> it(std::cin);
  std::istreambuf_iterator<char> end;

  auto status = JsonStringToMessage(std::string(it, end), trace);
  return status.ok();
}

void OutputTrace(const quic_trace::Trace& trace) {
  if (absl::GetFlag(FLAGS_output_format) == "protobuf") {
    trace.SerializeToOstream(&std::cout);
    return;
  }

  std::string output;
  JsonOptions options;
  options.add_whitespace = absl::GetFlag(FLAGS_whitespace);
  MessageToJsonString(trace, &output, options);
  std::cout << output;
}

void MaybeTruncateTrace(quic_trace::Trace* trace) {
  if (absl::GetFlag(FLAGS_truncate_after_time_us) <= 0) {
    return;
  }

  auto* events = trace->mutable_events();
  while (!events->empty() &&
         events->rbegin()->time_us() > static_cast<uint64_t>(absl::GetFlag(
                                           FLAGS_truncate_after_time_us))) {
    events->RemoveLast();
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);

  if (!IsValidFormatString(absl::GetFlag(FLAGS_input_format))) {
    std::cerr << "Invalid format specified: "
              << absl::GetFlag(FLAGS_input_format) << std::endl;
    return 1;
  }
  if (!IsValidFormatString(absl::GetFlag(FLAGS_output_format))) {
    std::cerr << "Invalid format specified: "
              << absl::GetFlag(FLAGS_output_format) << std::endl;
    return 1;
  }

  quic_trace::Trace trace;
  bool success = InputTrace(&trace);
  if (!success) {
    std::cerr << "Failed to parse input." << std::endl;
    return 1;
  }

  MaybeTruncateTrace(&trace);
  OutputTrace(trace);
  return 0;
}
