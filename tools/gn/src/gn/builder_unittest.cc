// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "gn/builder.h"
#include "gn/config.h"
#include "gn/loader.h"
#include "gn/target.h"
#include "gn/test_with_scheduler.h"
#include "gn/test_with_scope.h"
#include "gn/toolchain.h"
#include "util/test/test.h"

namespace gn_builder_unittest {

class MockLoader : public Loader {
 public:
  MockLoader() = default;

  // Loader implementation:
  void Load(const SourceFile& file,
            const LocationRange& origin,
            const Label& toolchain_name) override {
    files_.push_back(file);
  }
  void ToolchainLoaded(const Toolchain* toolchain) override {}
  Label GetDefaultToolchain() const override { return Label(); }
  const Settings* GetToolchainSettings(const Label& label) const override {
    return nullptr;
  }
  SourceFile BuildFileForLabel(const Label& label) const override {
    return SourceFile(label.dir().value() + "BUILD.gn");
  }

  bool HasLoadedNone() const { return files_.empty(); }

  // Returns true if one/two loads have been requested and they match the given
  // file(s). This will clear the records so it will be empty for the next call.
  bool HasLoadedOne(const SourceFile& file) {
    if (files_.size() != 1u) {
      files_.clear();
      return false;
    }
    bool match = (files_[0] == file);
    files_.clear();
    return match;
  }
  bool HasLoadedTwo(const SourceFile& a, const SourceFile& b) {
    if (files_.size() != 2u) {
      files_.clear();
      return false;
    }

    bool match = ((files_[0] == a && files_[1] == b) ||
                  (files_[0] == b && files_[1] == a));
    files_.clear();
    return match;
  }
  bool HasLoadedOnce(const SourceFile& f) {
    return count(files_.begin(), files_.end(), f) == 1;
  }

 private:
  ~MockLoader() override = default;

  std::vector<SourceFile> files_;
};

class BuilderTest : public TestWithScheduler {
 public:
  BuilderTest()
      : loader_(new MockLoader),
        builder_(loader_.get()),
        settings_(&build_settings_, std::string()),
        scope_(&settings_) {
    build_settings_.SetBuildDir(SourceDir("//out/"));
    settings_.set_toolchain_label(Label(SourceDir("//tc/"), "default"));
    settings_.set_default_toolchain_label(settings_.toolchain_label());
  }

  Toolchain* DefineToolchain() {
    Toolchain* tc = new Toolchain(&settings_, settings_.toolchain_label());
    TestWithScope::SetupToolchain(tc);
    builder_.ItemDefined(std::unique_ptr<Item>(tc));
    return tc;
  }

 protected:
  scoped_refptr<MockLoader> loader_;
  Builder builder_;
  BuildSettings build_settings_;
  Settings settings_;
  Scope scope_;
};

TEST_F(BuilderTest, BasicDeps) {
  SourceDir toolchain_dir = settings_.toolchain_label().dir();
  std::string toolchain_name = settings_.toolchain_label().name();

  // Construct a dependency chain: A -> B -> C. Define A first with a
  // forward-reference to B, then C, then B to test the different orders that
  // the dependencies are hooked up.
  Label a_label(SourceDir("//a/"), "a", toolchain_dir, toolchain_name);
  Label b_label(SourceDir("//b/"), "b", toolchain_dir, toolchain_name);
  Label c_label(SourceDir("//c/"), "c", toolchain_dir, toolchain_name);

  // The builder will take ownership of the pointers.
  Target* a = new Target(&settings_, a_label);
  a->public_deps().push_back(LabelTargetPair(b_label));
  a->set_output_type(Target::EXECUTABLE);
  builder_.ItemDefined(std::unique_ptr<Item>(a));

  // Should have requested that B and the toolchain is loaded.
  EXPECT_TRUE(loader_->HasLoadedTwo(SourceFile("//tc/BUILD.gn"),
                                    SourceFile("//b/BUILD.gn")));

  // Define the toolchain.
  DefineToolchain();
  BuilderRecord* toolchain_record =
      builder_.GetRecord(settings_.toolchain_label());
  ASSERT_TRUE(toolchain_record);
  EXPECT_EQ(BuilderRecord::ITEM_TOOLCHAIN, toolchain_record->type());

  // A should be unresolved with an item
  BuilderRecord* a_record = builder_.GetRecord(a_label);
  EXPECT_TRUE(a_record->item());
  EXPECT_FALSE(a_record->resolved());
  EXPECT_FALSE(a_record->can_resolve());

  // B should be unresolved, have no item, and no deps.
  BuilderRecord* b_record = builder_.GetRecord(b_label);
  EXPECT_FALSE(b_record->item());
  EXPECT_FALSE(b_record->resolved());
  EXPECT_FALSE(b_record->can_resolve());
  EXPECT_TRUE(b_record->all_deps().empty());

  // A should have two deps: B and the toolchain. Only B should be unresolved.
  EXPECT_EQ(2u, a_record->all_deps().size());
  EXPECT_TRUE(a_record->all_deps().contains(toolchain_record));
  EXPECT_TRUE(a_record->all_deps().contains(b_record));

  std::vector<const BuilderRecord*> a_unresolved =
      a_record->GetSortedUnresolvedDeps();
  EXPECT_EQ(1u, a_unresolved.size());
  EXPECT_EQ(a_unresolved[0], b_record);

  // B should be marked as having A waiting on it.
  EXPECT_EQ(1u, b_record->waiting_on_resolution().size());
  EXPECT_TRUE(b_record->waiting_on_resolution().contains(a_record));

  // Add the C target.
  Target* c = new Target(&settings_, c_label);
  c->set_output_type(Target::STATIC_LIBRARY);
  c->visibility().SetPublic();
  builder_.ItemDefined(std::unique_ptr<Item>(c));

  // C only depends on the already-loaded toolchain so we shouldn't have
  // requested anything else.
  EXPECT_TRUE(loader_->HasLoadedNone());

  // Add the B target.
  Target* b = new Target(&settings_, b_label);
  a->public_deps().push_back(LabelTargetPair(c_label));
  b->set_output_type(Target::SHARED_LIBRARY);
  b->visibility().SetPublic();
  builder_.ItemDefined(std::unique_ptr<Item>(b));

  // B depends only on the already-loaded C and toolchain so we shouldn't have
  // requested anything else.
  EXPECT_TRUE(loader_->HasLoadedNone());

  // All targets should now be resolved.
  BuilderRecord* c_record = builder_.GetRecord(c_label);
  EXPECT_TRUE(a_record->resolved());
  EXPECT_TRUE(b_record->resolved());
  EXPECT_TRUE(c_record->resolved());

  EXPECT_TRUE(a_record->GetSortedUnresolvedDeps().empty());
  EXPECT_TRUE(b_record->GetSortedUnresolvedDeps().empty());
  EXPECT_TRUE(c_record->GetSortedUnresolvedDeps().empty());

  EXPECT_TRUE(a_record->waiting_on_resolution().empty());
  EXPECT_TRUE(b_record->waiting_on_resolution().empty());
  EXPECT_TRUE(c_record->waiting_on_resolution().empty());
}

TEST_F(BuilderTest, SortedUnresolvedDeps) {
  SourceDir toolchain_dir = settings_.toolchain_label().dir();
  std::string toolchain_name = settings_.toolchain_label().name();

  // Construct a dependency graph with:
  //    A -> B
  //    A -> D
  //    A -> C
  //
  // Ensure that the unresolved list of A is always [B, C, D]
  //
  Label a_label(SourceDir("//a/"), "a", toolchain_dir, toolchain_name);
  Label b_label(SourceDir("//b/"), "b", toolchain_dir, toolchain_name);
  Label c_label(SourceDir("//c/"), "c", toolchain_dir, toolchain_name);
  Label d_label(SourceDir("//d/"), "d", toolchain_dir, toolchain_name);

  BuilderRecord* a_record = builder_.GetOrCreateRecordForTesting(a_label);
  BuilderRecord* b_record = builder_.GetOrCreateRecordForTesting(b_label);
  BuilderRecord* c_record = builder_.GetOrCreateRecordForTesting(c_label);
  BuilderRecord* d_record = builder_.GetOrCreateRecordForTesting(d_label);

  a_record->AddDep(b_record);
  a_record->AddDep(d_record);
  a_record->AddDep(c_record);

  std::vector<const BuilderRecord*> a_unresolved = a_record->GetSortedUnresolvedDeps();
  EXPECT_EQ(3u, a_unresolved.size()) << a_unresolved.size();
  EXPECT_EQ(b_record, a_unresolved[0]);
  EXPECT_EQ(c_record, a_unresolved[1]);
  EXPECT_EQ(d_record, a_unresolved[2]);
}

// Tests that the "should generate" flag is set and propagated properly.
TEST_F(BuilderTest, ShouldGenerate) {
  DefineToolchain();

  // Define a secondary toolchain.
  Settings settings2(&build_settings_, "secondary/");
  Label toolchain_label2(SourceDir("//tc/"), "secondary");
  settings2.set_toolchain_label(toolchain_label2);
  Toolchain* tc2 = new Toolchain(&settings2, toolchain_label2);
  TestWithScope::SetupToolchain(tc2);
  builder_.ItemDefined(std::unique_ptr<Item>(tc2));

  // Construct a dependency chain: A -> B. A is in the default toolchain, B
  // is not.
  Label a_label(SourceDir("//foo/"), "a", settings_.toolchain_label().dir(),
                "a");
  Label b_label(SourceDir("//foo/"), "b", toolchain_label2.dir(),
                toolchain_label2.name());

  // First define B.
  Target* b = new Target(&settings2, b_label);
  b->visibility().SetPublic();
  b->set_output_type(Target::EXECUTABLE);
  builder_.ItemDefined(std::unique_ptr<Item>(b));

  // B should not be marked generated by default.
  BuilderRecord* b_record = builder_.GetRecord(b_label);
  EXPECT_FALSE(b_record->should_generate());

  // Define A with a dependency on B.
  Target* a = new Target(&settings_, a_label);
  a->public_deps().push_back(LabelTargetPair(b_label));
  a->set_output_type(Target::EXECUTABLE);
  builder_.ItemDefined(std::unique_ptr<Item>(a));

  // A should have the generate bit set since it's in the default toolchain.
  BuilderRecord* a_record = builder_.GetRecord(a_label);
  EXPECT_TRUE(a_record->should_generate());

  // It should have gotten pushed to B.
  EXPECT_TRUE(b_record->should_generate());
}

// Test that "gen_deps" forces targets to be generated.
TEST_F(BuilderTest, GenDeps) {
  DefineToolchain();

  // Define another toolchain
  Settings settings2(&build_settings_, "alternate/");
  Label alt_tc(SourceDir("//tc/"), "alternate");
  settings2.set_toolchain_label(alt_tc);
  Toolchain* tc2 = new Toolchain(&settings2, alt_tc);
  TestWithScope::SetupToolchain(tc2);
  builder_.ItemDefined(std::unique_ptr<Item>(tc2));

  // Construct the dependency chain A -> B -gen-> C -gen-> D where A is the only
  // target in the default toolchain. This should cause all 4 targets to be
  // generated.
  Label a_label(SourceDir("//a/"), "a", settings_.toolchain_label().dir(),
                settings_.toolchain_label().name());
  Label b_label(SourceDir("//b/"), "b", alt_tc.dir(), alt_tc.name());
  Label c_label(SourceDir("//c/"), "c", alt_tc.dir(), alt_tc.name());
  Label d_label(SourceDir("//d/"), "d", alt_tc.dir(), alt_tc.name());

  Target* c = new Target(&settings2, c_label);
  c->set_output_type(Target::EXECUTABLE);
  c->gen_deps().push_back(LabelTargetPair(d_label));
  builder_.ItemDefined(std::unique_ptr<Item>(c));

  Target* b = new Target(&settings2, b_label);
  b->set_output_type(Target::EXECUTABLE);
  b->gen_deps().push_back(LabelTargetPair(c_label));
  builder_.ItemDefined(std::unique_ptr<Item>(b));

  Target* a = new Target(&settings_, a_label);
  a->set_output_type(Target::EXECUTABLE);
  a->private_deps().push_back(LabelTargetPair(b_label));
  builder_.ItemDefined(std::unique_ptr<Item>(a));

  // At this point, "should generate" should have propogated to C which should
  // request for D to be loaded
  EXPECT_TRUE(loader_->HasLoadedOnce(SourceFile("//d/BUILD.gn")));

  Target* d = new Target(&settings2, d_label);
  d->set_output_type(Target::EXECUTABLE);
  builder_.ItemDefined(std::unique_ptr<Item>(d));

  BuilderRecord* a_record = builder_.GetRecord(a_label);
  BuilderRecord* b_record = builder_.GetRecord(b_label);
  BuilderRecord* c_record = builder_.GetRecord(c_label);
  BuilderRecord* d_record = builder_.GetRecord(d_label);
  EXPECT_TRUE(a_record->should_generate());
  EXPECT_TRUE(b_record->should_generate());
  EXPECT_TRUE(c_record->should_generate());
  EXPECT_TRUE(d_record->should_generate());
}

// Test that circular dependencies between gen_deps and deps are allowed
TEST_F(BuilderTest, GenDepsCircle) {
  DefineToolchain();
  Settings settings2(&build_settings_, "alternate/");
  Label alt_tc(SourceDir("//tc/"), "alternate");
  settings2.set_toolchain_label(alt_tc);
  Toolchain* tc2 = new Toolchain(&settings2, alt_tc);
  TestWithScope::SetupToolchain(tc2);
  builder_.ItemDefined(std::unique_ptr<Item>(tc2));

  // A is in the default toolchain and lists B as a gen_dep
  // B is in an alternate toolchain and lists A as a normal dep
  Label a_label(SourceDir("//a/"), "a", settings_.toolchain_label().dir(),
                settings_.toolchain_label().name());
  Label b_label(SourceDir("//b/"), "b", alt_tc.dir(), alt_tc.name());

  Target* a = new Target(&settings_, a_label);
  a->gen_deps().push_back(LabelTargetPair(b_label));
  a->set_output_type(Target::EXECUTABLE);
  builder_.ItemDefined(std::unique_ptr<Item>(a));

  Target* b = new Target(&settings2, b_label);
  b->private_deps().push_back(LabelTargetPair(a_label));
  b->set_output_type(Target::EXECUTABLE);
  builder_.ItemDefined(std::unique_ptr<Item>(b));

  Err err;
  EXPECT_TRUE(builder_.CheckForBadItems(&err));
  BuilderRecord* b_record = builder_.GetRecord(b_label);
  EXPECT_TRUE(b_record->should_generate());
}

// Tests that configs applied to a config get loaded (bug 536844).
TEST_F(BuilderTest, ConfigLoad) {
  SourceDir toolchain_dir = settings_.toolchain_label().dir();
  std::string toolchain_name = settings_.toolchain_label().name();

  // Construct a dependency chain: A -> B -> C. Define A first with a
  // forward-reference to B, then C, then B to test the different orders that
  // the dependencies are hooked up.
  Label a_label(SourceDir("//a/"), "a", toolchain_dir, toolchain_name);
  Label b_label(SourceDir("//b/"), "b", toolchain_dir, toolchain_name);
  Label c_label(SourceDir("//c/"), "c", toolchain_dir, toolchain_name);

  // The builder will take ownership of the pointers.
  Config* a = new Config(&settings_, a_label);
  a->configs().push_back(LabelConfigPair(b_label));
  builder_.ItemDefined(std::unique_ptr<Item>(a));

  // Should have requested that B is loaded.
  EXPECT_TRUE(loader_->HasLoadedOne(SourceFile("//b/BUILD.gn")));
}

}  // namespace gn_builder_unittest
