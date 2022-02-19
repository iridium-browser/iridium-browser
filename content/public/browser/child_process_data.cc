// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/child_process_data.h"

namespace content {

ChildProcessData::ChildProcessData(int process_type)
    : process_type(process_type) {}

ChildProcessData::ChildProcessData(ChildProcessData&& rhs) = default;

ChildProcessData::~ChildProcessData() {}

ChildProcessData ChildProcessData::Duplicate() const {
  ChildProcessData result(process_type);
  result.name = name;
  result.metrics_name = metrics_name;
  result.id = id;
  result.SetProcess(process_.Duplicate());
  return result;
}

}  // namespace content
