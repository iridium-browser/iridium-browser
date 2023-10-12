// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_TEST_MOCK_TLS_CONNECTION_H_
#define PLATFORM_TEST_MOCK_TLS_CONNECTION_H_

#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "platform/api/tls_connection.h"

namespace openscreen {

class TaskRunner;

class MockTlsConnection : public TlsConnection {
 public:
  MockTlsConnection(IPEndpoint local_address, IPEndpoint remote_address)
      : local_address_(local_address), remote_address_(remote_address) {}

  ~MockTlsConnection() override = default;

  using TlsConnection::Client;
  void SetClient(Client* client) override { client_ = client; }

  MOCK_METHOD(bool, Send, (const void* data, size_t len), (override));

  IPEndpoint GetRemoteEndpoint() const override { return remote_address_; }

  void OnError(Error error) {
    if (client_) {
      client_->OnError(this, std::move(error));
    }
  }
  void OnRead(std::vector<uint8_t> block) {
    if (client_) {
      client_->OnRead(this, std::move(block));
    }
  }

 private:
  Client* client_;
  const IPEndpoint local_address_;
  const IPEndpoint remote_address_;
};

}  // namespace openscreen

#endif  // PLATFORM_TEST_MOCK_TLS_CONNECTION_H_
