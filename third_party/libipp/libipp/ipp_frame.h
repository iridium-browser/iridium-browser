// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_IPP_FRAME_H_
#define LIBIPP_IPP_FRAME_H_

#include <cstdint>
#include <list>
#include <vector>

#include "ipp_enums.h"

// Internal structures used during parsing & building IPP frames. You probably
// do not want to use it directly. See ipp.h for information how to use this
// library.

namespace ipp {

// This is basic structure used by IPP protocol. See 3.1.4-3.1.7 from rfc8010.
struct TagNameValue {
  uint8_t tag;
  std::vector<uint8_t> name;
  std::vector<uint8_t> value;
};

// This represents single IPP frame, described in rfc8010.
struct FrameData {
  // Variables save to/load from a frame's header.
  uint16_t version_;
  int16_t operation_id_or_status_code_;
  int32_t request_id_;
  // The content of frame being (internal buffer).
  std::vector<GroupTag> groups_tags_;
  std::vector<std::list<TagNameValue>> groups_content_;
  std::vector<uint8_t> data_;
};

}  // namespace ipp

#endif  //  LIBIPP_IPP_FRAME_H_
