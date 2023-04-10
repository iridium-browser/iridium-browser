// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/swift_variables.h"

namespace variables {

// Swift target vars -----------------------------------------------------

const char kSwiftBridgeHeader[] = "bridge_header";
const char kSwiftBridgeHeader_HelpShort[] =
    "bridge_header: [string] Path to C/Objective-C compatibility header.";
const char kSwiftBridgeHeader_Help[] =
    R"(bridge_header: [string] Path to C/Objective-C compatibility header.

  Valid for binary targets that contain Swift sources.

  Path to an header that includes C/Objective-C functions and types that
  needs to be made available to the Swift module.
)";

const char kSwiftModuleName[] = "module_name";
const char kSwiftModuleName_HelpShort[] =
    "module_name: [string] The name for the compiled module.";
const char kSwiftModuleName_Help[] =
    R"(module_name: [string] The name for the compiled module.

  Valid for binary targets that contain Swift sources.

  If module_name is not set, then this rule will use the target name.
)";

void InsertSwiftVariables(VariableInfoMap* info_map) {
  info_map->insert(std::make_pair(
      kSwiftBridgeHeader,
      VariableInfo(kSwiftBridgeHeader_HelpShort, kSwiftBridgeHeader_Help)));

  info_map->insert(std::make_pair(
      kSwiftModuleName,
      VariableInfo(kSwiftModuleName_HelpShort, kSwiftModuleName_Help)));
}

}  // namespace variables
