// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/ninja_c_binary_target_writer.h"

#include <memory>
#include <sstream>
#include <utility>

#include "gn/config.h"
#include "gn/ninja_target_command_util.h"
#include "gn/pool.h"
#include "gn/scheduler.h"
#include "gn/target.h"
#include "gn/test_with_scheduler.h"
#include "gn/test_with_scope.h"
#include "util/build_config.h"
#include "util/test/test.h"

using NinjaCBinaryTargetWriterTest = TestWithScheduler;

TEST_F(NinjaCBinaryTargetWriterTest, SourceSet) {
  Err err;
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::SOURCE_SET);
  target.visibility().SetPublic();
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.sources().push_back(SourceFile("//foo/input2.cc"));
  // Also test object files, which should be just passed through to the
  // dependents to link.
  target.sources().push_back(SourceFile("//foo/input3.o"));
  target.sources().push_back(SourceFile("//foo/input4.obj"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.source_types_used().Set(SourceFile::SOURCE_O);
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  // Source set itself.
  {
    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_cc =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = bar\n"
        "\n"
        "build obj/foo/bar.input1.o: cxx ../../foo/input1.cc\n"
        "  source_file_part = input1.cc\n"
        "  source_name_part = input1\n"
        "build obj/foo/bar.input2.o: cxx ../../foo/input2.cc\n"
        "  source_file_part = input2.cc\n"
        "  source_name_part = input2\n"
        "\n"
        "build obj/foo/bar.stamp: stamp obj/foo/bar.input1.o "
        "obj/foo/bar.input2.o ../../foo/input3.o ../../foo/input4.obj\n";
    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }

  // A shared library that depends on the source set.
  Target shlib_target(setup.settings(), Label(SourceDir("//foo/"), "shlib"));
  shlib_target.set_output_type(Target::SHARED_LIBRARY);
  shlib_target.public_deps().push_back(LabelTargetPair(&target));
  shlib_target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(shlib_target.OnResolved(&err));

  {
    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&shlib_target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = libshlib\n"
        "\n"
        "\n"
        // Ordering of the obj files here should come out in the order
        // specified, with the target's first, followed by the source set's, in
        // order.
        "build ./libshlib.so: solink obj/foo/bar.input1.o "
        "obj/foo/bar.input2.o ../../foo/input3.o ../../foo/input4.obj "
        "|| obj/foo/bar.stamp\n"
        "  ldflags =\n"
        "  libs =\n"
        "  frameworks =\n"
        "  swiftmodules =\n"
        "  output_extension = .so\n"
        "  output_dir = \n";
    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }

  // A static library that depends on the source set (should not link it).
  Target stlib_target(setup.settings(), Label(SourceDir("//foo/"), "stlib"));
  stlib_target.set_output_type(Target::STATIC_LIBRARY);
  stlib_target.public_deps().push_back(LabelTargetPair(&target));
  stlib_target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(stlib_target.OnResolved(&err));

  {
    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&stlib_target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = libstlib\n"
        "\n"
        "\n"
        // There are no sources so there are no params to alink. (In practice
        // this will probably fail in the archive tool.)
        "build obj/foo/libstlib.a: alink || obj/foo/bar.stamp\n"
        "  arflags =\n"
        "  output_extension = \n"
        "  output_dir = \n";
    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }

  // Make the static library 'complete', which means it should be linked.
  stlib_target.set_complete_static_lib(true);
  {
    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&stlib_target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = libstlib\n"
        "\n"
        "\n"
        // Ordering of the obj files here should come out in the order
        // specified, with the target's first, followed by the source set's, in
        // order.
        "build obj/foo/libstlib.a: alink obj/foo/bar.input1.o "
        "obj/foo/bar.input2.o ../../foo/input3.o ../../foo/input4.obj "
        "|| obj/foo/bar.stamp\n"
        "  arflags =\n"
        "  output_extension = \n"
        "  output_dir = \n";
    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }
}

TEST_F(NinjaCBinaryTargetWriterTest, EscapeDefines) {
  TestWithScope setup;
  Err err;

  TestTarget target(setup, "//foo:bar", Target::STATIC_LIBRARY);
  target.config_values().defines().push_back("BOOL_DEF");
  target.config_values().defines().push_back("INT_DEF=123");
  target.config_values().defines().push_back("STR_DEF=\"ABCD-1\"");
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expectedSubstr[] =
#if defined(OS_WIN)
      "defines = -DBOOL_DEF -DINT_DEF=123 \"-DSTR_DEF=\\\"ABCD-1\\\"\"";
#else
      "defines = -DBOOL_DEF -DINT_DEF=123 -DSTR_DEF=\\\"ABCD-1\\\"";
#endif
  std::string out_str = out.str();
  EXPECT_TRUE(out_str.find(expectedSubstr) != std::string::npos);
}

TEST_F(NinjaCBinaryTargetWriterTest, StaticLibrary) {
  TestWithScope setup;
  Err err;

  TestTarget target(setup, "//foo:bar", Target::STATIC_LIBRARY);
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.config_values().arflags().push_back("--asdf");
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libbar\n"
      "\n"
      "build obj/foo/libbar.input1.o: cxx ../../foo/input1.cc\n"
      "  source_file_part = input1.cc\n"
      "  source_name_part = input1\n"
      "\n"
      "build obj/foo/libbar.a: alink obj/foo/libbar.input1.o\n"
      "  arflags = --asdf\n"
      "  output_extension = \n"
      "  output_dir = \n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

TEST_F(NinjaCBinaryTargetWriterTest, CompleteStaticLibrary) {
  TestWithScope setup;
  Err err;

  TestTarget target(setup, "//foo:bar", Target::STATIC_LIBRARY);
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.config_values().arflags().push_back("--asdf");
  target.set_complete_static_lib(true);

  TestTarget baz(setup, "//foo:baz", Target::STATIC_LIBRARY);
  baz.sources().push_back(SourceFile("//foo/input2.cc"));
  baz.source_types_used().Set(SourceFile::SOURCE_CPP);

  target.public_deps().push_back(LabelTargetPair(&baz));

  ASSERT_TRUE(target.OnResolved(&err));
  ASSERT_TRUE(baz.OnResolved(&err));

  // A complete static library that depends on an incomplete static library
  // should link in the dependent object files as if the dependent target
  // were a source set.
  {
    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_cc =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = libbar\n"
        "\n"
        "build obj/foo/libbar.input1.o: cxx ../../foo/input1.cc\n"
        "  source_file_part = input1.cc\n"
        "  source_name_part = input1\n"
        "\n"
        "build obj/foo/libbar.a: alink obj/foo/libbar.input1.o "
        "obj/foo/libbaz.input2.o || obj/foo/libbaz.a\n"
        "  arflags = --asdf\n"
        "  output_extension = \n"
        "  output_dir = \n";
    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }

  // Make the dependent static library complete.
  baz.set_complete_static_lib(true);

  // Dependent complete static libraries should not be linked directly.
  {
    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_cc =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = libbar\n"
        "\n"
        "build obj/foo/libbar.input1.o: cxx ../../foo/input1.cc\n"
        "  source_file_part = input1.cc\n"
        "  source_name_part = input1\n"
        "\n"
        "build obj/foo/libbar.a: alink obj/foo/libbar.input1.o "
        "|| obj/foo/libbaz.a\n"
        "  arflags = --asdf\n"
        "  output_extension = \n"
        "  output_dir = \n";
    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }
}

// This tests that output extension and output dir overrides apply, and input
// dependencies are applied.
TEST_F(NinjaCBinaryTargetWriterTest, OutputExtensionAndInputDeps) {
  Err err;
  TestWithScope setup;

  // An action for our library to depend on.
  Target action(setup.settings(), Label(SourceDir("//foo/"), "action"));
  action.set_output_type(Target::ACTION_FOREACH);
  action.visibility().SetPublic();
  action.SetToolchain(setup.toolchain());
  ASSERT_TRUE(action.OnResolved(&err));

  // A shared library w/ the output_extension set to a custom value.
  Target target(setup.settings(), Label(SourceDir("//foo/"), "shlib"));
  target.set_output_type(Target::SHARED_LIBRARY);
  target.set_output_extension(std::string("so.6"));
  target.set_output_dir(SourceDir("//out/Debug/foo/"));
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.sources().push_back(SourceFile("//foo/input2.cc"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.public_deps().push_back(LabelTargetPair(&action));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libshlib\n"
      "\n"
      "build obj/foo/libshlib.input1.o: cxx ../../foo/input1.cc"
      " || obj/foo/action.stamp\n"
      "  source_file_part = input1.cc\n"
      "  source_name_part = input1\n"
      "build obj/foo/libshlib.input2.o: cxx ../../foo/input2.cc"
      " || obj/foo/action.stamp\n"
      "  source_file_part = input2.cc\n"
      "  source_name_part = input2\n"
      "\n"
      "build ./libshlib.so.6: solink obj/foo/libshlib.input1.o "
      // The order-only dependency here is stricly unnecessary since the
      // sources list this as an order-only dep. See discussion in the code
      // that writes this.
      "obj/foo/libshlib.input2.o || obj/foo/action.stamp\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = .so.6\n"
      "  output_dir = foo\n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

TEST_F(NinjaCBinaryTargetWriterTest, NoHardDepsToNoPublicHeaderTarget) {
  Err err;
  TestWithScope setup;

  SourceFile generated_file("//out/Debug/generated.cc");

  // An action does code generation.
  Target action(setup.settings(), Label(SourceDir("//foo/"), "generate"));
  action.set_output_type(Target::ACTION);
  action.visibility().SetPublic();
  action.SetToolchain(setup.toolchain());
  action.set_output_dir(SourceDir("//out/Debug/foo/"));
  action.action_values().outputs() =
      SubstitutionList::MakeForTest("//out/Debug/generated.cc");
  ASSERT_TRUE(action.OnResolved(&err));

  // A source set compiling generated code, this target does not publicize any
  // headers.
  Target gen_obj(setup.settings(), Label(SourceDir("//foo/"), "gen_obj"));
  gen_obj.set_output_type(Target::SOURCE_SET);
  gen_obj.set_output_dir(SourceDir("//out/Debug/foo/"));
  gen_obj.sources().push_back(generated_file);
  gen_obj.source_types_used().Set(SourceFile::SOURCE_CPP);
  gen_obj.visibility().SetPublic();
  gen_obj.private_deps().push_back(LabelTargetPair(&action));
  gen_obj.set_all_headers_public(false);
  gen_obj.SetToolchain(setup.toolchain());
  ASSERT_TRUE(gen_obj.OnResolved(&err));

  std::ostringstream obj_out;
  NinjaCBinaryTargetWriter obj_writer(&gen_obj, obj_out);
  obj_writer.Run();

  const char obj_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = gen_obj\n"
      "\n"
      "build obj/BUILD_DIR/gen_obj.generated.o: cxx generated.cc"
      " || obj/foo/generate.stamp\n"
      "  source_file_part = generated.cc\n"
      "  source_name_part = generated\n"
      "\n"
      "build obj/foo/gen_obj.stamp: stamp obj/BUILD_DIR/gen_obj.generated.o"
      // The order-only dependency here is strictly unnecessary since the
      // sources list this as an order-only dep.
      " || obj/foo/generate.stamp\n";

  std::string obj_str = obj_out.str();
  EXPECT_EQ(std::string(obj_expected), obj_str);

  // A shared library depends on gen_obj, having corresponding header for
  // generated obj.
  Target gen_lib(setup.settings(), Label(SourceDir("//foo/"), "gen_lib"));
  gen_lib.set_output_type(Target::SHARED_LIBRARY);
  gen_lib.set_output_dir(SourceDir("//out/Debug/foo/"));
  gen_lib.sources().push_back(SourceFile("//foor/generated.h"));
  gen_lib.source_types_used().Set(SourceFile::SOURCE_H);
  gen_lib.visibility().SetPublic();
  gen_lib.private_deps().push_back(LabelTargetPair(&gen_obj));
  gen_lib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(gen_lib.OnResolved(&err));

  std::ostringstream lib_out;
  NinjaCBinaryTargetWriter lib_writer(&gen_lib, lib_out);
  lib_writer.Run();

  const char lib_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libgen_lib\n"
      "\n"
      "\n"
      "build ./libgen_lib.so: solink obj/BUILD_DIR/gen_obj.generated.o"
      // The order-only dependency here is strictly unnecessary since
      // obj/out/Debug/gen_obj.generated.o has dependency to
      // obj/foo/gen_obj.stamp
      " || obj/foo/gen_obj.stamp\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = .so\n"
      "  output_dir = foo\n";

  std::string lib_str = lib_out.str();
  EXPECT_EQ(lib_expected, lib_str);

  // An executable depends on gen_lib.
  Target executable(setup.settings(),
                    Label(SourceDir("//foo/"), "final_target"));
  executable.set_output_type(Target::EXECUTABLE);
  executable.set_output_dir(SourceDir("//out/Debug/foo/"));
  executable.sources().push_back(SourceFile("//foo/main.cc"));
  executable.source_types_used().Set(SourceFile::SOURCE_CPP);
  executable.private_deps().push_back(LabelTargetPair(&gen_lib));
  executable.SetToolchain(setup.toolchain());
  ASSERT_TRUE(executable.OnResolved(&err)) << err.message();

  std::ostringstream final_out;
  NinjaCBinaryTargetWriter final_writer(&executable, final_out);
  final_writer.Run();

  // There is no order only dependency to action target.
  const char final_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = final_target\n"
      "\n"
      "build obj/foo/final_target.main.o: cxx ../../foo/main.cc\n"
      "  source_file_part = main.cc\n"
      "  source_name_part = main\n"
      "\n"
      "build ./final_target: link obj/foo/final_target.main.o"
      " ./libgen_lib.so\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = \n"
      "  output_dir = foo\n";

  std::string final_str = final_out.str();
  EXPECT_EQ(final_expected, final_str);
}

// Tests libs are applied.
TEST_F(NinjaCBinaryTargetWriterTest, LibsAndLibDirs) {
  Err err;
  TestWithScope setup;

  // A shared library w/ libs and lib_dirs.
  Target target(setup.settings(), Label(SourceDir("//foo/"), "shlib"));
  target.set_output_type(Target::SHARED_LIBRARY);
  target.config_values().libs().push_back(LibFile(SourceFile("//foo/lib1.a")));
  target.config_values().libs().push_back(LibFile(SourceFile("//sysroot/DIA SDK/diaguids.lib")));
  target.config_values().libs().push_back(LibFile("foo"));
  target.config_values().lib_dirs().push_back(SourceDir("//foo/bar/"));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libshlib\n"
      "\n"
      "\n"
      "build ./libshlib.so: solink | ../../foo/lib1.a ../../sysroot/DIA$ SDK/diaguids.lib\n"
      "  ldflags = -L../../foo/bar\n"
#ifdef _WIN32
      "  libs = ../../foo/lib1.a \"../../sysroot/DIA$ SDK/diaguids.lib\" -lfoo\n"
#else
      "  libs = ../../foo/lib1.a ../../sysroot/DIA\\$ SDK/diaguids.lib -lfoo\n"
#endif
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = .so\n"
      "  output_dir = \n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

// Tests frameworks are applied.
TEST_F(NinjaCBinaryTargetWriterTest, FrameworksAndFrameworkDirs) {
  Err err;
  TestWithScope setup;

  // A config that force linking with the framework.
  Config framework_config(setup.settings(),
                          Label(SourceDir("//bar"), "framework_config"));
  framework_config.visibility().SetPublic();
  framework_config.own_values().frameworks().push_back("Bar.framework");
  framework_config.own_values().framework_dirs().push_back(
      SourceDir("//out/Debug/"));
  ASSERT_TRUE(framework_config.OnResolved(&err));

  // A target creating a framework bundle.
  Target framework(setup.settings(), Label(SourceDir("//bar"), "framework"));
  framework.set_output_type(Target::CREATE_BUNDLE);
  framework.bundle_data().product_type() = "com.apple.product-type.framework";
  framework.public_configs().push_back(LabelConfigPair(&framework_config));
  framework.SetToolchain(setup.toolchain());
  framework.visibility().SetPublic();
  ASSERT_TRUE(framework.OnResolved(&err));

  // A shared library w/ libs and lib_dirs.
  Target target(setup.settings(), Label(SourceDir("//foo/"), "shlib"));
  target.set_output_type(Target::SHARED_LIBRARY);
  target.config_values().frameworks().push_back("System.framework");
  target.config_values().weak_frameworks().push_back("Whizbang.framework");
  target.private_deps().push_back(LabelTargetPair(&framework));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libshlib\n"
      "\n"
      "\n"
      "build ./libshlib.so: solink | obj/bar/framework.stamp\n"
      "  ldflags = -F.\n"
      "  libs =\n"
      "  frameworks = -framework System -framework Bar "
      "-weak_framework Whizbang\n"
      "  swiftmodules =\n"
      "  output_extension = .so\n"
      "  output_dir = \n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

TEST_F(NinjaCBinaryTargetWriterTest, EmptyOutputExtension) {
  Err err;
  TestWithScope setup;

  // This test is the same as OutputExtensionAndInputDeps, except that we call
  // set_output_extension("") and ensure that we get an empty one and override
  // the output prefix so that the name matches the target exactly.
  Target target(setup.settings(), Label(SourceDir("//foo/"), "shlib"));
  target.set_output_type(Target::SHARED_LIBRARY);
  target.set_output_prefix_override(true);
  target.set_output_extension(std::string());
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.sources().push_back(SourceFile("//foo/input2.cc"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);

  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = shlib\n"
      "\n"
      "build obj/foo/shlib.input1.o: cxx ../../foo/input1.cc\n"
      "  source_file_part = input1.cc\n"
      "  source_name_part = input1\n"
      "build obj/foo/shlib.input2.o: cxx ../../foo/input2.cc\n"
      "  source_file_part = input2.cc\n"
      "  source_name_part = input2\n"
      "\n"
      "build ./shlib: solink obj/foo/shlib.input1.o "
      "obj/foo/shlib.input2.o\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = \n"
      "  output_dir = \n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

TEST_F(NinjaCBinaryTargetWriterTest, SourceSetDataDeps) {
  Err err;
  TestWithScope setup;

  // This target is a data (runtime) dependency of the intermediate target.
  Target data(setup.settings(), Label(SourceDir("//foo/"), "data_target"));
  data.set_output_type(Target::EXECUTABLE);
  data.visibility().SetPublic();
  data.SetToolchain(setup.toolchain());
  ASSERT_TRUE(data.OnResolved(&err));

  // Intermediate source set target.
  Target inter(setup.settings(), Label(SourceDir("//foo/"), "inter"));
  inter.set_output_type(Target::SOURCE_SET);
  inter.visibility().SetPublic();
  inter.data_deps().push_back(LabelTargetPair(&data));
  inter.SetToolchain(setup.toolchain());
  inter.sources().push_back(SourceFile("//foo/inter.cc"));
  inter.source_types_used().Set(SourceFile::SOURCE_CPP);
  ASSERT_TRUE(inter.OnResolved(&err)) << err.message();

  // Write out the intermediate target.
  std::ostringstream inter_out;
  NinjaCBinaryTargetWriter inter_writer(&inter, inter_out);
  inter_writer.Run();

  // The intermediate source set will be a stamp file that depends on the
  // object files, and will have an order-only dependency on its data dep and
  // data file.
  const char inter_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = inter\n"
      "\n"
      "build obj/foo/inter.inter.o: cxx ../../foo/inter.cc\n"
      "  source_file_part = inter.cc\n"
      "  source_name_part = inter\n"
      "\n"
      "build obj/foo/inter.stamp: stamp obj/foo/inter.inter.o || "
      "./data_target\n";
  EXPECT_EQ(inter_expected, inter_out.str());

  // Final target.
  Target exe(setup.settings(), Label(SourceDir("//foo/"), "exe"));
  exe.set_output_type(Target::EXECUTABLE);
  exe.public_deps().push_back(LabelTargetPair(&inter));
  exe.SetToolchain(setup.toolchain());
  exe.sources().push_back(SourceFile("//foo/final.cc"));
  exe.source_types_used().Set(SourceFile::SOURCE_CPP);
  ASSERT_TRUE(exe.OnResolved(&err));

  std::ostringstream final_out;
  NinjaCBinaryTargetWriter final_writer(&exe, final_out);
  final_writer.Run();

  // The final output depends on both object files (one from the final target,
  // one from the source set) and has an order-only dependency on the source
  // set's stamp file and the final target's data file. The source set stamp
  // dependency will create an implicit order-only dependency on the data
  // target.
  const char final_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = exe\n"
      "\n"
      "build obj/foo/exe.final.o: cxx ../../foo/final.cc\n"
      "  source_file_part = final.cc\n"
      "  source_name_part = final\n"
      "\n"
      "build ./exe: link obj/foo/exe.final.o obj/foo/inter.inter.o || "
      "obj/foo/inter.stamp\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = \n"
      "  output_dir = \n";
  EXPECT_EQ(final_expected, final_out.str());
}

TEST_F(NinjaCBinaryTargetWriterTest, SharedLibraryModuleDefinitionFile) {
  Err err;
  TestWithScope setup;

  Target shared_lib(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  shared_lib.set_output_type(Target::SHARED_LIBRARY);
  shared_lib.SetToolchain(setup.toolchain());
  shared_lib.sources().push_back(SourceFile("//foo/sources.cc"));
  shared_lib.sources().push_back(SourceFile("//foo/bar.def"));
  shared_lib.source_types_used().Set(SourceFile::SOURCE_CPP);
  shared_lib.source_types_used().Set(SourceFile::SOURCE_DEF);
  ASSERT_TRUE(shared_lib.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&shared_lib, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libbar\n"
      "\n"
      "build obj/foo/libbar.sources.o: cxx ../../foo/sources.cc\n"
      "  source_file_part = sources.cc\n"
      "  source_name_part = sources\n"
      "\n"
      "build ./libbar.so: solink obj/foo/libbar.sources.o | ../../foo/bar.def\n"
      "  ldflags = /DEF:../../foo/bar.def\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = .so\n"
      "  output_dir = \n";
  EXPECT_EQ(expected, out.str());
}

TEST_F(NinjaCBinaryTargetWriterTest, LoadableModule) {
  Err err;
  TestWithScope setup;

  Target loadable_module(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  loadable_module.set_output_type(Target::LOADABLE_MODULE);
  loadable_module.visibility().SetPublic();
  loadable_module.SetToolchain(setup.toolchain());
  loadable_module.sources().push_back(SourceFile("//foo/sources.cc"));
  loadable_module.source_types_used().Set(SourceFile::SOURCE_CPP);
  ASSERT_TRUE(loadable_module.OnResolved(&err)) << err.message();

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&loadable_module, out);
  writer.Run();

  const char loadable_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libbar\n"
      "\n"
      "build obj/foo/libbar.sources.o: cxx ../../foo/sources.cc\n"
      "  source_file_part = sources.cc\n"
      "  source_name_part = sources\n"
      "\n"
      "build ./libbar.so: solink_module obj/foo/libbar.sources.o\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = .so\n"
      "  output_dir = \n";
  EXPECT_EQ(loadable_expected, out.str());

  // Final target.
  Target exe(setup.settings(), Label(SourceDir("//foo/"), "exe"));
  exe.set_output_type(Target::EXECUTABLE);
  exe.public_deps().push_back(LabelTargetPair(&loadable_module));
  exe.SetToolchain(setup.toolchain());
  exe.sources().push_back(SourceFile("//foo/final.cc"));
  exe.source_types_used().Set(SourceFile::SOURCE_CPP);
  ASSERT_TRUE(exe.OnResolved(&err)) << err.message();

  std::ostringstream final_out;
  NinjaCBinaryTargetWriter final_writer(&exe, final_out);
  final_writer.Run();

  // The final output depends on the loadable module so should have an
  // order-only dependency on the loadable modules's output file.
  const char final_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = exe\n"
      "\n"
      "build obj/foo/exe.final.o: cxx ../../foo/final.cc\n"
      "  source_file_part = final.cc\n"
      "  source_name_part = final\n"
      "\n"
      "build ./exe: link obj/foo/exe.final.o || ./libbar.so\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = \n"
      "  output_dir = \n";
  EXPECT_EQ(final_expected, final_out.str());
}

TEST_F(NinjaCBinaryTargetWriterTest, WinPrecompiledHeaders) {
  Err err;

  // This setup's toolchain does not have precompiled headers defined.
  TestWithScope setup;

  // A precompiled header toolchain.
  Settings pch_settings(setup.build_settings(), "withpch/");
  Toolchain pch_toolchain(&pch_settings,
                          Label(SourceDir("//toolchain/"), "withpch"));
  pch_settings.set_toolchain_label(pch_toolchain.label());
  pch_settings.set_default_toolchain_label(setup.toolchain()->label());

  // Declare a C++ compiler that supports PCH.
  std::unique_ptr<Tool> cxx = std::make_unique<CTool>(CTool::kCToolCxx);
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
  std::unique_ptr<Tool> cc = std::make_unique<CTool>(CTool::kCToolCc);
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
    Target no_pch_target(&pch_settings,
                         Label(SourceDir("//foo/"), "no_pch_target"));
    no_pch_target.set_output_type(Target::SOURCE_SET);
    no_pch_target.visibility().SetPublic();
    no_pch_target.sources().push_back(SourceFile("//foo/input1.cc"));
    no_pch_target.sources().push_back(SourceFile("//foo/input2.c"));
    no_pch_target.source_types_used().Set(SourceFile::SOURCE_CPP);
    no_pch_target.source_types_used().Set(SourceFile::SOURCE_C);
    no_pch_target.config_values().cflags_c().push_back("-std=c99");
    no_pch_target.SetToolchain(&pch_toolchain);
    ASSERT_TRUE(no_pch_target.OnResolved(&err));

    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&no_pch_target, out);
    writer.Run();

    const char no_pch_expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_c = -std=c99\n"
        "cflags_cc =\n"
        "target_output_name = no_pch_target\n"
        "\n"
        "build withpch/obj/foo/no_pch_target.input1.o: "
        "withpch_cxx ../../foo/input1.cc\n"
        "  source_file_part = input1.cc\n"
        "  source_name_part = input1\n"
        "build withpch/obj/foo/no_pch_target.input2.o: "
        "withpch_cc ../../foo/input2.c\n"
        "  source_file_part = input2.c\n"
        "  source_name_part = input2\n"
        "\n"
        "build withpch/obj/foo/no_pch_target.stamp: "
        "withpch_stamp withpch/obj/foo/no_pch_target.input1.o "
        "withpch/obj/foo/no_pch_target.input2.o\n";
    EXPECT_EQ(no_pch_expected, out.str());
  }

  // This target specifies PCH.
  {
    Target pch_target(&pch_settings, Label(SourceDir("//foo/"), "pch_target"));
    pch_target.config_values().set_precompiled_header("build/precompile.h");
    pch_target.config_values().set_precompiled_source(
        SourceFile("//build/precompile.cc"));
    pch_target.set_output_type(Target::SOURCE_SET);
    pch_target.visibility().SetPublic();
    pch_target.sources().push_back(SourceFile("//foo/input1.cc"));
    pch_target.sources().push_back(SourceFile("//foo/input2.c"));
    pch_target.source_types_used().Set(SourceFile::SOURCE_CPP);
    pch_target.source_types_used().Set(SourceFile::SOURCE_C);
    pch_target.SetToolchain(&pch_toolchain);
    ASSERT_TRUE(pch_target.OnResolved(&err));

    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&pch_target, out);
    writer.Run();

    const char pch_win_expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        // It should output language-specific pch files.
        "cflags_c = /Fpwithpch/obj/foo/pch_target_c.pch "
        "/Yubuild/precompile.h\n"
        "cflags_cc = /Fpwithpch/obj/foo/pch_target_cc.pch "
        "/Yubuild/precompile.h\n"
        "target_output_name = pch_target\n"
        "\n"
        // Compile the precompiled source files with /Yc.
        "build withpch/obj/build/pch_target.precompile.c.o: "
        "withpch_cc ../../build/precompile.cc\n"
        "  source_file_part = precompile.cc\n"
        "  source_name_part = precompile\n"
        "  cflags_c = ${cflags_c} /Ycbuild/precompile.h\n"
        "\n"
        "build withpch/obj/build/pch_target.precompile.cc.o: "
        "withpch_cxx ../../build/precompile.cc\n"
        "  source_file_part = precompile.cc\n"
        "  source_name_part = precompile\n"
        "  cflags_cc = ${cflags_cc} /Ycbuild/precompile.h\n"
        "\n"
        "build withpch/obj/foo/pch_target.input1.o: "
        "withpch_cxx ../../foo/input1.cc | "
        // Explicit dependency on the PCH build step.
        "withpch/obj/build/pch_target.precompile.cc.o\n"
        "  source_file_part = input1.cc\n"
        "  source_name_part = input1\n"
        "build withpch/obj/foo/pch_target.input2.o: "
        "withpch_cc ../../foo/input2.c | "
        // Explicit dependency on the PCH build step.
        "withpch/obj/build/pch_target.precompile.c.o\n"
        "  source_file_part = input2.c\n"
        "  source_name_part = input2\n"
        "\n"
        "build withpch/obj/foo/pch_target.stamp: withpch_stamp "
        "withpch/obj/foo/pch_target.input1.o "
        "withpch/obj/foo/pch_target.input2.o "
        // The precompiled object files were added to the outputs.
        "withpch/obj/build/pch_target.precompile.c.o "
        "withpch/obj/build/pch_target.precompile.cc.o\n";
    EXPECT_EQ(pch_win_expected, out.str());
  }
}

TEST_F(NinjaCBinaryTargetWriterTest, GCCPrecompiledHeaders) {
  Err err;

  // This setup's toolchain does not have precompiled headers defined.
  TestWithScope setup;

  // A precompiled header toolchain.
  Settings pch_settings(setup.build_settings(), "withpch/");
  Toolchain pch_toolchain(&pch_settings,
                          Label(SourceDir("//toolchain/"), "withpch"));
  pch_settings.set_toolchain_label(pch_toolchain.label());
  pch_settings.set_default_toolchain_label(setup.toolchain()->label());

  // Declare a C++ compiler that supports PCH.
  std::unique_ptr<Tool> cxx = std::make_unique<CTool>(CTool::kCToolCxx);
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
  std::unique_ptr<Tool> cc = std::make_unique<CTool>(CTool::kCToolCc);
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
    Target no_pch_target(&pch_settings,
                         Label(SourceDir("//foo/"), "no_pch_target"));
    no_pch_target.set_output_type(Target::SOURCE_SET);
    no_pch_target.visibility().SetPublic();
    no_pch_target.sources().push_back(SourceFile("//foo/input1.cc"));
    no_pch_target.sources().push_back(SourceFile("//foo/input2.c"));
    no_pch_target.source_types_used().Set(SourceFile::SOURCE_CPP);
    no_pch_target.source_types_used().Set(SourceFile::SOURCE_C);
    no_pch_target.config_values().cflags_c().push_back("-std=c99");
    no_pch_target.SetToolchain(&pch_toolchain);
    ASSERT_TRUE(no_pch_target.OnResolved(&err));

    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&no_pch_target, out);
    writer.Run();

    const char no_pch_expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_c = -std=c99\n"
        "cflags_cc =\n"
        "target_output_name = no_pch_target\n"
        "\n"
        "build withpch/obj/foo/no_pch_target.input1.o: "
        "withpch_cxx ../../foo/input1.cc\n"
        "  source_file_part = input1.cc\n"
        "  source_name_part = input1\n"
        "build withpch/obj/foo/no_pch_target.input2.o: "
        "withpch_cc ../../foo/input2.c\n"
        "  source_file_part = input2.c\n"
        "  source_name_part = input2\n"
        "\n"
        "build withpch/obj/foo/no_pch_target.stamp: "
        "withpch_stamp withpch/obj/foo/no_pch_target.input1.o "
        "withpch/obj/foo/no_pch_target.input2.o\n";
    EXPECT_EQ(no_pch_expected, out.str());
  }

  // This target specifies PCH.
  {
    Target pch_target(&pch_settings, Label(SourceDir("//foo/"), "pch_target"));
    pch_target.config_values().set_precompiled_source(
        SourceFile("//build/precompile.h"));
    pch_target.config_values().cflags_c().push_back("-std=c99");
    pch_target.set_output_type(Target::SOURCE_SET);
    pch_target.visibility().SetPublic();
    pch_target.sources().push_back(SourceFile("//foo/input1.cc"));
    pch_target.sources().push_back(SourceFile("//foo/input2.c"));
    pch_target.source_types_used().Set(SourceFile::SOURCE_CPP);
    pch_target.source_types_used().Set(SourceFile::SOURCE_C);
    pch_target.SetToolchain(&pch_toolchain);
    ASSERT_TRUE(pch_target.OnResolved(&err));

    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&pch_target, out);
    writer.Run();

    const char pch_gcc_expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_c = -std=c99 "
        "-include withpch/obj/build/pch_target.precompile.h-c\n"
        "cflags_cc = -include withpch/obj/build/pch_target.precompile.h-cc\n"
        "target_output_name = pch_target\n"
        "\n"
        // Compile the precompiled sources with -x <lang>.
        "build withpch/obj/build/pch_target.precompile.h-c.gch: "
        "withpch_cc ../../build/precompile.h\n"
        "  source_file_part = precompile.h\n"
        "  source_name_part = precompile\n"
        "  cflags_c = -std=c99 -x c-header\n"
        "\n"
        "build withpch/obj/build/pch_target.precompile.h-cc.gch: "
        "withpch_cxx ../../build/precompile.h\n"
        "  source_file_part = precompile.h\n"
        "  source_name_part = precompile\n"
        "  cflags_cc = -x c++-header\n"
        "\n"
        "build withpch/obj/foo/pch_target.input1.o: "
        "withpch_cxx ../../foo/input1.cc | "
        // Explicit dependency on the PCH build step.
        "withpch/obj/build/pch_target.precompile.h-cc.gch\n"
        "  source_file_part = input1.cc\n"
        "  source_name_part = input1\n"
        "build withpch/obj/foo/pch_target.input2.o: "
        "withpch_cc ../../foo/input2.c | "
        // Explicit dependency on the PCH build step.
        "withpch/obj/build/pch_target.precompile.h-c.gch\n"
        "  source_file_part = input2.c\n"
        "  source_name_part = input2\n"
        "\n"
        "build withpch/obj/foo/pch_target.stamp: "
        "withpch_stamp withpch/obj/foo/pch_target.input1.o "
        "withpch/obj/foo/pch_target.input2.o\n";
    EXPECT_EQ(pch_gcc_expected, out.str());
  }
}

// Should throw an error with the scheduler if a duplicate object file exists.
// This is dependent on the toolchain's object file mapping.
TEST_F(NinjaCBinaryTargetWriterTest, DupeObjFileError) {
  TestWithScope setup;
  TestTarget target(setup, "//foo:bar", Target::EXECUTABLE);
  target.sources().push_back(SourceFile("//a.cc"));
  target.sources().push_back(SourceFile("//a.cc"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);

  EXPECT_FALSE(scheduler().is_failed());

  scheduler().SuppressOutputForTesting(true);

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  scheduler().SuppressOutputForTesting(false);

  // Should have issued an error.
  EXPECT_TRUE(scheduler().is_failed());
}

// This tests that output extension and output dir overrides apply, and input
// dependencies are applied.
TEST_F(NinjaCBinaryTargetWriterTest, InputFiles) {
  Err err;
  TestWithScope setup;

  // This target has one input.
  {
    Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
    target.set_output_type(Target::SOURCE_SET);
    target.visibility().SetPublic();
    target.sources().push_back(SourceFile("//foo/input1.cc"));
    target.sources().push_back(SourceFile("//foo/input2.cc"));
    target.source_types_used().Set(SourceFile::SOURCE_CPP);
    target.config_values().inputs().push_back(SourceFile("//foo/input.data"));
    target.SetToolchain(setup.toolchain());
    ASSERT_TRUE(target.OnResolved(&err));

    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_cc =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = bar\n"
        "\n"
        "build obj/foo/bar.input1.o: cxx ../../foo/input1.cc"
        " | ../../foo/input.data\n"
        "  source_file_part = input1.cc\n"
        "  source_name_part = input1\n"
        "build obj/foo/bar.input2.o: cxx ../../foo/input2.cc"
        " | ../../foo/input.data\n"
        "  source_file_part = input2.cc\n"
        "  source_name_part = input2\n"
        "\n"
        "build obj/foo/bar.stamp: stamp obj/foo/bar.input1.o "
        "obj/foo/bar.input2.o\n";

    EXPECT_EQ(expected, out.str());
  }

  // This target has one input but no source files.
  {
    Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
    target.set_output_type(Target::SHARED_LIBRARY);
    target.visibility().SetPublic();
    target.config_values().inputs().push_back(SourceFile("//foo/input.data"));
    target.SetToolchain(setup.toolchain());
    ASSERT_TRUE(target.OnResolved(&err));

    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = libbar\n"
        "\n"
        "\n"
        "build ./libbar.so: solink | ../../foo/input.data\n"
        "  ldflags =\n"
        "  libs =\n"
        "  frameworks =\n"
        "  swiftmodules =\n"
        "  output_extension = .so\n"
        "  output_dir = \n";

    EXPECT_EQ(expected, out.str());
  }

  // This target has multiple inputs.
  {
    Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
    target.set_output_type(Target::SOURCE_SET);
    target.visibility().SetPublic();
    target.sources().push_back(SourceFile("//foo/input1.cc"));
    target.sources().push_back(SourceFile("//foo/input2.cc"));
    target.source_types_used().Set(SourceFile::SOURCE_CPP);
    target.config_values().inputs().push_back(SourceFile("//foo/input1.data"));
    target.config_values().inputs().push_back(SourceFile("//foo/input2.data"));
    target.SetToolchain(setup.toolchain());
    ASSERT_TRUE(target.OnResolved(&err));

    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_cc =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = bar\n"
        "\n"
        "build obj/foo/bar.inputs.stamp: stamp"
        " ../../foo/input1.data ../../foo/input2.data\n"
        "build obj/foo/bar.input1.o: cxx ../../foo/input1.cc"
        " | obj/foo/bar.inputs.stamp\n"
        "  source_file_part = input1.cc\n"
        "  source_name_part = input1\n"
        "build obj/foo/bar.input2.o: cxx ../../foo/input2.cc"
        " | obj/foo/bar.inputs.stamp\n"
        "  source_file_part = input2.cc\n"
        "  source_name_part = input2\n"
        "\n"
        "build obj/foo/bar.stamp: stamp obj/foo/bar.input1.o "
        "obj/foo/bar.input2.o\n";

    EXPECT_EQ(expected, out.str());
  }

  // This target has one input itself, one from an immediate config, and one
  // from a config tacked on to said config.
  {
    Config far_config(setup.settings(), Label(SourceDir("//foo/"), "qux"));
    far_config.own_values().inputs().push_back(SourceFile("//foo/input3.data"));
    ASSERT_TRUE(far_config.OnResolved(&err));

    Config config(setup.settings(), Label(SourceDir("//foo/"), "baz"));
    config.visibility().SetPublic();
    config.own_values().inputs().push_back(SourceFile("//foo/input2.data"));
    config.configs().push_back(LabelConfigPair(&far_config));
    ASSERT_TRUE(config.OnResolved(&err));

    Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
    target.set_output_type(Target::SOURCE_SET);
    target.visibility().SetPublic();
    target.sources().push_back(SourceFile("//foo/input1.cc"));
    target.sources().push_back(SourceFile("//foo/input2.cc"));
    target.source_types_used().Set(SourceFile::SOURCE_CPP);
    target.config_values().inputs().push_back(SourceFile("//foo/input1.data"));
    target.configs().push_back(LabelConfigPair(&config));
    target.SetToolchain(setup.toolchain());
    ASSERT_TRUE(target.OnResolved(&err));

    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_cc =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = bar\n"
        "\n"
        "build obj/foo/bar.inputs.stamp: stamp"
        " ../../foo/input1.data ../../foo/input2.data ../../foo/input3.data\n"
        "build obj/foo/bar.input1.o: cxx ../../foo/input1.cc"
        " | obj/foo/bar.inputs.stamp\n"
        "  source_file_part = input1.cc\n"
        "  source_name_part = input1\n"
        "build obj/foo/bar.input2.o: cxx ../../foo/input2.cc"
        " | obj/foo/bar.inputs.stamp\n"
        "  source_file_part = input2.cc\n"
        "  source_name_part = input2\n"
        "\n"
        "build obj/foo/bar.stamp: stamp obj/foo/bar.input1.o "
        "obj/foo/bar.input2.o\n";

    EXPECT_EQ(expected, out.str());
  }
}

// Test linking of Rust dependencies into C targets.
TEST_F(NinjaCBinaryTargetWriterTest, RustStaticLib) {
  Err err;
  TestWithScope setup;

  Target library_target(setup.settings(), Label(SourceDir("//foo/"), "foo"));
  library_target.set_output_type(Target::STATIC_LIBRARY);
  library_target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  library_target.sources().push_back(lib);
  library_target.source_types_used().Set(SourceFile::SOURCE_RS);
  library_target.rust_values().set_crate_root(lib);
  library_target.rust_values().crate_name() = "foo";
  library_target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(library_target.OnResolved(&err));

  Target target(setup.settings(), Label(SourceDir("//bar/"), "bar"));
  target.set_output_type(Target::EXECUTABLE);
  target.visibility().SetPublic();
  target.sources().push_back(SourceFile("//bar/bar.cc"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.private_deps().push_back(LabelTargetPair(&library_target));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/bar\n"
      "target_output_name = bar\n"
      "\n"
      "build obj/bar/bar.bar.o: cxx ../../bar/bar.cc\n"
      "  source_file_part = bar.cc\n"
      "  source_name_part = bar\n"
      "\n"
      "build ./bar: link obj/bar/bar.bar.o obj/foo/libfoo.a\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = \n"
      "  output_dir = \n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

// Test linking of Rust dependencies into C targets.
TEST_F(NinjaCBinaryTargetWriterTest, RlibInLibrary) {
  Err err;
  TestWithScope setup;

  // This source_set() is depended on by an rlib, which is a private dep of a
  // static lib.
  Target priv_sset_in_staticlib(
      setup.settings(),
      Label(SourceDir("//priv_sset_in_staticlib/"), "priv_sset_in_staticlib"));
  priv_sset_in_staticlib.set_output_type(Target::SOURCE_SET);
  priv_sset_in_staticlib.visibility().SetPublic();
  priv_sset_in_staticlib.sources().push_back(
      SourceFile("//priv_sset_in_staticlib/lib.cc"));
  priv_sset_in_staticlib.source_types_used().Set(SourceFile::SOURCE_CPP);
  priv_sset_in_staticlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(priv_sset_in_staticlib.OnResolved(&err));

  // This source_set() is depended on by an rlib, which is a public dep of a
  // static lib.
  Target pub_sset_in_staticlib(
      setup.settings(),
      Label(SourceDir("//pub_sset_in_staticlib/"), "pub_sset_in_staticlib"));
  pub_sset_in_staticlib.set_output_type(Target::SOURCE_SET);
  pub_sset_in_staticlib.visibility().SetPublic();
  pub_sset_in_staticlib.sources().push_back(
      SourceFile("//pub_sset_in_staticlib/lib.cc"));
  pub_sset_in_staticlib.source_types_used().Set(SourceFile::SOURCE_CPP);
  pub_sset_in_staticlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(pub_sset_in_staticlib.OnResolved(&err));

  // This source_set() is depended on by an rlib, which is a private dep of a
  // shared lib.
  Target priv_sset_in_dylib(
      setup.settings(),
      Label(SourceDir("//priv_sset_in_dylib/"), "priv_sset_in_dylib"));
  priv_sset_in_dylib.set_output_type(Target::SOURCE_SET);
  priv_sset_in_dylib.visibility().SetPublic();
  priv_sset_in_dylib.sources().push_back(
      SourceFile("//priv_sset_in_dylib/lib.cc"));
  priv_sset_in_dylib.source_types_used().Set(SourceFile::SOURCE_CPP);
  priv_sset_in_dylib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(priv_sset_in_dylib.OnResolved(&err));

  // This source_set() is depended on by an rlib, which is a public dep of a
  // shared lib.
  Target pub_sset_in_dylib(
      setup.settings(),
      Label(SourceDir("//pub_sset_in_dylib"), "pub_sset_in_dylib"));
  pub_sset_in_dylib.set_output_type(Target::SOURCE_SET);
  pub_sset_in_dylib.visibility().SetPublic();
  pub_sset_in_dylib.sources().push_back(
      SourceFile("//pub_sset_in_dylib/lib.cc"));
  pub_sset_in_dylib.source_types_used().Set(SourceFile::SOURCE_CPP);
  pub_sset_in_dylib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(pub_sset_in_dylib.OnResolved(&err));

  Target priv_in_staticlib(
      setup.settings(),
      Label(SourceDir("//priv_in_staticlib/"), "priv_in_staticlib"));
  priv_in_staticlib.set_output_type(Target::RUST_LIBRARY);
  priv_in_staticlib.visibility().SetPublic();
  SourceFile priv_in_staticlib_root("//priv_in_staticlib/lib.rs");
  priv_in_staticlib.sources().push_back(priv_in_staticlib_root);
  priv_in_staticlib.source_types_used().Set(SourceFile::SOURCE_RS);
  priv_in_staticlib.rust_values().set_crate_root(priv_in_staticlib_root);
  priv_in_staticlib.rust_values().crate_name() = "priv_in_staticlib";
  priv_in_staticlib.SetToolchain(setup.toolchain());
  priv_in_staticlib.private_deps().push_back(
      LabelTargetPair(&priv_sset_in_staticlib));
  ASSERT_TRUE(priv_in_staticlib.OnResolved(&err));

  Target pub_in_staticlib(
      setup.settings(),
      Label(SourceDir("//pub_in_staticlib/"), "pub_in_staticlib"));
  pub_in_staticlib.set_output_type(Target::RUST_LIBRARY);
  pub_in_staticlib.visibility().SetPublic();
  SourceFile pub_in_staticlib_root("//pub_in_staticlib/lib.rs");
  pub_in_staticlib.sources().push_back(pub_in_staticlib_root);
  pub_in_staticlib.source_types_used().Set(SourceFile::SOURCE_RS);
  pub_in_staticlib.rust_values().set_crate_root(pub_in_staticlib_root);
  pub_in_staticlib.rust_values().crate_name() = "pub_in_staticlib";
  pub_in_staticlib.SetToolchain(setup.toolchain());
  pub_in_staticlib.private_deps().push_back(
      LabelTargetPair(&pub_sset_in_staticlib));
  ASSERT_TRUE(pub_in_staticlib.OnResolved(&err));

  Target priv_in_dylib(setup.settings(),
                       Label(SourceDir("//priv_in_dylib/"), "priv_in_dylib"));
  priv_in_dylib.set_output_type(Target::RUST_LIBRARY);
  priv_in_dylib.visibility().SetPublic();
  SourceFile priv_in_dylib_root("//priv_in_dylib/lib.rs");
  priv_in_dylib.sources().push_back(priv_in_dylib_root);
  priv_in_dylib.source_types_used().Set(SourceFile::SOURCE_RS);
  priv_in_dylib.rust_values().set_crate_root(priv_in_dylib_root);
  priv_in_dylib.rust_values().crate_name() = "priv_in_dylib";
  priv_in_dylib.SetToolchain(setup.toolchain());
  priv_in_dylib.private_deps().push_back(LabelTargetPair(&priv_sset_in_dylib));
  ASSERT_TRUE(priv_in_dylib.OnResolved(&err));

  Target pub_in_dylib(setup.settings(),
                      Label(SourceDir("//pub_in_dylib/"), "pub_in_dylib"));
  pub_in_dylib.set_output_type(Target::RUST_LIBRARY);
  pub_in_dylib.visibility().SetPublic();
  SourceFile pub_in_dylib_root("//pub_in_dylib/lib.rs");
  pub_in_dylib.sources().push_back(pub_in_dylib_root);
  pub_in_dylib.source_types_used().Set(SourceFile::SOURCE_RS);
  pub_in_dylib.rust_values().set_crate_root(pub_in_dylib_root);
  pub_in_dylib.rust_values().crate_name() = "pub_in_dylib";
  pub_in_dylib.SetToolchain(setup.toolchain());
  pub_in_dylib.private_deps().push_back(LabelTargetPair(&pub_sset_in_dylib));
  ASSERT_TRUE(pub_in_dylib.OnResolved(&err));

  Target staticlib(setup.settings(),
                   Label(SourceDir("//staticlib/"), "staticlib"));
  staticlib.set_output_type(Target::STATIC_LIBRARY);
  staticlib.visibility().SetPublic();
  staticlib.sources().push_back(SourceFile("//staticlib/lib.cc"));
  staticlib.source_types_used().Set(SourceFile::SOURCE_CPP);
  staticlib.public_deps().push_back(LabelTargetPair(&pub_in_staticlib));
  staticlib.private_deps().push_back(LabelTargetPair(&priv_in_staticlib));
  staticlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(staticlib.OnResolved(&err));

  Target dylib(setup.settings(), Label(SourceDir("//dylib/"), "dylib"));
  dylib.set_output_type(Target::SHARED_LIBRARY);
  dylib.visibility().SetPublic();
  SourceFile dylib_root("//dylib/lib.rs");
  dylib.sources().push_back(dylib_root);
  dylib.source_types_used().Set(SourceFile::SOURCE_RS);
  dylib.rust_values().set_crate_root(dylib_root);
  dylib.rust_values().crate_name() = "dylib";
  dylib.public_deps().push_back(LabelTargetPair(&pub_in_dylib));
  dylib.private_deps().push_back(LabelTargetPair(&priv_in_dylib));
  dylib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(dylib.OnResolved(&err));

  Target target(setup.settings(), Label(SourceDir("//exe/"), "exe"));
  target.set_output_type(Target::EXECUTABLE);
  target.visibility().SetPublic();
  target.sources().push_back(SourceFile("//exe/main.cc"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.private_deps().push_back(LabelTargetPair(&staticlib));
  target.private_deps().push_back(LabelTargetPair(&dylib));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));


  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/exe\n"
      "target_output_name = exe\n"
      "\n"
      "build obj/exe/exe.main.o: cxx ../../exe/main.cc\n"
      "  source_file_part = main.cc\n"
      "  source_name_part = main\n"
      "\n"
      "build ./exe: link obj/exe/exe.main.o "
      "obj/pub_sset_in_staticlib/pub_sset_in_staticlib.lib.o "
      "obj/priv_sset_in_staticlib/priv_sset_in_staticlib.lib.o "
      "obj/staticlib/libstaticlib.a "
      "obj/dylib/libdylib.so | "
      "obj/pub_in_staticlib/libpub_in_staticlib.rlib "
      "obj/priv_in_staticlib/libpriv_in_staticlib.rlib || "
      "obj/pub_sset_in_staticlib/pub_sset_in_staticlib.stamp "
      "obj/priv_sset_in_staticlib/priv_sset_in_staticlib.stamp\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = \n"
      "  output_dir = \n"
      "  rlibs = obj/pub_in_staticlib/libpub_in_staticlib.rlib "
      "obj/priv_in_staticlib/libpriv_in_staticlib.rlib\n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

// Test linking of Rust dependencies into C targets. Proc-macro dependencies are
// not inherited by the targets that depend on them, even from public_deps,
// since they are not built into those targets, but instead used to build them.
TEST_F(NinjaCBinaryTargetWriterTest, RlibsWithProcMacros) {
  Err err;
  TestWithScope setup;

  Target pub_in_staticlib(
      setup.settings(),
      Label(SourceDir("//pub_in_staticlib/"), "pub_in_staticlib"));
  pub_in_staticlib.set_output_type(Target::RUST_LIBRARY);
  pub_in_staticlib.visibility().SetPublic();
  SourceFile pub_in_staticlib_root("//pub_in_staticlib/lib.rs");
  pub_in_staticlib.sources().push_back(pub_in_staticlib_root);
  pub_in_staticlib.source_types_used().Set(SourceFile::SOURCE_RS);
  pub_in_staticlib.rust_values().set_crate_root(pub_in_staticlib_root);
  pub_in_staticlib.rust_values().crate_name() = "pub_in_staticlib";
  pub_in_staticlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(pub_in_staticlib.OnResolved(&err));

  Target priv_in_staticlib(
      setup.settings(),
      Label(SourceDir("//priv_in_staticlib/"), "priv_in_staticlib"));
  priv_in_staticlib.set_output_type(Target::RUST_LIBRARY);
  priv_in_staticlib.visibility().SetPublic();
  SourceFile priv_in_staticlib_root("//priv_in_staticlib/lib.rs");
  priv_in_staticlib.sources().push_back(priv_in_staticlib_root);
  priv_in_staticlib.source_types_used().Set(SourceFile::SOURCE_RS);
  priv_in_staticlib.rust_values().set_crate_root(priv_in_staticlib_root);
  priv_in_staticlib.rust_values().crate_name() = "priv_in_staticlib";
  priv_in_staticlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(priv_in_staticlib.OnResolved(&err));

  Target staticlib(setup.settings(),
                   Label(SourceDir("//staticlib/"), "staticlib"));
  staticlib.set_output_type(Target::STATIC_LIBRARY);
  staticlib.visibility().SetPublic();
  staticlib.sources().push_back(SourceFile("//staticlib/lib.cc"));
  staticlib.source_types_used().Set(SourceFile::SOURCE_CPP);
  staticlib.public_deps().push_back(LabelTargetPair(&pub_in_staticlib));
  staticlib.private_deps().push_back(LabelTargetPair(&priv_in_staticlib));
  staticlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(staticlib.OnResolved(&err));

  Target priv_in_procmacro(
      setup.settings(),
      Label(SourceDir("//priv_in_procmacro/"), "priv_in_procmacro"));
  priv_in_procmacro.set_output_type(Target::RUST_LIBRARY);
  priv_in_procmacro.visibility().SetPublic();
  SourceFile priv_in_procmacro_root("//priv_in_procmacro/lib.rs");
  priv_in_procmacro.sources().push_back(priv_in_procmacro_root);
  priv_in_procmacro.source_types_used().Set(SourceFile::SOURCE_RS);
  priv_in_procmacro.rust_values().set_crate_root(priv_in_procmacro_root);
  priv_in_procmacro.rust_values().crate_name() = "priv_in_procmacro";
  priv_in_procmacro.SetToolchain(setup.toolchain());
  ASSERT_TRUE(priv_in_procmacro.OnResolved(&err));

  // Public deps in a proc-macro are not inherited, since the proc-macro is not
  // compiled into targets that depend on it.
  Target pub_in_procmacro(
      setup.settings(),
      Label(SourceDir("//pub_in_procmacro/"), "pub_in_procmacro"));
  pub_in_procmacro.set_output_type(Target::RUST_LIBRARY);
  pub_in_procmacro.visibility().SetPublic();
  SourceFile pub_in_procmacro_root("//pub_in_procmacro/lib.rs");
  pub_in_procmacro.sources().push_back(pub_in_procmacro_root);
  pub_in_procmacro.source_types_used().Set(SourceFile::SOURCE_RS);
  pub_in_procmacro.rust_values().set_crate_root(pub_in_procmacro_root);
  pub_in_procmacro.rust_values().crate_name() = "pub_in_procmacro";
  pub_in_procmacro.SetToolchain(setup.toolchain());
  ASSERT_TRUE(pub_in_procmacro.OnResolved(&err));

  Target pub_in_procmacro_and_rlib(
      setup.settings(), Label(SourceDir("//pub_in_procmacro_and_rlib/"),
                              "pub_in_procmacro_and_rlib"));
  pub_in_procmacro_and_rlib.set_output_type(Target::RUST_LIBRARY);
  pub_in_procmacro_and_rlib.visibility().SetPublic();
  SourceFile pub_in_procmacro_and_rlib_root(
      "//pub_in_procmacro_and_rlib/lib.rs");
  pub_in_procmacro_and_rlib.sources().push_back(pub_in_procmacro_and_rlib_root);
  pub_in_procmacro_and_rlib.source_types_used().Set(SourceFile::SOURCE_RS);
  pub_in_procmacro_and_rlib.rust_values().set_crate_root(
      pub_in_procmacro_and_rlib_root);
  pub_in_procmacro_and_rlib.rust_values().crate_name() = "lib2";
  pub_in_procmacro_and_rlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(pub_in_procmacro_and_rlib.OnResolved(&err));

  Target procmacro(setup.settings(),
                   Label(SourceDir("//procmacro/"), "procmacro"));
  procmacro.set_output_type(Target::RUST_PROC_MACRO);
  procmacro.visibility().SetPublic();
  SourceFile procmacrolib("//procmacro/lib.rs");
  procmacro.sources().push_back(procmacrolib);
  procmacro.source_types_used().Set(SourceFile::SOURCE_RS);
  procmacro.public_deps().push_back(LabelTargetPair(&pub_in_procmacro));
  procmacro.public_deps().push_back(LabelTargetPair(&priv_in_procmacro));
  procmacro.public_deps().push_back(
      LabelTargetPair(&pub_in_procmacro_and_rlib));
  procmacro.rust_values().set_crate_root(procmacrolib);
  procmacro.rust_values().crate_name() = "procmacro";
  procmacro.SetToolchain(setup.toolchain());
  ASSERT_TRUE(procmacro.OnResolved(&err));

  Target rlib(setup.settings(), Label(SourceDir("//rlib/"), "rlib"));
  rlib.set_output_type(Target::RUST_LIBRARY);
  rlib.visibility().SetPublic();
  SourceFile rlib_root("//rlib/lib.rs");
  rlib.sources().push_back(rlib_root);
  rlib.source_types_used().Set(SourceFile::SOURCE_RS);
  rlib.public_deps().push_back(LabelTargetPair(&pub_in_procmacro_and_rlib));
  // Transitive proc macros should not impact C++ targets; we're
  // adding one to ensure the ninja instructions below are unaffected.
  rlib.public_deps().push_back(LabelTargetPair(&procmacro));
  rlib.rust_values().set_crate_root(rlib_root);
  rlib.rust_values().crate_name() = "rlib";
  rlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(rlib.OnResolved(&err));

  Target target(setup.settings(), Label(SourceDir("//exe/"), "exe"));
  target.set_output_type(Target::EXECUTABLE);
  target.visibility().SetPublic();
  target.sources().push_back(SourceFile("//exe/main.cc"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.private_deps().push_back(LabelTargetPair(&staticlib));
  target.private_deps().push_back(LabelTargetPair(&rlib));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/exe\n"
      "target_output_name = exe\n"
      "\n"
      "build obj/exe/exe.main.o: cxx ../../exe/main.cc\n"
      "  source_file_part = main.cc\n"
      "  source_name_part = main\n"
      "\n"
      "build ./exe: link obj/exe/exe.main.o "
      "obj/staticlib/libstaticlib.a | "
      "obj/pub_in_staticlib/libpub_in_staticlib.rlib "
      "obj/priv_in_staticlib/libpriv_in_staticlib.rlib "
      "obj/rlib/librlib.rlib "
      "obj/pub_in_procmacro_and_rlib/libpub_in_procmacro_and_rlib.rlib\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = \n"
      "  output_dir = \n"
      "  rlibs = obj/pub_in_staticlib/libpub_in_staticlib.rlib "
      "obj/priv_in_staticlib/libpriv_in_staticlib.rlib "
      "obj/rlib/librlib.rlib "
      "obj/pub_in_procmacro_and_rlib/libpub_in_procmacro_and_rlib.rlib\n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

// Test linking of Rust dependencies into C targets.
TEST_F(NinjaCBinaryTargetWriterTest, ProcMacroInRustStaticLib) {
  Err err;
  TestWithScope setup;

  Target procmacro(setup.settings(), Label(SourceDir("//baz/"), "macro"));
  procmacro.set_output_type(Target::LOADABLE_MODULE);
  procmacro.visibility().SetPublic();
  SourceFile bazlib("//baz/lib.rs");
  procmacro.sources().push_back(bazlib);
  procmacro.source_types_used().Set(SourceFile::SOURCE_RS);
  procmacro.rust_values().set_crate_root(bazlib);
  procmacro.rust_values().crate_name() = "macro";
  procmacro.rust_values().set_crate_type(RustValues::CRATE_PROC_MACRO);
  procmacro.SetToolchain(setup.toolchain());
  ASSERT_TRUE(procmacro.OnResolved(&err));

  Target library_target(setup.settings(), Label(SourceDir("//foo/"), "foo"));
  library_target.set_output_type(Target::STATIC_LIBRARY);
  library_target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  library_target.sources().push_back(lib);
  library_target.source_types_used().Set(SourceFile::SOURCE_RS);
  library_target.rust_values().set_crate_root(lib);
  library_target.rust_values().crate_name() = "foo";
  library_target.public_deps().push_back(LabelTargetPair(&procmacro));
  library_target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(library_target.OnResolved(&err));

  Target target(setup.settings(), Label(SourceDir("//bar/"), "bar"));
  target.set_output_type(Target::EXECUTABLE);
  target.visibility().SetPublic();
  target.sources().push_back(SourceFile("//bar/bar.cc"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.private_deps().push_back(LabelTargetPair(&library_target));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/bar\n"
      "target_output_name = bar\n"
      "\n"
      "build obj/bar/bar.bar.o: cxx ../../bar/bar.cc\n"
      "  source_file_part = bar.cc\n"
      "  source_name_part = bar\n"
      "\n"
      "build ./bar: link obj/bar/bar.bar.o obj/foo/libfoo.a\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = \n"
      "  output_dir = \n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

TEST_F(NinjaCBinaryTargetWriterTest, RustDepsOverDynamicLinking) {
  Err err;
  TestWithScope setup;

  Target rlib3(setup.settings(), Label(SourceDir("//baz/"), "baz"));
  rlib3.set_output_type(Target::RUST_LIBRARY);
  rlib3.visibility().SetPublic();
  SourceFile lib3("//baz/lib.rs");
  rlib3.sources().push_back(lib3);
  rlib3.source_types_used().Set(SourceFile::SOURCE_RS);
  rlib3.rust_values().set_crate_root(lib3);
  rlib3.rust_values().crate_name() = "baz";
  rlib3.SetToolchain(setup.toolchain());
  ASSERT_TRUE(rlib3.OnResolved(&err));

  Target rlib2(setup.settings(), Label(SourceDir("//bar/"), "bar"));
  rlib2.set_output_type(Target::RUST_LIBRARY);
  rlib2.visibility().SetPublic();
  SourceFile lib2("//bar/lib.rs");
  rlib2.sources().push_back(lib2);
  rlib2.source_types_used().Set(SourceFile::SOURCE_RS);
  rlib2.rust_values().set_crate_root(lib2);
  rlib2.rust_values().crate_name() = "bar";
  rlib2.SetToolchain(setup.toolchain());
  ASSERT_TRUE(rlib2.OnResolved(&err));

  Target rlib(setup.settings(), Label(SourceDir("//foo/"), "foo"));
  rlib.set_output_type(Target::RUST_LIBRARY);
  rlib.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  rlib.sources().push_back(lib);
  rlib.source_types_used().Set(SourceFile::SOURCE_RS);
  rlib.rust_values().set_crate_root(lib);
  rlib.rust_values().crate_name() = "foo";
  rlib.public_deps().push_back(LabelTargetPair(&rlib2));
  rlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(rlib.OnResolved(&err));

  Target cdylib(setup.settings(), Label(SourceDir("//sh/"), "mylib"));
  cdylib.set_output_type(Target::SHARED_LIBRARY);
  cdylib.visibility().SetPublic();
  SourceFile barlib("//sh/lib.rs");
  cdylib.sources().push_back(barlib);
  cdylib.source_types_used().Set(SourceFile::SOURCE_RS);
  cdylib.rust_values().set_crate_type(RustValues::CRATE_CDYLIB);
  cdylib.rust_values().set_crate_root(barlib);
  cdylib.rust_values().crate_name() = "mylib";
  cdylib.private_deps().push_back(LabelTargetPair(&rlib));
  cdylib.public_deps().push_back(LabelTargetPair(&rlib3));
  cdylib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(cdylib.OnResolved(&err));

  Target nearrlib(setup.settings(), Label(SourceDir("//near/"), "near"));
  nearrlib.set_output_type(Target::RUST_LIBRARY);
  nearrlib.visibility().SetPublic();
  SourceFile nearlib("//near/lib.rs");
  nearrlib.sources().push_back(nearlib);
  nearrlib.source_types_used().Set(SourceFile::SOURCE_RS);
  nearrlib.rust_values().set_crate_root(nearlib);
  nearrlib.rust_values().crate_name() = "near";
  nearrlib.public_deps().push_back(LabelTargetPair(&cdylib));
  nearrlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(nearrlib.OnResolved(&err));

  Target target(setup.settings(), Label(SourceDir("//exe/"), "binary"));
  target.set_output_type(Target::EXECUTABLE);
  target.visibility().SetPublic();
  target.sources().push_back(SourceFile("//exe/main.cc"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.private_deps().push_back(LabelTargetPair(&nearrlib));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/exe\n"
      "target_output_name = binary\n"
      "\n"
      "build obj/exe/binary.main.o: cxx ../../exe/main.cc\n"
      "  source_file_part = main.cc\n"
      "  source_name_part = main\n"
      "\n"
      "build ./binary: link obj/exe/binary.main.o obj/sh/libmylib.so | "
      "obj/near/libnear.rlib\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = \n"
      "  output_dir = \n"
      "  rlibs = obj/near/libnear.rlib\n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

TEST_F(NinjaCBinaryTargetWriterTest, LinkingWithRustLibraryDepsOnCdylib) {
  Err err;
  TestWithScope setup;

  // A non-rust shared library.
  Target cc_shlib(setup.settings(), Label(SourceDir("//cc_shlib"), "cc_shlib"));
  cc_shlib.set_output_type(Target::SHARED_LIBRARY);
  cc_shlib.set_output_name("cc_shlib");
  cc_shlib.SetToolchain(setup.toolchain());
  cc_shlib.visibility().SetPublic();
  ASSERT_TRUE(cc_shlib.OnResolved(&err));

  // A Rust CDYLIB shared library that will be in deps.
  Target rust_shlib(setup.settings(),
                    Label(SourceDir("//rust_shlib/"), "rust_shlib"));
  rust_shlib.set_output_type(Target::SHARED_LIBRARY);
  rust_shlib.visibility().SetPublic();
  SourceFile rust_shlib_rs("//rust_shlib/lib.rs");
  rust_shlib.sources().push_back(rust_shlib_rs);
  rust_shlib.source_types_used().Set(SourceFile::SOURCE_RS);
  rust_shlib.rust_values().set_crate_type(RustValues::CRATE_CDYLIB);
  rust_shlib.rust_values().set_crate_root(rust_shlib_rs);
  rust_shlib.rust_values().crate_name() = "rust_shlib";
  rust_shlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(rust_shlib.OnResolved(&err));

  // A Rust DYLIB shared library that will be in public_deps.
  Target pub_rust_shlib(setup.settings(), Label(SourceDir("//pub_rust_shlib/"),
                                                "pub_rust_shlib"));
  pub_rust_shlib.set_output_type(Target::SHARED_LIBRARY);
  pub_rust_shlib.visibility().SetPublic();
  SourceFile pub_rust_shlib_rs("//pub_rust_shlib/lib.rs");
  pub_rust_shlib.sources().push_back(pub_rust_shlib_rs);
  pub_rust_shlib.source_types_used().Set(SourceFile::SOURCE_RS);
  pub_rust_shlib.rust_values().set_crate_type(RustValues::CRATE_CDYLIB);
  pub_rust_shlib.rust_values().set_crate_root(pub_rust_shlib_rs);
  pub_rust_shlib.rust_values().crate_name() = "pub_rust_shlib";
  pub_rust_shlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(pub_rust_shlib.OnResolved(&err));

  // An rlib that depends on both shared libraries.
  Target rlib(setup.settings(), Label(SourceDir("//rlib/"), "rlib"));
  rlib.set_output_type(Target::RUST_LIBRARY);
  rlib.visibility().SetPublic();
  SourceFile rlib_rs("//rlib/lib.rs");
  rlib.sources().push_back(rlib_rs);
  rlib.source_types_used().Set(SourceFile::SOURCE_RS);
  rlib.rust_values().set_crate_root(rlib_rs);
  rlib.rust_values().crate_name() = "rlib";
  rlib.private_deps().push_back(LabelTargetPair(&rust_shlib));
  rlib.private_deps().push_back(LabelTargetPair(&cc_shlib));
  rlib.public_deps().push_back(LabelTargetPair(&pub_rust_shlib));
  rlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(rlib.OnResolved(&err));

  Target target(setup.settings(), Label(SourceDir("//exe/"), "binary"));
  target.set_output_type(Target::EXECUTABLE);
  target.visibility().SetPublic();
  target.sources().push_back(SourceFile("//exe/main.cc"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.private_deps().push_back(LabelTargetPair(&rlib));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/exe\n"
      "target_output_name = binary\n"
      "\n"
      "build obj/exe/binary.main.o: cxx ../../exe/main.cc\n"
      "  source_file_part = main.cc\n"
      "  source_name_part = main\n"
      "\n"
      "build ./binary: link obj/exe/binary.main.o "
      "obj/pub_rust_shlib/libpub_rust_shlib.so obj/rust_shlib/librust_shlib.so "
      "./libcc_shlib.so | "
      "obj/rlib/librlib.rlib\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = \n"
      "  output_dir = \n"
      "  rlibs = obj/rlib/librlib.rlib\n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

TEST_F(NinjaCBinaryTargetWriterTest, LinkingWithRustLibraryDepsOnDylib) {
  Err err;
  TestWithScope setup;

  // A non-rust shared library.
  Target cc_shlib(setup.settings(), Label(SourceDir("//cc_shlib"), "cc_shlib"));
  cc_shlib.set_output_type(Target::SHARED_LIBRARY);
  cc_shlib.set_output_name("cc_shlib");
  cc_shlib.SetToolchain(setup.toolchain());
  cc_shlib.visibility().SetPublic();
  ASSERT_TRUE(cc_shlib.OnResolved(&err));

  // A Rust DYLIB shared library that will be in deps.
  Target rust_shlib(setup.settings(),
                    Label(SourceDir("//rust_shlib/"), "rust_shlib"));
  rust_shlib.set_output_type(Target::SHARED_LIBRARY);
  rust_shlib.visibility().SetPublic();
  SourceFile rust_shlib_rs("//rust_shlib/lib.rs");
  rust_shlib.sources().push_back(rust_shlib_rs);
  rust_shlib.source_types_used().Set(SourceFile::SOURCE_RS);
  rust_shlib.rust_values().set_crate_type(RustValues::CRATE_DYLIB);
  rust_shlib.rust_values().set_crate_root(rust_shlib_rs);
  rust_shlib.rust_values().crate_name() = "rust_shlib";
  rust_shlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(rust_shlib.OnResolved(&err));

  // A Rust DYLIB shared library that will be in public_deps.
  Target pub_rust_shlib(setup.settings(), Label(SourceDir("//pub_rust_shlib/"),
                                                "pub_rust_shlib"));
  pub_rust_shlib.set_output_type(Target::SHARED_LIBRARY);
  pub_rust_shlib.visibility().SetPublic();
  SourceFile pub_rust_shlib_rs("//pub_rust_shlib/lib.rs");
  pub_rust_shlib.sources().push_back(pub_rust_shlib_rs);
  pub_rust_shlib.source_types_used().Set(SourceFile::SOURCE_RS);
  pub_rust_shlib.rust_values().set_crate_type(RustValues::CRATE_DYLIB);
  pub_rust_shlib.rust_values().set_crate_root(pub_rust_shlib_rs);
  pub_rust_shlib.rust_values().crate_name() = "pub_rust_shlib";
  pub_rust_shlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(pub_rust_shlib.OnResolved(&err));

  // An rlib that depends on both shared libraries.
  Target rlib(setup.settings(), Label(SourceDir("//rlib/"), "rlib"));
  rlib.set_output_type(Target::RUST_LIBRARY);
  rlib.visibility().SetPublic();
  SourceFile rlib_rs("//rlib/lib.rs");
  rlib.sources().push_back(rlib_rs);
  rlib.source_types_used().Set(SourceFile::SOURCE_RS);
  rlib.rust_values().set_crate_root(rlib_rs);
  rlib.rust_values().crate_name() = "rlib";
  rlib.private_deps().push_back(LabelTargetPair(&rust_shlib));
  rlib.private_deps().push_back(LabelTargetPair(&cc_shlib));
  rlib.public_deps().push_back(LabelTargetPair(&pub_rust_shlib));
  rlib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(rlib.OnResolved(&err));

  Target target(setup.settings(), Label(SourceDir("//exe/"), "binary"));
  target.set_output_type(Target::EXECUTABLE);
  target.visibility().SetPublic();
  target.sources().push_back(SourceFile("//exe/main.cc"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.private_deps().push_back(LabelTargetPair(&rlib));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/exe\n"
      "target_output_name = binary\n"
      "\n"
      "build obj/exe/binary.main.o: cxx ../../exe/main.cc\n"
      "  source_file_part = main.cc\n"
      "  source_name_part = main\n"
      "\n"
      "build ./binary: link obj/exe/binary.main.o "
      "obj/pub_rust_shlib/libpub_rust_shlib.so obj/rust_shlib/librust_shlib.so "
      "./libcc_shlib.so | "
      "obj/rlib/librlib.rlib\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = \n"
      "  output_dir = \n"
      "  rlibs = obj/rlib/librlib.rlib\n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

// Verify dependencies of a shared library and a rust library are inherited
// independently.
TEST_F(NinjaCBinaryTargetWriterTest, RustLibAfterSharedLib) {
  Err err;
  TestWithScope setup;

  Target static1(setup.settings(),
                Label(SourceDir("//static1/"), "staticlib1"));
  static1.set_output_type(Target::STATIC_LIBRARY);
  static1.visibility().SetPublic();
  static1.sources().push_back(SourceFile("//static1/c.cc"));
  static1.source_types_used().Set(SourceFile::SOURCE_CPP);
  static1.SetToolchain(setup.toolchain());
  ASSERT_TRUE(static1.OnResolved(&err));

  Target static2(setup.settings(),
                Label(SourceDir("//static2/"), "staticlib2"));
  static2.set_output_type(Target::STATIC_LIBRARY);
  static2.visibility().SetPublic();
  static2.sources().push_back(SourceFile("//static2/c.cc"));
  static2.source_types_used().Set(SourceFile::SOURCE_CPP);
  static2.SetToolchain(setup.toolchain());
  ASSERT_TRUE(static2.OnResolved(&err));

  Target static3(setup.settings(),
                Label(SourceDir("//static3/"), "staticlib3"));
  static3.set_output_type(Target::STATIC_LIBRARY);
  static3.visibility().SetPublic();
  static3.sources().push_back(SourceFile("//static3/c.cc"));
  static3.source_types_used().Set(SourceFile::SOURCE_CPP);
  static3.SetToolchain(setup.toolchain());
  ASSERT_TRUE(static3.OnResolved(&err));

  Target shared1(setup.settings(),
                    Label(SourceDir("//shared1"), "mysharedlib1"));
  shared1.set_output_type(Target::SHARED_LIBRARY);
  shared1.set_output_name("mysharedlib1");
  shared1.set_output_prefix_override("");
  shared1.SetToolchain(setup.toolchain());
  shared1.visibility().SetPublic();
  shared1.private_deps().push_back(LabelTargetPair(&static1));
  ASSERT_TRUE(shared1.OnResolved(&err));

  Target rlib2(setup.settings(), Label(SourceDir("//rlib2/"), "myrlib2"));
  rlib2.set_output_type(Target::RUST_LIBRARY);
  rlib2.visibility().SetPublic();
  SourceFile lib2("//rlib2/lib.rs");
  rlib2.sources().push_back(lib2);
  rlib2.source_types_used().Set(SourceFile::SOURCE_RS);
  rlib2.rust_values().set_crate_root(lib2);
  rlib2.rust_values().crate_name() = "foo";
  rlib2.private_deps().push_back(LabelTargetPair(&static2));
  rlib2.SetToolchain(setup.toolchain());
  ASSERT_TRUE(rlib2.OnResolved(&err));

  Target shared3(setup.settings(),
                    Label(SourceDir("//shared3"), "mysharedlib3"));
  shared3.set_output_type(Target::SHARED_LIBRARY);
  shared3.set_output_name("mysharedlib3");
  shared3.set_output_prefix_override("");
  shared3.SetToolchain(setup.toolchain());
  shared3.visibility().SetPublic();
  shared3.private_deps().push_back(LabelTargetPair(&static3));
  ASSERT_TRUE(shared3.OnResolved(&err));

  Target target(setup.settings(), Label(SourceDir("//exe/"), "binary"));
  target.set_output_type(Target::EXECUTABLE);
  target.visibility().SetPublic();
  target.sources().push_back(SourceFile("//exe/main.cc"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.private_deps().push_back(LabelTargetPair(&shared1));
  target.private_deps().push_back(LabelTargetPair(&rlib2));
  target.private_deps().push_back(LabelTargetPair(&shared3));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/exe\n"
      "target_output_name = binary\n"
      "\n"
      "build obj/exe/binary.main.o: cxx ../../exe/main.cc\n"
      "  source_file_part = main.cc\n"
      "  source_name_part = main\n"
      "\n"
      "build ./binary: link obj/exe/binary.main.o "
      "./mysharedlib1.so ./mysharedlib3.so "
      "obj/static2/libstaticlib2.a | obj/rlib2/libmyrlib2.rlib\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = \n"
      "  output_dir = \n"
      "  rlibs = obj/rlib2/libmyrlib2.rlib\n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

TEST_F(NinjaCBinaryTargetWriterTest, ModuleMapInStaticLibrary) {
  TestWithScope setup;
  Err err;

  std::unique_ptr<Tool> cxx_module_tool =
      Tool::CreateTool(CTool::kCToolCxxModule);
  cxx_module_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.pcm"));
  setup.toolchain()->SetTool(std::move(cxx_module_tool));

  TestTarget target(setup, "//foo:bar", Target::STATIC_LIBRARY);
  target.sources().push_back(SourceFile("//foo/bar.cc"));
  target.sources().push_back(SourceFile("//foo/bar.modulemap"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.source_types_used().Set(SourceFile::SOURCE_MODULEMAP);
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libbar\n"
      "\n"
      "build obj/foo/libbar.bar.o: cxx ../../foo/bar.cc | obj/foo/libbar.bar.pcm\n"
      "  source_file_part = bar.cc\n"
      "  source_name_part = bar\n"
      "build obj/foo/libbar.bar.pcm: cxx_module ../../foo/bar.modulemap\n"
      "  source_file_part = bar.modulemap\n"
      "  source_name_part = bar\n"
      "\n"
      "build obj/foo/libbar.a: alink obj/foo/libbar.bar.o\n"
      "  arflags =\n"
      "  output_extension = \n"
      "  output_dir = \n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

// Test linking of targets containing Swift modules.
TEST_F(NinjaCBinaryTargetWriterTest, SwiftModule) {
  Err err;
  TestWithScope setup;

  // A single Swift module.
  Target foo_target(setup.settings(), Label(SourceDir("//foo/"), "foo"));
  foo_target.set_output_type(Target::SOURCE_SET);
  foo_target.visibility().SetPublic();
  foo_target.sources().push_back(SourceFile("//foo/file1.swift"));
  foo_target.sources().push_back(SourceFile("//foo/file2.swift"));
  foo_target.source_types_used().Set(SourceFile::SOURCE_SWIFT);
  foo_target.swift_values().module_name() = "Foo";
  foo_target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(foo_target.OnResolved(&err));

  {
    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&foo_target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "module_name = Foo\n"
        "module_dirs =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = foo\n"
        "\n"
        "build obj/foo/Foo.swiftmodule: swift"
        " ../../foo/file1.swift ../../foo/file2.swift\n"
        "\n"
        "build obj/foo/file1.o obj/foo/file2.o: stamp obj/foo/Foo.swiftmodule\n"
        "\n"
        "build obj/foo/foo.stamp: stamp"
        " obj/foo/file1.o obj/foo/file2.o\n";

    const std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }

  // Swift module_dirs correctly set if dependency between Swift modules.
  {
    Target bar_target(setup.settings(), Label(SourceDir("//bar/"), "bar"));
    bar_target.set_output_type(Target::SOURCE_SET);
    bar_target.visibility().SetPublic();
    bar_target.sources().push_back(SourceFile("//bar/bar.swift"));
    bar_target.source_types_used().Set(SourceFile::SOURCE_SWIFT);
    bar_target.swift_values().module_name() = "Bar";
    bar_target.private_deps().push_back(LabelTargetPair(&foo_target));
    bar_target.SetToolchain(setup.toolchain());
    ASSERT_TRUE(bar_target.OnResolved(&err));

    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&bar_target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "module_name = Bar\n"
        "module_dirs = -Iobj/foo\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/bar\n"
        "target_output_name = bar\n"
        "\n"
        "build obj/bar/Bar.swiftmodule: swift ../../bar/bar.swift"
        " || obj/foo/foo.stamp\n"
        "\n"
        "build obj/bar/bar.o: stamp obj/bar/Bar.swiftmodule"
        " || obj/foo/foo.stamp\n"
        "\n"
        "build obj/bar/bar.stamp: stamp obj/bar/bar.o "
        "|| obj/foo/foo.stamp\n";

    const std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }

  // Swift module_dirs correctly set if dependency between Swift modules,
  // even if the dependency is indirect (via public_deps).
  {
    Target group(setup.settings(), Label(SourceDir("//bar/"), "group"));
    group.set_output_type(Target::GROUP);
    group.visibility().SetPublic();
    group.public_deps().push_back(LabelTargetPair(&foo_target));
    group.SetToolchain(setup.toolchain());
    ASSERT_TRUE(group.OnResolved(&err));

    Target bar_target(setup.settings(), Label(SourceDir("//bar/"), "bar"));
    bar_target.set_output_type(Target::SOURCE_SET);
    bar_target.visibility().SetPublic();
    bar_target.sources().push_back(SourceFile("//bar/bar.swift"));
    bar_target.source_types_used().Set(SourceFile::SOURCE_SWIFT);
    bar_target.swift_values().module_name() = "Bar";
    bar_target.private_deps().push_back(LabelTargetPair(&group));
    bar_target.SetToolchain(setup.toolchain());
    ASSERT_TRUE(bar_target.OnResolved(&err));

    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&bar_target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "module_name = Bar\n"
        "module_dirs = -Iobj/foo\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/bar\n"
        "target_output_name = bar\n"
        "\n"
        "build obj/bar/Bar.swiftmodule: swift ../../bar/bar.swift"
        " || obj/bar/group.stamp obj/foo/foo.stamp\n"
        "\n"
        "build obj/bar/bar.o: stamp obj/bar/Bar.swiftmodule"
        " || obj/bar/group.stamp obj/foo/foo.stamp\n"
        "\n"
        "build obj/bar/bar.stamp: stamp obj/bar/bar.o "
        "|| obj/bar/group.stamp obj/foo/foo.stamp\n";

    const std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }

  // C target links with module.
  {
    Target bar_target(setup.settings(), Label(SourceDir("//bar/"), "bar"));
    bar_target.set_output_type(Target::EXECUTABLE);
    bar_target.visibility().SetPublic();
    bar_target.private_deps().push_back(LabelTargetPair(&foo_target));
    bar_target.SetToolchain(setup.toolchain());
    ASSERT_TRUE(bar_target.OnResolved(&err));

    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&bar_target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/bar\n"
        "target_output_name = bar\n"
        "\n"
        "\n"
        "build ./bar: link obj/foo/file1.o obj/foo/file2.o "
        "| obj/foo/Foo.swiftmodule "
        "|| obj/foo/foo.stamp\n"
        "  ldflags =\n"
        "  libs =\n"
        "  frameworks =\n"
        "  swiftmodules = obj/foo/Foo.swiftmodule\n"
        "  output_extension = \n"
        "  output_dir = \n";

    const std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }
}

TEST_F(NinjaCBinaryTargetWriterTest, DependOnModule) {
  TestWithScope setup;
  Err err;

  // There's no cxx_module or flags in the test toolchain, set up a
  // custom one here.
  Settings module_settings(setup.build_settings(), "withmodules/");
  Toolchain module_toolchain(&module_settings,
                             Label(SourceDir("//toolchain/"), "withmodules"));
  module_settings.set_toolchain_label(module_toolchain.label());
  module_settings.set_default_toolchain_label(module_toolchain.label());

  std::unique_ptr<Tool> cxx = std::make_unique<CTool>(CTool::kCToolCxx);
  CTool* cxx_tool = cxx->AsC();
  TestWithScope::SetCommandForTool(
      "c++ {{source}} {{cflags}} {{cflags_cc}} {{module_deps}} "
      "{{defines}} {{include_dirs}} -o {{output}}",
      cxx_tool);
  cxx_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o"));
  cxx_tool->set_precompiled_header_type(CTool::PCH_GCC);
  module_toolchain.SetTool(std::move(cxx));

  std::unique_ptr<Tool> cxx_module_tool =
      Tool::CreateTool(CTool::kCToolCxxModule);
  TestWithScope::SetCommandForTool(
      "c++ {{source}} {{cflags}} {{cflags_cc}} {{module_deps_no_self}} "
      "{{defines}} {{include_dirs}} -fmodule-name={{label}} -c -x c++ "
      "-Xclang -emit-module -o {{output}}",
      cxx_module_tool.get());
  cxx_module_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.pcm"));
  module_toolchain.SetTool(std::move(cxx_module_tool));

  std::unique_ptr<Tool> alink = Tool::CreateTool(CTool::kCToolAlink);
  CTool* alink_tool = alink->AsC();
  TestWithScope::SetCommandForTool("ar {{output}} {{source}}", alink_tool);
  alink_tool->set_lib_switch("-l");
  alink_tool->set_lib_dir_switch("-L");
  alink_tool->set_output_prefix("lib");
  alink_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{target_out_dir}}/{{target_output_name}}.a"));
  module_toolchain.SetTool(std::move(alink));

  std::unique_ptr<Tool> link = Tool::CreateTool(CTool::kCToolLink);
  CTool* link_tool = link->AsC();
  TestWithScope::SetCommandForTool(
      "ld -o {{target_output_name}} {{source}} "
      "{{ldflags}} {{libs}}",
      link_tool);
  link_tool->set_lib_switch("-l");
  link_tool->set_lib_dir_switch("-L");
  link_tool->set_outputs(
      SubstitutionList::MakeForTest("{{root_out_dir}}/{{target_output_name}}"));
  module_toolchain.SetTool(std::move(link));

  module_toolchain.ToolchainSetupComplete();

  Target target(&module_settings, Label(SourceDir("//blah/"), "a"));
  target.set_output_type(Target::STATIC_LIBRARY);
  target.visibility().SetPublic();
  target.sources().push_back(SourceFile("//blah/a.modulemap"));
  target.sources().push_back(SourceFile("//blah/a.cc"));
  target.sources().push_back(SourceFile("//blah/a.h"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.source_types_used().Set(SourceFile::SOURCE_MODULEMAP);
  target.SetToolchain(&module_toolchain);
  ASSERT_TRUE(target.OnResolved(&err));

  // The library first.
  {
    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] = R"(defines =
include_dirs =
cflags =
cflags_cc =
module_deps = -Xclang -fmodules-embed-all-files -fmodule-file=obj/blah/liba.a.pcm
module_deps_no_self = -Xclang -fmodules-embed-all-files
label = //blah$:a
root_out_dir = withmodules
target_out_dir = obj/blah
target_output_name = liba

build obj/blah/liba.a.pcm: cxx_module ../../blah/a.modulemap
  source_file_part = a.modulemap
  source_name_part = a
build obj/blah/liba.a.o: cxx ../../blah/a.cc | obj/blah/liba.a.pcm
  source_file_part = a.cc
  source_name_part = a

build obj/blah/liba.a: alink obj/blah/liba.a.o
  arflags =
  output_extension = 
  output_dir = 
)";

    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }

  Target target2(&module_settings, Label(SourceDir("//stuff/"), "b"));
  target2.set_output_type(Target::STATIC_LIBRARY);
  target2.visibility().SetPublic();
  target2.sources().push_back(SourceFile("//stuff/b.modulemap"));
  target2.sources().push_back(SourceFile("//stuff/b.cc"));
  target2.sources().push_back(SourceFile("//stuff/b.h"));
  target2.source_types_used().Set(SourceFile::SOURCE_CPP);
  target2.source_types_used().Set(SourceFile::SOURCE_MODULEMAP);
  target2.SetToolchain(&module_toolchain);
  ASSERT_TRUE(target2.OnResolved(&err));

  // A second library to make sure the depender includes both.
  {
    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&target2, out);
    writer.Run();

    const char expected[] = R"(defines =
include_dirs =
cflags =
cflags_cc =
module_deps = -Xclang -fmodules-embed-all-files -fmodule-file=obj/stuff/libb.b.pcm
module_deps_no_self = -Xclang -fmodules-embed-all-files
label = //stuff$:b
root_out_dir = withmodules
target_out_dir = obj/stuff
target_output_name = libb

build obj/stuff/libb.b.pcm: cxx_module ../../stuff/b.modulemap
  source_file_part = b.modulemap
  source_name_part = b
build obj/stuff/libb.b.o: cxx ../../stuff/b.cc | obj/stuff/libb.b.pcm
  source_file_part = b.cc
  source_name_part = b

build obj/stuff/libb.a: alink obj/stuff/libb.b.o
  arflags =
  output_extension = 
  output_dir = 
)";

    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }

  Target target3(&module_settings, Label(SourceDir("//things/"), "c"));
  target3.set_output_type(Target::STATIC_LIBRARY);
  target3.visibility().SetPublic();
  target3.sources().push_back(SourceFile("//stuff/c.modulemap"));
  target3.source_types_used().Set(SourceFile::SOURCE_MODULEMAP);
  target3.private_deps().push_back(LabelTargetPair(&target));
  target3.SetToolchain(&module_toolchain);
  ASSERT_TRUE(target3.OnResolved(&err));

  // A third library that depends on one of the previous static libraries, to
  // check module_deps_no_self.
  {
    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&target3, out);
    writer.Run();

    const char expected[] = R"(defines =
include_dirs =
cflags =
cflags_cc =
module_deps = -Xclang -fmodules-embed-all-files -fmodule-file=obj/stuff/libc.c.pcm -fmodule-file=obj/blah/liba.a.pcm
module_deps_no_self = -Xclang -fmodules-embed-all-files -fmodule-file=obj/blah/liba.a.pcm
label = //things$:c
root_out_dir = withmodules
target_out_dir = obj/things
target_output_name = libc

build obj/stuff/libc.c.pcm: cxx_module ../../stuff/c.modulemap | obj/blah/liba.a.pcm
  source_file_part = c.modulemap
  source_name_part = c

build obj/things/libc.a: alink || obj/blah/liba.a
  arflags =
  output_extension = 
  output_dir = 
)";

    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }

  Target depender(&module_settings, Label(SourceDir("//zap/"), "c"));
  depender.set_output_type(Target::EXECUTABLE);
  depender.sources().push_back(SourceFile("//zap/x.cc"));
  depender.sources().push_back(SourceFile("//zap/y.cc"));
  depender.source_types_used().Set(SourceFile::SOURCE_CPP);
  depender.private_deps().push_back(LabelTargetPair(&target));
  depender.private_deps().push_back(LabelTargetPair(&target2));
  depender.SetToolchain(&module_toolchain);
  ASSERT_TRUE(depender.OnResolved(&err));

  // Then the executable that depends on it.
  {
    std::ostringstream out;
    NinjaCBinaryTargetWriter writer(&depender, out);
    writer.Run();

    const char expected[] = R"(defines =
include_dirs =
cflags =
cflags_cc =
module_deps = -Xclang -fmodules-embed-all-files -fmodule-file=obj/blah/liba.a.pcm -fmodule-file=obj/stuff/libb.b.pcm
module_deps_no_self = -Xclang -fmodules-embed-all-files -fmodule-file=obj/blah/liba.a.pcm -fmodule-file=obj/stuff/libb.b.pcm
label = //zap$:c
root_out_dir = withmodules
target_out_dir = obj/zap
target_output_name = c

build obj/zap/c.x.o: cxx ../../zap/x.cc | obj/blah/liba.a.pcm obj/stuff/libb.b.pcm
  source_file_part = x.cc
  source_name_part = x
build obj/zap/c.y.o: cxx ../../zap/y.cc | obj/blah/liba.a.pcm obj/stuff/libb.b.pcm
  source_file_part = y.cc
  source_name_part = y

build withmodules/c: link obj/zap/c.x.o obj/zap/c.y.o obj/blah/liba.a obj/stuff/libb.a
  ldflags =
  libs =
  frameworks =
  swiftmodules =
  output_extension = 
  output_dir = 
)";

    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
  }
}

TEST_F(NinjaCBinaryTargetWriterTest, SolibsEscaping) {
  Err err;
  TestWithScope setup;

  Toolchain toolchain_with_toc(
      setup.settings(), Label(SourceDir("//toolchain_with_toc/"), "with_toc"));
  TestWithScope::SetupToolchain(&toolchain_with_toc, true);

  // Create a shared library with a space in the output name.
  Target shared_lib(setup.settings(),
                    Label(SourceDir("//rocket"), "space_cadet"));
  shared_lib.set_output_type(Target::SHARED_LIBRARY);
  shared_lib.set_output_name("Space Cadet");
  shared_lib.set_output_prefix_override("");
  shared_lib.SetToolchain(&toolchain_with_toc);
  shared_lib.visibility().SetPublic();
  ASSERT_TRUE(shared_lib.OnResolved(&err));

  // Set up an executable to depend on it.
  Target target(setup.settings(), Label(SourceDir("//launchpad"), "main"));
  target.sources().push_back(SourceFile("//launchpad/main.cc"));
  target.set_output_type(Target::EXECUTABLE);
  target.private_deps().push_back(LabelTargetPair(&shared_lib));
  target.SetToolchain(&toolchain_with_toc);
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaCBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] = R"(defines =
include_dirs =
root_out_dir = .
target_out_dir = obj/launchpad
target_output_name = main

build obj/launchpad/main.main.o: cxx ../../launchpad/main.cc
  source_file_part = main.cc
  source_name_part = main

build ./main: link obj/launchpad/main.main.o | ./Space$ Cadet.so.TOC
  ldflags =
  libs =
  frameworks =
  swiftmodules =
  output_extension = 
  output_dir = 
)"
#if defined(OS_WIN)
  "  solibs = \"./Space$ Cadet.so\"\n";
#else
  "  solibs = ./Space\\$ Cadet.so\n";
#endif

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}

TEST_F(NinjaCBinaryTargetWriterTest, Pool) {
  Err err;
  TestWithScope setup;

  Pool pool(setup.settings(),
            Label(SourceDir("//foo/"), "pool", setup.toolchain()->label().dir(),
                  setup.toolchain()->label().name()));
  pool.set_depth(42);

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.sources().push_back(SourceFile("//foo/source.cc"));
  target.set_output_type(Target::EXECUTABLE);
  target.set_pool(LabelPtrPair<Pool>(&pool));
  target.visibility().SetPublic();
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = bar\n"
      "\n"
      "build obj/foo/bar.source.o: cxx ../../foo/source.cc\n"
      "  source_file_part = source.cc\n"
      "  source_name_part = source\n"
      "  pool = foo_pool\n"
      "\n"
      "build ./bar: link obj/foo/bar.source.o\n"
      "  ldflags =\n"
      "  libs =\n"
      "  frameworks =\n"
      "  swiftmodules =\n"
      "  output_extension = \n"
      "  output_dir = \n"
      "  pool = foo_pool\n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str) << expected << "\n" << out_str;
}
