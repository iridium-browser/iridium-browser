// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_RECEIVER_APPLICATION_AGENT_H_
#define CAST_RECEIVER_APPLICATION_AGENT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "cast/common/channel/cast_socket_message_port.h"
#include "cast/common/channel/connection_namespace_handler.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "cast/common/public/cast_socket.h"
#include "cast/receiver/channel/device_auth_namespace_handler.h"
#include "cast/receiver/public/receiver_socket_factory.h"
#include "platform/api/serial_delete_ptr.h"
#include "platform/api/task_runner.h"
#include "platform/base/error.h"
#include "platform/base/ip_address.h"
#include "util/json/json_value.h"

namespace openscreen {
namespace cast {

class CastSocket;

// A service accepting CastSocket connections, and providing a minimal
// implementation of the CastV2 application control protocol to launch receiver
// applications and route messages to/from them.
//
// Workflow: One or more Applications are registered under this ApplicationAgent
// (e.g., a "mirroring" app). Later, a ReceiverSocketFactory (external to this
// class) will listen and establish CastSocket connections, and then pass
// CastSockets to this ApplicationAgent via the OnConnect() method. As each
// connection is made, device authentication will take place. Then, Cast V2
// application messages asking about application availability are received and
// responded to, based on what Applications are registered. Finally, the remote
// may request the LAUNCH of an Application (and later a STOP).
//
// In the meantime, this ApplicationAgent broadcasts RECEIVER_STATUS about what
// application is running. In addition, it attempts to launch an "idle screen"
// Application whenever no other Application is running. The "idle screen"
// Application is usually some kind of screen saver or wallpaper/clock display.
// Registering the "idle screen" Application is optional, and if it's not
// registered, then nothing will be running during idle periods.
class ApplicationAgent final
    : public ReceiverSocketFactory::Client,
      public CastMessageHandler,
      public ConnectionNamespaceHandler::VirtualConnectionPolicy,
      public VirtualConnectionRouter::SocketErrorHandler {
 public:
  class Application {
   public:
    // Returns the one or more application IDs that are supported. This list
    // must not mutate while the Application is registered.
    virtual const std::vector<std::string>& GetAppIds() const = 0;

    // Launches the application and returns true if successful. |app_id| is the
    // specific ID that was used to launch the app, and |app_params| is a
    // pass-through for any arbitrary app-specfic structure (or null if not
    // provided). If the Application wishes to send/receive messages, it uses
    // the provided |message_port| and must call MessagePort::SetClient() before
    // any flow will occur.
    virtual bool Launch(const std::string& app_id,
                        const Json::Value& app_params,
                        MessagePort* message_port) = 0;

    // These reflect the current state of the application, and the data is used
    // to populate RECEIVER_STATUS messages.
    virtual std::string GetSessionId() = 0;
    virtual std::string GetDisplayName() = 0;
    virtual std::vector<std::string> GetSupportedNamespaces() = 0;

    // Stops the application, if running.
    virtual void Stop() = 0;

   protected:
    virtual ~Application();
  };

  ApplicationAgent(
      TaskRunner* task_runner,
      DeviceAuthNamespaceHandler::CredentialsProvider* credentials_provider);

  ~ApplicationAgent() final;

  // Return the interface by which the CastSocket inbound traffic is delivered
  // into this agent and any running Applications.
  CastSocket::Client* cast_socket_client() { return &router_; }

  // Registers an Application for launching by this agent. |app| must outlive
  // this ApplicationAgent, or until UnregisterApplication() is called.
  void RegisterApplication(Application* app,
                           bool auto_launch_for_idle_screen = false);
  void UnregisterApplication(Application* app);

  // Stops the given |app| if it is the one currently running. This is used by
  // applications that encounter "exit" conditions where they need to STOP
  // (e.g., due to timeout of user activity, end of media playback, or fatal
  // errors).
  void StopApplicationIfRunning(Application* app);

 private:
  // ReceiverSocketFactory::Client overrides.
  void OnConnected(ReceiverSocketFactory* factory,
                   const IPEndpoint& endpoint,
                   std::unique_ptr<CastSocket> socket) final;
  void OnError(ReceiverSocketFactory* factory, Error error) final;

  // CastMessageHandler overrides.
  void OnMessage(VirtualConnectionRouter* router,
                 CastSocket* socket,
                 ::cast::channel::CastMessage message) final;

  // ConnectionNamespaceHandler::VirtualConnectionPolicy overrides.
  bool IsConnectionAllowed(const VirtualConnection& virtual_conn) const final;

  // VirtualConnectionRouter::SocketErrorHandler overrides.
  void OnClose(CastSocket* socket) final;
  void OnError(CastSocket* socket, Error error) final;

  // OnMessage() delegates to these to take action for each |request|. Each of
  // these returns a non-empty response message if a reply should be sent back
  // to the requestor.
  Json::Value HandlePing();
  Json::Value HandleGetAppAvailability(const Json::Value& request);
  Json::Value HandleGetStatus(const Json::Value& request);
  Json::Value HandleLaunch(const Json::Value& request, CastSocket* socket);
  Json::Value HandleStop(const Json::Value& request);
  Json::Value HandleInvalidCommand(const Json::Value& request);

  // Stops the currently-running Application and attempts to launch the
  // Application referred to by |app_id|. If this fails, the "idle screen"
  // Application will be automatically launched as a failure fall-back. |socket|
  // is non-null only when the application switch was caused by a remote LAUNCH
  // request.
  Error SwitchToApplication(std::string app_id,
                            const Json::Value& app_params,
                            CastSocket* socket);

  // Stops the currently-running Application and launches the "idle screen."
  void GoIdle();

  // Populates the given |message| object with the RECEIVER_STATUS fields,
  // reflecting the currently-launched app (if any), and a fake volume level
  // status.
  void PopulateReceiverStatus(Json::Value* message);

  // Broadcasts new RECEIVER_STATUS to all endpoints. This is called after an
  // Application LAUNCH or STOP.
  void BroadcastReceiverStatus();

  TaskRunner* const task_runner_;
  DeviceAuthNamespaceHandler auth_handler_;
  VirtualConnectionRouter router_;
  ConnectionNamespaceHandler connection_handler_;

  std::map<std::string, Application*> registered_applications_;
  Application* idle_screen_app_ = nullptr;

  CastSocketMessagePort message_port_;
  Application* launched_app_ = nullptr;
  std::string launched_via_app_id_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_RECEIVER_APPLICATION_AGENT_H_
