// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_package.h"

#include <gtest/gtest.h>

#include "libipp/ipp_attribute.h"
#include "libipp/ipp_enums.h"

namespace ipp {

namespace {

struct TestSubcollection : public Collection {
  SingleValue<int> attr{AttrName::auth_info, AttrType::integer};
  std::vector<Attribute*> GetKnownAttributes() override { return {&attr}; }
};

struct TestCollection : public Collection {
  SingleValue<int> attr_single_val{AttrName::job_id, AttrType::integer};
  SetOfValues<int> attr_set_of_val{AttrName::job_name, AttrType::integer};
  SingleCollection<TestSubcollection> attr_single_coll{AttrName::printer_info};
  SetOfCollections<TestSubcollection> attr_set_of_coll{
      AttrName::printer_supply};
  std::vector<Attribute*> GetKnownAttributes() override {
    return {&attr_single_val, &attr_set_of_val, &attr_single_coll,
            &attr_set_of_coll};
  }
};

struct TestPackage : public Package {
  SingleGroup<TestCollection> grp_single{GroupTag::operation_attributes};
  SetOfGroups<TestCollection> grp_set{GroupTag::printer_attributes};
  std::vector<Group*> GetKnownGroups() override {
    return {&grp_single, &grp_set};
  }
};

TEST(package, Collection) {
  TestCollection coll;
  // add attribute - name exists
  EXPECT_EQ(coll.AddUnknownAttribute("job-name", true, AttrType::integer),
            nullptr);
  EXPECT_EQ(coll.AddUnknownAttribute("job-name", false, AttrType::collection),
            nullptr);
  // add attribute - incorrect name
  EXPECT_EQ(coll.AddUnknownAttribute("", true, AttrType::boolean), nullptr);
  // add attribute
  Attribute* new_attr =
      coll.AddUnknownAttribute("other-name", true, AttrType::boolean);
  ASSERT_NE(new_attr, nullptr);
  EXPECT_EQ(new_attr->GetName(), "other-name");
  EXPECT_EQ(new_attr->GetType(), AttrType::boolean);
  // get known/all attributes
  std::vector<Attribute*> all = coll.GetAllAttributes();
  for (auto a : all)
    ASSERT_NE(a, nullptr);
  ASSERT_EQ(all.size(), 5);
  EXPECT_EQ(all[4], new_attr);
  std::vector<Attribute*> known = coll.GetKnownAttributes();
  all.pop_back();
  EXPECT_EQ(known, all);
  // get attribute by name
  EXPECT_EQ(coll.GetAttribute("printer-info"), &(coll.attr_single_coll));
  EXPECT_EQ(coll.GetAttribute(AttrName::job_name), &(coll.attr_set_of_val));
  EXPECT_EQ(coll.GetAttribute("other-name"), new_attr);
  EXPECT_EQ(coll.GetAttribute("adasad"), nullptr);
  // reset all attributes
  coll.attr_single_val.Set(1234);
  coll.attr_set_of_val.Set({1, 2, 123});
  coll.attr_single_coll.attr.Set(324);
  coll.attr_set_of_coll[2].attr.Set(44);
  coll.attr_set_of_coll[0].attr.SetState(AttrState::unsupported);
  new_attr->SetValue(true, 4);
  coll.ResetAllAttributes();
  EXPECT_EQ(coll.attr_single_val.GetState(), AttrState::unset);
  EXPECT_EQ(coll.attr_set_of_val.GetState(), AttrState::unset);
  EXPECT_EQ(coll.attr_single_coll.GetState(), AttrState::unset);
  EXPECT_EQ(coll.attr_single_coll.attr.GetState(), AttrState::unset);
  EXPECT_EQ(coll.attr_set_of_coll.GetState(), AttrState::unset);
  EXPECT_EQ(new_attr->GetState(), AttrState::unset);
  EXPECT_EQ(coll.attr_set_of_val.GetSize(), 0);
  EXPECT_EQ(coll.attr_set_of_coll.GetSize(), 0);
  EXPECT_EQ(new_attr->GetSize(), 0);
}

TEST(package, Package) {
  TestPackage pkg;
  // add new group - the same tag
  EXPECT_EQ(pkg.AddUnknownGroup(GroupTag::printer_attributes, false), nullptr);
  EXPECT_EQ(pkg.AddUnknownGroup(GroupTag::printer_attributes, true), nullptr);
  Group* new_grp = pkg.AddUnknownGroup(GroupTag::job_attributes, true);
  ASSERT_NE(new_grp, nullptr);
  EXPECT_EQ(new_grp->GetName(), GroupTag::job_attributes);
  EXPECT_EQ(new_grp->IsASet(), true);
  // get known/all groups
  std::vector<Group*> all = pkg.GetAllGroups();
  for (auto a : all)
    ASSERT_NE(a, nullptr);
  ASSERT_EQ(all.size(), 3);
  EXPECT_EQ(all.back(), new_grp);
  std::vector<Group*> known = pkg.GetKnownGroups();
  all.pop_back();
  EXPECT_EQ(known, all);
  // get group by name
  EXPECT_EQ(pkg.GetGroup(GroupTag::operation_attributes), &(pkg.grp_single));
  EXPECT_EQ(pkg.GetGroup(GroupTag::printer_attributes), &(pkg.grp_set));
  EXPECT_EQ(pkg.GetGroup(GroupTag::job_attributes), new_grp);
  EXPECT_EQ(pkg.GetGroup(GroupTag::subscription_attributes), nullptr);
}

}  // end of namespace

}  // end of namespace ipp
