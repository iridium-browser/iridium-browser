/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef THIRD_PARTY_LIBJINGLE_XMPP_XMPP_UTIL_UNITTEST_H_
#define THIRD_PARTY_LIBJINGLE_XMPP_XMPP_UTIL_UNITTEST_H_

#include <sstream>
#include <string>
#include "third_party/libjingle_xmpp/xmpp/xmppengine.h"

namespace jingle_xmpp {

// This class captures callbacks from engine.
class XmppTestHandler : public XmppOutputHandler,  public XmppSessionHandler,
                        public XmppStanzaHandler {
 public:
  explicit XmppTestHandler(XmppEngine* engine) : engine_(engine) {}
  virtual ~XmppTestHandler() {}

  void SetEngine(XmppEngine* engine);

  // Output handler
  virtual void WriteOutput(const char * bytes, size_t len);
  virtual void StartTls(const std::string & cname);
  virtual void CloseConnection();

  // Session handler
  virtual void OnStateChange(int state);

  // Stanza handler
  virtual bool HandleStanza(const XmlElement* stanza);

  std::string OutputActivity();
  std::string SessionActivity();
  std::string StanzaActivity();

 private:
  XmppEngine* engine_;
  std::stringstream output_;
  std::stringstream session_;
  std::stringstream stanza_;
};

}  // namespace jingle_xmpp

inline std::ostream& operator<<(std::ostream& os, const jingle_xmpp::Jid& jid) {
  os << jid.Str();
  return os;
}

#endif  // THIRD_PARTY_LIBJINGLE_XMPP_XMPP_UTIL_UNITTEST_H_
