// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/ninja_generated_file_target_writer.h"

#include "gn/source_file.h"
#include "gn/target.h"
#include "gn/test_with_scheduler.h"
#include "gn/test_with_scope.h"
#include "util/test/test.h"

using NinjaGeneratedFileTargetWriterTest = TestWithScheduler;

TEST_F(NinjaGeneratedFileTargetWriterTest, Run) {
  Err err;
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::GENERATED_FILE);
  target.visibility().SetPublic();
  target.action_values().outputs() =
      SubstitutionList::MakeForTest("//out/Debug/foo.json");
  target.set_contents(Value(nullptr, true));
  target.set_output_conversion(Value(nullptr, "json"));

  Target dep(setup.settings(), Label(SourceDir("//foo/"), "dep"));
  dep.set_output_type(Target::ACTION);
  dep.visibility().SetPublic();
  dep.SetToolchain(setup.toolchain());
  ASSERT_TRUE(dep.OnResolved(&err));

  Target dep2(setup.settings(), Label(SourceDir("//foo/"), "dep2"));
  dep2.set_output_type(Target::ACTION);
  dep2.visibility().SetPublic();
  dep2.SetToolchain(setup.toolchain());
  ASSERT_TRUE(dep2.OnResolved(&err));

  Target bundle_data_dep(setup.settings(),
                         Label(SourceDir("//foo/"), "bundle_data_dep"));
  bundle_data_dep.sources().push_back(SourceFile("//foo/some_data.txt"));
  bundle_data_dep.set_output_type(Target::BUNDLE_DATA);
  bundle_data_dep.visibility().SetPublic();
  bundle_data_dep.SetToolchain(setup.toolchain());
  ASSERT_TRUE(bundle_data_dep.OnResolved(&err));

  Target datadep(setup.settings(), Label(SourceDir("//foo/"), "datadep"));
  datadep.set_output_type(Target::ACTION);
  datadep.visibility().SetPublic();
  datadep.SetToolchain(setup.toolchain());
  ASSERT_TRUE(datadep.OnResolved(&err));

  target.public_deps().push_back(LabelTargetPair(&dep));
  target.public_deps().push_back(LabelTargetPair(&dep2));
  target.public_deps().push_back(LabelTargetPair(&bundle_data_dep));
  target.data_deps().push_back(LabelTargetPair(&datadep));

  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err)) << err.message();

  std::ostringstream out;
  NinjaGeneratedFileTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "build obj/foo/bar.stamp: stamp obj/foo/dep.stamp obj/foo/dep2.stamp || "
      "obj/foo/bundle_data_dep.stamp obj/foo/datadep.stamp\n";
  EXPECT_EQ(expected, out.str());
}
