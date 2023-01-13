// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/protocol_connection_client_factory.h"

#include <memory>

#include "osp/impl/quic/quic_client.h"
#include "osp/impl/quic/quic_connection_factory_impl.h"
#include "osp/public/network_service_manager.h"
#include "platform/api/task_runner.h"
#include "platform/api/time.h"

namespace openscreen {
namespace osp {

// static
std::unique_ptr<ProtocolConnectionClient>
ProtocolConnectionClientFactory::Create(
    MessageDemuxer* demuxer,
    ProtocolConnectionServiceObserver* observer,
    TaskRunner* task_runner) {
  return std::make_unique<QuicClient>(
      demuxer, std::make_unique<QuicConnectionFactoryImpl>(task_runner),
      observer, &Clock::now, task_runner);
}

}  // namespace osp
}  // namespace openscreen
