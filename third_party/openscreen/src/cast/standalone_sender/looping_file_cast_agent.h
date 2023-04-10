// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_SENDER_LOOPING_FILE_CAST_AGENT_H_
#define CAST_STANDALONE_SENDER_LOOPING_FILE_CAST_AGENT_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "absl/types/optional.h"
#include "cast/common/channel/cast_message_handler.h"
#include "cast/common/channel/cast_socket_message_port.h"
#include "cast/common/channel/connection_namespace_handler.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "cast/common/public/cast_socket.h"
#include "cast/common/public/trust_store.h"
#include "cast/sender/public/sender_socket_factory.h"
#include "cast/standalone_sender/connection_settings.h"
#include "cast/standalone_sender/looping_file_sender.h"
#include "cast/standalone_sender/remoting_sender.h"
#include "cast/streaming/environment.h"
#include "cast/streaming/sender_session.h"
#include "platform/api/scoped_wake_lock.h"
#include "platform/api/serial_delete_ptr.h"
#include "platform/base/error.h"
#include "platform/base/interface_info.h"
#include "platform/impl/task_runner.h"

namespace Json {
class Value;
}

namespace openscreen {
namespace cast {

// A single-use sender-side Cast Agent that manages the workflow for a mirroring
// session, casting the content from a local file indefinitely. After being
// constructed and having its Connect() method called, the LoopingFileCastAgent
// steps through the following workflow:
//
//   1. Waits for a CastSocket representing a successful connection to a remote
//      Cast Receiver's agent.
//   2. Sends a LAUNCH request to the Cast Receiver to start its Mirroring App.
//   3. Waits for a RECEIVER_STATUS message from the Receiver indicating launch
//      success, or a LAUNCH_ERROR.
//   4. Once launched, message routing (i.e., a VirtualConnection) is requested,
//      for messaging between the SenderSession (locally) and the remote
//      Mirroring App.
//   5. Once message routing is established, the local SenderSession is created
//      and begins the mirroring-specific OFFER/ANSWER messaging to negotiate
//      the streaming parameters.
//   6. Streaming commences.
//
// If at any point an error occurs, the LoopingFileCastAgent executes a clean
// shut-down (both locally, and with the remote Cast Receiver), and then invokes
// the ShutdownCallback that was passed to the constructor.
//
// Normal shutdown happens when either:
//
//   1. Receiver-side, the Mirroring App is shut down. This will cause the
//      ShutdownCallback passed to the constructor to be invoked.
//   2. This LoopingFileCastAgent is destroyed (automatic shutdown is part of
//      the destruction procedure).
class LoopingFileCastAgent final
    : public SenderSocketFactory::Client,
      public VirtualConnectionRouter::SocketErrorHandler,
      public ConnectionNamespaceHandler::VirtualConnectionPolicy,
      public CastMessageHandler,
      public SenderSession::Client,
      public RemotingSender::Client {
 public:
  using ShutdownCallback = std::function<void()>;

  // |shutdown_callback| is invoked after normal shutdown, whether initiated
  // sender- or receiver-side; or, for any fatal error.
  LoopingFileCastAgent(TaskRunner* task_runner,
                       std::unique_ptr<TrustStore> cast_trust_store,
                       ShutdownCallback shutdown_callback);
  ~LoopingFileCastAgent();

  // Connect to a Cast Receiver, and start the workflow to establish a
  // mirroring/streaming session. Destroy the LoopingFileCastAgent to shutdown
  // and disconnect.
  void Connect(ConnectionSettings settings);

 private:
  // SenderSocketFactory::Client overrides.
  void OnConnected(SenderSocketFactory* factory,
                   const IPEndpoint& endpoint,
                   std::unique_ptr<CastSocket> socket) override;
  void OnError(SenderSocketFactory* factory,
               const IPEndpoint& endpoint,
               Error error) override;

  // VirtualConnectionRouter::SocketErrorHandler overrides.
  void OnClose(CastSocket* cast_socket) override;
  void OnError(CastSocket* socket, Error error) override;

  // ConnectionNamespaceHandler::VirtualConnectionPolicy overrides.
  bool IsConnectionAllowed(
      const VirtualConnection& virtual_conn) const override;

  // CastMessageHandler overrides.
  void OnMessage(VirtualConnectionRouter* router,
                 CastSocket* socket,
                 ::cast::channel::CastMessage message) override;

  // RemotingSender::Client overrides.
  void OnReady() override;
  void OnPlaybackRateChange(double rate) override;

  // Returns the Cast application ID for either audio+video Cast Streaming or
  // audio-only streaming, as configured by the ConnectionSettings.
  const char* GetStreamingAppId() const;

  // Called by OnMessage() to determine whether the Cast Receiver has launched
  // or unlaunched the Mirroring App. If the former, a VirtualConnection is
  // requested. Otherwise, the workflow is aborted and Shutdown() is called.
  void HandleReceiverStatus(const Json::Value& status);

  // Called by the |connection_handler_| after message routing to the Cast
  // Receiver's Mirroring App has been established (if |success| is true).
  void OnRemoteMessagingOpened(bool success);

  // Once we have a connection to the receiver we need to create and start
  // a sender session. This method results in the OFFER/ANSWER exchange
  // being completed and a session should be started.
  void CreateAndStartSession();

  // SenderSession::Client overrides.
  void OnNegotiated(const SenderSession* session,
                    SenderSession::ConfiguredSenders senders,
                    capture_recommendations::Recommendations
                        capture_recommendations) override;
  void OnError(const SenderSession* session, Error error) override;

  // Starts the file sender. This may occur when mirroring or remoting is
  // "ready" if the session is already negotiated, or upon session negotiation
  // if the receiver is already ready.
  void StartFileSender();

  // Helper for stopping the current session, and/or unwinding a remote
  // connection request (pre-session). This ensures LoopingFileCastAgent is in a
  // terminal shutdown state.
  void Shutdown();

  // Member variables set as part of construction.
  TaskRunner* const task_runner_;
  ShutdownCallback shutdown_callback_;
  VirtualConnectionRouter router_;
  ConnectionNamespaceHandler connection_handler_;
  SenderSocketFactory socket_factory_;
  std::unique_ptr<TlsConnectionFactory> connection_factory_;
  CastSocketMessagePort message_port_;

  // Counter for distinguishing request messages sent to the Cast Receiver.
  int next_request_id_ = 1;

  // Initialized by Connect().
  absl::optional<ConnectionSettings> connection_settings_;
  SerialDeletePtr<ScopedWakeLock> wake_lock_;

  // If non-empty, this is the sessionId associated with the Cast Receiver
  // application that this LoopingFileCastAgent launched.
  std::string app_session_id_;

  // This is set once LoopingFileCastAgent has requested to start messaging to
  // the mirroring app on a Cast Receiver.
  absl::optional<VirtualConnection> remote_connection_;

  CastMode cast_mode_ = CastMode::kMirroring;

  // Member variables set while a streaming to the mirroring app on a Cast
  // Receiver.
  std::unique_ptr<Environment> environment_;
  std::unique_ptr<SenderSession> current_session_;
  std::unique_ptr<LoopingFileSender> file_sender_;

  // Remoting specific member variables.
  std::unique_ptr<RemotingSender> remoting_sender_;

  // Set when remoting is successfully negotiated. However, remoting streams
  // won't start until |is_ready_for_remoting_| is true.
  std::unique_ptr<SenderSession::ConfiguredSenders> current_negotiation_;

  // Set to true when the remoting receiver is ready.  However, remoting streams
  // won't start until remoting is successfully negotiated.
  bool is_ready_for_remoting_ = false;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_SENDER_LOOPING_FILE_CAST_AGENT_H_
