// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_SWIFT_TARGET_VARIABLES_H_
#define TOOLS_GN_SWIFT_TARGET_VARIABLES_H_

#include "gn/variables.h"

namespace variables {

// Swift target vars -----------------------------------------------------

extern const char kSwiftBridgeHeader[];
extern const char kSwiftBridgeHeader_HelpShort[];
extern const char kSwiftBridgeHeader_Help[];

extern const char kSwiftModuleName[];
extern const char kSwiftModuleName_HelpShort[];
extern const char kSwiftModuleName_Help[];

void InsertSwiftVariables(VariableInfoMap* info_map);

}  // namespace variables

#endif  // TOOLS_GN_SWIFT_TARGET_VARIABLES_H_
