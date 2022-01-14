// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/notifier/listener/push_notifications_subscribe_task.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "jingle/notifier/listener/notification_constants.h"
#include "jingle/notifier/listener/xml_element_util.h"
#include "third_party/libjingle_xmpp/task_runner/task.h"
#include "third_party/libjingle_xmpp/xmllite/qname.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"
#include "third_party/libjingle_xmpp/xmpp/constants.h"
#include "third_party/libjingle_xmpp/xmpp/xmppclient.h"
#include "third_party/libjingle_xmpp/xmpp/xmppengine.h"

namespace notifier {

PushNotificationsSubscribeTask::PushNotificationsSubscribeTask(
    jingle_xmpp::XmppTaskParentInterface* parent,
    const SubscriptionList& subscriptions,
    Delegate* delegate)
    : XmppTask(parent, jingle_xmpp::XmppEngine::HL_SINGLE),
      subscriptions_(subscriptions), delegate_(delegate) {
}

PushNotificationsSubscribeTask::~PushNotificationsSubscribeTask() {
}

bool PushNotificationsSubscribeTask::HandleStanza(
    const jingle_xmpp::XmlElement* stanza) {
  if (!MatchResponseIq(stanza, GetClient()->jid().BareJid(), task_id()))
    return false;
  QueueStanza(stanza);
  return true;
}

int PushNotificationsSubscribeTask::ProcessStart() {
  DVLOG(1) << "Push notifications: Subscription task started.";
  std::unique_ptr<jingle_xmpp::XmlElement> iq_stanza(
      MakeSubscriptionMessage(subscriptions_, GetClient()->jid(), task_id()));
  DVLOG(1) << "Push notifications: Subscription stanza: "
          << XmlElementToString(*iq_stanza.get());

  if (SendStanza(iq_stanza.get()) != jingle_xmpp::XMPP_RETURN_OK) {
    if (delegate_)
      delegate_->OnSubscriptionError();
    return STATE_ERROR;
  }
  return STATE_RESPONSE;
}

int PushNotificationsSubscribeTask::ProcessResponse() {
  DVLOG(1) << "Push notifications: Subscription response received.";
  const jingle_xmpp::XmlElement* stanza = NextStanza();
  if (stanza == NULL) {
    return STATE_BLOCKED;
  }
  DVLOG(1) << "Push notifications: Subscription response: "
           << XmlElementToString(*stanza);
  // We've received a response to our subscription request.
  if (stanza->HasAttr(jingle_xmpp::QN_TYPE) &&
    stanza->Attr(jingle_xmpp::QN_TYPE) == jingle_xmpp::STR_RESULT) {
    if (delegate_)
      delegate_->OnSubscribed();
    return STATE_DONE;
  }
  // An error response was received.
  if (delegate_)
    delegate_->OnSubscriptionError();
  return STATE_ERROR;
}

jingle_xmpp::XmlElement* PushNotificationsSubscribeTask::MakeSubscriptionMessage(
    const SubscriptionList& subscriptions,
    const jingle_xmpp::Jid& jid, const std::string& task_id) {
  DCHECK(jid.IsFull());
  const jingle_xmpp::QName kQnSubscribe(
      kPushNotificationsNamespace, "subscribe");

  // Create the subscription stanza using the notifications protocol.
  // <iq from={full_jid} to={bare_jid} type="set" id={id}>
  //  <subscribe xmlns="google:push">
  //    <item channel={channel_name} from={domain_name or bare_jid}/>
  //    <item channel={channel_name2} from={domain_name or bare_jid}/>
  //    <item channel={channel_name3} from={domain_name or bare_jid}/>
  //  </subscribe>
  // </iq>
  jingle_xmpp::XmlElement* iq = MakeIq(jingle_xmpp::STR_SET, jid.BareJid(), task_id);
  jingle_xmpp::XmlElement* subscribe = new jingle_xmpp::XmlElement(kQnSubscribe, true);
  iq->AddElement(subscribe);

  for (SubscriptionList::const_iterator iter =
           subscriptions.begin(); iter != subscriptions.end(); ++iter) {
    jingle_xmpp::XmlElement* item = new jingle_xmpp::XmlElement(
        jingle_xmpp::QName(kPushNotificationsNamespace, "item"));
    item->AddAttr(jingle_xmpp::QName(jingle_xmpp::STR_EMPTY, "channel"),
                  iter->channel.c_str());
    item->AddAttr(jingle_xmpp::QN_FROM, iter->from.c_str());
    subscribe->AddElement(item);
  }
  return iq;
}

}  // namespace notifier
