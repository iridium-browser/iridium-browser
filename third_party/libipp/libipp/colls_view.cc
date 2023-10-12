// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "colls_view.h"

#include <vector>

namespace ipp {

namespace {

// Returns a pointer to an empty vector that is always valid.
std::vector<Collection*>* EmptyVectorOfColls() {
  static std::vector<Collection*> empty_vector;
  return &empty_vector;
}

}  // namespace

CollsView::CollsView() : colls_(EmptyVectorOfColls()) {}

ConstCollsView::ConstCollsView() : colls_(EmptyVectorOfColls()) {}

}  // namespace ipp
