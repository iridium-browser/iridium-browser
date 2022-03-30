// Copyright 2022 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_HIBERMAN_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_HIBERMAN_DBUS_CONSTANTS_H_

namespace hiberman {

// hiberman
const char kHibernateResumeInterface[] =
    "org.chromium.HibernateResumeInterface";
const char kHibernateServicePath[] = "/org/chromium/Hibernate";
const char kHibernateServiceName[] = "org.chromium.Hibernate";

// Methods exposed by hiberman.
const char kResumeFromHibernateMethod[] = "ResumeFromHibernate";

}  // namespace hiberman

#endif  // SYSTEM_API_DBUS_HIBERMAN_DBUS_CONSTANTS_H_
