// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_attribute.h"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "libipp/ipp_package.h"

namespace {

template <typename Type>
bool LoadValue(const std::vector<Type>& data, size_t index, Type* v) {
  if (data.size() > index) {
    *v = data[index];
    return true;
  }
  return false;
}

template <typename Type>
bool SaveValue(std::vector<Type>* data,
               size_t index,
               const Type& v,
               bool is_a_set,
               ipp::AttrState* state) {
  if (data->size() <= index) {
    if (is_a_set)
      data->resize(index + 1);
    else
      return false;
  }
  (*data)[index] = v;
  *state = ipp::AttrState::set;
  return true;
}

ipp::InternalType CppTypeForUnknownAttribute(ipp::AttrType type) {
  switch (type) {
    case ipp::AttrType::boolean:
    case ipp::AttrType::integer:
    case ipp::AttrType::enum_:
      return ipp::InternalType::kInteger;
    case ipp::AttrType::dateTime:
      return ipp::InternalType::kDateTime;
    case ipp::AttrType::resolution:
      return ipp::InternalType::kResolution;
    case ipp::AttrType::rangeOfInteger:
      return ipp::InternalType::kRange;
    case ipp::AttrType::name:
    case ipp::AttrType::text:
      return ipp::InternalType::kStringWithLanguage;
    default:
      return ipp::InternalType::kString;
  }
}

ipp::AttrName ToAttrName(const std::string& s) {
  ipp::AttrName r = ipp::AttrName::_unknown;
  FromString(s, &r);
  return r;
}

std::string UnsignedToString(size_t x) {
  std::string s;
  do {
    s.push_back('0' + (x % 10));
    x /= 10;
  } while (x > 0);
  std::reverse(s.begin(), s.end());
  return s;
}
}  // namespace

namespace ipp {

std::string ToString(AttrState s) {
  switch (s) {
    case AttrState::unset:
      return "unset";
    case AttrState::set:
      return "set";
    case AttrState::unsupported:
      return "unsupported";
    case AttrState::unknown:
      return "unknown";
    case AttrState::novalue_:
      return "novalue";
    case AttrState::not_settable:
      return "not-settable";
    case AttrState::delete_attribute:
      return "delete-attribute";
    case AttrState::admin_define:
      return "admin-define";
  }
  return "";
}

std::string ToString(AttrType at) {
  switch (at) {
    case AttrType::integer:
      return "integer";
    case AttrType::boolean:
      return "boolean";
    case AttrType::enum_:
      return "enum";
    case AttrType::octetString:
      return "octetString";
    case AttrType::dateTime:
      return "dateTime";
    case AttrType::resolution:
      return "resolution";
    case AttrType::rangeOfInteger:
      return "rangeOfInteger";
    case AttrType::collection:
      return "collection";
    case AttrType::text:
      return "text";
    case AttrType::name:
      return "name";
    case AttrType::keyword:
      return "keyword";
    case AttrType::uri:
      return "uri";
    case AttrType::uriScheme:
      return "uriScheme";
    case AttrType::charset:
      return "charset";
    case AttrType::naturalLanguage:
      return "naturalLanguage";
    case AttrType::mimeMediaType:
      return "mimeMediaType";
  }
  return "";
}

std::string ToString(bool v) {
  return (v ? "true" : "false");
}

std::string ToString(int v) {
  if (v < 0) {
    // 2 x incrementation in case of (v == numeric_limit<int>::min())
    const std::string s = UnsignedToString(static_cast<size_t>(-(++v)) + 1);
    return "-" + s;
  }
  return UnsignedToString(v);
}

std::string ToString(const Resolution& v) {
  std::string s = ToString(v.size1) + "x" + ToString(v.size2);
  if (v.units == Resolution::kDotsPerInch)
    s += "dpi";
  else
    s += "dpc";
  return s;
}

std::string ToString(const RangeOfInteger& v) {
  return ("(" + ToString(v.min_value) + ":" + ToString(v.max_value) + ")");
}

std::string ToString(const DateTime& v) {
  return (ToString(v.year) + "-" + ToString(v.month) + "-" + ToString(v.day) +
          "," + ToString(v.hour) + ":" + ToString(v.minutes) + ":" +
          ToString(v.seconds) + "." + ToString(v.deci_seconds) + "," +
          std::string(1, v.UTC_direction) + ToString(v.UTC_hours) + ":" +
          ToString(v.UTC_minutes));
}

bool FromString(const std::string& s, bool* v) {
  if (v == nullptr)
    return false;
  if (s == "false") {
    *v = false;
    return true;
  }
  if (s == "true") {
    *v = true;
    return true;
  }
  return false;
}

// JSON-like integer format: first character may be '-', the rest must be
// digits. Leading zeroes allowed.
bool FromString(const std::string& s, int* out) {
  if (out == nullptr)
    return false;
  if (s.empty())
    return false;
  auto it = s.begin();
  int vv = 0;
  if (*it == '-') {
    ++it;
    if (it == s.end())
      return false;
    // negative number
    for (; it != s.end(); ++it) {
      if (std::numeric_limits<int>::min() / 10 > vv)
        return false;
      vv *= 10;
      if (*it < '0' || *it > '9')
        return false;
      const int d = (*it - '0');
      if (std::numeric_limits<int>::min() + d > vv)
        return false;
      vv -= d;
    }
  } else {
    // positive number
    for (; it != s.end(); ++it) {
      if (std::numeric_limits<int>::max() / 10 < vv)
        return false;
      vv *= 10;
      if (*it < '0' || *it > '9')
        return false;
      const int d = (*it - '0');
      if (std::numeric_limits<int>::max() - d < vv)
        return false;
      vv += d;
    }
  }
  *out = vv;
  return true;
}

AttrState Attribute::GetState() {
  if (this->state_ != AttrState::unset)
    return this->state_;
  for (size_t i = 0; true; ++i) {
    Collection* c = this->GetCollection(i);
    if (c == nullptr)
      break;
    std::vector<Attribute*> attrs = c->GetAllAttributes();
    for (auto a : attrs)
      if (a->GetState() != AttrState::unset)
        return AttrState::set;
  }
  return AttrState::unset;
}

void Attribute::SetState(AttrState status) {
  if (type_ != AttrType::collection) {
    state_ = status;
    return;
  }
  // special algorithm for collection
  if (status == AttrState::unset) {
    state_ = AttrState::unset;
    for (size_t i = 0; true; ++i) {
      Collection* coll = GetCollection(i);
      if (coll == nullptr)
        break;
      auto attrs = coll->GetAllAttributes();
      for (Attribute* a : attrs)
        a->SetState(AttrState::unset);
    }
  } else if (status == AttrState::set) {
    // for collections, the field state_ never equals AttrState::set
    state_ = AttrState::unset;
  } else {
    state_ = status;
  }
}

ValueAttribute::ValueAttribute(AttrName name,
                               bool is_a_set,
                               AttrType type,
                               InternalType cpp_type)
    : Attribute(name, is_a_set, type), cpp_type_(cpp_type) {
  const size_t n = (is_a_set_ ? 0 : 1);
  switch (cpp_type_) {
    case InternalType::kInteger:
      new (&(data_.as_int)) std::vector<int>(n);
      break;
    case InternalType::kString:
      new (&(data_.as_string)) std::vector<std::string>(n);
      break;
    case InternalType::kResolution:
      new (&(data_.as_resolution)) std::vector<Resolution>(n);
      break;
    case InternalType::kRange:
      new (&(data_.as_ranges)) std::vector<RangeOfInteger>(n);
      break;
    case InternalType::kDateTime:
      new (&(data_.as_datetime)) std::vector<DateTime>(n);
      break;
    case InternalType::kStringWithLanguage:
      new (&(data_.as_string_with_language)) std::vector<StringWithLanguage>(n);
      break;
  }
}

ValueAttribute::~ValueAttribute() {
  switch (cpp_type_) {
    case InternalType::kInteger:
      data_.as_int.~vector();
      break;
    case InternalType::kString:
      data_.as_string.~vector();
      break;
    case InternalType::kResolution:
      data_.as_resolution.~vector();
      break;
    case InternalType::kRange:
      data_.as_ranges.~vector();
      break;
    case InternalType::kDateTime:
      data_.as_datetime.~vector();
      break;
    case InternalType::kStringWithLanguage:
      data_.as_string_with_language.~vector();
      break;
  }
}

size_t ValueAttribute::GetSize() const {
  switch (cpp_type_) {
    case InternalType::kInteger:
      return data_.as_int.size();
    case InternalType::kString:
      return data_.as_string.size();
    case InternalType::kResolution:
      return data_.as_resolution.size();
    case InternalType::kRange:
      return data_.as_ranges.size();
    case InternalType::kDateTime:
      return data_.as_datetime.size();
    case InternalType::kStringWithLanguage:
      return data_.as_string_with_language.size();
  }
  return 0;
}

void ValueAttribute::Resize(size_t new_size) {
  if (!is_a_set_)
    return;
  switch (cpp_type_) {
    case InternalType::kInteger:
      data_.as_int.resize(new_size);
      break;
    case InternalType::kString:
      data_.as_string.resize(new_size);
      break;
    case InternalType::kResolution:
      data_.as_resolution.resize(new_size);
      break;
    case InternalType::kRange:
      data_.as_ranges.resize(new_size);
      break;
    case InternalType::kDateTime:
      data_.as_datetime.resize(new_size);
      break;
    case InternalType::kStringWithLanguage:
      data_.as_string_with_language.resize(new_size);
      break;
  }
}

bool ValueAttribute::GetValue(std::string* v, size_t index) const {
  if (v == nullptr)
    return false;
  if (cpp_type_ == InternalType::kString) {
    return LoadValue(data_.as_string, index, v);
  }
  if (cpp_type_ == InternalType::kStringWithLanguage) {
    StringWithLanguage s;
    if (!LoadValue(data_.as_string_with_language, index, &s))
      return false;
    *v = s;
    return true;
  }
  if (cpp_type_ == InternalType::kInteger) {
    int x;
    if (!LoadValue(data_.as_int, index, &x))
      return false;
    if (type_ == AttrType::enum_ || type_ == AttrType::keyword) {
      const std::string x2 = ToString(name_, x);
      if (x2.empty())
        return false;
      *v = x2;
    } else if (type_ == AttrType::boolean) {
      *v = ToString(static_cast<bool>(x));
    } else {
      *v = ToString(x);
    }
    return true;
  }
  if (cpp_type_ == InternalType::kDateTime) {
    DateTime x;
    if (!LoadValue(data_.as_datetime, index, &x))
      return false;
    *v = ToString(x);
    return true;
  }
  if (cpp_type_ == InternalType::kRange) {
    RangeOfInteger x;
    if (!LoadValue(data_.as_ranges, index, &x))
      return false;
    *v = ToString(x);
    return true;
  }
  if (cpp_type_ == InternalType::kResolution) {
    Resolution x;
    if (!LoadValue(data_.as_resolution, index, &x))
      return false;
    *v = ToString(x);
    return true;
  }
  return false;
}

bool ValueAttribute::GetValue(StringWithLanguage* v, size_t index) const {
  if (v == nullptr || cpp_type_ != InternalType::kStringWithLanguage)
    return false;
  return LoadValue(data_.as_string_with_language, index, v);
}

bool ValueAttribute::GetValue(int* v, size_t index) const {
  if (v == nullptr || cpp_type_ != InternalType::kInteger)
    return false;
  return LoadValue(data_.as_int, index, v);
}

bool ValueAttribute::GetValue(Resolution* v, size_t index) const {
  if (v == nullptr || cpp_type_ != InternalType::kResolution)
    return false;
  return LoadValue(data_.as_resolution, index, v);
}

bool ValueAttribute::GetValue(RangeOfInteger* v, size_t index) const {
  if (v == nullptr || cpp_type_ != InternalType::kRange)
    return false;
  return LoadValue(data_.as_ranges, index, v);
}

bool ValueAttribute::GetValue(DateTime* v, size_t index) const {
  if (v == nullptr || cpp_type_ != InternalType::kDateTime)
    return false;
  return LoadValue(data_.as_datetime, index, v);
}

bool ValueAttribute::SetValue(const std::string& v, size_t index) {
  if (cpp_type_ == InternalType::kString) {
    return SaveValue(&data_.as_string, index, v, is_a_set_, &state_);
  }
  if (cpp_type_ == InternalType::kStringWithLanguage) {
    StringWithLanguage s(v);
    return SaveValue(&data_.as_string_with_language, index, s, is_a_set_,
                     &state_);
  }
  if (cpp_type_ == InternalType::kInteger) {
    bool result = false;
    int x;
    if (type_ == AttrType::enum_ || type_ == AttrType::keyword) {
      result = FromString(v, name_, &x);
    } else if (type_ == AttrType::boolean) {
      bool b = false;
      result = FromString(v, &b);
      x = static_cast<int>(b);
    } else {
      result = FromString(v, &x);
    }
    if (result)
      result = SaveValue(&data_.as_int, index, x, is_a_set_, &state_);
    return result;
  }
  return false;
}

bool ValueAttribute::SetValue(const StringWithLanguage& v, size_t index) {
  if (cpp_type_ == InternalType::kStringWithLanguage)
    return SaveValue(&data_.as_string_with_language, index, v, is_a_set_,
                     &state_);
  return false;
}

bool ValueAttribute::SetValue(const int& v, size_t index) {
  if (cpp_type_ == InternalType::kInteger)
    return SaveValue(&data_.as_int, index, v, is_a_set_, &state_);
  return false;
}

bool ValueAttribute::SetValue(const Resolution& v, size_t index) {
  if (cpp_type_ == InternalType::kResolution)
    return SaveValue(&data_.as_resolution, index, v, is_a_set_, &state_);
  return false;
}

bool ValueAttribute::SetValue(const RangeOfInteger& v, size_t index) {
  if (cpp_type_ == InternalType::kRange)
    return SaveValue(&data_.as_ranges, index, v, is_a_set_, &state_);
  return false;
}

bool ValueAttribute::SetValue(const DateTime& v, size_t index) {
  if (cpp_type_ == InternalType::kDateTime)
    return SaveValue(&data_.as_datetime, index, v, is_a_set_, &state_);
  return false;
}

UnknownValueAttribute::UnknownValueAttribute(const std::string& name,
                                             bool is_a_set,
                                             AttrType type)
    : ValueAttribute(
          ToAttrName(name), is_a_set, type, CppTypeForUnknownAttribute(type)),
      str_name_(name) {}

UnknownCollectionAttribute::UnknownCollectionAttribute(const std::string& name,
                                                       bool is_a_set)
    : Attribute(ToAttrName(name), is_a_set, AttrType::collection),
      str_name_(name) {}

void UnknownCollectionAttribute::Resize(size_t new_size) {
  if (!IsASet())
    return;
  while (collections_.size() < new_size)
    collections_.push_back(std::make_unique<Collection>());
  collections_.resize(new_size);
}
}  // namespace ipp
