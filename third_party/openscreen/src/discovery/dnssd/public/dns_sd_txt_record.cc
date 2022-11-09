// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/public/dns_sd_txt_record.h"

#include <cctype>
#include <utility>

namespace openscreen {
namespace discovery {

// static
bool DnsSdTxtRecord::IsValidTxtValue(const std::string& key,
                                     const std::vector<uint8_t>& value) {
  // The max length of any individual TXT record is 255 bytes.
  if (key.size() + value.size() + 1 /* for equals */ > 255) {
    return false;
  }

  return IsKeyValid(key);
}

// static
bool DnsSdTxtRecord::IsValidTxtValue(const std::string& key, uint8_t value) {
  return IsValidTxtValue(key, std::vector<uint8_t>{value});
}

// static
bool DnsSdTxtRecord::IsValidTxtValue(const std::string& key,
                                     const std::string& value) {
  return IsValidTxtValue(key, std::vector<uint8_t>(value.begin(), value.end()));
}

Error DnsSdTxtRecord::SetValue(const std::string& key,
                               std::vector<uint8_t> value) {
  if (!IsValidTxtValue(key, value)) {
    return Error::Code::kParameterInvalid;
  }

  key_value_txt_[key] = std::move(value);
  ClearFlag(key);
  return Error::None();
}

Error DnsSdTxtRecord::SetValue(const std::string& key,
                               const std::string& value) {
  return SetValue(key, std::vector<uint8_t>(value.begin(), value.end()));
}

Error DnsSdTxtRecord::SetFlag(const std::string& key, bool value) {
  if (!IsKeyValid(key)) {
    return Error::Code::kParameterInvalid;
  }

  if (value) {
    boolean_txt_.insert(key);
  } else {
    ClearFlag(key);
  }
  ClearValue(key);
  return Error::None();
}

ErrorOr<DnsSdTxtRecord::ValueRef> DnsSdTxtRecord::GetValue(
    const std::string& key) const {
  if (!IsKeyValid(key)) {
    return Error::Code::kParameterInvalid;
  }

  auto it = key_value_txt_.find(key);
  if (it != key_value_txt_.end()) {
    return std::cref(it->second);
  }

  return Error::Code::kItemNotFound;
}

ErrorOr<bool> DnsSdTxtRecord::GetFlag(const std::string& key) const {
  if (!IsKeyValid(key)) {
    return Error::Code::kParameterInvalid;
  }

  return boolean_txt_.find(key) != boolean_txt_.end();
}

Error DnsSdTxtRecord::ClearValue(const std::string& key) {
  if (!IsKeyValid(key)) {
    return Error::Code::kParameterInvalid;
  }

  key_value_txt_.erase(key);
  return Error::None();
}

Error DnsSdTxtRecord::ClearFlag(const std::string& key) {
  if (!IsKeyValid(key)) {
    return Error::Code::kParameterInvalid;
  }

  boolean_txt_.erase(key);
  return Error::None();
}

// static
bool DnsSdTxtRecord::IsKeyValid(const std::string& key) {
  // The max length of any individual TXT record is 255 bytes.
  if (key.size() > 255) {
    return false;
  }

  // The length of a key must be at least 1.
  if (key.size() < 1) {
    return false;
  }

  // Keys must contain only valid characters.
  for (char key_char : key) {
    if (key_char < char{0x20} || key_char > char{0x7E} || key_char == '=') {
      return false;
    }
  }

  return true;
}

std::vector<std::vector<uint8_t>> DnsSdTxtRecord::GetData() const {
  std::vector<std::vector<uint8_t>> data;
  for (const auto& pair : key_value_txt_) {
    data.emplace_back();
    std::vector<uint8_t>* new_entry = &data.back();
    new_entry->reserve(pair.first.size() + 1 + pair.second.size());
    new_entry->insert(new_entry->end(), pair.first.begin(), pair.first.end());
    new_entry->push_back('=');
    new_entry->insert(new_entry->end(), pair.second.begin(), pair.second.end());
  }

  for (const auto& flag : boolean_txt_) {
    data.emplace_back();
    std::vector<uint8_t>* new_entry = &data.back();
    new_entry->insert(new_entry->end(), flag.begin(), flag.end());
  }

  return data;
}

bool DnsSdTxtRecord::CaseInsensitiveComparison::operator()(
    const std::string& lhs,
    const std::string& rhs) const {
  if (lhs.size() != rhs.size()) {
    return lhs < rhs;
  }

  for (size_t i = 0; i < lhs.size(); i++) {
    int lhs_char = tolower(lhs[i]);
    int rhs_char = tolower(rhs[i]);

    if (lhs_char != rhs_char) {
      return lhs_char < rhs_char;
    }
  }

  return false;
}

}  // namespace discovery
}  // namespace openscreen
