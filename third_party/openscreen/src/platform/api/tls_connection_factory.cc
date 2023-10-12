// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/tls_connection_factory.h"

namespace openscreen {

TlsConnectionFactory::TlsConnectionFactory() = default;
TlsConnectionFactory::~TlsConnectionFactory() = default;

TlsConnectionFactory::Client::~Client() = default;

}  // namespace openscreen
