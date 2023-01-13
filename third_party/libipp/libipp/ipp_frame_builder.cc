// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_frame_builder.h"

#include <algorithm>

#include "libipp/ipp_attribute.h"
#include "libipp/ipp_encoding.h"

namespace ipp {

namespace {

// Saves 1-,2- or 4-bytes integer to buf using two's-complement binary encoding.
// The "buf" parameter is always resized to BytesCount. Returns false when
// given integer is out of range. In this case 0 is saved to buf.
template <size_t BytesCount>
bool SaveInteger(int v, std::vector<uint8_t>* buf) {
  buf->resize(BytesCount);
  uint8_t* ptr = buf->data();
  if (!WriteInteger<BytesCount>(&ptr, v)) {
    buf->clear();
    buf->resize(BytesCount, 0);
    return false;
  }
  return true;
}

// Saves simple string to buf, buf is resized accordingly.
void SaveOctetString(const std::string& s, std::vector<uint8_t>* buf) {
  buf->assign(s.begin(), s.end());
}

// Writes textWithLanguage/nameWithLanguage (see [rfc8010]) to buf, which is
// resized accordingly. Returns false if given string(s) is too long. In this
// case an empty string is saved to the buffer.
bool SaveStringWithLanguage(const ipp::StringWithLanguage& s,
                            std::vector<uint8_t>* buf) {
  buf->clear();
  buf->resize(4 + s.value.size() + s.language.size());
  uint8_t* ptr = buf->data();
  if (!WriteInteger<2>(&ptr, s.language.size())) {
    std::vector<uint8_t> empty_buffer(4, 0);
    buf->swap(empty_buffer);
    return false;
  }
  ptr = std::copy(s.language.begin(), s.language.end(), ptr);
  if (!WriteInteger<2>(&ptr, s.value.size())) {
    std::vector<uint8_t> empty_buffer(4, 0);
    buf->swap(empty_buffer);
    return false;
  }
  std::copy(s.value.begin(), s.value.end(), ptr);
  return true;
}

// Saves dateTime (see [rfc8010]) to buf, which is resized accordingly. Returns
// false when the given dateTime is incorrect; in this case |buf| is set to
// 2000/1/1 00:00:00 +00:00.
bool SaveDateTime(const ipp::DateTime& v, std::vector<uint8_t>* buf) {
  buf->resize(11);
  uint8_t* ptr = buf->data();
  if (WriteInteger<2>(&ptr, v.year) && WriteInteger<1>(&ptr, v.month) &&
      WriteInteger<1>(&ptr, v.day) && WriteInteger<1>(&ptr, v.hour) &&
      WriteInteger<1>(&ptr, v.minutes) && WriteInteger<1>(&ptr, v.seconds) &&
      WriteInteger<1>(&ptr, v.deci_seconds) &&
      WriteInteger<1>(&ptr, v.UTC_direction) &&
      WriteInteger<1>(&ptr, v.UTC_hours) &&
      WriteInteger<1>(&ptr, v.UTC_minutes))
    return true;
  buf->clear();
  buf->resize(11, 0);
  ptr = buf->data();
  WriteInteger<int16_t>(&ptr, 2000);
  (*buf)[8] = '+';
  return false;
}

// Writes resolution (see [rfc8010]) to buf, which is resized accordingly.
// Returns false when given value is incorrect; in this case |buf| is set to
// 256x256dpi.
bool SaveResolution(const ipp::Resolution& v, std::vector<uint8_t>* buf) {
  buf->resize(9);
  uint8_t* ptr = buf->data();
  if (WriteInteger<4>(&ptr, v.size1) && WriteInteger<4>(&ptr, v.size2) &&
      (v.units == ipp::Resolution::kDotsPerCentimeter ||
       v.units == ipp::Resolution::kDotsPerInch)) {
    WriteInteger<int8_t>(&ptr, v.units);
    return true;
  }
  buf->clear();
  // set to 256x256 dpi
  buf->resize(9, 0);
  (*buf)[2] = (*buf)[6] = 1;
  (*buf)[8] = ipp::Resolution::kDotsPerInch;
  return false;
}

// Writes rangeOfInteger (see [rfc8010]) to buf, which is resized accordingly.
// Returns false when given value is incorrect; in this case |buf| is set to 0.
bool SaveRangeOfInteger(const ipp::RangeOfInteger& v,
                        std::vector<uint8_t>* buf) {
  buf->resize(8);
  uint8_t* ptr = buf->data();
  if (WriteInteger<4>(&ptr, v.min_value) && WriteInteger<4>(&ptr, v.max_value))
    return true;
  buf->clear();
  buf->resize(8, 0);
  return false;
}

}  //  namespace

void FrameBuilder::LogFrameBuilderError(const std::string& message) {
  Log l;
  l.message = "Error when building frame: " + message + ".";
  errors_->push_back(l);
}

void FrameBuilder::SaveAttrValue(Attribute* attr,
                                 size_t index,
                                 uint8_t* tag,
                                 std::vector<uint8_t>* buf) {
  *tag = static_cast<uint8_t>(attr->GetType());
  bool result = true;
  switch (attr->GetType()) {
    case AttrType::boolean: {
      int v = 0;
      attr->GetValue(&v, index);
      if (v != 0)
        v = 1;
      result = SaveInteger<1>(v, buf);
      break;
    }
    case AttrType::integer:
    case AttrType::enum_: {
      int v = 0;
      attr->GetValue(&v, index);
      result = SaveInteger<4>(v, buf);
      break;
    }
    case AttrType::dateTime: {
      DateTime v;
      attr->GetValue(&v, index);
      result = SaveDateTime(v, buf);
      break;
    }
    case AttrType::resolution: {
      Resolution v;
      attr->GetValue(&v, index);
      result = SaveResolution(v, buf);
      break;
    }
    case AttrType::rangeOfInteger: {
      RangeOfInteger v;
      attr->GetValue(&v, index);
      result = SaveRangeOfInteger(v, buf);
      break;
    }
    case AttrType::text: {
      StringWithLanguage s;
      attr->GetValue(&s, index);
      if (s.language.empty()) {
        *tag = textWithoutLanguage_value_tag;
        SaveOctetString(s, buf);
      } else {
        result = SaveStringWithLanguage(s, buf);
      }
    } break;
    case AttrType::name: {
      StringWithLanguage s;
      attr->GetValue(&s, index);
      if (s.language.empty()) {
        *tag = nameWithoutLanguage_value_tag;
        SaveOctetString(s, buf);
      } else {
        result = SaveStringWithLanguage(s, buf);
      }
    } break;
    case AttrType::octetString:
    case AttrType::keyword:
    case AttrType::uri:
    case AttrType::uriScheme:
    case AttrType::charset:
    case AttrType::naturalLanguage:
    case AttrType::mimeMediaType: {
      std::string s = "";
      attr->GetValue(&s, index);
      SaveOctetString(s, buf);
      break;
    }
    default:
      LogFrameBuilderError(
          "Internal error: cannot recognize value type of the attribute " +
          attr->GetName());
      buf->clear();
      break;
  }
  if (!result)
    LogFrameBuilderError("Incorrect value of the attribute " + attr->GetName() +
                         ". Default value was written instead");
}

void FrameBuilder::SaveCollection(Collection* coll,
                                  std::list<TagNameValue>* data_chunks) {
  // get list of all attributes
  std::vector<Attribute*> attrs = coll->GetAllAttributes();
  // save the attributes
  for (Attribute* attr : attrs) {
    if (attr->GetState() == AttrState::unset)
      continue;
    TagNameValue tnv;
    tnv.tag = memberAttrName_value_tag;
    tnv.name.clear();
    SaveOctetString(attr->GetName(), &tnv.value);
    data_chunks->push_back(tnv);
    for (size_t val_index = 0; val_index < attr->GetSize(); ++val_index) {
      if (attr->GetState() != AttrState::set) {
        tnv.tag = static_cast<uint8_t>(attr->GetState());
        tnv.value.clear();
      } else if (attr->GetType() == AttrType::collection) {
        tnv.tag = begCollection_value_tag;
        tnv.value.clear();
        data_chunks->push_back(tnv);
        SaveCollection(attr->GetCollection(val_index), data_chunks);
        tnv.tag = endCollection_value_tag;
        tnv.value.clear();
      } else {
        SaveAttrValue(attr, val_index, &tnv.tag, &tnv.value);
      }
      data_chunks->push_back(tnv);
    }
  }
}

void FrameBuilder::SaveGroup(Collection* coll,
                             std::list<TagNameValue>* data_chunks) {
  // get list of all attributes
  std::vector<Attribute*> attrs = coll->GetAllAttributes();
  // save the attributes
  for (Attribute* attr : attrs) {
    if (attr->GetState() == AttrState::unset)
      continue;
    TagNameValue tnv;
    SaveOctetString(attr->GetName(), &tnv.name);
    if (attr->GetState() != AttrState::set) {
      tnv.tag = static_cast<uint8_t>(attr->GetState());
      tnv.value.clear();
      data_chunks->push_back(tnv);
      continue;
    }
    for (size_t val_index = 0; val_index < attr->GetSize(); ++val_index) {
      if (attr->GetType() == AttrType::collection) {
        tnv.tag = begCollection_value_tag;
        tnv.value.clear();
        data_chunks->push_back(tnv);
        SaveCollection(attr->GetCollection(val_index), data_chunks);
        tnv.tag = endCollection_value_tag;
        tnv.name.clear();
        tnv.value.clear();
      } else {
        SaveAttrValue(attr, val_index, &tnv.tag, &tnv.value);
      }
      data_chunks->push_back(tnv);
      tnv.name.clear();
    }
  }
}

void FrameBuilder::BuildFrameFromPackage(Package* package) {
  frame_->groups_tags_.clear();
  frame_->groups_content_.clear();
  // save frame data
  std::vector<Group*> groups = package->GetAllGroups();
  for (Group* grp : groups) {
    for (size_t i = 0; true; ++i) {
      Collection* coll = grp->GetCollection(i);
      if (coll == nullptr)
        break;
      frame_->groups_tags_.push_back(static_cast<uint8_t>(grp->GetName()));
      frame_->groups_content_.resize(frame_->groups_tags_.size());
      SaveGroup(coll, &(frame_->groups_content_.back()));
      // skip if it this is a single empty group
      if (frame_->groups_content_.back().empty() && !grp->IsASet()) {
        frame_->groups_tags_.pop_back();
        frame_->groups_content_.pop_back();
      }
    }
  }
  frame_->data_ = package->Data();
}

bool FrameBuilder::WriteFrameToBuffer(uint8_t* ptr) {
  bool error_in_header = true;
  if (!WriteInteger<1>(&ptr, frame_->major_version_number_)) {
    LogFrameBuilderError("major-version-number is out of range");
  } else if (!WriteInteger<1>(&ptr, frame_->minor_version_number_)) {
    LogFrameBuilderError("minor-version-number is out of range");
  } else if (!WriteInteger<2>(&ptr, frame_->operation_id_or_status_code_)) {
    LogFrameBuilderError("operation-id or status-code is out of range");
  } else if (!WriteInteger<4>(&ptr, frame_->request_id_)) {
    LogFrameBuilderError("request-id is out of range");
  } else {
    error_in_header = false;
  }
  if (error_in_header)
    return false;
  for (size_t i = 0; i < frame_->groups_tags_.size(); ++i) {
    if (frame_->groups_tags_[i] > max_begin_attribute_group_tag) {
      LogFrameBuilderError("begin-attribute-group-tag is out of range");
      return false;
    }
    // write a group to the buffer
    if (!WriteInteger<1>(&ptr, frame_->groups_tags_[i]) ||
        !WriteTNVsToBuffer(frame_->groups_content_[i], &ptr))
      return false;
  }
  WriteInteger<int8_t>(&ptr, end_of_attributes_tag);
  std::copy(frame_->data_.begin(), frame_->data_.end(), ptr);
  return true;
}

bool FrameBuilder::WriteTNVsToBuffer(const std::list<TagNameValue>& tnvs,
                                     uint8_t** ptr) {
  for (auto& tnv : tnvs) {
    if (!WriteInteger<1>(ptr, tnv.tag)) {
      LogFrameBuilderError("value-tag is out of range");
      return false;
    }
    if (!WriteInteger<2>(ptr, tnv.name.size())) {
      LogFrameBuilderError("name-length is out of range");
      return false;
    }
    *ptr = std::copy(tnv.name.begin(), tnv.name.end(), *ptr);
    if (!WriteInteger<2>(ptr, tnv.value.size())) {
      LogFrameBuilderError("value-length is out of range");
      return false;
    }
    *ptr = std::copy(tnv.value.begin(), tnv.value.end(), *ptr);
  }
  return true;
}

std::size_t FrameBuilder::GetFrameLength() const {
  // Header has always 8 bytes (ipp_version + operation_id/status + request_id).
  size_t length = 8;
  // The header is followed by a list of groups.
  for (const auto& tnvs : frame_->groups_content_) {
    // Each group starts with 1-byte group-tag ...
    length += 1;
    // ... and consists of list of tag-name-value.
    for (const auto& tnv : tnvs)
      // Tag + name_size + name + value_size + value.
      length += (1 + 2 + tnv.name.size() + 2 + tnv.value.size());
  }
  // end-of-attributes-tag + blob_with_data.
  length += 1 + frame_->data_.size();
  return length;
}

}  // namespace ipp
