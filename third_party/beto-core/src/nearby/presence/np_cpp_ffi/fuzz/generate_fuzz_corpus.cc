// Copyright 2023 Google LLC
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

#include "nearby_protocol.h"
#include "shared_test_util.h"

#include <fstream>
#include <iostream>

#define STRING(x) #x
#define XSTRING(x) STRING(x)

void write_corpus_data_file(std::string file_name,
                            nearby_protocol::RawAdvertisementPayload payload) {
  // Get the current source directory.
  std::string current_source_dir = XSTRING(CORPUS_DIR);

  // Print the current source directory to the console.
  std::cout << "Writing to file: " << current_source_dir << "/" << file_name
            << std::endl;

  std::ofstream out(current_source_dir + "/" + file_name,
                    std::ios::out | std::ios::binary);
  auto vec_data = payload.buffer_.ToVector();
  out.write(reinterpret_cast<const char *>(vec_data.data()), vec_data.size());
  out.close();
}

int main() {
  write_corpus_data_file("V0AdvSimple", V0AdvSimple);
  write_corpus_data_file("V1AdvSimple", V1AdvSimple);
  write_corpus_data_file("V0AdvEmpty", V0AdvEmpty);
}
