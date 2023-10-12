// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/tls_connection.h"

namespace openscreen {

TlsConnection::TlsConnection() = default;
TlsConnection::~TlsConnection() = default;

TlsConnection::Client::~Client() = default;

}  // namespace openscreen
