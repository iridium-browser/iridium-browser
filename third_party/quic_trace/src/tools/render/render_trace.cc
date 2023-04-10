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

#include <fstream>
#include <string>

#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/util/json_util.h"
#include "tools/render/trace_program.h"

enum InputFormat {
  INPUT_JSON,
  INPUT_QTR,
};

namespace {
InputFormat GuessInputFileFormat(absl::string_view filename) {
  if (filename.find(".json") != std::string::npos) {
    return INPUT_JSON;
  } else {
    return INPUT_QTR;
  }
}
}  // namespace

// render_trace renders the specified trace file using an OpenGL-based viewer.
int main(int argc, char* argv[]) {
  std::vector<char*> args = absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();

  CHECK_GE(argc, 2) << "Specify file path";
  auto trace = std::make_unique<quic_trace::Trace>();
  {
    std::string filename(args[1]);
    std::ifstream f(filename);
    switch (GuessInputFileFormat(filename)) {
      case INPUT_QTR: {
        trace->ParseFromIstream(&f);
        break;
      }
      case INPUT_JSON: {
        std::istreambuf_iterator<char> it(f);
        std::istreambuf_iterator<char> end;

        auto status = google::protobuf::util::JsonStringToMessage(
            std::string(it, end), &*trace);
        if (!status.ok()) {
          LOG(FATAL) << "Failed to load '" << filename << "': " << status;
        }
        break;
      }
      default:
        LOG(FATAL) << "Unexpected format";
    }
  }

  quic_trace::render::TraceProgram program;
  program.LoadTrace(std::move(trace));
  program.Loop();
  return 0;
}
