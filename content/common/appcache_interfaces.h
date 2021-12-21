// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_APPCACHE_INTERFACES_H_
#define CONTENT_COMMON_APPCACHE_INTERFACES_H_

#include <string>

#include "content/common/content_export.h"
#include "url/gurl.h"

namespace content {

// Useful string constants.
CONTENT_EXPORT extern const char kHttpGETMethod[];
CONTENT_EXPORT extern const char kHttpHEADMethod[];

CONTENT_EXPORT bool IsSchemeSupportedForAppCache(const GURL& url);
CONTENT_EXPORT bool IsMethodSupportedForAppCache(const std::string& method);

}  // namespace

#endif  // CONTENT_COMMON_APPCACHE_INTERFACES_H_
