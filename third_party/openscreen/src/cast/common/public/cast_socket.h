// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_PUBLIC_CAST_SOCKET_H_
#define CAST_COMMON_PUBLIC_CAST_SOCKET_H_

#include <array>
#include <memory>
#include <vector>

#include "platform/api/tls_connection.h"
#include "util/weak_ptr.h"

namespace cast {
namespace channel {
class CastMessage;
}  // namespace channel
}  // namespace cast

namespace openscreen {
namespace cast {

// Represents a simple message-oriented socket for communicating with the Cast
// V2 protocol.  It isn't thread-safe, so it should only be used on the same
// TaskRunner thread as its TlsConnection.
class CastSocket : public TlsConnection::Client {
 public:
  class Client {
   public:

    // Called when a terminal error on |socket| has occurred.
    virtual void OnError(CastSocket* socket, Error error) = 0;

    virtual void OnMessage(CastSocket* socket,
                           ::cast::channel::CastMessage message) = 0;

   protected:
    virtual ~Client();
  };

  CastSocket(std::unique_ptr<TlsConnection> connection, Client* client);
  ~CastSocket();

  // Sends |message| immediately unless the underlying TLS connection is
  // write-blocked, in which case |message| will be queued.  An error will be
  // returned if |message| cannot be serialized for any reason, even while
  // write-blocked.
  //
  // NOTE: Send() does not validate that |message| is well-formed or
  // semantically correct according to the Cast protocol.  Callers should use
  // the functions in {sender,receiver}/channel/message_util.h to construct a
  // valid CastMessage to pass into Send().
  [[nodiscard]] Error Send(const ::cast::channel::CastMessage& message);

  void SetClient(Client* client);

  std::array<uint8_t, 2> GetSanitizedIpAddress();

  int socket_id() const { return socket_id_; }

  void set_audio_only(bool audio_only) { audio_only_ = audio_only; }
  bool audio_only() const { return audio_only_; }

  // TlsConnection::Client overrides.
  void OnError(TlsConnection* connection, Error error) override;
  void OnRead(TlsConnection* connection, std::vector<uint8_t> block) override;

  WeakPtr<CastSocket> GetWeakPtr() const { return weak_factory_.GetWeakPtr(); }

 private:
  enum class State : bool {
    kOpen = true,
    kError = false,
  };

  static int g_next_socket_id_;

  const std::unique_ptr<TlsConnection> connection_;
  Client* client_;  // May never be null.
  const int socket_id_;
  bool audio_only_ = false;
  std::vector<uint8_t> read_buffer_;
  State state_ = State::kOpen;

  WeakPtrFactory<CastSocket> weak_factory_{this};
};

// Returns socket->socket_id() if |socket| is not null, otherwise 0.
constexpr int ToCastSocketId(CastSocket* socket) {
  return socket ? socket->socket_id() : 0;
}

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_PUBLIC_CAST_SOCKET_H_
