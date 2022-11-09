// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>

#include "gn/c_substitution_type.h"
#include "gn/err.h"
#include "gn/escape.h"
#include "gn/substitution_list.h"
#include "gn/substitution_pattern.h"
#include "gn/substitution_writer.h"
#include "gn/target.h"
#include "gn/test_with_scope.h"
#include "util/build_config.h"
#include "util/test/test.h"

TEST(SubstitutionWriter, GetListAs) {
  TestWithScope setup;

  SubstitutionList list =
      SubstitutionList::MakeForTest("//foo/bar/a.cc", "//foo/bar/b.cc");

  std::vector<SourceFile> sources;
  SubstitutionWriter::GetListAsSourceFiles(list, &sources);
  ASSERT_EQ(2u, sources.size());
  EXPECT_EQ("//foo/bar/a.cc", sources[0].value());
  EXPECT_EQ("//foo/bar/b.cc", sources[1].value());

  std::vector<OutputFile> outputs;
  SubstitutionWriter::GetListAsOutputFiles(setup.settings(), list, &outputs);
  ASSERT_EQ(2u, outputs.size());
  EXPECT_EQ("../../foo/bar/a.cc", outputs[0].value());
  EXPECT_EQ("../../foo/bar/b.cc", outputs[1].value());
}

TEST(SubstitutionWriter, ApplyPatternToSource) {
  TestWithScope setup;

  SubstitutionPattern pattern;
  Err err;
  ASSERT_TRUE(pattern.Parse("{{source_gen_dir}}/{{source_name_part}}.tmp",
                            nullptr, &err));

  SourceFile result = SubstitutionWriter::ApplyPatternToSource(
      nullptr, setup.settings(), pattern, SourceFile("//foo/bar/myfile.txt"));
  ASSERT_EQ("//out/Debug/gen/foo/bar/myfile.tmp", result.value());

  result = SubstitutionWriter::ApplyPatternToSource(
      nullptr, setup.settings(), pattern,
      SourceFile("//out/Debug/gen/generated_file.cc"));
  ASSERT_EQ("//out/Debug/gen/BUILD_DIR/gen/generated_file.tmp", result.value())
      << result.value();
}

TEST(SubstitutionWriter, ApplyPatternToSourceAsOutputFile) {
  TestWithScope setup;

  SubstitutionPattern pattern;
  Err err;
  ASSERT_TRUE(pattern.Parse("{{source_gen_dir}}/{{source_name_part}}.tmp",
                            nullptr, &err));

  OutputFile result = SubstitutionWriter::ApplyPatternToSourceAsOutputFile(
      nullptr, setup.settings(), pattern, SourceFile("//foo/bar/myfile.txt"));
  ASSERT_EQ("gen/foo/bar/myfile.tmp", result.value());
}

TEST(SubstitutionWriter, WriteNinjaVariablesForSource) {
  TestWithScope setup;

  std::vector<const Substitution*> types;
  types.push_back(&SubstitutionSource);
  types.push_back(&SubstitutionSourceNamePart);
  types.push_back(&SubstitutionSourceDir);

  EscapeOptions options;
  options.mode = ESCAPE_NONE;

  std::ostringstream out;
  SubstitutionWriter::WriteNinjaVariablesForSource(
      nullptr, setup.settings(), SourceFile("//foo/bar/baz.txt"), types,
      options, out);

  // The "source" should be skipped since that will expand to $in which is
  // implicit.
  EXPECT_EQ(
      "  source_name_part = baz\n"
      "  source_dir = ../../foo/bar\n",
      out.str());
}

TEST(SubstitutionWriter, WriteWithNinjaVariables) {
  Err err;
  SubstitutionPattern pattern;
  ASSERT_TRUE(pattern.Parse("-i {{source}} --out=bar\"{{source_name_part}}\".o",
                            nullptr, &err));
  EXPECT_FALSE(err.has_error());

  EscapeOptions options;
  options.mode = ESCAPE_NONE;

  std::ostringstream out;
  SubstitutionWriter::WriteWithNinjaVariables(pattern, options, out);

  EXPECT_EQ("-i ${in} --out=bar\"${source_name_part}\".o", out.str());
}

TEST(SubstitutionWriter, SourceSubstitutions) {
  TestWithScope setup;
  Err err;

  Target target(setup.settings(), Label(SourceDir("//foo/bar/"), "baz"));
  target.set_output_type(Target::STATIC_LIBRARY);
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

// Call to get substitutions relative to the build dir.
#define GetRelSubst(str, what)                          \
  SubstitutionWriter::GetSourceSubstitution(            \
      &target, setup.settings(), SourceFile(str), what, \
      SubstitutionWriter::OUTPUT_RELATIVE,              \
      setup.settings()->build_settings()->build_dir())

// Call to get absolute directory substitutions.
#define GetAbsSubst(str, what)                          \
  SubstitutionWriter::GetSourceSubstitution(            \
      &target, setup.settings(), SourceFile(str), what, \
      SubstitutionWriter::OUTPUT_ABSOLUTE, SourceDir())

  // Try all possible templates with a normal looking string.
  EXPECT_EQ("../../foo/bar/baz.txt",
            GetRelSubst("//foo/bar/baz.txt", &SubstitutionSource));
  EXPECT_EQ("//foo/bar/baz.txt",
            GetAbsSubst("//foo/bar/baz.txt", &SubstitutionSource));

  EXPECT_EQ("baz",
            GetRelSubst("//foo/bar/baz.txt", &SubstitutionSourceNamePart));
  EXPECT_EQ("baz",
            GetAbsSubst("//foo/bar/baz.txt", &SubstitutionSourceNamePart));

  EXPECT_EQ("baz.txt",
            GetRelSubst("//foo/bar/baz.txt", &SubstitutionSourceFilePart));
  EXPECT_EQ("baz.txt",
            GetAbsSubst("//foo/bar/baz.txt", &SubstitutionSourceFilePart));

  EXPECT_EQ("../../foo/bar",
            GetRelSubst("//foo/bar/baz.txt", &SubstitutionSourceDir));
  EXPECT_EQ("//foo/bar",
            GetAbsSubst("//foo/bar/baz.txt", &SubstitutionSourceDir));

  EXPECT_EQ("foo/bar", GetRelSubst("//foo/bar/baz.txt",
                                   &SubstitutionSourceRootRelativeDir));
  EXPECT_EQ("foo/bar", GetAbsSubst("//foo/bar/baz.txt",
                                   &SubstitutionSourceRootRelativeDir));

  EXPECT_EQ("gen/foo/bar",
            GetRelSubst("//foo/bar/baz.txt", &SubstitutionSourceGenDir));
  EXPECT_EQ("//out/Debug/gen/foo/bar",
            GetAbsSubst("//foo/bar/baz.txt", &SubstitutionSourceGenDir));

  EXPECT_EQ("obj/foo/bar",
            GetRelSubst("//foo/bar/baz.txt", &SubstitutionSourceOutDir));
  EXPECT_EQ("//out/Debug/obj/foo/bar",
            GetAbsSubst("//foo/bar/baz.txt", &SubstitutionSourceOutDir));

  // Operations on an absolute path.
  EXPECT_EQ("/baz.txt", GetRelSubst("/baz.txt", &SubstitutionSource));
  EXPECT_EQ("/.", GetRelSubst("/baz.txt", &SubstitutionSourceDir));
  EXPECT_EQ("gen/ABS_PATH", GetRelSubst("/baz.txt", &SubstitutionSourceGenDir));
  EXPECT_EQ("obj/ABS_PATH", GetRelSubst("/baz.txt", &SubstitutionSourceOutDir));
#if defined(OS_WIN)
  EXPECT_EQ("gen/ABS_PATH/C",
            GetRelSubst("/C:/baz.txt", &SubstitutionSourceGenDir));
  EXPECT_EQ("obj/ABS_PATH/C",
            GetRelSubst("/C:/baz.txt", &SubstitutionSourceOutDir));
#endif

  EXPECT_EQ(".", GetRelSubst("//baz.txt", &SubstitutionSourceRootRelativeDir));

  EXPECT_EQ("baz.txt", GetRelSubst("//foo/bar/baz.txt",
                                   &SubstitutionSourceTargetRelative));
  EXPECT_EQ("baz.txt", GetAbsSubst("//foo/bar/baz.txt",
                                   &SubstitutionSourceTargetRelative));

#undef GetAbsSubst
#undef GetRelSubst
}

TEST(SubstitutionWriter, TargetSubstitutions) {
  TestWithScope setup;
  Err err;

  Target target(setup.settings(), Label(SourceDir("//foo/bar/"), "baz"));
  target.set_output_type(Target::STATIC_LIBRARY);
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::string result;
  EXPECT_TRUE(SubstitutionWriter::GetTargetSubstitution(
      &target, &SubstitutionLabel, &result));
  EXPECT_EQ("//foo/bar:baz", result);

  EXPECT_TRUE(SubstitutionWriter::GetTargetSubstitution(
      &target, &SubstitutionLabelName, &result));
  EXPECT_EQ("baz", result);

  EXPECT_TRUE(SubstitutionWriter::GetTargetSubstitution(
      &target, &SubstitutionLabelNoToolchain, &result));
  EXPECT_EQ("//foo/bar:baz", result);

  EXPECT_TRUE(SubstitutionWriter::GetTargetSubstitution(
      &target, &SubstitutionRootGenDir, &result));
  EXPECT_EQ("gen", result);

  EXPECT_TRUE(SubstitutionWriter::GetTargetSubstitution(
      &target, &SubstitutionRootOutDir, &result));
  EXPECT_EQ(".", result);

  EXPECT_TRUE(SubstitutionWriter::GetTargetSubstitution(
      &target, &SubstitutionTargetGenDir, &result));
  EXPECT_EQ("gen/foo/bar", result);

  EXPECT_TRUE(SubstitutionWriter::GetTargetSubstitution(
      &target, &SubstitutionTargetOutDir, &result));
  EXPECT_EQ("obj/foo/bar", result);

  EXPECT_TRUE(SubstitutionWriter::GetTargetSubstitution(
      &target, &SubstitutionTargetOutputName, &result));
  EXPECT_EQ("libbaz", result);
}

TEST(SubstitutionWriter, CompilerSubstitutions) {
  TestWithScope setup;
  Err err;

  Target target(setup.settings(), Label(SourceDir("//foo/bar/"), "baz"));
  target.set_output_type(Target::STATIC_LIBRARY);
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  // The compiler substitution is just source + target combined. So test one
  // of each of those classes of things to make sure this is hooked up.
  EXPECT_EQ("file", SubstitutionWriter::GetCompilerSubstitution(
                        &target, SourceFile("//foo/bar/file.txt"),
                        &SubstitutionSourceNamePart));
  EXPECT_EQ("gen/foo/bar", SubstitutionWriter::GetCompilerSubstitution(
                               &target, SourceFile("//foo/bar/file.txt"),
                               &SubstitutionTargetGenDir));
}

TEST(SubstitutionWriter, LinkerSubstitutions) {
  TestWithScope setup;
  Err err;

  Target target(setup.settings(), Label(SourceDir("//foo/bar/"), "baz"));
  target.set_output_type(Target::SHARED_LIBRARY);
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  const Tool* tool = setup.toolchain()->GetToolForTargetFinalOutput(&target);

  // The compiler substitution is just target + OUTPUT_EXTENSION combined. So
  // test one target one plus the output extension.
  EXPECT_EQ(".so", SubstitutionWriter::GetLinkerSubstitution(
                       &target, tool, &SubstitutionOutputExtension));
  EXPECT_EQ("gen/foo/bar", SubstitutionWriter::GetLinkerSubstitution(
                               &target, tool, &SubstitutionTargetGenDir));

  // Test that we handle paths that end up in the root build dir properly
  // (no leading "./" or "/").
  SubstitutionPattern pattern;
  ASSERT_TRUE(pattern.Parse("{{root_out_dir}}/{{target_output_name}}.so",
                            nullptr, &err));

  OutputFile output = SubstitutionWriter::ApplyPatternToLinkerAsOutputFile(
      &target, tool, pattern);
  EXPECT_EQ("./libbaz.so", output.value());

  // Output extensions can be overridden.
  target.set_output_extension("extension");
  EXPECT_EQ(".extension", SubstitutionWriter::GetLinkerSubstitution(
                              &target, tool, &SubstitutionOutputExtension));
  target.set_output_extension("");
  EXPECT_EQ("", SubstitutionWriter::GetLinkerSubstitution(
                    &target, tool, &SubstitutionOutputExtension));

  // Output directory is tested in a separate test below.
}

TEST(SubstitutionWriter, OutputDir) {
  TestWithScope setup;
  Err err;

  // This tool has an output directory pattern and uses that for the
  // output name.
  std::unique_ptr<Tool> tool = Tool::CreateTool(CTool::kCToolLink);
  SubstitutionPattern out_dir_pattern;
  ASSERT_TRUE(out_dir_pattern.Parse("{{root_out_dir}}/{{target_output_name}}",
                                    nullptr, &err));
  tool->set_default_output_dir(out_dir_pattern);
  tool->SetComplete();

  // Default target with no output dir overrides.
  Target target(setup.settings(), Label(SourceDir("//foo/"), "baz"));
  target.set_output_type(Target::EXECUTABLE);
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  // The output should expand the default from the patterns in the tool.
  SubstitutionPattern output_name;
  ASSERT_TRUE(output_name.Parse("{{output_dir}}/{{target_output_name}}.exe",
                                nullptr, &err));
  EXPECT_EQ("./baz/baz.exe",
            SubstitutionWriter::ApplyPatternToLinkerAsOutputFile(
                &target, tool.get(), output_name)
                .value());

  // Override the output name to the root build dir.
  target.set_output_dir(SourceDir("//out/Debug/"));
  EXPECT_EQ("./baz.exe", SubstitutionWriter::ApplyPatternToLinkerAsOutputFile(
                             &target, tool.get(), output_name)
                             .value());

  // Override the output name to a new subdirectory.
  target.set_output_dir(SourceDir("//out/Debug/foo/bar"));
  EXPECT_EQ("foo/bar/baz.exe",
            SubstitutionWriter::ApplyPatternToLinkerAsOutputFile(
                &target, tool.get(), output_name)
                .value());
}
