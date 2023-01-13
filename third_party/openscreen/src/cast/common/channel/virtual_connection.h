// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CHANNEL_VIRTUAL_CONNECTION_H_
#define CAST_COMMON_CHANNEL_VIRTUAL_CONNECTION_H_

#include <array>
#include <cstdint>
#include <string>

namespace openscreen {
namespace cast {

// Transport system on top of CastSocket that allows routing messages over a
// single socket to different virtual endpoints (e.g. system messages vs.
// messages for a particular app).
struct VirtualConnection {
  // Virtual connections can have slightly different semantics for a particular
  // endpoint based on its type.
  enum class Type : int8_t {
    // Normal connections.  Receiver applications should not exit while they
    // still have strong connections open (e.g. active senders).
    kStrong = 0,

    // Same as strong except if the connected endpoint is a receiver
    // application, it may stop if its only remaining open connections are all
    // weak.
    kWeak = 1,

    // Receiver applications do not receive connected/disconnected notifications
    // about these connections.  The following additional conditions apply:
    //  - Receiver app can still receive "urn:x-cast:com.google.cast.media"
    //    messages over invisible connections.
    //  - Receiver app can only send broadcast messages over an invisible
    //    connection.
    kInvisible = 2,

    kMinValue = kStrong,
    kMaxValue = kInvisible,
  };

  // Cast V2 protocol version constants.  Must be in sync with
  // proto/cast_channel.proto.
  enum class ProtocolVersion {
    kV2_1_0,
    kV2_1_1,
    kV2_1_2,
    kV2_1_3,
  };

  enum CloseReason {
    kUnknown,
    kFirstReason = kUnknown,

    // Underlying socket has been closed by peer. This happens when Cast sender
    // closed transport connection normally without graceful virtual connection
    // close. Though it is not an error, graceful virtual connection in advance
    // is better.
    kTransportClosed,

    // Underlying socket has been aborted by peer. Peer is no longer reachable
    // because of app crash or network error.
    kTransportAborted,

    // Messages sent from peer are in wrong format or too long.
    kTransportInvalidMessage,

    // Underlying socket has been idle for a long period. This only happens when
    // heartbeat is enabled and there is a network error.
    kTransportTooLongInactive,

    // The virtual connection has been closed by this endpoint.
    kClosedBySelf,

    // The virtual connection has been closed by the peer gracefully.
    kClosedByPeer,
    kLastReason = kClosedByPeer,
  };

  struct AssociatedData {
    Type type;
    std::string user_agent;

    // Last two bytes of the peer's IP address, whether IPv4 or IPv6.
    std::array<uint8_t, 2> ip_fragment;

    ProtocolVersion max_protocol_version;
  };

  // |local_id| and |peer_id| can be one of several formats:
  //  - sender-0 or receiver-0: identifies the appropriate platform endpoint of
  //    the device.  Authentication and transport-related messages use these.
  //  - sender-12345: Possible form of a Cast sender ID.  The number portion is
  //    intended to be unique within that device (i.e., unique per CastSocket).
  //  - Random decimal number: Possible form of a Cast sender ID.  Also randomly
  //    intended to be unique within that device (i.e., unique per CastSocket).
  //  - GUID-style hex string: Random string identifying a particular receiver
  //    app on the device.
  //
  // Additionally, |peer_id| can be an asterisk when broadcast-sending.
  std::string local_id;
  std::string peer_id;
  int socket_id;
};

inline bool operator==(const VirtualConnection& a, const VirtualConnection& b) {
  return a.local_id == b.local_id && a.peer_id == b.peer_id &&
         a.socket_id == b.socket_id;
}

inline bool operator!=(const VirtualConnection& a, const VirtualConnection& b) {
  return !(a == b);
}

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CHANNEL_VIRTUAL_CONNECTION_H_
