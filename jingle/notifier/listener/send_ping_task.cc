// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/notifier/listener/send_ping_task.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "jingle/notifier/listener/xml_element_util.h"
#include "third_party/libjingle_xmpp/xmllite/qname.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"
#include "third_party/libjingle_xmpp/xmpp/constants.h"
#include "third_party/libjingle_xmpp/xmpp/jid.h"
#include "third_party/libjingle_xmpp/xmpp/xmppclient.h"

namespace notifier {

SendPingTask::Delegate::~Delegate() {
}

SendPingTask::SendPingTask(jingle_xmpp::XmppTaskParentInterface* parent,
                           Delegate* delegate)
    : XmppTask(parent, jingle_xmpp::XmppEngine::HL_SINGLE), delegate_(delegate) {
}

SendPingTask::~SendPingTask() {
}

int SendPingTask::ProcessStart() {
  ping_task_id_ = task_id();
  std::unique_ptr<jingle_xmpp::XmlElement> stanza(MakePingStanza(ping_task_id_));
  DVLOG(1) << "Sending ping stanza " << XmlElementToString(*stanza);
  if (SendStanza(stanza.get()) != jingle_xmpp::XMPP_RETURN_OK) {
    DLOG(WARNING) << "Could not send stanza " << XmlElementToString(*stanza);
    return STATE_ERROR;
  }
  return STATE_RESPONSE;
}

int SendPingTask::ProcessResponse() {
  const jingle_xmpp::XmlElement* stanza = NextStanza();
  if (stanza == NULL) {
    return STATE_BLOCKED;
  }

  DVLOG(1) << "Received stanza " << XmlElementToString(*stanza);

  std::string type = stanza->Attr(jingle_xmpp::QN_TYPE);
  if (type != jingle_xmpp::STR_RESULT) {
    DLOG(WARNING) << "No type=\"result\" attribute found in stanza "
                  << XmlElementToString(*stanza);
    return STATE_ERROR;
  }

  delegate_->OnPingResponseReceived();
  return STATE_DONE;
}

bool SendPingTask::HandleStanza(const jingle_xmpp::XmlElement* stanza) {
  // MatchResponseIq() matches the given Jid with the "from" field of the given
  // stanza, which in this case should be the empty string
  // (signifying the server).
  if (MatchResponseIq(stanza, jingle_xmpp::Jid(jingle_xmpp::STR_EMPTY), ping_task_id_)) {
    QueueStanza(stanza);
    return true;
  }
  return false;
}

jingle_xmpp::XmlElement* SendPingTask::MakePingStanza(const std::string& task_id) {
  jingle_xmpp::XmlElement* stanza = MakeIq(jingle_xmpp::STR_GET,
                                    jingle_xmpp::Jid(jingle_xmpp::STR_EMPTY),
                                    task_id);
  stanza->AddElement(new jingle_xmpp::XmlElement(jingle_xmpp::QN_PING));
  return stanza;
}

}  // namespace notifier
