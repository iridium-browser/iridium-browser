// Copyright 2019 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "examples/file_reader_factory.h"

#include <new>

#include "examples/logging.h"

namespace libgav1 {
namespace {

std::vector<FileReaderFactory::OpenFunction>* GetFileReaderOpenFunctions() {
  static auto* open_functions =
      new (std::nothrow) std::vector<FileReaderFactory::OpenFunction>();
  return open_functions;
}

}  // namespace

bool FileReaderFactory::RegisterReader(OpenFunction open_function) {
  if (open_function == nullptr) return false;
  auto* open_functions = GetFileReaderOpenFunctions();
  const size_t num_readers = open_functions->size();
  open_functions->push_back(open_function);
  return open_functions->size() == num_readers + 1;
}

std::unique_ptr<FileReaderInterface> FileReaderFactory::OpenReader(
    const std::string& file_name, const bool error_tolerant /*= false*/) {
  for (auto* open_function : *GetFileReaderOpenFunctions()) {
    auto reader = open_function(file_name, error_tolerant);
    if (reader == nullptr) continue;
    return reader;
  }
  LIBGAV1_EXAMPLES_LOG_ERROR("No file reader able to open input");
  return nullptr;
}

}  // namespace libgav1
