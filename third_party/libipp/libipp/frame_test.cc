// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "frame.h"

#include <cstdint>
#include <string>
#include <vector>

#include "attribute.h"
#include <gtest/gtest.h>

namespace ipp {
namespace {

TEST(Frame, Constructor1) {
  Frame frame;
  EXPECT_EQ(frame.OperationIdOrStatusCode(), 0);
  EXPECT_EQ(frame.RequestId(), 0);
  EXPECT_EQ(static_cast<int>(frame.VersionNumber()), 0);
  EXPECT_TRUE(frame.Data().empty());
  for (GroupTag gt : kGroupTags) {
    EXPECT_TRUE(frame.Groups(gt).empty());
  }
}

TEST(Frame, Constructor2) {
  const Frame frame(Operation::Activate_Printer, Version::_2_1, 123);
  EXPECT_EQ(frame.OperationId(), Operation::Activate_Printer);
  EXPECT_EQ(frame.RequestId(), 123);
  EXPECT_EQ(frame.VersionNumber(), Version::_2_1);
  EXPECT_TRUE(frame.Data().empty());
  for (GroupTag gt : kGroupTags) {
    auto groups = frame.Groups(gt);
    if (gt == GroupTag::operation_attributes) {
      ASSERT_EQ(groups.size(), 1);
      auto att = groups[0].GetAttr("attributes-charset");
      ASSERT_NE(att, groups[0].end());
      std::string value;
      EXPECT_EQ(att->GetValue(0, value), Code::kOK);
      EXPECT_EQ(value, "utf-8");
      att = groups[0].GetAttr("attributes-natural-language");
      ASSERT_NE(att, groups[0].end());
      EXPECT_EQ(att->GetValue(0, value), Code::kOK);
      EXPECT_EQ(value, "en-us");
    } else {
      EXPECT_TRUE(groups.empty());
    }
  }
}

TEST(Frame, Constructor2empty) {
  const Frame frame(Operation::Activate_Printer, Version::_2_1, 123, false);
  EXPECT_EQ(frame.OperationId(), Operation::Activate_Printer);
  EXPECT_EQ(frame.RequestId(), 123);
  EXPECT_EQ(frame.VersionNumber(), Version::_2_1);
  EXPECT_TRUE(frame.Data().empty());
  for (GroupTag gt : kGroupTags) {
    EXPECT_TRUE(frame.Groups(gt).empty());
  }
}

TEST(Frame, Constructor3) {
  Frame frame(Status::client_error_gone, Version::_1_0, 123);
  EXPECT_EQ(frame.StatusCode(), Status::client_error_gone);
  EXPECT_EQ(frame.RequestId(), 123);
  EXPECT_EQ(frame.VersionNumber(), Version::_1_0);
  EXPECT_TRUE(frame.Data().empty());
  for (GroupTag gt : kGroupTags) {
    auto groups = frame.Groups(gt);
    if (gt == GroupTag::operation_attributes) {
      ASSERT_EQ(groups.size(), 1);
      auto att = groups[0].GetAttr("attributes-charset");
      ASSERT_NE(att, groups[0].end());
      std::string value;
      EXPECT_EQ(att->GetValue(0, value), Code::kOK);
      EXPECT_EQ(value, "utf-8");
      att = groups[0].GetAttr("attributes-natural-language");
      ASSERT_NE(att, groups[0].end());
      EXPECT_EQ(att->GetValue(0, value), Code::kOK);
      EXPECT_EQ(value, "en-us");
      att = groups[0].GetAttr("status-message");
      ASSERT_NE(att, groups[0].end());
      EXPECT_EQ(att->GetValue(0, value), Code::kOK);
      EXPECT_EQ(value, "client-error-gone");
    } else {
      EXPECT_TRUE(groups.empty());
    }
  }
}

TEST(Frame, Constructor3empty) {
  const Frame frame(Status::client_error_gone, Version::_2_1, 123, false);
  EXPECT_EQ(frame.StatusCode(), Status::client_error_gone);
  EXPECT_EQ(frame.RequestId(), 123);
  EXPECT_EQ(frame.VersionNumber(), Version::_2_1);
  EXPECT_TRUE(frame.Data().empty());
  for (GroupTag gt : kGroupTags) {
    EXPECT_TRUE(frame.Groups(gt).empty());
  }
}

TEST(Frame, Data) {
  Frame frame;
  std::vector<uint8_t> raw = {0x01, 0x02, 0x03, 0x04};
  EXPECT_EQ(frame.SetData(std::move(raw)), Code::kOK);
  EXPECT_EQ(frame.Data(), std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04}));
  auto raw2 = frame.TakeData();
  EXPECT_EQ(raw2, std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04}));
  EXPECT_TRUE(frame.Data().empty());
}

TEST(Frame, Groups) {
  Frame frame(Operation::Cancel_Job);
  EXPECT_EQ(frame.Groups(GroupTag::operation_attributes).size(), 1);
  EXPECT_EQ(frame.Groups(static_cast<GroupTag>(0x00)).size(), 0);
  EXPECT_EQ(frame.Groups(static_cast<GroupTag>(0x0f)).size(), 0);
  EXPECT_EQ(frame.Groups(static_cast<GroupTag>(123)).size(), 0);
  EXPECT_EQ(frame.Groups(GroupTag::job_attributes).begin(),
            frame.Groups(GroupTag::job_attributes).end());
  EXPECT_EQ(frame.Groups(static_cast<GroupTag>(3)).begin(),
            frame.Groups(static_cast<GroupTag>(3)).end());
}

TEST(Frame, GroupsConst) {
  const Frame frame(Operation::Cancel_Job);
  EXPECT_EQ(frame.Groups(GroupTag::operation_attributes).size(), 1);
  EXPECT_EQ(frame.Groups(static_cast<GroupTag>(0x00)).size(), 0);
  EXPECT_EQ(frame.Groups(static_cast<GroupTag>(0x0f)).size(), 0);
  EXPECT_EQ(frame.Groups(static_cast<GroupTag>(123)).size(), 0);
  EXPECT_EQ(frame.Groups(GroupTag::job_attributes).begin(),
            frame.Groups(GroupTag::job_attributes).end());
  EXPECT_EQ(frame.Groups(static_cast<GroupTag>(3)).begin(),
            frame.Groups(static_cast<GroupTag>(3)).end());
}

TEST(Frame, AddGroup) {
  Frame frame(Operation::Cancel_Job, Version::_2_0);
  CollsView::iterator grp1;
  CollsView::iterator grp2;
  EXPECT_EQ(frame.AddGroup(GroupTag::document_attributes, grp1), Code::kOK);
  ASSERT_NE(grp1, frame.Groups(GroupTag::document_attributes).end());
  EXPECT_EQ(grp1, frame.Groups(GroupTag::document_attributes).begin());
  EXPECT_EQ(frame.AddGroup(GroupTag::job_attributes, grp2), Code::kOK);
  EXPECT_EQ(frame.AddGroup(GroupTag::document_attributes, grp1), Code::kOK);
  ASSERT_NE(grp2, frame.Groups(GroupTag::job_attributes).end());
  ASSERT_NE(grp1, frame.Groups(GroupTag::document_attributes).end());
  EXPECT_EQ(&*grp1, &frame.Groups(GroupTag::document_attributes)[1]);
  EXPECT_EQ(&*grp2, &frame.Groups(GroupTag::job_attributes)[0]);
  EXPECT_EQ(frame.Groups(GroupTag::document_attributes).size(), 2);
  EXPECT_EQ(frame.Groups(GroupTag::job_attributes).size(), 1);
}

TEST(Frame, AddGroupErrorCodes) {
  CollsView::iterator grp;
  Frame frame(Operation::Cancel_Job, Version::_2_0);
  EXPECT_EQ(frame.AddGroup(GroupTag::document_attributes, grp), Code::kOK);
  ASSERT_NE(grp, frame.Groups(GroupTag::document_attributes).end());
  EXPECT_EQ(grp, frame.Groups(GroupTag::document_attributes).begin());
  EXPECT_EQ(frame.AddGroup(static_cast<GroupTag>(0x10), grp),
            Code::kInvalidGroupTag);
  // `grp` was not modified.
  EXPECT_EQ(grp, frame.Groups(GroupTag::document_attributes).begin());
}

}  // namespace
}  // namespace ipp
