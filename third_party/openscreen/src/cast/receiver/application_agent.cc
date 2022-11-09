// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/receiver/application_agent.h"

#include <utility>

#include "cast/common/channel/message_util.h"
#include "cast/common/channel/virtual_connection.h"
#include "cast/common/public/cast_socket.h"
#include "platform/base/tls_credentials.h"
#include "platform/base/tls_listen_options.h"
#include "util/json/json_helpers.h"
#include "util/json/json_serialization.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

// Returns the first app ID for the given |app|, or the empty string if there is
// none.
std::string GetFirstAppId(ApplicationAgent::Application* app) {
  const auto& app_ids = app->GetAppIds();
  return app_ids.empty() ? std::string() : app_ids.front();
}

}  // namespace

ApplicationAgent::ApplicationAgent(
    TaskRunner* task_runner,
    DeviceAuthNamespaceHandler::CredentialsProvider* credentials_provider)
    : task_runner_(task_runner),
      auth_handler_(credentials_provider),
      connection_handler_(&router_, this),
      message_port_(&router_) {
  router_.AddHandlerForLocalId(kPlatformReceiverId, this);
}

ApplicationAgent::~ApplicationAgent() {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  idle_screen_app_ = nullptr;  // Prevent re-launching the idle screen app.
  SwitchToApplication({}, {}, nullptr);

  router_.RemoveHandlerForLocalId(kPlatformReceiverId);
}

void ApplicationAgent::RegisterApplication(Application* app,
                                           bool auto_launch_for_idle_screen) {
  OSP_DCHECK(app);

  for (const std::string& app_id : app->GetAppIds()) {
    OSP_DCHECK(!app_id.empty());
    const auto insert_result = registered_applications_.insert({app_id, app});
    // The insert must not fail (prior entry for same key).
    OSP_DCHECK(insert_result.second);
  }

  if (auto_launch_for_idle_screen) {
    OSP_DCHECK(!idle_screen_app_);
    idle_screen_app_ = app;
    // Launch the idle screen app, if no app was running.
    if (!launched_app_) {
      GoIdle();
    }
  }
}

void ApplicationAgent::UnregisterApplication(Application* app) {
  for (auto it = registered_applications_.begin();
       it != registered_applications_.end();) {
    if (it->second == app) {
      it = registered_applications_.erase(it);
    } else {
      ++it;
    }
  }

  if (idle_screen_app_ == app) {
    idle_screen_app_ = nullptr;
  }

  if (launched_app_ == app) {
    GoIdle();
  }
}

void ApplicationAgent::StopApplicationIfRunning(Application* app) {
  if (launched_app_ == app) {
    GoIdle();
  }
}

void ApplicationAgent::OnConnected(ReceiverSocketFactory* factory,
                                   const IPEndpoint& endpoint,
                                   std::unique_ptr<CastSocket> socket) {
  router_.TakeSocket(this, std::move(socket));
}

void ApplicationAgent::OnError(ReceiverSocketFactory* factory, Error error) {
  OSP_LOG_ERROR << "Cast agent received socket factory error: " << error;
}

void ApplicationAgent::OnMessage(VirtualConnectionRouter* router,
                                 CastSocket* socket,
                                 ::cast::channel::CastMessage message) {
  if (message_port_.GetSocketId() == ToCastSocketId(socket) &&
      !message_port_.source_id().empty() &&
      message_port_.source_id() == message.destination_id()) {
    OSP_DCHECK(message_port_.source_id() != kPlatformReceiverId);
    message_port_.OnMessage(router, socket, std::move(message));
    return;
  }

  if (message.destination_id() != kPlatformReceiverId &&
      message.destination_id() != kBroadcastId) {
    return;  // Message not for us.
  }

  const std::string& ns = message.namespace_();
  if (ns == kAuthNamespace) {
    auth_handler_.OnMessage(router, socket, std::move(message));
    return;
  }

  const ErrorOr<Json::Value> request = json::Parse(message.payload_utf8());
  if (request.is_error() || request.value().type() != Json::objectValue) {
    return;
  }

  Json::Value response;
  if (ns == kHeartbeatNamespace) {
    if (HasType(request.value(), CastMessageType::kPing)) {
      response = HandlePing();
    }
  } else if (ns == kReceiverNamespace) {
    if (request.value()[kMessageKeyRequestId].isNull()) {
      response = HandleInvalidCommand(request.value());
    } else if (HasType(request.value(), CastMessageType::kGetAppAvailability)) {
      response = HandleGetAppAvailability(request.value());
    } else if (HasType(request.value(), CastMessageType::kGetStatus)) {
      response = HandleGetStatus(request.value());
    } else if (HasType(request.value(), CastMessageType::kLaunch)) {
      response = HandleLaunch(request.value(), socket);
    } else if (HasType(request.value(), CastMessageType::kStop)) {
      response = HandleStop(request.value());
    } else {
      response = HandleInvalidCommand(request.value());
    }
  } else {
    // Ignore messages for all other namespaces.
  }

  if (!response.empty()) {
    router_.Send(VirtualConnection{message.destination_id(),
                                   message.source_id(), ToCastSocketId(socket)},
                 MakeSimpleUTF8Message(ns, json::Stringify(response).value()));
  }
}

bool ApplicationAgent::IsConnectionAllowed(
    const VirtualConnection& virtual_conn) const {
  if (virtual_conn.local_id == kPlatformReceiverId) {
    return true;
  }
  if (!launched_app_ || message_port_.source_id().empty()) {
    // No app currently launched. Or, there is a launched app, but it did not
    // call MessagePort::SetClient() to indicate it wants messages routed to it.
    return false;
  }
  return virtual_conn.local_id == message_port_.source_id();
}

void ApplicationAgent::OnClose(CastSocket* socket) {
  if (message_port_.GetSocketId() == ToCastSocketId(socket)) {
    OSP_VLOG << "Cast agent socket closed.";
    GoIdle();
  }
}

void ApplicationAgent::OnError(CastSocket* socket, Error error) {
  if (message_port_.GetSocketId() == ToCastSocketId(socket)) {
    OSP_LOG_ERROR << "Cast agent received socket error: " << error;
    GoIdle();
  }
}

Json::Value ApplicationAgent::HandlePing() {
  Json::Value response;
  response[kMessageKeyType] = CastMessageTypeToString(CastMessageType::kPong);
  return response;
}

Json::Value ApplicationAgent::HandleGetAppAvailability(
    const Json::Value& request) {
  Json::Value response;
  const Json::Value& app_ids = request[kMessageKeyAppId];
  if (app_ids.isArray()) {
    response[kMessageKeyRequestId] = request[kMessageKeyRequestId];
    response[kMessageKeyResponseType] = request[kMessageKeyType];
    Json::Value& availability = response[kMessageKeyAvailability];
    for (const Json::Value& app_id : app_ids) {
      if (app_id.isString()) {
        const auto app_id_str = app_id.asString();
        availability[app_id_str] = registered_applications_.count(app_id_str)
                                       ? kMessageValueAppAvailable
                                       : kMessageValueAppUnavailable;
      }
    }
  }
  return response;
}

Json::Value ApplicationAgent::HandleGetStatus(const Json::Value& request) {
  Json::Value response;
  PopulateReceiverStatus(&response);
  response[kMessageKeyRequestId] = request[kMessageKeyRequestId];
  return response;
}

Json::Value ApplicationAgent::HandleLaunch(const Json::Value& request,
                                           CastSocket* socket) {
  const Json::Value& app_id = request[kMessageKeyAppId];
  Error error;
  if (app_id.isString() && !app_id.asString().empty()) {
    error = SwitchToApplication(app_id.asString(),
                                request[kMessageKeyAppParams], socket);
  } else {
    error = Error(Error::Code::kParameterInvalid, kMessageValueBadParameter);
  }
  if (!error.ok()) {
    Json::Value response;
    response[kMessageKeyRequestId] = request[kMessageKeyRequestId];
    response[kMessageKeyType] =
        CastMessageTypeToString(CastMessageType::kLaunchError);
    response[kMessageKeyReason] = error.message();
    return response;
  }

  // Note: No reply is sent. Instead, the requestor will get a RECEIVER_STATUS
  // broadcast message from SwitchToApplication(), which is how it will see that
  // the launch succeeded.
  return {};
}

Json::Value ApplicationAgent::HandleStop(const Json::Value& request) {
  const Json::Value& session_id = request[kMessageKeySessionId];
  if (session_id.isNull()) {
    GoIdle();
    return {};
  }

  if (session_id.isString() && launched_app_ &&
      session_id.asString() == launched_app_->GetSessionId()) {
    GoIdle();
    return {};
  }

  Json::Value response;
  response[kMessageKeyRequestId] = request[kMessageKeyRequestId];
  response[kMessageKeyType] =
      CastMessageTypeToString(CastMessageType::kInvalidRequest);
  response[kMessageKeyReason] = kMessageValueInvalidSessionId;
  return response;
}

Json::Value ApplicationAgent::HandleInvalidCommand(const Json::Value& request) {
  Json::Value response;
  if (request[kMessageKeyRequestId].isNull()) {
    return response;
  }
  response[kMessageKeyRequestId] = request[kMessageKeyRequestId];
  response[kMessageKeyType] =
      CastMessageTypeToString(CastMessageType::kInvalidRequest);
  response[kMessageKeyReason] = kMessageValueInvalidCommand;
  return response;
}

Error ApplicationAgent::SwitchToApplication(std::string app_id,
                                            const Json::Value& app_params,
                                            CastSocket* socket) {
  Error error = Error::Code::kNone;
  Application* desired_app = nullptr;
  Application* fallback_app = nullptr;
  if (!app_id.empty()) {
    const auto it = registered_applications_.find(app_id);
    if (it != registered_applications_.end()) {
      desired_app = it->second;
      if (desired_app != idle_screen_app_) {
        fallback_app = idle_screen_app_;
      }
    } else {
      return Error(Error::Code::kItemNotFound, kMessageValueNotFound);
    }
  }

  if (launched_app_ == desired_app) {
    return error;
  }

  if (launched_app_) {
    launched_app_->Stop();
    message_port_.SetSocket({});
    launched_app_ = nullptr;
    launched_via_app_id_ = {};
  }

  if (desired_app) {
    if (socket) {
      message_port_.SetSocket(socket->GetWeakPtr());
    }
    if (desired_app->Launch(app_id, app_params, &message_port_)) {
      launched_app_ = desired_app;
      launched_via_app_id_ = std::move(app_id);
    } else {
      error = Error(Error::Code::kUnknownError, kMessageValueSystemError);
      message_port_.SetSocket({});
    }
  }

  if (!launched_app_ && fallback_app) {
    app_id = GetFirstAppId(fallback_app);
    if (fallback_app->Launch(app_id, {}, &message_port_)) {
      launched_app_ = fallback_app;
      launched_via_app_id_ = std::move(app_id);
    }
  }

  BroadcastReceiverStatus();

  return error;
}

void ApplicationAgent::GoIdle() {
  std::string app_id;
  if (idle_screen_app_) {
    app_id = GetFirstAppId(idle_screen_app_);
  }
  SwitchToApplication(app_id, {}, nullptr);
}

void ApplicationAgent::PopulateReceiverStatus(Json::Value* out) {
  Json::Value& message = *out;
  message[kMessageKeyType] =
      CastMessageTypeToString(CastMessageType::kReceiverStatus);
  Json::Value& status = message[kMessageKeyStatus];

  if (launched_app_) {
    Json::Value& details = status[kMessageKeyApplications][0];
    // If the Application can send/receive messages, the destination for such
    // messages is provided here, in |transportId|. However, the other end must
    // first set up the virtual connection by issuing a CONNECT request.
    // Otherwise, messages will not get routed to the Application by the
    // VirtualConnectionRouter.
    if (!message_port_.source_id().empty()) {
      details[kMessageKeyTransportId] = message_port_.source_id();
    }
    details[kMessageKeySessionId] = launched_app_->GetSessionId();
    details[kMessageKeyAppId] = launched_via_app_id_;
    details[kMessageKeyUniversalAppId] = launched_via_app_id_;
    details[kMessageKeyDisplayName] = launched_app_->GetDisplayName();
    details[kMessageKeyIsIdleScreen] = (launched_app_ == idle_screen_app_);
    details[kMessageKeyLaunchedFromCloud] = false;
    std::vector<std::string> app_namespaces =
        launched_app_->GetSupportedNamespaces();
    Json::Value& namespaces =
        (details[kMessageKeyNamespaces] = Json::Value(Json::arrayValue));
    for (int i = 0, count = app_namespaces.size(); i < count; ++i) {
      namespaces[i][kMessageKeyName] = std::move(app_namespaces[i]);
    }
  }

  status[kMessageKeyUserEq] = Json::Value(Json::objectValue);

  // Indicate a fixed 100% volume level.
  Json::Value& volume = status[kMessageKeyVolume];
  volume[kMessageKeyControlType] = kMessageValueAttenuation;
  volume[kMessageKeyLevel] = 1.0;
  volume[kMessageKeyMuted] = false;
  volume[kMessageKeyStepInterval] = 0.05;
}

void ApplicationAgent::BroadcastReceiverStatus() {
  Json::Value message;
  PopulateReceiverStatus(&message);
  message[kMessageKeyRequestId] = Json::Value(0);  // Indicates no requestor.
  router_.BroadcastFromLocalPeer(
      kPlatformReceiverId,
      MakeSimpleUTF8Message(kReceiverNamespace,
                            json::Stringify(message).value()));
}

ApplicationAgent::Application::~Application() = default;

}  // namespace cast
}  // namespace openscreen
