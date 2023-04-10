// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_METADATAWALK_H_
#define TOOLS_GN_METADATAWALK_H_

#include "gn/build_settings.h"
#include "gn/target.h"
#include "gn/unique_vector.h"
#include "gn/value.h"

// Function to collect metadata from resolved targets listed in targets_walked.
// Intended to be called after all targets are resolved.
//
// This populates targets_walked with all targets touched by this walk, and
// returns the list of metadata values.
std::vector<Value> WalkMetadata(
    const UniqueVector<const Target*>& targets_to_walk,
    const std::vector<std::string>& keys_to_extract,
    const std::vector<std::string>& keys_to_walk,
    const SourceDir& rebase_dir,
    TargetSet* targets_walked,
    Err* err);

#endif  // TOOLS_GN_METADATAWALK_H_
