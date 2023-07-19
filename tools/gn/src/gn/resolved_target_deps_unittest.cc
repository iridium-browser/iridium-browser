// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/resolved_target_deps.h"
#include "gn/test_with_scope.h"
#include "util/test/test.h"

TEST(ResolvedTargetDeps, DefaultConstruction) {
  ResolvedTargetDeps deps;
  EXPECT_EQ(0u, deps.size());
  EXPECT_TRUE(deps.public_deps().empty());
  EXPECT_TRUE(deps.private_deps().empty());
  EXPECT_TRUE(deps.data_deps().empty());
  EXPECT_TRUE(deps.linked_deps().empty());
  EXPECT_TRUE(deps.all_deps().empty());
}

TEST(ResolvedTargetDeps, Construction) {
  TestWithScope setup;
  TestTarget a(setup, "//foo:a", Target::STATIC_LIBRARY);
  TestTarget b(setup, "//foo:b", Target::SOURCE_SET);
  TestTarget c(setup, "//foo:c", Target::SOURCE_SET);
  TestTarget d(setup, "//foo:d", Target::SOURCE_SET);
  TestTarget e(setup, "//foo:e", Target::EXECUTABLE);

  LabelTargetVector public_vec;
  LabelTargetVector private_vec;
  LabelTargetVector data_vec;

  public_vec.emplace_back(&a);
  public_vec.emplace_back(&b);
  private_vec.emplace_back(&c);
  private_vec.emplace_back(&d);
  data_vec.emplace_back(&e);

  ResolvedTargetDeps deps(public_vec, private_vec, data_vec);
  EXPECT_EQ(5u, deps.size());

  EXPECT_EQ(2u, deps.public_deps().size());
  EXPECT_EQ(&a, deps.public_deps()[0]);
  EXPECT_EQ(&b, deps.public_deps()[1]);

  EXPECT_EQ(2u, deps.private_deps().size());
  EXPECT_EQ(&c, deps.private_deps()[0]);
  EXPECT_EQ(&d, deps.private_deps()[1]);

  EXPECT_EQ(1u, deps.data_deps().size());
  EXPECT_EQ(&e, deps.data_deps()[0]);

  EXPECT_EQ(4u, deps.linked_deps().size());
  EXPECT_EQ(&a, deps.linked_deps()[0]);
  EXPECT_EQ(&b, deps.linked_deps()[1]);
  EXPECT_EQ(&c, deps.linked_deps()[2]);
  EXPECT_EQ(&d, deps.linked_deps()[3]);

  EXPECT_EQ(5u, deps.all_deps().size());
  EXPECT_EQ(&a, deps.all_deps()[0]);
  EXPECT_EQ(&b, deps.all_deps()[1]);
  EXPECT_EQ(&c, deps.all_deps()[2]);
  EXPECT_EQ(&d, deps.all_deps()[3]);
  EXPECT_EQ(&e, deps.all_deps()[4]);
}
