// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_URL_H_
#define UTIL_URL_H_

#include <cstdint>
#include <string>

#include "util/osp_logging.h"

namespace openscreen {

// Parses a URL and stores its components separately.  If parsing is successful,
// is_valid() will return true, otherwise no other members should be accessed.
// This is a thin wrapper around //third_party/mozilla.  It does not handle
// file: or mailto: URLs.
class Url {
 public:
  explicit Url(const std::string& source);
  Url(const Url&);
  Url(Url&&) noexcept;
  ~Url();

  Url& operator=(const Url&);
  Url& operator=(Url&&);

  // No other members should be accessed if this is false.
  bool is_valid() const { return is_valid_; }

  // A successfully parsed URL will always have a scheme.  All the other
  // components are optional and therefore have has_*() accessors for checking
  // their presence.
  const std::string& scheme() const {
    OSP_DCHECK(is_valid_);
    return scheme_;
  }
  bool has_host() const {
    OSP_DCHECK(is_valid_);
    return has_host_;
  }
  const std::string& host() const {
    OSP_DCHECK(is_valid_);
    return host_;
  }
  bool has_port() const {
    OSP_DCHECK(is_valid_);
    return has_port_;
  }
  int32_t port() const {
    OSP_DCHECK(is_valid_);
    return port_;
  }
  bool has_path() const {
    OSP_DCHECK(is_valid_);
    return has_path_;
  }
  const std::string& path() const {
    OSP_DCHECK(is_valid_);
    return path_;
  }
  bool has_query() const {
    OSP_DCHECK(is_valid_);
    return has_query_;
  }
  const std::string& query() const {
    OSP_DCHECK(is_valid_);
    return query_;
  }

 private:
  bool is_valid_ = false;
  bool has_host_ = false;
  bool has_port_ = false;
  bool has_path_ = false;
  bool has_query_ = false;

  std::string scheme_;
  std::string host_;
  int32_t port_ = 0;
  std::string path_;
  std::string query_;
};

}  // namespace openscreen

#endif  // UTIL_URL_H_
