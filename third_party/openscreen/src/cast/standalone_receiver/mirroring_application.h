// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_RECEIVER_MIRRORING_APPLICATION_H_
#define CAST_STANDALONE_RECEIVER_MIRRORING_APPLICATION_H_

#include <memory>
#include <string>
#include <vector>

#include "cast/receiver/application_agent.h"
#include "cast/standalone_receiver/streaming_playback_controller.h"
#include "platform/base/error.h"
#include "platform/base/ip_address.h"
#include "util/scoped_wake_lock.h"
#include "util/serial_delete_ptr.h"

namespace openscreen {

class TaskRunner;

namespace cast {

class MessagePort;
class ReceiverSession;

// Implements a basic Cast V2 Mirroring Application which, at launch time,
// bootstraps a ReceiverSession and StreamingPlaybackController, which set-up
// and manage the media data streaming and play it out in an on-screen window.
class MirroringApplication final : public ApplicationAgent::Application,
                                   public StreamingPlaybackController::Client {
 public:
  MirroringApplication(TaskRunner& task_runner,
                       const IPAddress& interface_address,
                       ApplicationAgent* agent);

  ~MirroringApplication() final;

  // ApplicationAgent::Application overrides.
  const std::vector<std::string>& GetAppIds() const final;
  bool Launch(const std::string& app_id,
              const Json::Value& app_params,
              MessagePort* message_port) final;
  std::string GetSessionId() final;
  std::string GetDisplayName() final;
  std::vector<std::string> GetSupportedNamespaces() final;
  void Stop() final;

  // StreamingPlaybackController::Client overrides
  void OnPlaybackError(StreamingPlaybackController* controller,
                       Error error) final;

 private:
  TaskRunner& task_runner_;
  const IPAddress interface_address_;
  const std::vector<std::string> app_ids_;
  ApplicationAgent* const agent_;

  SerialDeletePtr<ScopedWakeLock> wake_lock_;
  std::unique_ptr<Environment> environment_;
  std::unique_ptr<StreamingPlaybackController> controller_;
  std::unique_ptr<ReceiverSession> current_session_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_RECEIVER_MIRRORING_APPLICATION_H_
