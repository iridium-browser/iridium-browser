// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ostream>
#include <vector>

#include "gn/config.h"
#include "gn/header_checker.h"
#include "gn/scheduler.h"
#include "gn/target.h"
#include "gn/test_with_scheduler.h"
#include "gn/test_with_scope.h"
#include "util/test/test.h"

namespace {

class HeaderCheckerTest : public TestWithScheduler {
 public:
  HeaderCheckerTest()
      : a_(setup_.settings(), Label(SourceDir("//a/"), "a")),
        b_(setup_.settings(), Label(SourceDir("//b/"), "b")),
        c_(setup_.settings(), Label(SourceDir("//c/"), "c")),
        d_(setup_.settings(), Label(SourceDir("//d/"), "d")) {
    a_.set_output_type(Target::SOURCE_SET);
    b_.set_output_type(Target::SOURCE_SET);
    c_.set_output_type(Target::SOURCE_SET);
    d_.set_output_type(Target::SOURCE_SET);

    Err err;
    a_.SetToolchain(setup_.toolchain(), &err);
    b_.SetToolchain(setup_.toolchain(), &err);
    c_.SetToolchain(setup_.toolchain(), &err);
    d_.SetToolchain(setup_.toolchain(), &err);

    a_.public_deps().push_back(LabelTargetPair(&b_));
    b_.public_deps().push_back(LabelTargetPair(&c_));

    // Start with all public visibility.
    a_.visibility().SetPublic();
    b_.visibility().SetPublic();
    c_.visibility().SetPublic();
    d_.visibility().SetPublic();

    d_.OnResolved(&err);
    c_.OnResolved(&err);
    b_.OnResolved(&err);
    a_.OnResolved(&err);

    targets_.push_back(&a_);
    targets_.push_back(&b_);
    targets_.push_back(&c_);
    targets_.push_back(&d_);
  }

 protected:
  scoped_refptr<HeaderChecker> CreateChecker() {
    bool check_generated = false;
    bool check_system = true;
    return base::MakeRefCounted<HeaderChecker>(
        setup_.build_settings(), targets_, check_generated, check_system);
  }

  TestWithScope setup_;

  // Some headers that are automatically set up with a public dependency chain.
  // a -> b -> c. D is unconnected.
  Target a_;
  Target b_;
  Target c_;
  Target d_;

  std::vector<const Target*> targets_;
};

}  // namespace

void PrintTo(const SourceFile& source_file, ::std::ostream* os) {
  *os << source_file.value();
}

TEST_F(HeaderCheckerTest, IsDependencyOf) {
  auto checker = CreateChecker();

  // Add a target P ("private") that privately depends on C, and hook up the
  // chain so that A -> P -> C. A will depend on C via two different paths.
  Err err;
  Target p(setup_.settings(), Label(SourceDir("//p/"), "p"));
  p.set_output_type(Target::SOURCE_SET);
  p.SetToolchain(setup_.toolchain(), &err);
  EXPECT_FALSE(err.has_error());
  p.private_deps().push_back(LabelTargetPair(&c_));
  p.visibility().SetPublic();
  p.OnResolved(&err);

  a_.public_deps().push_back(LabelTargetPair(&p));

  // A does not depend on itself.
  bool is_permitted = false;
  HeaderChecker::Chain chain;
  EXPECT_FALSE(checker->IsDependencyOf(&a_, &a_, &chain, &is_permitted));

  // A depends publicly on B.
  chain.clear();
  is_permitted = false;
  EXPECT_TRUE(checker->IsDependencyOf(&b_, &a_, &chain, &is_permitted));
  ASSERT_EQ(2u, chain.size());
  EXPECT_EQ(HeaderChecker::ChainLink(&b_, true), chain[0]);
  EXPECT_EQ(HeaderChecker::ChainLink(&a_, true), chain[1]);
  EXPECT_TRUE(is_permitted);

  // A indirectly depends on C. The "public" dependency path through B should
  // be identified.
  chain.clear();
  is_permitted = false;
  EXPECT_TRUE(checker->IsDependencyOf(&c_, &a_, &chain, &is_permitted));
  ASSERT_EQ(3u, chain.size());
  EXPECT_EQ(HeaderChecker::ChainLink(&c_, true), chain[0]);
  EXPECT_EQ(HeaderChecker::ChainLink(&b_, true), chain[1]);
  EXPECT_EQ(HeaderChecker::ChainLink(&a_, true), chain[2]);
  EXPECT_TRUE(is_permitted);

  // C does not depend on A.
  chain.clear();
  is_permitted = false;
  EXPECT_FALSE(checker->IsDependencyOf(&a_, &c_, &chain, &is_permitted));
  EXPECT_TRUE(chain.empty());
  EXPECT_FALSE(is_permitted);

  // Remove the B -> C public dependency, leaving P's private dep on C the only
  // path from A to C. This should now be found.
  chain.clear();
  EXPECT_EQ(&c_, b_.public_deps()[0].ptr);  // Validate it's the right one.
  b_.public_deps().erase(b_.public_deps().begin());
  EXPECT_TRUE(checker->IsDependencyOf(&c_, &a_, &chain, &is_permitted));
  EXPECT_EQ(3u, chain.size());
  EXPECT_EQ(HeaderChecker::ChainLink(&c_, false), chain[0]);
  EXPECT_EQ(HeaderChecker::ChainLink(&p, true), chain[1]);
  EXPECT_EQ(HeaderChecker::ChainLink(&a_, true), chain[2]);
  EXPECT_FALSE(is_permitted);

  // P privately depends on C. That dependency should be OK since it's only
  // one hop.
  chain.clear();
  is_permitted = false;
  EXPECT_TRUE(checker->IsDependencyOf(&c_, &p, &chain, &is_permitted));
  ASSERT_EQ(2u, chain.size());
  EXPECT_EQ(HeaderChecker::ChainLink(&c_, false), chain[0]);
  EXPECT_EQ(HeaderChecker::ChainLink(&p, true), chain[1]);
  EXPECT_TRUE(is_permitted);
}

TEST_F(HeaderCheckerTest, CheckInclude) {
  InputFile input_file(SourceFile("//some_file.cc"));
  input_file.SetContents(std::string());
  LocationRange range;  // Dummy value.

  // Add a disconnected target d with a header to check that you have to have
  // to depend on a target listing a header.
  SourceFile d_header("//d_header.h");
  d_.sources().push_back(SourceFile(d_header));

  // Add a header on B and say everything in B is public.
  SourceFile b_public("//b_public.h");
  b_.sources().push_back(b_public);
  c_.set_all_headers_public(true);

  // Add a public and private header on C.
  SourceFile c_public("//c_public.h");
  SourceFile c_private("//c_private.h");
  c_.sources().push_back(c_private);
  c_.public_headers().push_back(c_public);
  c_.set_all_headers_public(false);

  // Create another toolchain.
  Settings other_settings(setup_.build_settings(), "other/");
  Toolchain other_toolchain(&other_settings,
                            Label(SourceDir("//toolchain/"), "other"));
  TestWithScope::SetupToolchain(&other_toolchain);
  other_settings.set_toolchain_label(other_toolchain.label());
  other_settings.set_default_toolchain_label(setup_.toolchain()->label());

  // Add a target in the other toolchain with a header in it that is not
  // connected to any targets in the main toolchain.
  Target otc(&other_settings,
             Label(SourceDir("//p/"), "otc", other_toolchain.label().dir(),
                   other_toolchain.label().name()));
  otc.set_output_type(Target::SOURCE_SET);
  Err err;
  EXPECT_TRUE(otc.SetToolchain(&other_toolchain, &err));
  otc.visibility().SetPublic();
  targets_.push_back(&otc);

  SourceFile otc_header("//otc_header.h");
  otc.sources().push_back(otc_header);
  EXPECT_TRUE(otc.OnResolved(&err));

  auto checker = CreateChecker();

  std::set<std::pair<const Target*, const Target*>> no_dependency_cache;
  // A file in target A can't include a header from D because A has no
  // dependency on D.
  std::vector<Err> errors;
  checker->CheckInclude(&a_, input_file, d_header, range, &no_dependency_cache,
                        &errors);
  EXPECT_GT(errors.size(), 0);

  // A can include the public header in B.
  errors.clear();
  no_dependency_cache.clear();
  checker->CheckInclude(&a_, input_file, b_public, range, &no_dependency_cache,
                        &errors);
  EXPECT_EQ(errors.size(), 0);

  // Check A depending on the public and private headers in C.
  errors.clear();
  no_dependency_cache.clear();
  checker->CheckInclude(&a_, input_file, c_public, range, &no_dependency_cache,
                        &errors);
  EXPECT_EQ(errors.size(), 0);
  errors.clear();
  no_dependency_cache.clear();
  checker->CheckInclude(&a_, input_file, c_private, range, &no_dependency_cache,
                        &errors);
  EXPECT_GT(errors.size(), 0);

  // A can depend on a random file unknown to the build.
  errors.clear();
  no_dependency_cache.clear();
  checker->CheckInclude(&a_, input_file, SourceFile("//random.h"), range,
                        &no_dependency_cache, &errors);
  EXPECT_EQ(errors.size(), 0);

  // A can depend on a file present only in another toolchain even with no
  // dependency path.
  errors.clear();
  no_dependency_cache.clear();
  checker->CheckInclude(&a_, input_file, otc_header, range,
                        &no_dependency_cache, &errors);
  EXPECT_EQ(errors.size(), 0);
}

// A public chain of dependencies should always be identified first, even if
// it is longer than a private one.
TEST_F(HeaderCheckerTest, PublicFirst) {
  // Now make a A -> Z -> D private dependency chain (one shorter than the
  // public one to get to D).
  Target z(setup_.settings(), Label(SourceDir("//a/"), "a"));
  z.set_output_type(Target::SOURCE_SET);
  Err err;
  EXPECT_TRUE(z.SetToolchain(setup_.toolchain(), &err));
  z.private_deps().push_back(LabelTargetPair(&d_));
  EXPECT_TRUE(z.OnResolved(&err));
  targets_.push_back(&z);

  a_.private_deps().push_back(LabelTargetPair(&z));

  // Check that D can be found from A, but since it's private, it will be
  // marked as not permitted.
  bool is_permitted = false;
  HeaderChecker::Chain chain;
  auto checker = CreateChecker();
  EXPECT_TRUE(checker->IsDependencyOf(&d_, &a_, &chain, &is_permitted));

  EXPECT_FALSE(is_permitted);
  ASSERT_EQ(3u, chain.size());
  EXPECT_EQ(HeaderChecker::ChainLink(&d_, false), chain[0]);
  EXPECT_EQ(HeaderChecker::ChainLink(&z, false), chain[1]);
  EXPECT_EQ(HeaderChecker::ChainLink(&a_, true), chain[2]);

  // Hook up D to the existing public A -> B -> C chain to make a long one, and
  // search for D again.
  c_.public_deps().push_back(LabelTargetPair(&d_));
  checker = CreateChecker();
  chain.clear();
  EXPECT_TRUE(checker->IsDependencyOf(&d_, &a_, &chain, &is_permitted));

  // This should have found the long public one.
  EXPECT_TRUE(is_permitted);
  ASSERT_EQ(4u, chain.size());
  EXPECT_EQ(HeaderChecker::ChainLink(&d_, true), chain[0]);
  EXPECT_EQ(HeaderChecker::ChainLink(&c_, true), chain[1]);
  EXPECT_EQ(HeaderChecker::ChainLink(&b_, true), chain[2]);
  EXPECT_EQ(HeaderChecker::ChainLink(&a_, true), chain[3]);
}

// Checks that the allow_circular_includes_from list works.
TEST_F(HeaderCheckerTest, CheckIncludeAllowCircular) {
  InputFile input_file(SourceFile("//some_file.cc"));
  input_file.SetContents(std::string());
  LocationRange range;  // Dummy value.

  // Add an include file to A.
  SourceFile a_public("//a_public.h");
  a_.sources().push_back(a_public);

  auto checker = CreateChecker();

  // A depends on B. So B normally can't include headers from A.
  std::vector<Err> errors;
  std::set<std::pair<const Target*, const Target*>> no_dependency_cache;
  checker->CheckInclude(&b_, input_file, a_public, range, &no_dependency_cache,
                        &errors);
  EXPECT_GT(errors.size(), 0);

  // Add an allow_circular_includes_from on A that lists B.
  a_.allow_circular_includes_from().insert(b_.label());

  // Now the include from B to A should be allowed.
  errors.clear();
  no_dependency_cache.clear();
  checker->CheckInclude(&b_, input_file, a_public, range, &no_dependency_cache,
                        &errors);
  EXPECT_EQ(errors.size(), 0);
}

TEST_F(HeaderCheckerTest, SourceFileForInclude) {
  using base::FilePath;
  const std::vector<SourceDir> kIncludeDirs = {
      SourceDir("/c/custom_include/"), SourceDir("//"), SourceDir("//subdir")};
  a_.sources().push_back(SourceFile("//lib/header1.h"));
  b_.sources().push_back(SourceFile("/c/custom_include/header2.h"));
  d_.sources().push_back(SourceFile("/d/subdir/header3.h"));

  InputFile dummy_input_file(SourceFile("/d/subdir/some_file.cc"));
  dummy_input_file.SetContents(std::string());

  auto checker = CreateChecker();
  {
    Err err;
    IncludeStringWithLocation include;
    include.contents = "lib/header1.h";
    SourceFile source_file = checker->SourceFileForInclude(
        include, kIncludeDirs, dummy_input_file, &err);
    EXPECT_FALSE(err.has_error());
    EXPECT_EQ(SourceFile("//lib/header1.h"), source_file);
  }

  {
    Err err;
    IncludeStringWithLocation include;
    include.contents = "header2.h";
    SourceFile source_file = checker->SourceFileForInclude(
        include, kIncludeDirs, dummy_input_file, &err);
    EXPECT_FALSE(err.has_error());
    EXPECT_EQ(SourceFile("/c/custom_include/header2.h"), source_file);
  }

  // A non system style include should find a header file in the same directory
  // as the source file, regardless of include dirs.
  {
    Err err;
    IncludeStringWithLocation include;
    include.contents = "header3.h";
    include.system_style_include = false;
    SourceFile source_file = checker->SourceFileForInclude(
        include, kIncludeDirs, dummy_input_file, &err);
    EXPECT_FALSE(err.has_error());
    EXPECT_EQ(SourceFile("/d/subdir/header3.h"), source_file);
  }

  // A system style include should *not* find a header file in the same
  // directory as the source file if that directory is not in the include dirs.
  {
    Err err;
    IncludeStringWithLocation include;
    include.contents = "header3.h";
    include.system_style_include = true;
    SourceFile source_file = checker->SourceFileForInclude(
        include, kIncludeDirs, dummy_input_file, &err);
    EXPECT_TRUE(source_file.is_null());
    EXPECT_FALSE(err.has_error());
  }
}

TEST_F(HeaderCheckerTest, SourceFileForInclude_FileNotFound) {
  using base::FilePath;
  const char kFileContents[] = "Some dummy contents";
  const std::vector<SourceDir> kIncludeDirs = {SourceDir("//")};
  auto checker = CreateChecker();

  Err err;
  InputFile input_file(SourceFile("//input.cc"));
  input_file.SetContents(std::string(kFileContents));

  IncludeStringWithLocation include;
  include.contents = "header.h";
  SourceFile source_file =
      checker->SourceFileForInclude(include, kIncludeDirs, input_file, &err);
  EXPECT_TRUE(source_file.is_null());
  EXPECT_FALSE(err.has_error());
}

TEST_F(HeaderCheckerTest, Friend) {
  // Note: we have a public dependency chain A -> B -> C set up already.
  InputFile input_file(SourceFile("//some_file.cc"));
  input_file.SetContents(std::string());
  LocationRange range;  // Dummy value.

  // Add a private header on C.
  SourceFile c_private("//c_private.h");
  c_.sources().push_back(c_private);
  c_.set_all_headers_public(false);

  // List A as a friend of C.
  Err err;
  c_.friends().push_back(LabelPattern::GetPattern(
      SourceDir("//"), std::string_view(), Value(nullptr, "//a:*"), &err));
  ASSERT_FALSE(err.has_error());

  // Must be after setting everything up for it to find the files.
  auto checker = CreateChecker();

  // B should not be allowed to include C's private header.
  std::vector<Err> errors;
  std::set<std::pair<const Target*, const Target*>> no_dependency_cache;
  checker->CheckInclude(&b_, input_file, c_private, range, &no_dependency_cache,
                        &errors);
  EXPECT_GT(errors.size(), 0);

  // A should be able to because of the friend declaration.
  errors.clear();
  no_dependency_cache.clear();
  checker->CheckInclude(&a_, input_file, c_private, range, &no_dependency_cache,
                        &errors);
  EXPECT_EQ(errors.size(), 0);
}
