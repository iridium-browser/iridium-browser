// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/metadata_walk.h"

std::vector<Value> WalkMetadata(
    const UniqueVector<const Target*>& targets_to_walk,
    const std::vector<std::string>& keys_to_extract,
    const std::vector<std::string>& keys_to_walk,
    const SourceDir& rebase_dir,
    std::set<const Target*>* targets_walked,
    Err* err) {
  std::vector<Value> result;
  for (const auto* target : targets_to_walk) {
    auto pair = targets_walked->insert(target);
    if (pair.second) {
      if (!target->GetMetadata(keys_to_extract, keys_to_walk, rebase_dir, false,
                               &result, targets_walked, err))
        return std::vector<Value>();
    }
  }
  return result;
}