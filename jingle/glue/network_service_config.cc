// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Configuration information for talking to the network service.

#include "jingle/glue/network_service_config.h"

namespace jingle_glue {

NetworkServiceConfig::NetworkServiceConfig() = default;
NetworkServiceConfig::NetworkServiceConfig(const NetworkServiceConfig& other) =
    default;
NetworkServiceConfig::~NetworkServiceConfig() = default;

}  // namespace jingle_glue
