// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_parser.h"
#include <set>

#include "libipp/ipp_encoding.h"

namespace ipp {

namespace {

// Converts the least significant 4 bits to hexadecimal digit (ASCII char).
char ToHexDigit(uint8_t v) {
  v &= 0x0f;
  if (v < 10)
    return ('0' + v);
  return ('a' + (v - 10));
}

// Converts byte to 2-digit hexadecimal representation.
std::string ToHexByte(uint8_t v) {
  std::string s(2, '0');
  s[0] = ToHexDigit(v >> 4);
  s[1] = ToHexDigit(v);
  return s;
}

// Converts sequence of bytes to a sequence of 2-digits hexadecimals
// separated by single space.
std::string ToHexSeq(const uint8_t* begin, const uint8_t* end) {
  std::string s;
  if (begin >= end)
    return s;
  s = ToHexByte(*begin);
  for (++begin; begin < end; ++begin) {
    s += " " + ToHexByte(*begin);
  }
  return s;
}

// Decodes 1-, 2- or 4-bytes integers (two's-complement binary encoding).
// Returns false if (data.size() != BytesCount) or (out == nullptr).
template <size_t BytesCount>
bool LoadInteger(const std::vector<uint8_t>& data, int* out) {
  if ((data.size() != BytesCount) || (out == nullptr))
    return false;
  const uint8_t* ptr = data.data();
  ParseSignedInteger<BytesCount>(&ptr, out);
  return true;
}

// Reads simple string from buf.
std::string LoadOctetString(const std::vector<uint8_t>& buf) {
  return std::string(buf.data(), buf.data() + buf.size());
}

// Reads textWithLanguage/nameWithLanguage (see [rfc8010], section 3.9) from
// buf. Returns false if given content is incorrect or (out == nullptr).
bool LoadStringWithLanguage(const std::vector<uint8_t>& buf,
                            ipp::StringWithLanguage* out) {
  // The shortest possible value has 4 bytes: 2 times 2-bytes zero.
  if ((buf.size() < 4) || (out == nullptr))
    return false;
  const uint8_t* ptr = buf.data();
  size_t length;
  if (!ParseUnsignedInteger<2>(&ptr, &length))
    return false;
  if (buf.size() < 4 + length)
    return false;
  out->language.assign(ptr, ptr + length);
  ptr += length;
  if (!ParseUnsignedInteger<2>(&ptr, &length))
    return false;
  if (buf.size() != 4 + out->language.size() + length)
    return false;
  out->value.assign(ptr, ptr + length);
  return true;
}

// Reads dateTime (see [rfc8010]) from buf.
// Fails when binary representation is incorrect or (out == nullptr).
bool LoadDateTime(const std::vector<uint8_t>& buf, ipp::DateTime* out) {
  if ((buf.size() != 11) || (out == nullptr))
    return false;
  const uint8_t* ptr = buf.data();
  return (ParseUnsignedInteger<2>(&ptr, &out->year) &&
          ParseUnsignedInteger<1>(&ptr, &out->month) &&
          ParseUnsignedInteger<1>(&ptr, &out->day) &&
          ParseUnsignedInteger<1>(&ptr, &out->hour) &&
          ParseUnsignedInteger<1>(&ptr, &out->minutes) &&
          ParseUnsignedInteger<1>(&ptr, &out->seconds) &&
          ParseUnsignedInteger<1>(&ptr, &out->deci_seconds) &&
          ParseUnsignedInteger<1>(&ptr, &out->UTC_direction) &&
          ParseUnsignedInteger<1>(&ptr, &out->UTC_hours) &&
          ParseUnsignedInteger<1>(&ptr, &out->UTC_minutes));
}

// Reads resolution (see [rfc8010]) from buf.
// Fails when binary representation is incorrect or (out == nullptr).
bool LoadResolution(const std::vector<uint8_t>& buf, ipp::Resolution* out) {
  if ((buf.size() != 9) || (out == nullptr))
    return false;
  const uint8_t* ptr = buf.data();
  ParseSignedInteger<4>(&ptr, &out->size1);
  ParseSignedInteger<4>(&ptr, &out->size2);
  switch (*ptr) {
    case static_cast<uint8_t>(ipp::Resolution::kDotsPerCentimeter):
      out->units = ipp::Resolution::kDotsPerCentimeter;
      break;
    case static_cast<uint8_t>(ipp::Resolution::kDotsPerInch):
      out->units = ipp::Resolution::kDotsPerInch;
      break;
    default:
      return false;
  }
  return true;
}

// Reads rangeOfInteger (see [rfc8010]) from buf.
// Fails when binary representation is incorrect or (out == nullptr).
bool LoadRangeOfInteger(const std::vector<uint8_t>& buf,
                        ipp::RangeOfInteger* out) {
  if ((buf.size() != 8) || (out == nullptr))
    return false;
  const uint8_t* ptr = buf.data();
  ParseSignedInteger<4>(&ptr, &out->min_value);
  ParseSignedInteger<4>(&ptr, &out->max_value);
  return true;
}

// Reads name as specified in 3.2 section of rfc8010 and stores it in |out|.
// Returns false <=> the name is incorrect. In this case |out| is set anyway,
// but incorrect characters are replaced by '_' (underscore). For empty |buf|,
// an empty string is set in |out| and false is returned. |out| must be not
// nullptr.
bool LoadName(const std::vector<uint8_t>& buf, std::string* out) {
  bool result = !buf.empty() && buf.front() >= 0x61 && buf.front() <= 0x7a;
  out->clear();
  out->reserve(buf.size());
  for (uint8_t c : buf) {
    if ((c < 0x30 || c > 0x39) && (c < 0x61 || c > 0x7a) && (c != '-') &&
        (c != '_') && (c != '.')) {
      c = '_';
      result = false;
    }
    out->push_back(c);
  }
  return result;
}

// Scope guard to control the context path. Constructor adds new a element to
// the path while destructor removes it from the path.
class ContextPathGuard {
 public:
  ContextPathGuard(std::vector<std::pair<std::string, bool>>* path,
                   const std::string& name,
                   bool is_known = true)
      : path_(path) {
    if (!path_->empty() && !path_->back().second)
      is_known = false;
    path_->push_back(std::make_pair(name, is_known));
  }
  ~ContextPathGuard() { path_->pop_back(); }

 private:
  ContextPathGuard(const ContextPathGuard&) = delete;
  ContextPathGuard(ContextPathGuard&&) = delete;
  ContextPathGuard& operator=(const ContextPathGuard&) = delete;
  ContextPathGuard& operator=(ContextPathGuard&&) = delete;
  std::vector<std::pair<std::string, bool>>* path_;
};

// Builds nice string with context path.
std::string PathAsString(
    const std::vector<std::pair<std::string, bool>>& path) {
  std::string s;
  for (auto& e : path) {
    if (!s.empty())
      s += "->";
    s += e.first;
  }
  return s;
}

// Return true if source type can be used in attribute of target type.
bool IsConvertibleTo(const ipp::AttrType source, const ipp::AttrType target) {
  if (source == target)
    return true;
  if (source == ipp::AttrType::integer &&
      target == ipp::AttrType::rangeOfInteger)
    return true;
  if (source == ipp::AttrType::integer && target == ipp::AttrType::enum_)
    return true;
  return false;
}

}  //  namespace

void Parser::LogScannerError(const std::string& message, const uint8_t* ptr) {
  Log l;
  l.message = "Scanner error: " + message + ".";
  // Let's try to save to frame_context the closest neighborhood of ptr.
  if (ptr != nullptr && ptr >= buffer_begin_ && ptr <= buffer_end_) {
    // Current position in the buffer.
    l.buf_offset = ptr - buffer_begin_;
    // Calculates the size in bytes of left neighborhood.
    int left_margin = 13;
    if (buffer_begin_ + left_margin > ptr)
      left_margin = ptr - buffer_begin_;
    // Calculates the size in bytes of right neighborhood.
    int right_margin = 14;
    if (ptr + right_margin > buffer_end_)
      right_margin = buffer_end_ - ptr;
    // Prints the content of the closest neighborhood to frame_context.
    l.frame_context = ToHexSeq(ptr - left_margin, ptr) + "|" +
                      ToHexSeq(ptr, ptr + right_margin);
  }
  errors_->push_back(l);
}

void Parser::LogParserError(const std::string& message, std::string action) {
  Log l;
  l.message = "Parser error: " + message + ". " + action + ".";
  l.parser_context = PathAsString(parser_context_);
  errors_->push_back(l);
}

void Parser::LogParserWarning(const std::string& message) {
  Log l;
  l.message = "Parser warning: " + message + ".";
  l.parser_context = PathAsString(parser_context_);
  errors_->push_back(l);
}

void Parser::LogParserNewElement() {
  const size_t path_size = parser_context_.size();
  if (path_size < 2 || parser_context_[path_size - 2].second) {
    Log l;
    l.message = "Parser notice: this element is not known (outside the schema)";
    l.parser_context = PathAsString(parser_context_);
    errors_->push_back(l);
  }
}

void Parser::LoadAttrValue(Attribute* attr,
                           size_t index,
                           const std::vector<uint8_t>& buf,
                           uint8_t tag) {
  // these two values are not in AttrType
  if (tag == nameWithoutLanguage_value_tag ||
      tag == textWithoutLanguage_value_tag) {
    attr->SetValue(LoadOctetString(buf), index);
    return;
  }
  // build the first part of error message
  const AttrType tag_type = static_cast<AttrType>(tag);
  const std::string msg_prefix = "Incorrect " + ToString(tag_type) + " value";
  // load value from the buffer
  switch (tag_type) {
    case AttrType::boolean: {
      int v = 0;
      if (!LoadInteger<1>(buf, &v) || v < 0 || v > 1)
        LogParserError(msg_prefix + ": 0x" + ToHexByte(v),
                       "The value set to false");
      attr->SetValue(v, index);
      break;
    }
    case AttrType::integer:
    case AttrType::enum_: {
      int v = 0;
      if (!LoadInteger<4>(buf, &v))
        LogParserError(msg_prefix, "The value set to 0");
      attr->SetValue(v, index);
      break;
    }
    case AttrType::dateTime: {
      DateTime v;
      if (!LoadDateTime(buf, &v))
        LogParserError(msg_prefix, "The value not set");
      attr->SetValue(v, index);
      break;
    }
    case AttrType::resolution: {
      Resolution v;
      if (!LoadResolution(buf, &v))
        LogParserError(msg_prefix, "The value not set");
      attr->SetValue(v, index);
      break;
    }
    case AttrType::rangeOfInteger: {
      RangeOfInteger v;
      if (!LoadRangeOfInteger(buf, &v))
        LogParserError(msg_prefix, "The value not set");
      attr->SetValue(v, index);
      break;
    }
    case AttrType::text:
    case AttrType::name: {
      StringWithLanguage v;
      if (!LoadStringWithLanguage(buf, &v))
        LogParserError(msg_prefix, "The value not set");
      attr->SetValue(v, index);
      break;
    }
    case AttrType::octetString:
    case AttrType::keyword:
    case AttrType::uri:
    case AttrType::uriScheme:
    case AttrType::charset:
    case AttrType::naturalLanguage:
    case AttrType::mimeMediaType:
      attr->SetValue(LoadOctetString(buf), index);
      break;
    default:
      LogParserError("Internal parser error: cannot recognize value type",
                     "The value was not set");
      break;
  }
}

// Temporary representation of an attribute's value parsed from TNVs.
struct RawValue {
  // Out-Of-Bond value or "set" when contains standard value or collection
  AttrState state;
  // corresponding attribute's type - verified
  AttrType type;
  // original tag - verified
  uint8_t tag;
  // original data, empty when (type == collection) - not verified
  std::vector<uint8_t> data;
  // (not nullptr) <=> (type == collection)
  std::unique_ptr<RawCollection> collection;
  // default constructor
  RawValue()
      : state(AttrState::set),
        type(AttrType::integer),
        tag(static_cast<uint8_t>(AttrType::integer)) {}
  // create as Out-Of-Band value
  explicit RawValue(uint8_t tag)
      : state(static_cast<AttrState>(tag)), type(AttrType::integer), tag(tag) {}
  // create as standard value
  RawValue(AttrType type, uint8_t tag, const std::vector<uint8_t>& data)
      : state(AttrState::set), type(type), tag(tag), data(data) {}
  // create as collection
  explicit RawValue(RawCollection* coll)
      : state(AttrState::set),
        type(AttrType::collection),
        tag(static_cast<uint8_t>(AttrType::collection)),
        collection(coll) {}
};

struct RawCollection;

// Temporary representation of an attribute parsed from TNVs.
struct RawAttribute {
  // verified (non-empty, correct syntax)
  std::string name;
  // parsed values (see RawValue)
  std::vector<RawValue> values;
  explicit RawAttribute(const std::string& name) : name(name) {}
};

// Temporary representation of a collection parsed from TNVs.
struct RawCollection {
  // parsed attributes (may have duplicate names)
  std::vector<RawAttribute> attributes;
};

bool Parser::SaveFrameToPackage(bool log_unknown_values, Package* package) {
  std::set<GroupTag> processed_single_groups;
  for (size_t i = 0; i < frame_->groups_tags_.size(); ++i) {
    GroupTag gn = static_cast<GroupTag>(frame_->groups_tags_[i]);
    bool report_unknowns =
        log_unknown_values && (gn != GroupTag::unsupported_attributes);
    std::string grp_name = ToString(gn);
    if (grp_name.empty())
      grp_name = "(" + ToHexByte(frame_->groups_tags_[i]) + ")";
    Group* grp = package->GetGroup(gn);
    ContextPathGuard path_update(&parser_context_, grp_name,
                                 report_unknowns && (grp != nullptr));
    if (grp == nullptr) {
      grp = package->AddUnknownGroup(gn, true);
      if (report_unknowns)
        LogParserNewElement();
    }
    Collection* coll = nullptr;
    if (grp->IsASet()) {
      const size_t index = grp->GetSize();
      grp->Resize(index + 1);
      coll = grp->GetCollection(index);
    } else {
      // single group - save it <=> it is the first occurrence
      if (processed_single_groups.insert(gn).second) {
        coll = grp->GetCollection();
      } else {
        LogParserError("Duplicated group " + grp_name + " was found",
                       "The group was omitted");
        continue;
      }
    }
    RawCollection raw_coll;
    if (!ParseRawGroup(&(frame_->groups_content_[i]), &raw_coll))
      return false;
    if (!DecodeCollection(&raw_coll, coll))
      return false;
  }
  package->Data() = frame_->data_;
  return true;
}

bool Parser::ReadFrameFromBuffer(const uint8_t* ptr,
                                 const uint8_t* const buf_end) {
  buffer_begin_ = ptr;
  buffer_end_ = buf_end;
  bool error_in_header = true;
  if (buf_end - ptr < 9) {
    LogScannerError("Frame is too short to be correct (less than 9 bytes)",
                    ptr);
  } else if (!ParseUnsignedInteger<1>(&ptr, &frame_->major_version_number_)) {
    LogScannerError("major-version-number is out of range", ptr);
  } else if (!ParseUnsignedInteger<1>(&ptr, &frame_->minor_version_number_)) {
    LogScannerError("minor-version-number is out of range", ptr);
  } else if (!ParseUnsignedInteger<2>(&ptr,
                                      &frame_->operation_id_or_status_code_)) {
    LogScannerError("operation-id or status-code is out of range", ptr);
  } else if (!ParseUnsignedInteger<4>(&ptr, &frame_->request_id_)) {
    LogScannerError("request-id is out of range", ptr);
  } else if (*ptr > max_begin_attribute_group_tag) {
    LogScannerError("begin-attribute-group-tag was expected", ptr);
  } else {
    error_in_header = false;
  }
  if (error_in_header)
    return false;
  while (*ptr != end_of_attributes_tag) {
    frame_->groups_tags_.push_back(*ptr);
    frame_->groups_content_.resize(frame_->groups_tags_.size());
    ++ptr;
    if (!ReadTNVsFromBuffer(&ptr, buf_end, &(frame_->groups_content_.back())))
      return false;
    if (ptr >= buf_end) {
      LogScannerError(
          "Unexpected end of frame, begin-attribute-group-tag was expected",
          ptr);
      return false;
    }
  }
  ++ptr;
  frame_->data_.assign(ptr, buf_end);
  ptr = buf_end;
  return true;
}

// Parses TNVs from given buffer until the end of the buffer is reached or next
// begin-attribute-group-tag is spotted. The pointer ptr is shifted accordingly.
// returns true <=> (ptr == buf_end) or (*ptr is begin-attribute-group-tag)
// returns false <=> parsing error occurs, ptr points to incorrect field
// Parsed TNVs are added to the end of |tnvs|. If |tnvs| is nullptr then no
// output is saved but parsing occurs as usual.
bool Parser::ReadTNVsFromBuffer(const uint8_t** ptr2,
                                const uint8_t* const buf_end,
                                std::list<TagNameValue>* tnvs) {
  const uint8_t*& ptr = *ptr2;
  while ((ptr < buf_end) && (*ptr > max_begin_attribute_group_tag)) {
    TagNameValue tnv;

    if (buf_end - ptr < 5) {
      LogScannerError(
          "Unexpected end of frame when reading tag-name-value (expected at "
          "least 1-byte tag, 2-bytes name-length and 2-bytes value-length)",
          ptr);
      return false;
    }
    if (!ParseUnsignedInteger<1>(&ptr, &tnv.tag)) {
      LogScannerError("value-tag is out of range", ptr);
      return false;
    }
    int length = 0;
    if (!ParseUnsignedInteger<2>(&ptr, &length)) {
      LogScannerError("name-length is out of range", ptr);
      return false;
    }
    if (buf_end - ptr < length + 2) {
      LogScannerError(
          "Unexpected end of frame when reading name (expected at least " +
              std::to_string(length) + "-bytes name and 2-bytes value-length)",
          ptr);
      return false;
    }
    tnv.name.assign(ptr, ptr + length);
    ptr += length;
    if (!ParseUnsignedInteger<2>(&ptr, &length)) {
      LogScannerError("value-length is out of range", ptr);
      return false;
    }
    if (buf_end - ptr < length) {
      LogScannerError("Unexpected end of frame when reading value (expected " +
                          std::to_string(length) + "-bytes value)",
                      ptr);
      return false;
    }
    tnv.value.assign(ptr, ptr + length);
    ptr += length;
    if (tnvs != nullptr)
      tnvs->push_back(std::move(tnv));
  }
  return true;
}

void Parser::ResetContent() {
  buffer_begin_ = nullptr;
  buffer_end_ = nullptr;
  parser_context_.clear();
}

// Parses single attribute's value and add it to |attr|. |tnv| is the first TNV
// with the value, |tnvs| contains all following TNVs. Both |tnvs| and |attr|
// cannot be nullptr. Returns false <=> critical parsing error was spotted.
// See section 3.5.2 from rfc8010 for details.
bool Parser::ParseRawValue(const TagNameValue& tnv,
                           std::list<TagNameValue>* tnvs,
                           RawAttribute* attr) {
  // Is it Ouf-Of-Band value ?
  if (tnv.tag >= min_out_of_band_value_tag &&
      tnv.tag <= max_out_of_band_value_tag) {
    if (!tnv.value.empty())
      LogParserError(
          "Tag-name-value with an out-of-band tag has a non-empty value",
          "The field is ignored");
    attr->values.emplace_back(tnv.tag);
    return true;
  }
  // Is it correct attribute's syntax tag? If not then fail.
  if (tnv.tag < min_attribute_syntax_tag ||
      tnv.tag > max_attribute_syntax_tag ||
      tnv.tag == endCollection_value_tag ||
      tnv.tag == memberAttrName_value_tag) {
    LogParserError(
        "Incorrect tag when parsing Tag-name-value with a value: 0x" +
        ToHexByte(tnv.tag));
    return false;
  }
  // Is it a collection ?
  if (tnv.tag == begCollection_value_tag) {
    if (!tnv.value.empty())
      LogParserError("Tag-name-value opening a collection has non-empty value",
                     "The field is ignored");
    std::unique_ptr<RawCollection> coll(new RawCollection);
    if (!ParseRawCollection(tnvs, coll.get()))
      return false;
    attr->values.emplace_back(coll.release());
    return true;
  }
  // Is is a standard attribute type ?
  if (tnv.tag == nameWithoutLanguage_value_tag) {
    attr->values.emplace_back(AttrType::name, tnv.tag, tnv.value);
    return true;
  }
  if (tnv.tag == textWithoutLanguage_value_tag) {
    attr->values.emplace_back(AttrType::text, tnv.tag, tnv.value);
    return true;
  }
  AttrType type = static_cast<AttrType>(tnv.tag);
  if (ToString(type).empty()) {
    // unknown attribute's syntax
    LogParserWarning(
        "Tag representing unknown attribute syntax was spotted: 0x" +
        ToHexByte(tnv.tag) + ". The attribute's value was omitted");
    return true;
  }
  attr->values.emplace_back(type, tnv.tag, tnv.value);
  return true;
}

// Parses single collections from given TNVs. Both |tnvs| and |coll| cannot be
// nullptr. Returns false <=> critical parsing error was spotted.
bool Parser::ParseRawCollection(std::list<TagNameValue>* tnvs,
                                RawCollection* coll) {
  while (true) {
    if (tnvs->empty()) {
      LogParserError(
          "The end of Group was reached when memberAttrName tag (0x4a) or "
          "endCollection tag (0x37) was expected");
      return false;
    }
    TagNameValue tnv = tnvs->front();
    tnvs->pop_front();
    // exit if the end of the collection was reached
    if (tnv.tag == endCollection_value_tag) {
      if (!tnv.name.empty())
        LogParserError("Tag-name-value closing a collection has non-empty name",
                       "The field is ignored");
      if (!tnv.value.empty())
        LogParserError(
            "Tag-name-value closing a collection has non-empty value",
            "The field is ignored");
      return true;
    }
    // still here, so we parse an attribute (collection's member)
    if (tnv.tag != memberAttrName_value_tag) {
      LogParserError("Expected tag memberAttrName (0x4a), found: 0x" +
                     ToHexByte(tnv.tag));
      return false;
    }
    // parse name & create attribute
    if (!tnv.name.empty())
      LogParserError(
          "Tag-name-value opening member attribute has non-empty name",
          "The field is ignored");
    std::string name;
    if (!LoadName(tnv.value, &name)) {
      if (name.empty()) {
        LogParserError("Attribute with an empty name was spotted");
        return false;
      } else {
        LogParserError("Attribute's name has incorrect format",
                       "Incorrect characters were replaced by '_'");
      }
    }
    coll->attributes.emplace_back(name);
    RawAttribute* attr = &coll->attributes.back();
    ContextPathGuard path_update(&parser_context_, name);
    // parse tag
    if (tnvs->empty()) {
      LogParserError(
          "The end of Group was reached when value-tag for collection's member "
          "was expected");
      return false;
    }
    // parse all values
    while (!tnvs->empty() && tnvs->front().tag != endCollection_value_tag &&
           tnvs->front().tag != memberAttrName_value_tag) {
      tnv = tnvs->front();
      tnvs->pop_front();
      if (!tnv.name.empty())
        LogParserError(
            "Tag-name-value opening member attribute has non-empty name",
            "The field is ignored");
      if (!ParseRawValue(tnv, tnvs, attr))
        return false;
    }
  }
}

// Parses attributes group from given TNVs and saves it to |coll|. Both |tnvs|
// and |coll| cannot be nullptr. Returns false <=> critical parsing error was
// spotted.
bool Parser::ParseRawGroup(std::list<TagNameValue>* tnvs, RawCollection* coll) {
  while (!tnvs->empty()) {
    TagNameValue tnv = tnvs->front();
    tnvs->pop_front();
    // parse name & create attribute
    std::string name;
    if (!LoadName(tnv.name, &name)) {
      if (name.empty()) {
        LogParserError("Attribute with an empty name was spotted");
        return false;
      } else {
        LogParserError("Attribute's name has incorrect format",
                       "Incorrect characters were replaced by '_'");
      }
    }
    coll->attributes.emplace_back(name);
    RawAttribute* attr = &coll->attributes.back();
    ContextPathGuard path_update(&parser_context_, name);
    // parse all values
    while (true) {
      // parse value
      if (!ParseRawValue(tnv, tnvs, attr))
        return false;
      // go to the next value or attribute
      if (tnvs->empty() || !tnvs->front().name.empty())
        break;  // end of the attribute
      // next value
      tnv = tnvs->front();
      tnvs->pop_front();
    }
  }
  return true;
}

// Removes from |attr| values/collections not matching given attribute's |type|.
// Returns number of element removed from |attr|. |attr| cannot be nullptr.
size_t RemoveIncompatibleValues(const ipp::AttrType type, RawAttribute* attr) {
  size_t incorrect_values = 0;
  // Is the attribute a Out-Of-Band value? In this case it must have single
  // value only.
  if (!attr->values.empty() && attr->values.front().state != AttrState::set) {
    incorrect_values = attr->values.size() - 1;
    attr->values.resize(1);
    return incorrect_values;
  }
  // Not Out-Of-Band value. Filter out incorrect values.
  std::vector<RawValue> new_values;
  new_values.reserve(attr->values.size());
  for (RawValue& val : attr->values) {
    if (val.state == AttrState::set && IsConvertibleTo(val.type, type)) {
      new_values.emplace_back(std::move(val));
    } else {
      ++incorrect_values;
    }
  }
  attr->values = std::move(new_values);
  return incorrect_values;
}

// Converts a collection/group saved in |raw_coll| to |coll|. Both |raw_coll|
// and |coll| must not be nullptr. Returns false <=> critical parsing error was
// spotted.
bool Parser::DecodeCollection(RawCollection* raw_coll, Collection* coll) {
  for (RawAttribute& raw_attr : raw_coll->attributes) {
    // Tries to match the attribute to existing one by name.
    Attribute* attr = coll->GetAttribute(raw_attr.name);
    ContextPathGuard path_update(&parser_context_, raw_attr.name,
                                 attr != nullptr);
    // Tries to detect a type.
    AttrType detectedType = AttrType::integer;
    if (attr == nullptr) {
      // It is unknown one, try to detect type and create UnknownAttribute.
      if (!raw_attr.values.empty()) {
        detectedType = static_cast<AttrType>(raw_attr.values.front().type);
        for (auto& raw_val : raw_attr.values)
          if (IsConvertibleTo(detectedType, raw_val.type))
            detectedType = raw_val.type;
      }
    } else {
      // The attribute exists.
      detectedType = attr->GetType();
      // Make sure there is no name conflict.
      if (attr->GetState() != AttrState::unset) {
        LogParserError("An attribute with duplicated name was spotted",
                       "The attribute was ignored");
        continue;
      }
    }
    // Remove values with incorrect type.
    size_t incorrect_values = RemoveIncompatibleValues(detectedType, &raw_attr);
    if (incorrect_values > 0)
      LogParserError(
          "An attribute contains at least one value with incorrect type",
          "Additional values were ignored");
    // Check if there are any values left.
    if (raw_attr.values.empty()) {
      LogParserError("An attribute contains no correct values",
                     "The attribute was ignored");
      continue;
    }
    // Register UnknownAtribute if it was not found.
    if (attr == nullptr) {
      attr = coll->AddUnknownAttribute(raw_attr.name, true, detectedType);
      if (attr == nullptr) {
        LogParserError("Internal parser error: cannot create unknown attribute",
                       "The attribute was ignored");
        continue;
      }
      LogParserNewElement();
    }
    // Is it an attribute with Ouf-Of-Band value? Then set it and finish.
    if (raw_attr.values.front().state != AttrState::set) {
      attr->SetState(raw_attr.values.front().state);
      continue;
    }
    // Parses all values.
    if (attr->IsASet()) {
      attr->Resize(raw_attr.values.size());
    } else {
      if (raw_attr.values.size() > 1) {
        LogParserError("An attribute is not a set and has more than one value",
                       "Only the first value was parsed");
        raw_attr.values.resize(1);
      }
    }
    for (size_t i = 0; i < attr->GetSize(); ++i)
      if (attr->GetType() == AttrType::collection) {
        if (!DecodeCollection(raw_attr.values[i].collection.get(),
                              attr->GetCollection(i)))
          return false;
      } else {
        LoadAttrValue(attr, i, raw_attr.values[i].data, raw_attr.values[i].tag);
      }
  }
  return true;
}

}  // namespace ipp
