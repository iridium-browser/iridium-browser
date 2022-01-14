// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_NOTIFIER_BASE_GAIA_TOKEN_PRE_XMPP_AUTH_H_
#define JINGLE_NOTIFIER_BASE_GAIA_TOKEN_PRE_XMPP_AUTH_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "third_party/libjingle_xmpp/xmpp/prexmppauth.h"

namespace notifier {

// This class implements jingle_xmpp::PreXmppAuth interface for token-based
// authentication in GTalk. It looks for the X-GOOGLE-TOKEN auth mechanism
// and uses that instead of the default auth mechanism (PLAIN).
class GaiaTokenPreXmppAuth : public jingle_xmpp::PreXmppAuth {
 public:
  GaiaTokenPreXmppAuth(const std::string& username, const std::string& token,
                       const std::string& token_service,
                       const std::string& auth_mechanism);

  ~GaiaTokenPreXmppAuth() override;

  // jingle_xmpp::PreXmppAuth (-jingle_xmpp::SaslHandler) implementation.  We stub
  // all the methods out as we don't actually do any authentication at
  // this point.
  void StartPreXmppAuth(const jingle_xmpp::Jid& jid,
                        const std::string& pass,
                        const std::string& auth_mechanism,
                        const std::string& auth_token) override;

  bool IsAuthDone() const override;

  bool IsAuthorized() const override;

  bool HadError() const override;

  int GetError() const override;

  jingle_xmpp::CaptchaChallenge GetCaptchaChallenge() const override;

  std::string GetAuthToken() const override;

  std::string GetAuthMechanism() const override;

  // jingle_xmpp::SaslHandler implementation.

  std::string ChooseBestSaslMechanism(
      const std::vector<std::string>& mechanisms,
      bool encrypted) override;

  jingle_xmpp::SaslMechanism* CreateSaslMechanism(
      const std::string& mechanism) override;

 private:
  std::string username_;
  std::string token_;
  std::string token_service_;
  std::string auth_mechanism_;
};

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_BASE_GAIA_TOKEN_PRE_XMPP_AUTH_H_
