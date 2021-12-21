// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_MULTIDEVICE_SETUP_PUBLIC_CPP_URL_PROVIDER_H_
#define CHROMEOS_SERVICES_MULTIDEVICE_SETUP_PUBLIC_CPP_URL_PROVIDER_H_

#include "url/gurl.h"

namespace chromeos {

namespace multidevice_setup {

GURL GetBoardSpecificBetterTogetherSuiteLearnMoreUrl();
GURL GetBoardSpecificMessagesLearnMoreUrl();

}  // namespace multidevice_setup

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_MULTIDEVICE_SETUP_PUBLIC_CPP_URL_PROVIDER_H_
