// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_IPP_FRAME_BUILDER_H_
#define LIBIPP_IPP_FRAME_BUILDER_H_

#include <cstdint>
#include <list>
#include <string>
#include <vector>

#include "ipp_base.h"  // NOLINT(build/include)
#include "ipp_frame.h"  // NOLINT(build/include)

namespace ipp {

// forward declarations
class Attribute;
class Collection;
class Package;

class FrameBuilder {
 public:
  // Constructor, both parameters must not be nullptr. |frame| is used as
  // internal buffer to store intermediate form of data to send. All spotted
  // issues are logged to |log| (by push_back()).
  FrameBuilder(Frame* frame, std::vector<Log>* log)
      : frame_(frame), errors_(log) {}

  // Build a content of the frame from the given Package.
  void BuildFrameFromPackage(Package* package);

  // Returns the current frame size in bytes. Call it after
  // BuildFrameFromPackage(...) to get the size of the output buffer.
  std::size_t GetFrameLength() const;

  // Write data to given buffer (use the method above to learn about required
  // size of the buffer).
  bool WriteFrameToBuffer(uint8_t* ptr);

 private:
  // Copy/move/assign constructors/operators are forbidden.
  FrameBuilder(const FrameBuilder&) = delete;
  FrameBuilder(FrameBuilder&&) = delete;
  FrameBuilder& operator=(const FrameBuilder&) = delete;
  FrameBuilder& operator=(FrameBuilder&&) = delete;

  // Method for adding entries to the log.
  void LogFrameBuilderError(const std::string& message);

  // Main functions for building a frame.
  void SaveGroup(Collection* coll, std::list<TagNameValue>* data_chunks);
  void SaveCollection(Collection* coll, std::list<TagNameValue>* data_chunks);

  // Helpers for converting individual values to binary form.
  void SaveAttrValue(Attribute* attr,
                     size_t index,
                     uint8_t* tag,
                     std::vector<uint8_t>* buf);

  // Write data to buffer (ptr is updated, whole list is written).
  bool WriteTNVsToBuffer(const std::list<TagNameValue>&, uint8_t** ptr);

  // Internal buffer.
  Frame* frame_;

  // Internal log: all errors & warnings are logged here.
  std::vector<Log>* errors_;
};

}  // namespace ipp

#endif  //  LIBIPP_IPP_FRAME_BUILDER_H_
