// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "frame.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "builder.h"
#include "ipp_frame.h"
#include "ipp_parser.h"
#include "parser.h"

namespace ipp {

constexpr size_t kMaxPayloadSize = 256 * 1024 * 1024;

namespace {

void SetCharsetAndLanguageAttributes(Frame* frame) {
  CollsView::iterator grp;
  frame->AddGroup(ipp::GroupTag::operation_attributes, grp);
  grp->AddAttr("attributes-charset", ValueTag::charset, "utf-8");
  grp->AddAttr("attributes-natural-language", ValueTag::naturalLanguage,
               "en-us");
}

}  // namespace

Frame::Frame()
    : version_(static_cast<ipp::Version>(0)),
      operation_id_or_status_code_(0),
      request_id_(0) {}

Frame::Frame(Operation operation_id,
             Version version_number,
             int32_t request_id,
             bool set_localization_en_us)
    : version_(version_number),
      operation_id_or_status_code_(static_cast<uint16_t>(operation_id)),
      request_id_(request_id) {
  if (set_localization_en_us) {
    SetCharsetAndLanguageAttributes(this);
  }
}

Frame::Frame(Status status_code,
             Version version_number,
             int32_t request_id,
             bool set_localization_en_us_and_status_message)
    : version_(version_number),
      operation_id_or_status_code_(static_cast<uint16_t>(status_code)),
      request_id_(request_id) {
  if (set_localization_en_us_and_status_message) {
    SetCharsetAndLanguageAttributes(this);
    CollsView::iterator grp = Groups(GroupTag::operation_attributes).begin();
    grp->AddAttr("status-message", ValueTag::textWithoutLanguage,
                 ToString(status_code));
  }
}

Frame::~Frame() {
  for (std::pair<GroupTag, Collection*>& group : groups_) {
    delete group.second;
  }
}

Version Frame::VersionNumber() const {
  return version_;
}

Version& Frame::VersionNumber() {
  return version_;
}

int16_t Frame::OperationIdOrStatusCode() const {
  return operation_id_or_status_code_;
}

int16_t& Frame::OperationIdOrStatusCode() {
  return operation_id_or_status_code_;
}

Operation Frame::OperationId() const {
  return static_cast<Operation>(operation_id_or_status_code_);
}

Status Frame::StatusCode() const {
  return static_cast<Status>(operation_id_or_status_code_);
}

int32_t& Frame::RequestId() {
  return request_id_;
}

int32_t Frame::RequestId() const {
  return request_id_;
}

const std::vector<uint8_t>& Frame::Data() const {
  return data_;
}

std::vector<uint8_t> Frame::TakeData() {
  std::vector<uint8_t> data;
  data.swap(data_);
  return data;
}

Code Frame::SetData(std::vector<uint8_t>&& data) {
  if (data.size() > kMaxPayloadSize) {
    return Code::kDataTooLong;
  }
  data_ = std::move(data);
  return Code::kOK;
}

CollsView Frame::Groups(GroupTag tag) {
  if (IsValid(tag)) {
    return CollsView(groups_by_tag_[static_cast<size_t>(tag)]);
  }
  return CollsView();
}

ConstCollsView Frame::Groups(GroupTag tag) const {
  if (IsValid(tag)) {
    return ConstCollsView(groups_by_tag_[static_cast<size_t>(tag)]);
  }
  return ConstCollsView();
}

std::vector<std::pair<GroupTag, Collection*>> Frame::GetGroups() {
  return groups_;
}

std::vector<std::pair<GroupTag, const Collection*>> Frame::GetGroups() const {
  return std::vector<std::pair<GroupTag, const Collection*>>(groups_.begin(),
                                                             groups_.end());
}

Code Frame::AddGroup(GroupTag tag, CollsView::iterator& new_group) {
  if (!IsValid(tag)) {
    return Code::kInvalidGroupTag;
  }
  if (groups_.size() >= kMaxCountOfAttributeGroups) {
    return Code::kTooManyGroups;
  }
  Collection* coll = new Collection;
  groups_.emplace_back(tag, coll);
  std::vector<ipp::Collection*>& vg = groups_by_tag_[static_cast<size_t>(tag)];
  new_group = CollsView::iterator(vg.insert(vg.end(), coll));
  return Code::kOK;
}

}  // namespace ipp
