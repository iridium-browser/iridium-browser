// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_NOTIFIER_COMMUNICATOR_LOGIN_SETTINGS_H_
#define JINGLE_NOTIFIER_COMMUNICATOR_LOGIN_SETTINGS_H_
#include <string>

#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "jingle/glue/network_service_config.h"
#include "jingle/notifier/base/server_information.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "third_party/libjingle_xmpp/xmpp/xmppclientsettings.h"

namespace notifier {

class LoginSettings {
 public:
  LoginSettings(const jingle_xmpp::XmppClientSettings& user_settings,
                jingle_glue::GetProxyResolvingSocketFactoryCallback
                    get_socket_factory_callback,
                const ServerList& default_servers,
                bool try_ssltcp_first,
                const std::string& auth_mechanism,
                const net::NetworkTrafficAnnotationTag& traffic_annotation);

  LoginSettings(const LoginSettings& other);

  ~LoginSettings();

  // Copy constructor and assignment operator welcome.

  const jingle_xmpp::XmppClientSettings& user_settings() const {
    return user_settings_;
  }

  void set_user_settings(const jingle_xmpp::XmppClientSettings& user_settings);

  jingle_glue::GetProxyResolvingSocketFactoryCallback
  get_socket_factory_callback() const {
    return get_socket_factory_callback_;
  }

  bool try_ssltcp_first() const {
    return try_ssltcp_first_;
  }

  const std::string& auth_mechanism() const {
    return auth_mechanism_;
  }

  ServerList GetServers() const;

  const net::NetworkTrafficAnnotationTag traffic_annotation() const {
    return traffic_annotation_;
  }

  // The redirect server will eventually expire.
  void SetRedirectServer(const ServerInformation& redirect_server);

  ServerList GetServersForTimeForTest(base::Time now) const;

  base::Time GetRedirectExpirationForTest() const;

 private:
  ServerList GetServersForTime(base::Time now) const;

  jingle_xmpp::XmppClientSettings user_settings_;
  jingle_glue::GetProxyResolvingSocketFactoryCallback
      get_socket_factory_callback_;
  ServerList default_servers_;
  bool try_ssltcp_first_;
  std::string auth_mechanism_;
  const net::NetworkTrafficAnnotationTag traffic_annotation_;

  // Used to handle redirects
  ServerInformation redirect_server_;
  base::Time redirect_expiration_;

};

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_COMMUNICATOR_LOGIN_SETTINGS_H_
