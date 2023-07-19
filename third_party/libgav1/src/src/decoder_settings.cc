// Copyright 2020 The libgav1 Authors
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

#include "src/gav1/decoder_settings.h"

extern "C" {

void Libgav1DecoderSettingsInitDefault(Libgav1DecoderSettings* settings) {
  settings->threads = 1;
  settings->frame_parallel = 0;    // false
  settings->blocking_dequeue = 0;  // false
  settings->on_frame_buffer_size_changed = nullptr;
  settings->get_frame_buffer = nullptr;
  settings->release_frame_buffer = nullptr;
  settings->release_input_buffer = nullptr;
  settings->callback_private_data = nullptr;
  settings->output_all_layers = 0;  // false
  settings->operating_point = 0;
  settings->post_filter_mask = 0x1f;
}

}  // extern "C"
