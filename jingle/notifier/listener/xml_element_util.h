// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_NOTIFIER_LISTENER_XML_ELEMENT_UTIL_H_
#define JINGLE_NOTIFIER_LISTENER_XML_ELEMENT_UTIL_H_

#include <string>

namespace jingle_xmpp {
class XmlElement;
}

namespace notifier {

std::string XmlElementToString(const jingle_xmpp::XmlElement& xml_element);

// The functions below are helpful for building notifications-related
// XML stanzas.

jingle_xmpp::XmlElement* MakeBoolXmlElement(const char* name, bool value);

jingle_xmpp::XmlElement* MakeIntXmlElement(const char* name, int value);

jingle_xmpp::XmlElement* MakeStringXmlElement(const char* name, const char* value);

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_LISTENER_XML_ELEMENT_UTIL_H_
