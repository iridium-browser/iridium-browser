// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_ERRORS_H_
#define LIBIPP_ERRORS_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "ipp_enums.h"
#include "ipp_export.h"

namespace ipp {

// Describes location of the attribute in the frame and the attribute's name.
class LIBIPP_EXPORT AttrPath {
 public:
  struct Segment {
    uint16_t collection_index;
    std::string attribute_name;
  };
  // Invalid value of GroupTag used to represent location in a frame's header.
  static constexpr GroupTag kHeader = static_cast<GroupTag>(0);
  explicit AttrPath(GroupTag group) : group_(group) {}
  // Returns a string representation of the attribute's locations.
  std::string AsString() const;
  // Adds a new segment at the end of attribute's path. Converts the attribute's
  // path to the path to one of its sub-attributes.
  void PushBack(uint16_t collection_index, std::string_view attribute_name) {
    path_.emplace_back(Segment{collection_index, std::string(attribute_name)});
  }
  // Removes the last segment from the attribute's path. Converts the
  // attribute's path to the path to its parent attribute.
  void PopBack() { path_.pop_back(); }
  // Returns reference to the last element.
  Segment& Back() { return path_.back(); }

 private:
  GroupTag group_;
  std::vector<Segment> path_;
};

}  // namespace ipp

#endif  //  LIBIPP_ERRORS_H_
