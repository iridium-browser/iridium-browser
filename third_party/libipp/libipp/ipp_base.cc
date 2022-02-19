// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_base.h"

#include "libipp/ipp_frame.h"
#include "libipp/ipp_frame_builder.h"
#include "libipp/ipp_parser.h"

namespace ipp {

namespace {

// Sets attributes-charset and attributes-natural-language.
void SetDefaultPackageAttributes(Package* package) {
  Group* oper_grp = package->GetGroup(GroupTag::operation_attributes);
  Collection* coll_grp = nullptr;
  if (oper_grp != nullptr &&
      (coll_grp = oper_grp->GetCollection()) != nullptr) {
    Attribute* attr_charset =
        coll_grp->GetAttribute(AttrName::attributes_charset);
    Attribute* attr_language =
        coll_grp->GetAttribute(AttrName::attributes_natural_language);
    if (attr_charset != nullptr && attr_charset->GetState() == AttrState::unset)
      attr_charset->SetValue(std::string("utf-8"));
    if (attr_language != nullptr &&
        attr_language->GetState() == AttrState::unset)
      attr_language->SetValue(std::string("en-us"));
  }
}

// Sets status-message basing on status-code.
void SetDefaultResponseAttributes(Response* package) {
  Group* oper_grp = package->GetGroup(GroupTag::operation_attributes);
  Collection* coll_grp = nullptr;
  if (oper_grp != nullptr &&
      (coll_grp = oper_grp->GetCollection()) != nullptr) {
    Attribute* attr = coll_grp->GetAttribute(AttrName::status_message);
    if (attr != nullptr && attr->GetState() == AttrState::unset)
      attr->SetValue(ipp::ToString(package->StatusCode()));
  }
}

// Clear all values in the package.
void ClearPackage(Package* package) {
  const std::vector<Group*> groups = package->GetAllGroups();
  for (auto g : groups) {
    if (g->IsASet()) {
      g->Resize(0);
    } else {
      g->GetCollection()->ResetAllAttributes();
    }
  }
}

Version GetVersion(const Frame* frame) {
  uint16_t v = frame->major_version_number_;
  v <<= 8;
  v += frame->minor_version_number_;
  return static_cast<Version>(v);
}

void SetVersion(Version version, Frame* frame) {
  uint16_t v = static_cast<uint16_t>(version);
  frame->major_version_number_ = (v >> 8);
  frame->minor_version_number_ = (v & 0xff);
}

}  // namespace

// internal structure with all data
struct Protocol {
  // internal buffer for current frame
  Frame frame;
  // log with errors
  std::vector<Log> log;
  // parser and frame builder, both work on the frame and the log defined above
  Parser parser;
  FrameBuilder frame_builder;
  // constructor
  Protocol() : parser(&frame, &log), frame_builder(&frame, &log) {}
  // resets everything to initial state
  void ResetContent() {
    frame.operation_id_or_status_code_ = 0;
    frame.groups_tags_.clear();
    frame.groups_content_.clear();
    frame.data_.clear();
    log.clear();
    parser.ResetContent();
  }
};

Client::Client(Version version, int32_t request_id)
    : protocol_(std::make_unique<Protocol>()) {
  SetVersionNumber(version);
  protocol_->frame.request_id_ = request_id;
}

// Destructor must be defined here, because we do not have definition of
// Protocol in the header.
Client::~Client() = default;

Version Client::GetVersionNumber() const {
  return GetVersion(&protocol_->frame);
}

void Client::SetVersionNumber(Version version) {
  SetVersion(version, &protocol_->frame);
}

void Client::BuildRequestFrom(Request* request) {
  protocol_->ResetContent();
  SetDefaultPackageAttributes(request);
  ++(protocol_->frame.request_id_);
  protocol_->frame.operation_id_or_status_code_ =
      static_cast<uint16_t>(request->GetOperationId());
  protocol_->frame_builder.BuildFrameFromPackage(request);
}

bool Client::WriteRequestFrameTo(std::vector<uint8_t>* data) const {
  if (data == nullptr)
    return false;
  data->resize(GetFrameLength());
  return protocol_->frame_builder.WriteFrameToBuffer(data->data());
}

std::size_t Client::GetFrameLength() const {
  return protocol_->frame_builder.GetFrameLength();
}

bool Client::ReadResponseFrameFrom(const uint8_t* ptr,
                                   const uint8_t* const buf_end) {
  protocol_->ResetContent();
  return protocol_->parser.ReadFrameFromBuffer(ptr, buf_end);
}

bool Client::ReadResponseFrameFrom(const std::vector<uint8_t>& data) {
  protocol_->ResetContent();
  const uint8_t* ptr = (data.empty()) ? (nullptr) : (&(data[0]));
  return protocol_->parser.ReadFrameFromBuffer(ptr, ptr + data.size());
}

bool Client::ParseResponseAndSaveTo(Response* response,
                                    bool log_unknown_values) {
  ClearPackage(response);
  response->StatusCode() =
      static_cast<Status>(protocol_->frame.operation_id_or_status_code_);
  return protocol_->parser.SaveFrameToPackage(log_unknown_values, response);
}

const std::vector<Log>& Client::GetErrorLog() const {
  return protocol_->log;
}

Server::Server(Version version, int32_t request_id)
    : protocol_(std::make_unique<Protocol>()) {
  SetVersionNumber(version);
  protocol_->frame.request_id_ = request_id;
}

// Destructor must be defined here, because we do not have definition of
// Protocol in the header.
Server::~Server() = default;

Version Server::GetVersionNumber() const {
  return GetVersion(&protocol_->frame);
}

void Server::SetVersionNumber(Version version) {
  SetVersion(version, &protocol_->frame);
}

bool Server::ReadRequestFrameFrom(const uint8_t* ptr,
                                  const uint8_t* const buf_end) {
  protocol_->ResetContent();
  return protocol_->parser.ReadFrameFromBuffer(ptr, buf_end);
}

bool Server::ReadRequestFrameFrom(const std::vector<uint8_t>& data) {
  protocol_->ResetContent();
  return protocol_->parser.ReadFrameFromBuffer(data.data(),
                                               data.data() + data.size());
}

Operation Server::GetOperationId() const {
  return static_cast<Operation>(protocol_->frame.operation_id_or_status_code_);
}

bool Server::ParseRequestAndSaveTo(Request* request, bool log_unknown_values) {
  ClearPackage(request);
  return protocol_->parser.SaveFrameToPackage(log_unknown_values, request);
}

void Server::BuildResponseFrom(Response* package) {
  protocol_->ResetContent();
  SetDefaultPackageAttributes(package);
  SetDefaultResponseAttributes(package);
  protocol_->frame.operation_id_or_status_code_ =
      static_cast<uint16_t>(package->StatusCode());
  protocol_->frame_builder.BuildFrameFromPackage(package);
}

std::size_t Server::GetFrameLength() const {
  return protocol_->frame_builder.GetFrameLength();
}

bool Server::WriteResponseFrameTo(std::vector<uint8_t>* data) const {
  if (data == nullptr)
    return false;
  data->resize(GetFrameLength());
  return protocol_->frame_builder.WriteFrameToBuffer(data->data());
}

const std::vector<Log>& Server::GetErrorLog() const {
  return protocol_->log;
}

}  // namespace ipp
