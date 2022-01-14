/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef THIRD_PARTY_LIBJINGLE_XMPP_XMPP_XMPPCLIENTSETTINGS_H_
#define THIRD_PARTY_LIBJINGLE_XMPP_XMPP_XMPPCLIENTSETTINGS_H_

#include "net/base/host_port_pair.h"
#include "third_party/libjingle_xmpp/xmpp/xmppengine.h"

namespace jingle_xmpp {

enum ProtocolType {
  PROTO_TCP = 1,
  PROTO_SSLTCP = 2,  // Pseudo-TLS.
};

class XmppUserSettings {
 public:
  XmppUserSettings()
    : use_tls_(jingle_xmpp::TLS_DISABLED),
      allow_plain_(false) {
  }

  void set_user(const std::string& user) { user_ = user; }
  void set_host(const std::string& host) { host_ = host; }
  void set_pass(const std::string& pass) { pass_ = pass; }
  void set_auth_token(const std::string& mechanism,
                      const std::string& token) {
    auth_mechanism_ = mechanism;
    auth_token_ = token;
  }
  void set_resource(const std::string& resource) { resource_ = resource; }
  void set_use_tls(const TlsOptions use_tls) { use_tls_ = use_tls; }
  void set_allow_plain(bool f) { allow_plain_ = f; }
  void set_token_service(const std::string& token_service) {
    token_service_ = token_service;
  }

  const std::string& user() const { return user_; }
  const std::string& host() const { return host_; }
  const std::string& pass() const { return pass_; }
  const std::string& auth_mechanism() const { return auth_mechanism_; }
  const std::string& auth_token() const { return auth_token_; }
  const std::string& resource() const { return resource_; }
  TlsOptions use_tls() const { return use_tls_; }
  bool allow_plain() const { return allow_plain_; }
  const std::string& token_service() const { return token_service_; }

 private:
  std::string user_;
  std::string host_;
  std::string pass_;
  std::string auth_mechanism_;
  std::string auth_token_;
  std::string resource_;
  TlsOptions use_tls_;
  bool allow_plain_;
  std::string token_service_;
};

class XmppClientSettings : public XmppUserSettings {
 public:
  XmppClientSettings() : protocol_(PROTO_TCP) {}

  void set_server(const net::HostPortPair& server) { server_ = server; }
  void set_protocol(ProtocolType protocol) { protocol_ = protocol; }

  const net::HostPortPair& server() const { return server_; }
  ProtocolType protocol() const { return protocol_; }

 private:
  net::HostPortPair server_;
  ProtocolType protocol_;
};

}

#endif  // THIRD_PARTY_LIBJINGLE_XMPP_XMPP_XMPPCLIENT_H_
