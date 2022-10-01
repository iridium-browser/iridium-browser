// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_NINJA_BUNDLE_DATA_TARGET_WRITER_H_
#define TOOLS_GN_NINJA_BUNDLE_DATA_TARGET_WRITER_H_

#include "gn/ninja_target_writer.h"

// Writes a .ninja file for a bundle_data target type.
class NinjaBundleDataTargetWriter : public NinjaTargetWriter {
 public:
  NinjaBundleDataTargetWriter(const Target* target, std::ostream& out);
  ~NinjaBundleDataTargetWriter() override;

  void Run() override;

 private:
  NinjaBundleDataTargetWriter(const NinjaBundleDataTargetWriter&) = delete;
  NinjaBundleDataTargetWriter& operator=(const NinjaBundleDataTargetWriter&) =
      delete;
};

#endif  // TOOLS_GN_NINJA_BUNDLE_DATA_TARGET_WRITER_H_
