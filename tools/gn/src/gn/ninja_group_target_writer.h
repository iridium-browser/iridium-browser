// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_NINJA_GROUP_TARGET_WRITER_H_
#define TOOLS_GN_NINJA_GROUP_TARGET_WRITER_H_

#include "gn/ninja_target_writer.h"

// Writes a .ninja file for a group target type.
class NinjaGroupTargetWriter : public NinjaTargetWriter {
 public:
  NinjaGroupTargetWriter(const Target* target, std::ostream& out);
  ~NinjaGroupTargetWriter() override;

  void Run() override;

 private:
  NinjaGroupTargetWriter(const NinjaGroupTargetWriter&) = delete;
  NinjaGroupTargetWriter& operator=(const NinjaGroupTargetWriter&) = delete;
};

#endif  // TOOLS_GN_NINJA_GROUP_TARGET_WRITER_H_
