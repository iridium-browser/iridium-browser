// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/ninja_writer.h"

#include "gn/builder.h"
#include "gn/loader.h"
#include "gn/location.h"
#include "gn/ninja_build_writer.h"
#include "gn/ninja_toolchain_writer.h"
#include "gn/settings.h"
#include "gn/target.h"

NinjaWriter::NinjaWriter(const Builder& builder) : builder_(builder) {}

NinjaWriter::~NinjaWriter() = default;

// static
bool NinjaWriter::RunAndWriteFiles(const BuildSettings* build_settings,
                                   const Builder& builder,
                                   const PerToolchainRules& per_toolchain_rules,
                                   Err* err) {
  NinjaWriter writer(builder);

  if (!writer.WriteToolchains(per_toolchain_rules, err))
    return false;
  return NinjaBuildWriter::RunAndWriteFile(build_settings, builder, err);
}

bool NinjaWriter::WriteToolchains(const PerToolchainRules& per_toolchain_rules,
                                  Err* err) {
  if (per_toolchain_rules.empty()) {
    *err = Err(Location(), "No targets.",
        "I could not find any targets to write, so I'm doing nothing.");
    return false;
  }

  for (const auto& i : per_toolchain_rules) {
    const Toolchain* toolchain = i.first;
    const Settings* settings =
        builder_.loader()->GetToolchainSettings(toolchain->label());
    if (!NinjaToolchainWriter::RunAndWriteFile(settings, toolchain, i.second)) {
      *err =
          Err(Location(), "Couldn't open toolchain buildfile(s) for writing");
      return false;
    }
  }

  return true;
}
