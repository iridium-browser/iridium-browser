// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/public/cpp/client/fake_client_channel.h"

#include "base/memory/ptr_util.h"

namespace chromeos {

namespace secure_channel {

FakeClientChannel::FakeClientChannel() = default;

FakeClientChannel::~FakeClientChannel() {
  if (destructor_callback_)
    std::move(destructor_callback_).Run();
}

void FakeClientChannel::InvokePendingGetConnectionMetadataCallback(
    mojom::ConnectionMetadataPtr connection_metadata) {
  std::move(get_connection_metadata_callback_queue_.front())
      .Run(std::move(connection_metadata));
  get_connection_metadata_callback_queue_.pop();
}

void FakeClientChannel::PerformGetConnectionMetadata(
    base::OnceCallback<void(mojom::ConnectionMetadataPtr)> callback) {
  get_connection_metadata_callback_queue_.push(std::move(callback));
}

void FakeClientChannel::PerformSendMessage(const std::string& payload,
                                           base::OnceClosure on_sent_callback) {
  sent_messages_.push_back(
      std::make_pair(payload, std::move(on_sent_callback)));
}

}  // namespace secure_channel

}  // namespace chromeos
