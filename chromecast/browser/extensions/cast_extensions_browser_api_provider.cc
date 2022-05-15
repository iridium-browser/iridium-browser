// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/extensions/cast_extensions_browser_api_provider.h"

#include "chromecast/browser/extensions/api/generated_api_registration.h"

namespace extensions {

CastExtensionsBrowserAPIProvider::CastExtensionsBrowserAPIProvider() = default;
CastExtensionsBrowserAPIProvider::~CastExtensionsBrowserAPIProvider() = default;

void CastExtensionsBrowserAPIProvider::RegisterExtensionFunctions(
    ExtensionFunctionRegistry* registry) {
  cast::api::CastGeneratedFunctionRegistry::RegisterAll(registry);
}

}  // namespace extensions
