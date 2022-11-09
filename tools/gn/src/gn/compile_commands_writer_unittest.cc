// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/compile_commands_writer.h"

#include <memory>
#include <sstream>
#include <utility>

#include "gn/config.h"
#include "gn/ninja_target_command_util.h"
#include "gn/scheduler.h"
#include "gn/target.h"
#include "gn/test_with_scheduler.h"
#include "gn/test_with_scope.h"
#include "util/build_config.h"
#include "util/test/test.h"

namespace {

bool CompareLabel(const Target* a, const Target* b) {
  return a->label() < b->label();
}

// InputConversion needs a global scheduler object.
class CompileCommandsTest : public TestWithScheduler {
 public:
  CompileCommandsTest() = default;

  const BuildSettings* build_settings() { return setup_.build_settings(); }
  const Settings* settings() { return setup_.settings(); }
  const TestWithScope& setup() { return setup_; }
  const Toolchain* toolchain() { return setup_.toolchain(); }

  // Returns true if the two target vectors contain the same targets, order
  // independent.
  bool VectorsEqual(std::vector<const Target*> a,
                    std::vector<const Target*> b) const {
    std::sort(a.begin(), a.end(), &CompareLabel);
    std::sort(b.begin(), b.end(), &CompareLabel);
    return a == b;
  }

 private:
  TestWithScope setup_;
};

}  // namespace

TEST_F(CompileCommandsTest, SourceSet) {
  Err err;

  std::vector<const Target*> targets;
  Target target(settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::SOURCE_SET);
  target.visibility().SetPublic();
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.sources().push_back(SourceFile("//foo/input2.cc"));
  // Also test object files, which should be just passed through to the
  // dependents to link.
  target.sources().push_back(SourceFile("//foo/input3.o"));
  target.sources().push_back(SourceFile("//foo/input4.obj"));
  target.SetToolchain(toolchain());
  ASSERT_TRUE(target.OnResolved(&err));
  targets.push_back(&target);

  // Source set itself.
  {
    CompileCommandsWriter writer;
    std::string out = writer.RenderJSON(build_settings(), targets);

#if defined(OS_WIN)
    const char expected[] =
        "[\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input1.cc\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"c++ ../../foo/input1.cc     -o  "
        "obj/foo/bar.input1.o\"\r\n"
        "  },\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input2.cc\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"c++ ../../foo/input2.cc     -o  "
        "obj/foo/bar.input2.o\"\r\n"
        "  }\r\n"
        "]\r\n";
#else
    const char expected[] =
        "[\n"
        "  {\n"
        "    \"file\": \"../../foo/input1.cc\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"c++ ../../foo/input1.cc     -o  "
        "obj/foo/bar.input1.o\"\n"
        "  },\n"
        "  {\n"
        "    \"file\": \"../../foo/input2.cc\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"c++ ../../foo/input2.cc     -o  "
        "obj/foo/bar.input2.o\"\n"
        "  }\n"
        "]\n";
#endif
    EXPECT_EQ(expected, out);
  }

  // A shared library that depends on the source set.
  Target shlib_target(settings(), Label(SourceDir("//foo/"), "shlib"));
  shlib_target.sources().push_back(SourceFile("//foo/input3.cc"));
  shlib_target.set_output_type(Target::SHARED_LIBRARY);
  shlib_target.public_deps().push_back(LabelTargetPair(&target));
  shlib_target.SetToolchain(toolchain());
  ASSERT_TRUE(shlib_target.OnResolved(&err));
  targets.push_back(&shlib_target);

  {
    CompileCommandsWriter writer;
    std::string out = writer.RenderJSON(build_settings(), targets);

#if defined(OS_WIN)
    const char expected[] =
        "[\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input1.cc\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"c++ ../../foo/input1.cc     -o  "
        "obj/foo/bar.input1.o\"\r\n"
        "  },\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input2.cc\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"c++ ../../foo/input2.cc     -o  "
        "obj/foo/bar.input2.o\"\r\n"
        "  },\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input3.cc\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"c++ ../../foo/input3.cc     -o  "
        "obj/foo/libshlib.input3.o\"\r\n"
        "  }\r\n"
        "]\r\n";
#else
    const char expected[] =
        "[\n"
        "  {\n"
        "    \"file\": \"../../foo/input1.cc\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"c++ ../../foo/input1.cc     -o  "
        "obj/foo/bar.input1.o\"\n"
        "  },\n"
        "  {\n"
        "    \"file\": \"../../foo/input2.cc\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"c++ ../../foo/input2.cc     -o  "
        "obj/foo/bar.input2.o\"\n"
        "  },\n"
        "  {\n"
        "    \"file\": \"../../foo/input3.cc\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"c++ ../../foo/input3.cc     -o  "
        "obj/foo/libshlib.input3.o\"\n"
        "  }\n"
        "]\n";
#endif
    EXPECT_EQ(expected, out);
  }

  // A static library that depends on the source set (should not link it).
  Target stlib_target(settings(), Label(SourceDir("//foo/"), "stlib"));
  stlib_target.sources().push_back(SourceFile("//foo/input4.cc"));
  stlib_target.set_output_type(Target::STATIC_LIBRARY);
  stlib_target.public_deps().push_back(LabelTargetPair(&target));
  stlib_target.SetToolchain(toolchain());
  ASSERT_TRUE(stlib_target.OnResolved(&err));
  targets.push_back(&stlib_target);

  {
    CompileCommandsWriter writer;
    std::string out = writer.RenderJSON(build_settings(), targets);

#if defined(OS_WIN)
    const char expected[] =
        "[\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input1.cc\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"c++ ../../foo/input1.cc     -o  "
        "obj/foo/bar.input1.o\"\r\n"
        "  },\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input2.cc\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"c++ ../../foo/input2.cc     -o  "
        "obj/foo/bar.input2.o\"\r\n"
        "  },\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input3.cc\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"c++ ../../foo/input3.cc     -o  "
        "obj/foo/libshlib.input3.o\"\r\n"
        "  },\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input4.cc\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"c++ ../../foo/input4.cc     -o  "
        "obj/foo/libstlib.input4.o\"\r\n"
        "  }\r\n"
        "]\r\n";
#else
    const char expected[] =
        "[\n"
        "  {\n"
        "    \"file\": \"../../foo/input1.cc\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"c++ ../../foo/input1.cc     -o  "
        "obj/foo/bar.input1.o\"\n"
        "  },\n"
        "  {\n"
        "    \"file\": \"../../foo/input2.cc\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"c++ ../../foo/input2.cc     -o  "
        "obj/foo/bar.input2.o\"\n"
        "  },\n"
        "  {\n"
        "    \"file\": \"../../foo/input3.cc\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"c++ ../../foo/input3.cc     -o  "
        "obj/foo/libshlib.input3.o\"\n"
        "  },\n"
        "  {\n"
        "    \"file\": \"../../foo/input4.cc\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"c++ ../../foo/input4.cc     -o  "
        "obj/foo/libstlib.input4.o\"\n"
        "  }\n"
        "]\n";
#endif
    EXPECT_EQ(expected, out);
  }
}

TEST_F(CompileCommandsTest, EscapeDefines) {
  Err err;

  std::vector<const Target*> targets;
  TestTarget target(setup(), "//foo:bar", Target::STATIC_LIBRARY);
  target.sources().push_back(SourceFile("//foo/input.cc"));
  target.config_values().defines().push_back("BOOL_DEF");
  target.config_values().defines().push_back("INT_DEF=123");
  target.config_values().defines().push_back("STR_DEF=\"ABCD 1\"");
  target.config_values().defines().push_back("INCLUDE=<header.h>");
  ASSERT_TRUE(target.OnResolved(&err));
  targets.push_back(&target);

  CompileCommandsWriter writer;
  std::string out = writer.RenderJSON(build_settings(), targets);

  const char expected[] =
      "-DBOOL_DEF -DINT_DEF=123 \\\"-DSTR_DEF=\\\\\\\"ABCD 1\\\\\\\"\\\" "
      "\\\"-DINCLUDE=\\u003Cheader.h>\\\"";
  EXPECT_TRUE(out.find(expected) != std::string::npos);
}

TEST_F(CompileCommandsTest, WinPrecompiledHeaders) {
  Err err;

  // This setup's toolchain does not have precompiled headers defined.
  // A precompiled header toolchain.
  Settings pch_settings(build_settings(), "withpch/");
  Toolchain pch_toolchain(&pch_settings,
                          Label(SourceDir("//toolchain/"), "withpch"));
  pch_settings.set_toolchain_label(pch_toolchain.label());
  pch_settings.set_default_toolchain_label(toolchain()->label());

  // Declare a C++ compiler that supports PCH.
  std::unique_ptr<Tool> cxx = Tool::CreateTool(CTool::kCToolCxx);
  CTool* cxx_tool = cxx->AsC();
  TestWithScope::SetCommandForTool(
      "c++ {{source}} {{cflags}} {{cflags_cc}} {{defines}} {{include_dirs}} "
      "-o {{output}}",
      cxx_tool);
  cxx_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o"));
  cxx_tool->set_precompiled_header_type(CTool::PCH_MSVC);
  pch_toolchain.SetTool(std::move(cxx));

  // Add a C compiler as well.
  std::unique_ptr<Tool> cc = Tool::CreateTool(CTool::kCToolCc);
  CTool* cc_tool = cc->AsC();
  TestWithScope::SetCommandForTool(
      "cc {{source}} {{cflags}} {{cflags_c}} {{defines}} {{include_dirs}} "
      "-o {{output}}",
      cc_tool);
  cc_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o"));
  cc_tool->set_precompiled_header_type(CTool::PCH_MSVC);
  pch_toolchain.SetTool(std::move(cc));
  pch_toolchain.ToolchainSetupComplete();

  // This target doesn't specify precompiled headers.
  {
    std::vector<const Target*> targets;
    Target no_pch_target(&pch_settings,
                         Label(SourceDir("//foo/"), "no_pch_target"));
    no_pch_target.set_output_type(Target::SOURCE_SET);
    no_pch_target.visibility().SetPublic();
    no_pch_target.sources().push_back(SourceFile("//foo/input1.cc"));
    no_pch_target.sources().push_back(SourceFile("//foo/input2.c"));
    no_pch_target.config_values().cflags_c().push_back("-std=c99");
    no_pch_target.SetToolchain(&pch_toolchain);
    ASSERT_TRUE(no_pch_target.OnResolved(&err));
    targets.push_back(&no_pch_target);

    CompileCommandsWriter writer;
    std::string out = writer.RenderJSON(build_settings(), targets);

#if defined(OS_WIN)
    const char no_pch_expected[] =
        "[\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input1.cc\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"c++ ../../foo/input1.cc     -o  "
        "withpch/obj/foo/no_pch_target.input1.o\"\r\n"
        "  },\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input2.c\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"cc ../../foo/input2.c   -std=c99   -o  "
        "withpch/obj/foo/no_pch_target.input2.o\"\r\n"
        "  }\r\n"
        "]\r\n";
#else
    const char no_pch_expected[] =
        "[\n"
        "  {\n"
        "    \"file\": \"../../foo/input1.cc\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"c++ ../../foo/input1.cc     -o  "
        "withpch/obj/foo/no_pch_target.input1.o\"\n"
        "  },\n"
        "  {\n"
        "    \"file\": \"../../foo/input2.c\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"cc ../../foo/input2.c   -std=c99   -o  "
        "withpch/obj/foo/no_pch_target.input2.o\"\n"
        "  }\n"
        "]\n";
#endif
    EXPECT_EQ(no_pch_expected, out);
  }

  // This target specifies PCH.
  {
    std::vector<const Target*> targets;
    Target pch_target(&pch_settings, Label(SourceDir("//foo/"), "pch_target"));
    pch_target.config_values().set_precompiled_header("build/precompile.h");
    pch_target.config_values().set_precompiled_source(
        SourceFile("//build/precompile.cc"));
    pch_target.set_output_type(Target::SOURCE_SET);
    pch_target.visibility().SetPublic();
    pch_target.sources().push_back(SourceFile("//foo/input1.cc"));
    pch_target.sources().push_back(SourceFile("//foo/input2.c"));
    pch_target.SetToolchain(&pch_toolchain);
    ASSERT_TRUE(pch_target.OnResolved(&err));
    targets.push_back(&pch_target);

    CompileCommandsWriter writer;
    std::string out = writer.RenderJSON(build_settings(), targets);

#if defined(OS_WIN)
    const char pch_win_expected[] =
        "[\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input1.cc\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"c++ ../../foo/input1.cc   "
        "/Fpwithpch/obj/foo/pch_target_cc.pch /Yubuild/precompile.h   -o  "
        "withpch/obj/foo/pch_target.input1.o\"\r\n"
        "  },\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input2.c\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"cc ../../foo/input2.c   "
        "/Fpwithpch/obj/foo/pch_target_c.pch /Yubuild/precompile.h   -o  "
        "withpch/obj/foo/pch_target.input2.o\"\r\n"
        "  }\r\n"
        "]\r\n";
#else
    const char pch_win_expected[] =
        "[\n"
        "  {\n"
        "    \"file\": \"../../foo/input1.cc\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"c++ ../../foo/input1.cc   "
        "/Fpwithpch/obj/foo/pch_target_cc.pch /Yubuild/precompile.h   -o  "
        "withpch/obj/foo/pch_target.input1.o\"\n"
        "  },\n"
        "  {\n"
        "    \"file\": \"../../foo/input2.c\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"cc ../../foo/input2.c   "
        "/Fpwithpch/obj/foo/pch_target_c.pch /Yubuild/precompile.h   -o  "
        "withpch/obj/foo/pch_target.input2.o\"\n"
        "  }\n"
        "]\n";
#endif
    EXPECT_EQ(pch_win_expected, out);
  }
}

TEST_F(CompileCommandsTest, GCCPrecompiledHeaders) {
  Err err;

  // This setup's toolchain does not have precompiled headers defined.
  // A precompiled header toolchain.
  Settings pch_settings(build_settings(), "withpch/");
  Toolchain pch_toolchain(&pch_settings,
                          Label(SourceDir("//toolchain/"), "withpch"));
  pch_settings.set_toolchain_label(pch_toolchain.label());
  pch_settings.set_default_toolchain_label(toolchain()->label());

  // Declare a C++ compiler that supports PCH.
  std::unique_ptr<Tool> cxx = Tool::CreateTool(CTool::kCToolCxx);
  CTool* cxx_tool = cxx->AsC();
  TestWithScope::SetCommandForTool(
      "c++ {{source}} {{cflags}} {{cflags_cc}} {{defines}} {{include_dirs}} "
      "-o {{output}}",
      cxx_tool);
  cxx_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o"));
  cxx_tool->set_precompiled_header_type(CTool::PCH_GCC);
  pch_toolchain.SetTool(std::move(cxx));
  pch_toolchain.ToolchainSetupComplete();

  // Add a C compiler as well.
  std::unique_ptr<Tool> cc = Tool::CreateTool(CTool::kCToolCc);
  CTool* cc_tool = cc->AsC();
  TestWithScope::SetCommandForTool(
      "cc {{source}} {{cflags}} {{cflags_c}} {{defines}} {{include_dirs}} "
      "-o {{output}}",
      cc_tool);
  cc_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o"));
  cc_tool->set_precompiled_header_type(CTool::PCH_GCC);
  pch_toolchain.SetTool(std::move(cc));
  pch_toolchain.ToolchainSetupComplete();

  // This target doesn't specify precompiled headers.
  {
    std::vector<const Target*> targets;
    Target no_pch_target(&pch_settings,
                         Label(SourceDir("//foo/"), "no_pch_target"));
    no_pch_target.set_output_type(Target::SOURCE_SET);
    no_pch_target.visibility().SetPublic();
    no_pch_target.sources().push_back(SourceFile("//foo/input1.cc"));
    no_pch_target.sources().push_back(SourceFile("//foo/input2.c"));
    no_pch_target.config_values().cflags_c().push_back("-std=c99");
    no_pch_target.SetToolchain(&pch_toolchain);
    ASSERT_TRUE(no_pch_target.OnResolved(&err));
    targets.push_back(&no_pch_target);

    CompileCommandsWriter writer;
    std::string out = writer.RenderJSON(build_settings(), targets);

#if defined(OS_WIN)
    const char no_pch_expected[] =
        "[\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input1.cc\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"c++ ../../foo/input1.cc     -o  "
        "withpch/obj/foo/no_pch_target.input1.o\"\r\n"
        "  },\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input2.c\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"cc ../../foo/input2.c   -std=c99   -o  "
        "withpch/obj/foo/no_pch_target.input2.o\"\r\n"
        "  }\r\n"
        "]\r\n";
#else
    const char no_pch_expected[] =
        "[\n"
        "  {\n"
        "    \"file\": \"../../foo/input1.cc\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"c++ ../../foo/input1.cc     -o  "
        "withpch/obj/foo/no_pch_target.input1.o\"\n"
        "  },\n"
        "  {\n"
        "    \"file\": \"../../foo/input2.c\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"cc ../../foo/input2.c   -std=c99   -o  "
        "withpch/obj/foo/no_pch_target.input2.o\"\n"
        "  }\n"
        "]\n";
#endif
    EXPECT_EQ(no_pch_expected, out);
  }

  // This target specifies PCH.
  {
    std::vector<const Target*> targets;
    Target pch_target(&pch_settings, Label(SourceDir("//foo/"), "pch_target"));
    pch_target.config_values().set_precompiled_source(
        SourceFile("//build/precompile.h"));
    pch_target.config_values().cflags_c().push_back("-std=c99");
    pch_target.set_output_type(Target::SOURCE_SET);
    pch_target.visibility().SetPublic();
    pch_target.sources().push_back(SourceFile("//foo/input1.cc"));
    pch_target.sources().push_back(SourceFile("//foo/input2.c"));
    pch_target.SetToolchain(&pch_toolchain);
    ASSERT_TRUE(pch_target.OnResolved(&err));
    targets.push_back(&pch_target);

    CompileCommandsWriter writer;
    std::string out = writer.RenderJSON(build_settings(), targets);

#if defined(OS_WIN)
    const char pch_gcc_expected[] =
        "[\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input1.cc\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"c++ ../../foo/input1.cc   -include "
        "withpch/obj/build/pch_target.precompile.h-cc   -o  "
        "withpch/obj/foo/pch_target.input1.o\"\r\n"
        "  },\r\n"
        "  {\r\n"
        "    \"file\": \"../../foo/input2.c\",\r\n"
        "    \"directory\": \"out/Debug\",\r\n"
        "    \"command\": \"cc ../../foo/input2.c   -std=c99 -include "
        "withpch/obj/build/pch_target.precompile.h-c   -o  "
        "withpch/obj/foo/pch_target.input2.o\"\r\n"
        "  }\r\n"
        "]\r\n";
#else
    const char pch_gcc_expected[] =
        "[\n"
        "  {\n"
        "    \"file\": \"../../foo/input1.cc\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"c++ ../../foo/input1.cc   -include "
        "withpch/obj/build/pch_target.precompile.h-cc   -o  "
        "withpch/obj/foo/pch_target.input1.o\"\n"
        "  },\n"
        "  {\n"
        "    \"file\": \"../../foo/input2.c\",\n"
        "    \"directory\": \"out/Debug\",\n"
        "    \"command\": \"cc ../../foo/input2.c   -std=c99 -include "
        "withpch/obj/build/pch_target.precompile.h-c   -o  "
        "withpch/obj/foo/pch_target.input2.o\"\n"
        "  }\n"
        "]\n";
#endif
    EXPECT_EQ(pch_gcc_expected, out);
  }
}

TEST_F(CompileCommandsTest, EscapedFlags) {
  Err err;

  std::vector<const Target*> targets;
  Target target(settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::SOURCE_SET);
  target.sources().push_back(SourceFile("//foo/input1.c"));
  target.config_values().cflags_c().push_back("-DCONFIG=\"/config\"");
  target.SetToolchain(toolchain());
  ASSERT_TRUE(target.OnResolved(&err));
  targets.push_back(&target);

  CompileCommandsWriter writer;
  std::string out = writer.RenderJSON(build_settings(), targets);

#if defined(OS_WIN)
  const char expected[] =
      "[\r\n"
      "  {\r\n"
      "    \"file\": \"../../foo/input1.c\",\r\n"
      "    \"directory\": \"out/Debug\",\r\n"
      "    \"command\": \"cc ../../foo/input1.c   -DCONFIG=\\\"/config\\\"   "
      "-o  obj/foo/bar.input1.o\"\r\n"
      "  }\r\n"
      "]\r\n";
#else
  const char expected[] =
      "[\n"
      "  {\n"
      "    \"file\": \"../../foo/input1.c\",\n"
      "    \"directory\": \"out/Debug\",\n"
      "    \"command\": \"cc ../../foo/input1.c   -DCONFIG=\\\"/config\\\"   "
      "-o  obj/foo/bar.input1.o\"\n"
      "  }\n"
      "]\n";
#endif
  EXPECT_EQ(expected, out);
}

TEST_F(CompileCommandsTest, CollectTargets) {
  // Contruct the dependency tree:
  //
  //   //foo:bar1
  //     //base:base
  //   //foo:bar2
  //     //base:i18n
  //       //base:base
  //       //third_party:icu
  //   //random:random
  Err err;
  std::vector<const Target*> targets;

  Target icu_target(settings(), Label(SourceDir("//third_party/"), "icu"));
  icu_target.set_output_type(Target::SOURCE_SET);
  icu_target.visibility().SetPublic();
  icu_target.SetToolchain(toolchain());
  ASSERT_TRUE(icu_target.OnResolved(&err));
  targets.push_back(&icu_target);

  Target base_target(settings(), Label(SourceDir("//base/"), "base"));
  base_target.set_output_type(Target::SOURCE_SET);
  base_target.visibility().SetPublic();
  base_target.SetToolchain(toolchain());
  ASSERT_TRUE(base_target.OnResolved(&err));
  targets.push_back(&base_target);

  Target base_i18n(settings(), Label(SourceDir("//base/"), "i18n"));
  base_i18n.set_output_type(Target::SOURCE_SET);
  base_i18n.visibility().SetPublic();
  base_i18n.private_deps().push_back(LabelTargetPair(&icu_target));
  base_i18n.public_deps().push_back(LabelTargetPair(&base_target));
  base_i18n.SetToolchain(toolchain());
  ASSERT_TRUE(base_i18n.OnResolved(&err))
      << err.message() << " " << err.help_text();
  targets.push_back(&base_i18n);

  Target target1(settings(), Label(SourceDir("//foo/"), "bar1"));
  target1.set_output_type(Target::SOURCE_SET);
  target1.public_deps().push_back(LabelTargetPair(&base_target));
  target1.SetToolchain(toolchain());
  ASSERT_TRUE(target1.OnResolved(&err));
  targets.push_back(&target1);

  Target target2(settings(), Label(SourceDir("//foo/"), "bar2"));
  target2.set_output_type(Target::SOURCE_SET);
  target2.public_deps().push_back(LabelTargetPair(&base_i18n));
  target2.SetToolchain(toolchain());
  ASSERT_TRUE(target2.OnResolved(&err));
  targets.push_back(&target2);

  Target random_target(settings(), Label(SourceDir("//random/"), "random"));
  random_target.set_output_type(Target::SOURCE_SET);
  random_target.SetToolchain(toolchain());
  ASSERT_TRUE(random_target.OnResolved(&err));
  targets.push_back(&random_target);

  // Collect everything, the result should match the input.
  const std::string source_root("/home/me/build/");
  LabelPattern wildcard_pattern = LabelPattern::GetPattern(
      SourceDir(), source_root, Value(nullptr, "//*"), &err);
  ASSERT_FALSE(err.has_error());
  std::vector<const Target*> output = CompileCommandsWriter::CollectTargets(
      build_settings(), targets, std::vector<LabelPattern>{wildcard_pattern},
      std::nullopt, &err);
  EXPECT_TRUE(VectorsEqual(output, targets));

  // Collect nothing.
  output = CompileCommandsWriter::CollectTargets(build_settings(), targets,
                                                 std::vector<LabelPattern>(),
                                                 std::nullopt, &err);
  EXPECT_TRUE(output.empty());

  // Collect all deps of "//foo/*".
  LabelPattern foo_wildcard = LabelPattern::GetPattern(
      SourceDir(), source_root, Value(nullptr, "//foo/*"), &err);
  ASSERT_FALSE(err.has_error());
  output = CompileCommandsWriter::CollectTargets(
      build_settings(), targets, std::vector<LabelPattern>{foo_wildcard},
      std::nullopt, &err);

  // The result should be everything except "random".
  std::sort(output.begin(), output.end(), &CompareLabel);
  ASSERT_EQ(5u, output.size());
  EXPECT_EQ(&base_target, output[0]);
  EXPECT_EQ(&base_i18n, output[1]);
  EXPECT_EQ(&target1, output[2]);
  EXPECT_EQ(&target2, output[3]);
  EXPECT_EQ(&icu_target, output[4]);

  // Collect everything using the legacy filter (present string but empty).
  output = CompileCommandsWriter::CollectTargets(build_settings(), targets,
                                                 std::vector<LabelPattern>{},
                                                 std::string(), &err);
  EXPECT_TRUE(VectorsEqual(output, targets));

  // Collect all deps of "bar2" using the legacy filter.
  output = CompileCommandsWriter::CollectTargets(build_settings(), targets,
                                                 std::vector<LabelPattern>{},
                                                 std::string("bar2"), &err);
  std::sort(output.begin(), output.end(), &CompareLabel);
  ASSERT_EQ(4u, output.size());
  EXPECT_EQ(&base_target, output[0]);
  EXPECT_EQ(&base_i18n, output[1]);
  EXPECT_EQ(&target2, output[2]);
  EXPECT_EQ(&icu_target, output[3]);

  // Collect all deps of "bar1" and "bar2" using the legacy filter.
  output = CompileCommandsWriter::CollectTargets(
      build_settings(), targets, std::vector<LabelPattern>{},
      std::string("bar2,bar1"), &err);
  std::sort(output.begin(), output.end(), &CompareLabel);
  ASSERT_EQ(5u, output.size());
  EXPECT_EQ(&base_target, output[0]);
  EXPECT_EQ(&base_i18n, output[1]);
  EXPECT_EQ(&target1, output[2]);
  EXPECT_EQ(&target2, output[3]);
  EXPECT_EQ(&icu_target, output[4]);

  // Combine the legacy (bar1) and pattern (bar2) filters, we should get the
  // union.
  LabelPattern foo_bar2 = LabelPattern::GetPattern(
      SourceDir(), source_root, Value(nullptr, "//foo:bar2"), &err);
  ASSERT_FALSE(err.has_error());
  output = CompileCommandsWriter::CollectTargets(
      build_settings(), targets, std::vector<LabelPattern>{foo_bar2},
      std::string("bar1"), &err);
  std::sort(output.begin(), output.end(), &CompareLabel);
  ASSERT_EQ(5u, output.size());
  EXPECT_EQ(&base_target, output[0]);
  EXPECT_EQ(&base_i18n, output[1]);
  EXPECT_EQ(&target1, output[2]);
  EXPECT_EQ(&target2, output[3]);
  EXPECT_EQ(&icu_target, output[4]);
}
