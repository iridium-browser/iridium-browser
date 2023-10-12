// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_TLS_DATA_ROUTER_POSIX_H_
#define PLATFORM_IMPL_TLS_DATA_ROUTER_POSIX_H_

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "platform/api/time.h"
#include "platform/impl/socket_handle_waiter.h"
#include "util/osp_logging.h"

namespace openscreen {

class StreamSocketPosix;
class TlsConnectionPosix;

// This class is responsible for 3 operations:
//   1) Listen for incoming connections on registed StreamSockets.
//   2) Check all registered TlsConnections for read data via boringSSL call
//      and pass all read data to the connection's observer.
//   3) Check all registered TlsConnections' write buffers for additional data
//      to be written out. If any is present, write it using boringSSL.
// The above operations also imply that this class must support registration
// of StreamSockets and TlsConnections.
// These operations will be called repeatedly on the networking thread, so none
// of them should block. Additionally, this class must ensure that deletions of
// the above types do not occur while a socket/connection is currently being
// accessed from the networking thread.
class TlsDataRouterPosix : public SocketHandleWaiter::Subscriber {
 public:
  class SocketObserver {
   public:
    virtual ~SocketObserver() = default;

    // Socket creation shouldn't occur on the Networking thread, so pass the
    // socket to the observer and expect them to call socket->Accept() on the
    // correct thread.
    virtual void OnConnectionPending(StreamSocketPosix* socket) = 0;
  };

  // The provided SocketHandleWaiter is expected to live for the duration of
  // this object's lifetime.
  TlsDataRouterPosix(
      SocketHandleWaiter* waiter,
      std::function<Clock::time_point()> now_function = Clock::now);
  ~TlsDataRouterPosix() override;

  // Register a TlsConnection that should be watched for readable and writable
  // data.
  void RegisterConnection(TlsConnectionPosix* connection);

  // Deregister a TlsConnection.
  void DeregisterConnection(TlsConnectionPosix* connection);

  // Takes ownership of a StreamSocket and registers that it should be watched
  // for incoming TCP connections with the SocketHandleWaiter.
  void RegisterAcceptObserver(std::unique_ptr<StreamSocketPosix> socket,
                              SocketObserver* observer);

  // Stops watching TCP sockets added by a particular observer for incoming
  // connections.
  void DeregisterAcceptObserver(SocketObserver* observer);

  // SocketHandleWaiter::Subscriber overrides.
  void ProcessReadyHandle(SocketHandleWaiter::SocketHandleRef handle,
                          uint32_t flags) override;

  OSP_DISALLOW_COPY_AND_ASSIGN(TlsDataRouterPosix);

 protected:
  // Determines if the provided socket is currently being watched by this
  // instance.
  bool IsSocketWatched(StreamSocketPosix* socket) const;

  virtual bool HasTimedOut(Clock::time_point start_time,
                           Clock::duration timeout);

  friend class TestingDataRouter;

  bool disable_locking_for_testing_ = false;

 private:
  SocketHandleWaiter* waiter_;

  // Mutex guarding connections_ vector.
  mutable std::mutex connections_mutex_;

  // Mutex guarding |accept_socket_mappings_|.
  mutable std::mutex accept_socket_mutex_;

  // Function to get the current time.
  std::function<Clock::time_point()> now_function_;

  // Mapping from all sockets to the observer that should be called when the
  // socket recognizes an incoming connection.
  std::unordered_map<StreamSocketPosix*, SocketObserver*>
      accept_socket_mappings_ ABSL_GUARDED_BY(accept_socket_mutex_);

  // Set of all TlsConnectionPosix objects currently registered.
  std::vector<TlsConnectionPosix*> connections_
      ABSL_GUARDED_BY(connections_mutex_);

  // StreamSockets currently owned by this object, being watched for
  std::vector<std::unique_ptr<StreamSocketPosix>> accept_stream_sockets_
      ABSL_GUARDED_BY(accept_socket_mutex_);
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_TLS_DATA_ROUTER_POSIX_H_
