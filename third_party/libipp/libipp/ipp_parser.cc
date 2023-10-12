// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_parser.h"
#include <set>

#include "libipp/frame.h"
#include "libipp/ipp_encoding.h"

namespace ipp {

namespace {

// This parameter defines how deep can be a package with recursive collections.
// A collection placed directly in attributes group has level 1, each
// sub-collection belonging directly to it has level 2 etc..
constexpr int kMaxCollectionLevel = 16;

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
bool LoadInteger(const std::vector<uint8_t>& data, int32_t* out) {
  if ((data.size() != BytesCount) || (out == nullptr))
    return false;
  const uint8_t* ptr = data.data();
  ParseSignedInteger<BytesCount>(&ptr, out);
  return true;
}

// Reads simple string from buf.
std::string LoadString(const std::vector<uint8_t>& buf) {
  return std::string(buf.data(), buf.data() + buf.size());
}

// Reads textWithLanguage/nameWithLanguage (see [rfc8010], section 3.9) from
// buf. Returns false if given content is invalid or (out == nullptr).
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
// Fails when binary representation has invalid size or (out == nullptr).
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
// Fails when binary representation has invalid size or (out == nullptr).
bool LoadResolution(const std::vector<uint8_t>& buf, ipp::Resolution* out) {
  if ((buf.size() != 9) || (out == nullptr))
    return false;
  const uint8_t* ptr = buf.data();
  ParseSignedInteger<4>(&ptr, &out->xres);
  ParseSignedInteger<4>(&ptr, &out->yres);
  int8_t units;
  ParseSignedInteger<1>(&ptr, &units);
  out->units = static_cast<Resolution::Units>(units);
  return true;
}

// Reads rangeOfInteger (see [rfc8010]) from buf.
// Fails when binary representation has invalid size or (out == nullptr).
bool LoadRangeOfInteger(const std::vector<uint8_t>& buf,
                        ipp::RangeOfInteger* out) {
  if ((buf.size() != 8) || (out == nullptr))
    return false;
  const uint8_t* ptr = buf.data();
  ParseSignedInteger<4>(&ptr, &out->min_value);
  ParseSignedInteger<4>(&ptr, &out->max_value);
  return true;
}

// Scope guard to control the context path. Constructor adds new a element to
// the path while destructor removes it from the path.
class ContextPathGuard {
 public:
  ContextPathGuard(AttrPath* path, uint16_t index) : path_(path) {
    path_->PushBack(index, "");
  }
  ~ContextPathGuard() { path_->PopBack(); }

 private:
  ContextPathGuard(const ContextPathGuard&) = delete;
  ContextPathGuard(ContextPathGuard&&) = delete;
  ContextPathGuard& operator=(const ContextPathGuard&) = delete;
  ContextPathGuard& operator=(ContextPathGuard&&) = delete;
  AttrPath* path_;
};

// Return true if source type can be used in attribute of target type.
bool IsConvertibleTo(const ipp::ValueTag source, const ipp::ValueTag target) {
  if (source == target)
    return true;
  if (source == ipp::ValueTag::integer &&
      target == ipp::ValueTag::rangeOfInteger)
    return true;
  if (source == ipp::ValueTag::integer && target == ipp::ValueTag::enum_)
    return true;
  if (source == ipp::ValueTag::nameWithoutLanguage &&
      target == ipp::ValueTag::nameWithLanguage)
    return true;
  if (source == ipp::ValueTag::textWithoutLanguage &&
      target == ipp::ValueTag::textWithLanguage)
    return true;
  return false;
}

// Parses two bytes from `ptr` and move it forward by 2 bytes.
uint16_t ParseUInt16(const uint8_t*& ptr) {
  uint16_t val = *ptr;
  val <<= 8;
  val += *++ptr;
  ++ptr;
  return val;
}

}  //  namespace

std::string_view ToStrViewVerbose(ParserCode code) {
  static const std::string kLimitOnCollectionLevelMsg =
      "The frame has too many recursive collections; the maximum allowed "
      "number of levels is " +
      ToString(kMaxCollectionLevel) + ".";
  static const std::string kLimitOnGroupsCountMsg =
      "The frame has too many attribute groups; the maximum allowed "
      "number is " +
      ToString(static_cast<int>(kMaxCountOfAttributeGroups)) + ".";
  switch (code) {
    case ParserCode::kOK:
      return "No errors.";
    case ParserCode::kAttributeNameIsEmpty:
      return "Attribute with an empty name was spotted.";
    case ParserCode::kValueMismatchTagConverted:
      return "Value with mismatched tag was spotted. The value was converted "
             "to the attribute's type.";
    case ParserCode::kValueMismatchTagOmitted:
      return "A value with incompatible tag was spotted. The value was "
             "ignored.";
    case ParserCode::kAttributeNameConflict:
      return "An attribute with duplicate name was spotted. The attribute was "
             "ignored.";
    case ParserCode::kBooleanValueOutOfRange:
      return "A boolean value has an integer different from 0 and 1. The value "
             "was set to true.";
    case ParserCode::kValueInvalidSize:
      return "A value has invalid size. The value was ignored.";
    case ParserCode::kAttributeNoValues:
      return "An attribute has no valid values. The attribute was ignored.";
    case ParserCode::kErrorWhenAddingAttribute:
      return "Internal parser error: cannot add an attribute. The attribute "
             "was ignored.";
    case ParserCode::kOutOfBandAttributeWithManyValues:
      return "An out-of-band attribute has more than one value. Additional "
             "values were ignored.";
    case ParserCode::kOutOfBandValueWithNonEmptyData:
      return "A value in an out-of-band attribute has a non-empty data field. "
             "Additional data was ignored.";
    case ParserCode::kUnexpectedEndOfFrame:
      return "Unexpected end of frame.";
    case ParserCode::kGroupTagWasExpected:
      return "begin-attribute-group-tag was expected but other value was read.";
    case ParserCode::kEmptyNameExpectedInTNV:
      return "Tag-Name-Value was supposed to have an empty name, but the name "
             "is non-empty.";
    case ParserCode::kEmptyValueExpectedInTNV:
      return "Tag-Name-Value was supposed to have an empty value, but the "
             "value is non-empty.";
    case ParserCode::kNegativeNameLengthInTNV:
      return "name-length in Tag-Name-Value is negative.";
    case ParserCode::kNegativeValueLengthInTNV:
      return "value-length in Tag-Name-Value is negative.";
    case ParserCode::kTNVWithUnexpectedValueTag:
      return "TNV with unexpected value-tag was spotted. The parser stopped.";
    case ParserCode::kUnsupportedValueTag:
      return "Attribute's value with unsupported syntax. The value was "
             "omitted.";
    case ParserCode::kUnexpectedEndOfGroup:
      return "Unexpected end of attribute-group. The parser stopped.";
    case ParserCode::kLimitOnCollectionsLevelExceeded:
      return kLimitOnCollectionLevelMsg;
    case ParserCode::kLimitOnGroupsCountExceeded:
      return kLimitOnGroupsCountMsg;
    case ParserCode::kErrorWhenAddingGroup:
      return "Internal parser error: cannot add a group. The group was "
             "omitted.";
  }
}

void Parser::LogParserError(ParserCode error_code, const uint8_t* ptr) {
  if (error_code == ParserCode::kOK) {
    // ignore
    return;
  }
  Log l;
  l.message = ToStrViewVerbose(error_code);
  l.parser_context = parser_context_.AsString();
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
  log_->AddParserError({parser_context_, error_code});
}

void Parser::LogParserErrors(const std::vector<ParserCode>& error_codes) {
  for (ParserCode error_code : error_codes) {
    LogParserError(error_code);
  }
}

// Temporary representation of an attribute's value parsed from TNVs.
struct RawValue {
  // original tag (IsValid(tag))
  ValueTag tag;
  // original data, empty when (tag == collection), content not verified
  std::vector<uint8_t> data;
  // (not nullptr) <=> (tag == collection)
  std::unique_ptr<RawCollection> collection;
  // create as standard value
  RawValue(ValueTag tag, const std::vector<uint8_t>& data)
      : tag(tag), data(data) {}
  // create as collection
  explicit RawValue(RawCollection* coll)
      : tag(ValueTag::collection), collection(coll) {}
};

struct RawCollection;

// Temporary representation of an attribute parsed from TNVs.
struct RawAttribute {
  // verified (non-empty)
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

// Parse a value of type `attr_type` from `raw_value` to `output` when possible.
template <typename ApiType>
ParserCode LoadAttrValue(ValueTag attr_type,
                         const RawValue& raw_value,
                         ApiType& output);

template <>
ParserCode LoadAttrValue<std::string>(ValueTag attr_type,
                                      const RawValue& raw_value,
                                      std::string& output) {
  if (!IsString(raw_value.tag) && raw_value.tag != ValueTag::octetString) {
    return ParserCode::kValueMismatchTagOmitted;
  }
  output = LoadString(raw_value.data);
  return (attr_type == raw_value.tag) ? ParserCode::kOK
                                      : ParserCode::kValueMismatchTagConverted;
}

template <>
ParserCode LoadAttrValue<int32_t>(ValueTag attr_type,
                                  const RawValue& raw_value,
                                  int32_t& output) {
  switch (raw_value.tag) {
    case ValueTag::boolean: {
      if (!LoadInteger<1>(raw_value.data, &output)) {
        return ParserCode::kValueInvalidSize;
      }
      if (attr_type != ValueTag::boolean) {
        return ParserCode::kValueMismatchTagConverted;
      }
      if (output < 0 || output > 1) {
        output = 1;
        return ParserCode::kBooleanValueOutOfRange;
      }
      return ParserCode::kOK;
    }
    case ValueTag::integer:
    case ValueTag::enum_: {
      if (!LoadInteger<4>(raw_value.data, &output)) {
        return ParserCode::kValueInvalidSize;
      }
      if (attr_type != raw_value.tag) {
        return ParserCode::kValueMismatchTagConverted;
      }
      return ParserCode::kOK;
    }
    default:
      return ParserCode::kValueMismatchTagOmitted;
  }
}

template <>
ParserCode LoadAttrValue<DateTime>(ValueTag attr_type,
                                   const RawValue& raw_value,
                                   DateTime& output) {
  if (raw_value.tag != ValueTag::dateTime) {
    return ParserCode::kValueMismatchTagOmitted;
  }
  if (!LoadDateTime(raw_value.data, &output)) {
    return ParserCode::kValueInvalidSize;
  }
  return ParserCode::kOK;
}

template <>
ParserCode LoadAttrValue<Resolution>(ValueTag attr_type,
                                     const RawValue& raw_value,
                                     Resolution& output) {
  if (raw_value.tag != ValueTag::resolution) {
    return ParserCode::kValueMismatchTagOmitted;
  }
  if (!LoadResolution(raw_value.data, &output)) {
    return ParserCode::kValueInvalidSize;
  }
  return ParserCode::kOK;
}

template <>
ParserCode LoadAttrValue<RangeOfInteger>(ValueTag attr_type,
                                         const RawValue& raw_value,
                                         RangeOfInteger& output) {
  if (raw_value.tag == ValueTag::integer) {
    if (!LoadInteger<4>(raw_value.data, &output.min_value)) {
      return ParserCode::kValueInvalidSize;
    }
    output.max_value = output.min_value;
    return ParserCode::kOK;
  }
  if (raw_value.tag != ValueTag::rangeOfInteger) {
    return ParserCode::kValueMismatchTagOmitted;
  }
  if (!LoadRangeOfInteger(raw_value.data, &output)) {
    return ParserCode::kValueInvalidSize;
  }
  return ParserCode::kOK;
}

template <>
ParserCode LoadAttrValue<StringWithLanguage>(ValueTag attr_type,
                                             const RawValue& raw_value,
                                             StringWithLanguage& output) {
  if (raw_value.tag == ValueTag::nameWithLanguage ||
      raw_value.tag == ValueTag::textWithLanguage) {
    if (!LoadStringWithLanguage(raw_value.data, &output)) {
      return ParserCode::kValueInvalidSize;
    }
    if (raw_value.tag != attr_type) {
      return ParserCode::kValueMismatchTagConverted;
    }
    return ParserCode::kOK;
  }
  if (IsString(raw_value.tag)) {
    output.language.clear();
    output.value = LoadString(raw_value.data);
    if (raw_value.tag == ValueTag::nameWithoutLanguage &&
        attr_type != ValueTag::nameWithLanguage) {
      return ParserCode::kValueMismatchTagConverted;
    }
    if (raw_value.tag == ValueTag::textWithoutLanguage &&
        attr_type != ValueTag::textWithLanguage) {
      return ParserCode::kValueMismatchTagConverted;
    }
    return ParserCode::kOK;
  }
  return ParserCode::kValueMismatchTagOmitted;
}

// Parse an attribute of type `attr_type` from `raw_attr` and add it to `coll`
// when possible. Return a list of parser errors. `coll` must not be nullptr.
template <typename ApiType>
std::vector<ParserCode> LoadAttrValues(Collection* coll,
                                       ValueTag attr_type,
                                       const RawAttribute& raw_attr) {
  std::vector<ParserCode> errors;
  std::vector<ApiType> vals;
  vals.reserve(raw_attr.values.size());
  for (const RawValue& raw_value : raw_attr.values) {
    ApiType val;
    ParserCode code = LoadAttrValue<ApiType>(attr_type, raw_value, val);
    if (code == ParserCode::kOK ||
        code == ParserCode::kValueMismatchTagConverted ||
        code == ParserCode::kBooleanValueOutOfRange) {
      vals.push_back(std::move(val));
    }
    if (code != ParserCode::kOK) {
      errors.push_back(code);
    }
  }
  if (vals.empty()) {
    errors.push_back(ParserCode::kAttributeNoValues);
  } else {
    const Code err = coll->AddAttr(raw_attr.name, attr_type, vals);
    if (err != Code::kOK) {
      errors.push_back(ParserCode::kErrorWhenAddingAttribute);
    }
  }
  return errors;
}

bool Parser::SaveFrameToPackage(bool log_unknown_values, Frame* package) {
  for (size_t i = 0; i < frame_->groups_tags_.size(); ++i) {
    GroupTag gn = frame_->groups_tags_[i];
    parser_context_ = AttrPath(gn);
    parser_context_.PushBack(package->Groups(gn).size(), "");
    CollsView::iterator coll;
    Code err = package->AddGroup(gn, coll);
    if (err != Code::kOK) {
      LogParserError(ParserCode::kErrorWhenAddingGroup);
      continue;
    }
    RawCollection raw_coll;
    if (!ParseRawGroup(&(frame_->groups_content_[i]), &raw_coll)) {
      if (!errors_->empty())
        errors_->back().message +=
            " This is critical error, parsing was cancelled.";
      return false;
    }
    DecodeCollection(&raw_coll, &*coll);
  }
  package->SetData(std::move(frame_->data_));
  return true;
}

bool Parser::ReadFrameFromBuffer(const uint8_t* ptr,
                                 const uint8_t* const buf_end) {
  parser_context_ = AttrPath(AttrPath::kHeader);
  buffer_begin_ = ptr;
  buffer_end_ = buf_end;
  if (buf_end - ptr < 9) {
    LogParserError(ParserCode::kUnexpectedEndOfFrame, ptr);
    return false;
  }
  frame_->version_ = ParseUInt16(ptr);
  ParseSignedInteger<2>(&ptr, &frame_->operation_id_or_status_code_);
  ParseSignedInteger<4>(&ptr, &frame_->request_id_);
  while (*ptr != end_of_attributes_tag) {
    GroupTag group_tag = static_cast<GroupTag>(*ptr);
    parser_context_ = AttrPath(group_tag);
    if (!IsValid(group_tag)) {
      // begin-attribute-group-tag was expected.
      LogParserError(ParserCode::kGroupTagWasExpected, ptr);
      return false;
    }
    if (frame_->groups_tags_.size() >= kMaxCountOfAttributeGroups) {
      LogParserError(ParserCode::kLimitOnGroupsCountExceeded, ptr);
      return false;
    }
    frame_->groups_tags_.push_back(group_tag);
    frame_->groups_content_.resize(frame_->groups_tags_.size());
    ++ptr;
    if (!ReadTNVsFromBuffer(&ptr, buf_end, &(frame_->groups_content_.back())))
      return false;
    if (ptr >= buf_end) {
      // begin-attribute-group-tag or end-of-attributes-tag was expected.
      LogParserError(ParserCode::kUnexpectedEndOfFrame);
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
      // Expected at least 1-byte tag, 2-bytes name-length and 2-bytes
      // value-length.
      LogParserError(ParserCode::kUnexpectedEndOfFrame, ptr);
      return false;
    }
    // *ptr is a ValueTag because it was already checked in while (...)
    tnv.tag = *ptr;
    ++ptr;
    int length = 0;
    if (!ParseUnsignedInteger<2>(&ptr, &length)) {
      LogParserError(ParserCode::kNegativeNameLengthInTNV, ptr);
      return false;
    }
    if (buf_end - ptr < length + 2) {
      // Expected at least `length`-bytes name and 2-bytes value-length.
      LogParserError(ParserCode::kUnexpectedEndOfFrame, ptr);
      return false;
    }
    tnv.name.assign(ptr, ptr + length);
    ptr += length;
    if (!ParseUnsignedInteger<2>(&ptr, &length)) {
      LogParserError(ParserCode::kNegativeValueLengthInTNV, ptr);
      return false;
    }
    if (buf_end - ptr < length) {
      LogParserError(ParserCode::kUnexpectedEndOfFrame, ptr);
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
  parser_context_ = AttrPath(AttrPath::kHeader);
}

// Parses single attribute's value and add it to |attr|. |tnv| is the first TNV
// with the value, |tnvs| contains all following TNVs. Both |tnvs| and |attr|
// cannot be nullptr. |coll_level| denotes how "deep" is the collection that
// contains the attribute; attributes defined directly in the attributes group
// have level 0. Returns false <=> critical parsing error was spotted.
// See section 3.5.2 from rfc8010 for details.
bool Parser::ParseRawValue(int coll_level,
                           const TagNameValue& tnv,
                           std::list<TagNameValue>* tnvs,
                           RawAttribute* attr) {
  // Is it correct attribute's value tag? If not then fail.
  if (tnv.tag == endCollection_value_tag ||
      tnv.tag == memberAttrName_value_tag) {
    LogParserError(ParserCode::kTNVWithUnexpectedValueTag);
    return false;
  }
  // Is it a collection ?
  if (tnv.tag == begCollection_value_tag) {
    ContextPathGuard path_update(&parser_context_, attr->values.size());
    if (!tnv.value.empty()) {
      LogParserError(ParserCode::kEmptyValueExpectedInTNV);
      return false;
    }
    std::unique_ptr<RawCollection> coll(new RawCollection);
    if (!ParseRawCollection(coll_level + 1, tnvs, coll.get()))
      return false;
    attr->values.emplace_back(coll.release());
    return true;
  }
  ValueTag type = static_cast<ValueTag>(tnv.tag);
  if (!IsValid(type)) {
    // unknown attribute's syntax
    LogParserError(ParserCode::kUnsupportedValueTag);
    return true;
  }
  attr->values.emplace_back(type, tnv.value);
  return true;
}

// Parses single collections from given TNVs. |coll_level| denotes how "deep"
// the collection is; collections defined directly in the attributes group
// have level 1. Both |tnvs| and |coll| cannot be nullptr.
// Returns false <=> critical parsing error was spotted.
bool Parser::ParseRawCollection(int coll_level,
                                std::list<TagNameValue>* tnvs,
                                RawCollection* coll) {
  if (coll_level > kMaxCollectionLevel) {
    LogParserError(ParserCode::kLimitOnCollectionsLevelExceeded);
    return false;
  }
  while (true) {
    if (tnvs->empty()) {
      LogParserError(ParserCode::kUnexpectedEndOfGroup);
      return false;
    }
    TagNameValue tnv = tnvs->front();
    tnvs->pop_front();
    // exit if the end of the collection was reached
    if (tnv.tag == endCollection_value_tag) {
      if (!tnv.name.empty()) {
        LogParserError(ParserCode::kEmptyNameExpectedInTNV);
        return false;
      }
      if (!tnv.value.empty()) {
        LogParserError(ParserCode::kEmptyValueExpectedInTNV);
        return false;
      }
      return true;
    }
    // still here, so we parse an attribute (collection's member)
    if (tnv.tag != memberAttrName_value_tag) {
      LogParserError(ParserCode::kTNVWithUnexpectedValueTag);
      return false;
    }
    // parse name & create attribute
    const std::string name = LoadString(tnv.value);
    parser_context_.Back().attribute_name = name;
    if (!tnv.name.empty()) {
      LogParserError(ParserCode::kEmptyNameExpectedInTNV);
      return false;
    }
    if (name.empty()) {
      LogParserErrors({ParserCode::kAttributeNameIsEmpty});
      return false;
    }
    coll->attributes.emplace_back(name);
    RawAttribute* attr = &coll->attributes.back();
    // parse tag
    if (tnvs->empty()) {
      LogParserError(ParserCode::kUnexpectedEndOfGroup);
      return false;
    }
    // parse all values
    while (!tnvs->empty() && tnvs->front().tag != endCollection_value_tag &&
           tnvs->front().tag != memberAttrName_value_tag) {
      tnv = tnvs->front();
      tnvs->pop_front();
      if (!tnv.name.empty()) {
        LogParserError(ParserCode::kEmptyNameExpectedInTNV);
        return false;
      }
      if (!ParseRawValue(coll_level, tnv, tnvs, attr))
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
    const std::string name = LoadString(tnv.name);
    parser_context_.Back().attribute_name = name;
    if (name.empty()) {
      LogParserErrors({ParserCode::kAttributeNameIsEmpty});
      return false;
    }
    coll->attributes.emplace_back(name);
    RawAttribute* attr = &coll->attributes.back();
    // parse all values
    while (true) {
      // parse value
      if (!ParseRawValue(0 /*collection level*/, tnv, tnvs, attr))
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

// Converts a collection/group saved in |raw_coll| to |coll|. Both |raw_coll|
// and |coll| must not be nullptr.
void Parser::DecodeCollection(RawCollection* raw_coll, Collection* coll) {
  for (RawAttribute& raw_attr : raw_coll->attributes) {
    // Tries to match the attribute to existing one by name.
    parser_context_.Back().attribute_name = raw_attr.name;
    if (coll->GetAttr(raw_attr.name) != coll->end()) {
      // The attribute exists.
      LogParserErrors({ParserCode::kAttributeNameConflict});
      continue;
    }

    if (raw_attr.values.empty()) {
      LogParserErrors({ParserCode::kAttributeNoValues});
      continue;
    }

    // Tries to detect an attribute's type.
    ValueTag detected_type = raw_attr.values.front().tag;
    for (auto& raw_val : raw_attr.values)
      if (IsConvertibleTo(detected_type, raw_val.tag))
        detected_type = raw_val.tag;

    // Is it an attribute with Ouf-Of-Band value? Then set it and finish.
    if (IsOutOfBand(detected_type)) {
      if (raw_attr.values.size() > 1) {
        LogParserError(ParserCode::kOutOfBandAttributeWithManyValues);
      }
      if (!raw_attr.values.front().data.empty()) {
        LogParserError(ParserCode::kOutOfBandValueWithNonEmptyData);
      }
      const Code err = coll->AddAttr(raw_attr.name, detected_type);
      if (err != Code::kOK) {
        LogParserErrors({ParserCode::kErrorWhenAddingAttribute});
      }
      continue;
    }

    // It is a collection?
    if (detected_type == ValueTag::collection) {
      std::vector<ParserCode> errors;
      std::vector<RawCollection*> raw_colls;
      raw_colls.reserve(raw_attr.values.size());
      for (const RawValue& raw_value : raw_attr.values) {
        if (raw_value.collection) {
          raw_colls.push_back(raw_value.collection.get());
        } else {
          errors.push_back(ParserCode::kValueMismatchTagOmitted);
        }
      }
      CollsView colls;
      Code err = coll->AddAttr(raw_attr.name, raw_colls.size(), colls);
      if (err == Code::kOK) {
        for (size_t i = 0; i < colls.size(); ++i) {
          ContextPathGuard path_update(&parser_context_, i);
          DecodeCollection(raw_colls[i], &colls[i]);
        }
      } else {
        errors.push_back(ParserCode::kErrorWhenAddingAttribute);
      }
      LogParserErrors(errors);
      continue;
    }

    // It is an attribute with standard values. Parse the values and create
    // a new attribute.
    if (IsInteger(detected_type)) {
      LogParserErrors(LoadAttrValues<int32_t>(coll, detected_type, raw_attr));
      continue;
    }
    if (IsString(detected_type) || detected_type == ValueTag::octetString) {
      LogParserErrors(
          LoadAttrValues<std::string>(coll, detected_type, raw_attr));
      continue;
    }
    switch (detected_type) {
      case ValueTag::dateTime:
        LogParserErrors(
            LoadAttrValues<DateTime>(coll, detected_type, raw_attr));
        break;
      case ValueTag::resolution:
        LogParserErrors(
            LoadAttrValues<Resolution>(coll, detected_type, raw_attr));
        break;
      case ValueTag::rangeOfInteger:
        LogParserErrors(
            LoadAttrValues<RangeOfInteger>(coll, detected_type, raw_attr));
        break;
      case ValueTag::nameWithLanguage:
      case ValueTag::textWithLanguage:
        LogParserErrors(
            LoadAttrValues<StringWithLanguage>(coll, detected_type, raw_attr));
        break;
      default:
        LogParserErrors({ParserCode::kErrorWhenAddingAttribute});
        break;
    }
  }
}

}  // namespace ipp
