// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/url.h"

#include <limits.h>

#include <utility>

#include "third_party/mozilla/url_parse.h"
#include "third_party/mozilla/url_parse_internal.h"

namespace openscreen {

Url::Url(const std::string& source) {
  Parsed parsed;
  Component scheme;
  const char* url = source.c_str();
  size_t length = source.size();
  if (length > INT_MAX) {
    return;
  }
  int url_length = static_cast<int>(length);

  if (!ExtractScheme(url, url_length, &scheme)) {
    return;
  }

  if (CompareSchemeComponent(url, scheme, kFileScheme) ||
      CompareSchemeComponent(url, scheme, kFileSystemScheme) ||
      CompareSchemeComponent(url, scheme, kMailtoScheme)) {
    // NOTE: Special schemes that are unsupported.
    return;
  } else if (IsStandard(url, scheme)) {
    ParseStandardURL(url, url_length, &parsed);
    if (!parsed.host.is_valid()) {
      return;
    }
  } else {
    ParsePathURL(url, url_length, true, &parsed);
  }

  if (!parsed.scheme.is_nonempty()) {
    return;
  }
  scheme_ = std::string(url + parsed.scheme.begin, url + parsed.scheme.end());

  if (parsed.host.is_valid()) {
    has_host_ = true;
    host_ = std::string(url + parsed.host.begin, url + parsed.host.end());
  }

  if (parsed.port.is_nonempty()) {
    int parse_result = ParsePort(url, parsed.port);
    if (parse_result == PORT_INVALID) {
      return;
    } else if (parse_result >= 0) {
      has_port_ = true;
      port_ = parse_result;
    }
  }

  if (parsed.path.is_nonempty()) {
    has_path_ = true;
    path_ = std::string(url + parsed.path.begin, url + parsed.path.end());
  }

  if (parsed.query.is_nonempty()) {
    has_query_ = true;
    query_ = std::string(url + parsed.query.begin, url + parsed.query.end());
  }

  is_valid_ = true;
}

Url::Url(const Url&) = default;

Url::Url(Url&& other) noexcept
    : is_valid_(other.is_valid_),
      has_host_(other.has_host_),
      has_port_(other.has_port_),
      has_path_(other.has_path_),
      has_query_(other.has_query_),
      scheme_(std::move(other.scheme_)),
      host_(std::move(other.host_)),
      port_(other.port_),
      path_(std::move(other.path_)),
      query_(std::move(other.query_)) {
  other.is_valid_ = false;
}

Url::~Url() = default;

Url& Url::operator=(const Url&) = default;

Url& Url::operator=(Url&& other) {
  is_valid_ = other.is_valid_;
  has_host_ = other.has_host_;
  has_port_ = other.has_port_;
  has_path_ = other.has_path_;
  has_query_ = other.has_query_;
  scheme_ = std::move(other.scheme_);
  host_ = std::move(other.host_);
  port_ = other.port_;
  path_ = std::move(other.path_);
  query_ = std::move(other.query_);
  other.is_valid_ = false;
  return *this;
}

}  // namespace openscreen
