// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sessions/session_restore_test_utils.h"

#include "chrome/browser/sessions/tab_loader_delegate.h"

namespace testing {

bool AlwayLoadSessionRestorePolicy::ShouldLoad(
    content::WebContents* contents) const {
  return true;
}

ScopedAlwaysLoadSessionRestoreTestPolicy::
    ScopedAlwaysLoadSessionRestoreTestPolicy() {
  TabLoaderDelegate::SetSessionRestorePolicyForTesting(&policy_);
}

ScopedAlwaysLoadSessionRestoreTestPolicy::
    ~ScopedAlwaysLoadSessionRestoreTestPolicy() {
  TabLoaderDelegate::SetSessionRestorePolicyForTesting(nullptr);
}

}  // namespace testing
