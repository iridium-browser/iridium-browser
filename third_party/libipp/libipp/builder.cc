// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "builder.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "frame.h"
#include "ipp_attribute.h"
#include "ipp_encoding.h"
#include "ipp_frame.h"

namespace ipp {

namespace {

struct GroupAsTNVs {
  GroupTag tag;
  std::list<TagNameValue> content;
};

// Saves boolean to buf.
void SaveBoolean(bool v, std::vector<uint8_t>* buf) {
  buf->resize(1);
  *(buf->data()) = v ? 1 : 0;
}

// Saves 4-bytes integer to buf using two's-complement binary encoding.
void SaveInt32(int32_t v, std::vector<uint8_t>* buf) {
  buf->resize(4);
  uint8_t* ptr = buf->data();
  WriteInteger<4>(&ptr, v);
}

// Saves simple string to buf, buf is resized accordingly.
// If `s` is too long, the written content is silently truncated.
// The length of values of this type is verified when they are passed to a
// Frame object, so the truncation here should never occur.
void SaveOctetString(std::string_view s, std::vector<uint8_t>* buf) {
  static constexpr size_t max_size = std::numeric_limits<int16_t>::max();
  const size_t value_size = std::min(s.size(), max_size);
  buf->resize(value_size);
  std::copy_n(s.begin(), value_size, buf->begin());
}

// Writes textWithLanguage/nameWithLanguage (see [rfc8010]) to buf, which is
// resized accordingly.
// If `s` is too long, the written content is silently truncated.
// The length of values of this type is verified when they are passed to a
// Frame object, so the truncation here should never occur.
void SaveStringWithLanguage(const ipp::StringWithLanguage& s,
                            std::vector<uint8_t>* buf) {
  // StringWithLanguage is used to save nameWithLanguage and textWithLanguage
  // values. They are saved as a sequence of the following fields (see section
  // 3.9 from rfc8010):
  // * int16_t (2 bytes) - length of the language field = L
  // * string (L bytes) - content of the language field
  // * int16_t (2 bytes) - length of the value field = V
  // * string (V bytes) - content of the value field
  // The total size (2 + L + 2 + V) cannot exceed the threshold.
  static constexpr size_t max_size = std::numeric_limits<int16_t>::max();
  const size_t lang_size = std::min(s.language.size(), max_size - 4);
  const size_t value_size = std::min(s.value.size(), max_size - 4 - lang_size);
  buf->resize(4 + lang_size + value_size);
  uint8_t* ptr = buf->data();
  WriteInteger<2>(&ptr, static_cast<int16_t>(lang_size));
  ptr = std::copy_n(s.language.begin(), lang_size, ptr);
  WriteInteger<2>(&ptr, static_cast<int16_t>(value_size));
  std::copy_n(s.value.begin(), value_size, ptr);
}

// Saves dateTime (see [rfc8010,rfc2579]) to buf, which is resized accordingly.
void SaveDateTime(const ipp::DateTime& v, std::vector<uint8_t>* buf) {
  buf->resize(11);
  uint8_t* ptr = buf->data();
  WriteUnsigned<2>(&ptr, v.year);
  WriteUnsigned<1>(&ptr, v.month);
  WriteUnsigned<1>(&ptr, v.day);
  WriteUnsigned<1>(&ptr, v.hour);
  WriteUnsigned<1>(&ptr, v.minutes);
  WriteUnsigned<1>(&ptr, v.seconds);
  WriteUnsigned<1>(&ptr, v.deci_seconds);
  WriteUnsigned<1>(&ptr, v.UTC_direction);
  WriteUnsigned<1>(&ptr, v.UTC_hours);
  WriteUnsigned<1>(&ptr, v.UTC_minutes);
}

// Writes resolution (see [rfc8010]) to buf, which is resized accordingly.
void SaveResolution(const ipp::Resolution& v, std::vector<uint8_t>* buf) {
  buf->resize(9);
  uint8_t* ptr = buf->data();
  WriteInteger<4>(&ptr, v.xres);
  WriteInteger<4>(&ptr, v.yres);
  WriteInteger<1>(&ptr, static_cast<int8_t>(v.units));
}

// Writes rangeOfInteger (see [rfc8010]) to buf, which is resized accordingly.
void SaveRangeOfInteger(const ipp::RangeOfInteger& v,
                        std::vector<uint8_t>* buf) {
  buf->resize(8);
  uint8_t* ptr = buf->data();
  WriteInteger<4>(&ptr, v.min_value);
  WriteInteger<4>(&ptr, v.max_value);
}

// Helpers for converting individual values to binary form.
void SaveAttrValue(const Attribute* attr,
                   size_t index,
                   uint8_t* tag,
                   std::vector<uint8_t>* buf) {
  *tag = static_cast<uint8_t>(attr->Tag());
  switch (attr->Tag()) {
    case ValueTag::boolean: {
      int v = 0;
      attr->GetValue(index, v);
      SaveBoolean((v != 0), buf);
      break;
    }
    case ValueTag::integer:
    case ValueTag::enum_: {
      int v = 0;
      attr->GetValue(index, v);
      SaveInt32(v, buf);
      break;
    }
    case ValueTag::dateTime: {
      DateTime v;
      attr->GetValue(index, v);
      SaveDateTime(v, buf);
      break;
    }
    case ValueTag::resolution: {
      Resolution v;
      attr->GetValue(index, v);
      SaveResolution(v, buf);
      break;
    }
    case ValueTag::rangeOfInteger: {
      RangeOfInteger v;
      attr->GetValue(index, v);
      SaveRangeOfInteger(v, buf);
      break;
    }
    case ValueTag::textWithLanguage: {
      StringWithLanguage s;
      attr->GetValue(index, s);
      if (s.language.empty()) {
        *tag = static_cast<uint8_t>(ValueTag::textWithoutLanguage);
        SaveOctetString(s.value, buf);
      } else {
        SaveStringWithLanguage(s, buf);
      }
      break;
    }
    case ValueTag::nameWithLanguage: {
      StringWithLanguage s;
      attr->GetValue(index, s);
      if (s.language.empty()) {
        *tag = static_cast<uint8_t>(ValueTag::nameWithoutLanguage);
        SaveOctetString(s.value, buf);
      } else {
        SaveStringWithLanguage(s, buf);
      }
      break;
    }
    default:
      if (IsString(attr->Tag()) || attr->Tag() == ValueTag::octetString) {
        std::string s = "";
        attr->GetValue(index, s);
        SaveOctetString(s, buf);
      } else {
        // attr->Tag() must be out-of-band or unsupported value.
        // Both of these should be handled earlier.
        buf->clear();
      }
      break;
  }
}

void SaveCollection(const Collection& coll,
                    std::list<TagNameValue>* data_chunks) {
  // save the attributes
  for (const Attribute& attr : coll) {
    TagNameValue tnv;
    tnv.tag = memberAttrName_value_tag;
    tnv.name.clear();
    SaveOctetString(attr.Name(), &tnv.value);
    data_chunks->push_back(tnv);
    if (IsOutOfBand(attr.Tag())) {
      tnv.tag = static_cast<uint8_t>(attr.Tag());
      tnv.value.clear();
      data_chunks->push_back(tnv);
    } else {
      // standard values (one or more)
      for (size_t val_index = 0; val_index < attr.Size(); ++val_index) {
        if (attr.Tag() == ValueTag::collection) {
          tnv.tag = begCollection_value_tag;
          tnv.value.clear();
          data_chunks->push_back(tnv);
          SaveCollection(attr.Colls()[val_index], data_chunks);
          tnv.tag = endCollection_value_tag;
          tnv.value.clear();
        } else {
          SaveAttrValue(&attr, val_index, &tnv.tag, &tnv.value);
        }
        data_chunks->push_back(tnv);
      }
    }
  }
}

void SaveGroup(const Collection* coll, std::list<TagNameValue>* data_chunks) {
  // save the attributes
  for (const Attribute& attr : *coll) {
    TagNameValue tnv;
    SaveOctetString(attr.Name(), &tnv.name);
    if (IsOutOfBand(attr.Tag())) {
      tnv.tag = static_cast<uint8_t>(attr.Tag());
      tnv.value.clear();
      data_chunks->push_back(tnv);
      continue;
    }
    for (size_t val_index = 0; val_index < attr.Size(); ++val_index) {
      if (attr.Tag() == ValueTag::collection) {
        tnv.tag = begCollection_value_tag;
        tnv.value.clear();
        data_chunks->push_back(tnv);
        SaveCollection(attr.Colls()[val_index], data_chunks);
        tnv.tag = endCollection_value_tag;
        tnv.name.clear();
        tnv.value.clear();
      } else {
        SaveAttrValue(&attr, val_index, &tnv.tag, &tnv.value);
      }
      data_chunks->push_back(tnv);
      tnv.name.clear();
    }
  }
}

// Write data to buffer (ptr is updated, whole list is written).
void WriteTNVsToBuffer(const std::list<TagNameValue>& tnvs, uint8_t** ptr) {
  for (auto& tnv : tnvs) {
    WriteUnsigned<1>(ptr, tnv.tag);
    WriteInteger<2>(ptr, static_cast<int16_t>(tnv.name.size()));
    *ptr = std::copy(tnv.name.begin(), tnv.name.end(), *ptr);
    WriteInteger<2>(ptr, static_cast<int16_t>(tnv.value.size()));
    *ptr = std::copy(tnv.value.begin(), tnv.value.end(), *ptr);
  }
}

std::vector<GroupAsTNVs> PreprocessFrame(const Frame& frame) {
  std::vector<GroupAsTNVs> groups_tnvs;
  // save frame data
  std::vector<std::pair<GroupTag, const Collection*>> groups =
      frame.GetGroups();
  for (std::pair<GroupTag, const Collection*> grp : groups) {
    groups_tnvs.emplace_back(GroupAsTNVs{.tag = grp.first});
    SaveGroup(grp.second, &(groups_tnvs.back().content));
  }
  return groups_tnvs;
}

void WriteFrameToBuffer(const Frame& frame,
                        const std::vector<GroupAsTNVs>& groups,
                        uint8_t* ptr) {
  WriteUnsigned<2>(&ptr, static_cast<uint16_t>(frame.VersionNumber()));
  WriteInteger<2>(&ptr, frame.OperationIdOrStatusCode());
  WriteInteger<4>(&ptr, frame.RequestId());
  for (size_t i = 0; i < groups.size(); ++i) {
    // write a group to the buffer
    WriteUnsigned<1>(&ptr, static_cast<uint8_t>(groups[i].tag));
    WriteTNVsToBuffer(groups[i].content, &ptr);
  }
  WriteUnsigned<1>(&ptr, end_of_attributes_tag);
  std::copy(frame.Data().begin(), frame.Data().end(), ptr);
}

std::size_t GetFrameLength(const Frame& frame,
                           const std::vector<GroupAsTNVs>& groups) {
  // Header has always 8 bytes (ipp_version + operation_id/status + request_id).
  size_t length = 8;
  // The header is followed by a list of groups.
  for (const GroupAsTNVs& grp : groups) {
    // Each group starts with 1-byte group-tag ...
    length += 1;
    // ... and consists of list of tag-name-value.
    for (const auto& tnv : grp.content)
      // Tag + name_size + name + value_size + value.
      length += (1 + 2 + tnv.name.size() + 2 + tnv.value.size());
  }
  // end-of-attributes-tag + blob_with_data.
  length += 1 + frame.Data().size();
  return length;
}

}  // namespace

size_t CalculateLengthOfBinaryFrame(const Frame& frame) {
  std::vector<GroupAsTNVs> tnvs = PreprocessFrame(frame);
  return GetFrameLength(frame, tnvs);
}

size_t BuildBinaryFrame(const Frame& frame,
                        uint8_t* buffer,
                        size_t buffer_length) {
  std::vector<GroupAsTNVs> tnvs = PreprocessFrame(frame);
  const size_t length = GetFrameLength(frame, tnvs);
  if (length > buffer_length) {
    return 0;
  }
  WriteFrameToBuffer(frame, tnvs, buffer);
  return length;
}

std::vector<uint8_t> BuildBinaryFrame(const Frame& frame) {
  std::vector<GroupAsTNVs> tnvs = PreprocessFrame(frame);
  std::vector<uint8_t> buffer(GetFrameLength(frame, tnvs));
  WriteFrameToBuffer(frame, tnvs, buffer.data());
  return buffer;
}

}  // namespace ipp
