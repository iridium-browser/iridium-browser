// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_ITEM_H_
#define TOOLS_GN_ITEM_H_

#include <set>
#include <string>

#include "gn/label.h"
#include "gn/source_file.h"
#include "gn/visibility.h"

class Config;
class ParseNode;
class Pool;
class Settings;
class SourceFile;
class Target;
class Toolchain;

// A named item (target, config, etc.) that participates in the dependency
// graph.
class Item {
 public:
  Item(const Settings* settings,
       const Label& label,
       const SourceFileSet& build_dependency_files = {});
  virtual ~Item();

  const Settings* settings() const { return settings_; }

  // This is guaranteed to never change after construction so this can be
  // accessed from any thread with no locking once the item is constructed.
  const Label& label() const { return label_; }

  const ParseNode* defined_from() const { return defined_from_; }
  void set_defined_from(const ParseNode* df) { defined_from_ = df; }

  bool testonly() const { return testonly_; }
  void set_testonly(bool value) { testonly_ = value; }

  Visibility& visibility() { return visibility_; }
  const Visibility& visibility() const { return visibility_; }

  // Manual RTTI.
  virtual Config* AsConfig();
  virtual const Config* AsConfig() const;
  virtual Pool* AsPool();
  virtual const Pool* AsPool() const;
  virtual Target* AsTarget();
  virtual const Target* AsTarget() const;
  virtual Toolchain* AsToolchain();
  virtual const Toolchain* AsToolchain() const;

  // Returns a name like "target" or "config" for the type of item this is, to
  // be used in logging and error messages.
  std::string GetItemTypeName() const;

  // Returns the set of build files that may affect this item, please refer to
  // Scope for how this is determined.
  const SourceFileSet& build_dependency_files() const {
    return build_dependency_files_;
  }

  SourceFileSet& build_dependency_files() { return build_dependency_files_; }

  // Called when this item is resolved, meaning it and all of its dependents
  // have no unresolved deps. Returns true on success. Sets the error and
  // returns false on failure.
  virtual bool OnResolved(Err* err);

 private:
  bool CheckTestonly(Err* err) const;

  const Settings* settings_;
  Label label_;
  SourceFileSet build_dependency_files_;
  const ParseNode* defined_from_;

  bool testonly_ = false;
  Visibility visibility_;
};

#endif  // TOOLS_GN_ITEM_H_
