// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_VERSION_H_
#define TOOLS_GN_VERSION_H_

#include <optional>
#include <string>

#ifdef major
#undef major
#endif
#ifdef minor
#undef minor
#endif

// Represents a semantic version.
class Version {
 public:
  Version(int major, int minor, int patch);

  static std::optional<Version> FromString(std::string s);

  int major() const { return major_; }
  int minor() const { return minor_; }
  int patch() const { return patch_; }

  bool operator==(const Version& other) const;
  bool operator<(const Version& other) const;
  bool operator!=(const Version& other) const;
  bool operator>=(const Version& other) const;
  bool operator>(const Version& other) const;
  bool operator<=(const Version& other) const;

  std::string Describe() const;

 private:
  int major_;
  int minor_;
  int patch_;
};

#endif  // TOOLS_GN_VERSION_H_
