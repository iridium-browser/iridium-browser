// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/json_project_writer.h"
#include "base/strings/string_util.h"
#include "gn/substitution_list.h"
#include "gn/target.h"
#include "gn/test_with_scheduler.h"
#include "gn/test_with_scope.h"
#include "util/build_config.h"
#include "util/test/test.h"

using JSONWriter = TestWithScheduler;

TEST_F(JSONWriter, ActionWithResponseFile) {
  Err err;
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::ACTION);

  target.sources().push_back(SourceFile("//foo/source1.txt"));
  target.config_values().inputs().push_back(SourceFile("//foo/input1.txt"));
  target.action_values().set_script(SourceFile("//foo/script.py"));

  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  // Make sure we get interesting substitutions for both the args and the
  // response file contents.
  target.action_values().args() =
      SubstitutionList::MakeForTest("{{response_file_name}}");
  target.action_values().rsp_file_contents() =
      SubstitutionList::MakeForTest("-j", "3");
  target.action_values().outputs() =
      SubstitutionList::MakeForTest("//out/Debug/output1.out");

  setup.build_settings()->set_python_path(
      base::FilePath(FILE_PATH_LITERAL("/usr/bin/python")));
  std::vector<const Target*> targets;
  targets.push_back(&target);
#if defined(OS_WIN)
  base::FilePath root_path =
      base::FilePath(FILE_PATH_LITERAL("c:/path/to/src"));
#else
  base::FilePath root_path = base::FilePath(FILE_PATH_LITERAL("/path/to/src"));
#endif
  setup.build_settings()->SetRootPath(root_path);
  g_scheduler->AddGenDependency(root_path.Append(FILE_PATH_LITERAL(".gn")));
  g_scheduler->AddGenDependency(
      root_path.Append(FILE_PATH_LITERAL("BUILD.gn")));
  g_scheduler->AddGenDependency(
      root_path.Append(FILE_PATH_LITERAL("build/BUILD.gn")));
  std::string out =
      JSONProjectWriter::RenderJSON(setup.build_settings(), targets);
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif
  const char expected_json[] =
      R"_({
   "build_settings": {
      "build_dir": "//out/Debug/",
      "default_toolchain": "//toolchain:default",
      "gen_input_files": [ "//.gn", "//BUILD.gn", "//build/BUILD.gn" ],
)_"
#if defined(OS_WIN)
      "      \"root_path\": \"c:/path/to/src\"\n"
#else
      "      \"root_path\": \"/path/to/src\"\n"
#endif
      R"_(   },
   "targets": {
      "//foo:bar()": {
         "args": [ "{{response_file_name}}" ],
         "deps": [  ],
         "inputs": [ "//foo/input1.txt" ],
         "metadata": {

         },
         "outputs": [ "//out/Debug/output1.out" ],
         "public": "*",
         "response_file_contents": [ "-j", "3" ],
         "script": "//foo/script.py",
         "sources": [ "//foo/source1.txt" ],
         "testonly": false,
         "toolchain": "",
         "type": "action",
         "visibility": [  ]
      }
   },
   "toolchains": {
      "//toolchain:default": {
         "alink": {
            "command": "ar {{output}} {{source}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}.a" ],
            "weak_framework_switch": "-weak_framework "
         },
         "cc": {
            "command": "cc {{source}} {{cflags}} {{cflags_c}} {{defines}} {{include_dirs}} -o {{output}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         },
         "compile_xcassets": {
            "command": "touch {{output}}"
         },
         "copy": {
            "command": "cp {{source}} {{output}}"
         },
         "copy_bundle_data": {
            "command": "cp {{source}} {{output}}"
         },
         "cxx": {
            "command": "c++ {{source}} {{cflags}} {{cflags_cc}} {{defines}} {{include_dirs}} -o {{output}}",
            "command_launcher": "launcher",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         },
         "link": {
            "command": "ld -o {{target_output_name}} {{source}} {{ldflags}} {{libs}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{root_out_dir}}/{{target_output_name}}" ],
            "weak_framework_switch": "-weak_framework "
         },
         "objc": {
            "command": "objcc {{source}} {{cflags}} {{cflags_objc}} {{defines}} {{include_dirs}} -o {{output}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         },
         "objcxx": {
            "command": "objcxx {{source}} {{cflags}} {{cflags_objcc}} {{defines}} {{include_dirs}} -o {{output}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         },
         "rust_bin": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "outputs": [ "{{root_out_dir}}/{{crate_name}}{{output_extension}}" ]
         },
         "rust_cdylib": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "rust_dylib": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "rust_macro": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "rust_rlib": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".rlib",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "rust_staticlib": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".a",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "solink": {
            "command": "ld -shared -o {{target_output_name}}.so {{inputs}} {{ldflags}} {{libs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "output_prefix": "lib",
            "outputs": [ "{{root_out_dir}}/{{target_output_name}}{{output_extension}}" ],
            "weak_framework_switch": "-weak_framework "
         },
         "solink_module": {
            "command": "ld -bundle -o {{target_output_name}}.so {{inputs}} {{ldflags}} {{libs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "output_prefix": "lib",
            "outputs": [ "{{root_out_dir}}/{{target_output_name}}{{output_extension}}" ],
            "weak_framework_switch": "-weak_framework "
         },
         "stamp": {
            "command": "touch {{output}}"
         },
         "swift": {
            "command": "swiftc --module-name {{module_name}} {{module_dirs}} {{inputs}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{target_out_dir}}/{{module_name}}.swiftmodule" ],
            "partial_outputs": [ "{{target_out_dir}}/{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         }
      }
   }
}
)_";
  EXPECT_EQ(expected_json, out) << out;
}

TEST_F(JSONWriter, RustTarget) {
  Err err;
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  target.sources().push_back(lib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(lib);
  target.rust_values().crate_name() = "foo";
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::vector<const Target*> targets;
  targets.push_back(&target);
  std::string out =
      JSONProjectWriter::RenderJSON(setup.build_settings(), targets);
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif
  const char expected_json[] =
      R"_({
   "build_settings": {
      "build_dir": "//out/Debug/",
      "default_toolchain": "//toolchain:default",
      "gen_input_files": [  ],
      "root_path": ""
   },
   "targets": {
      "//foo:bar()": {
         "allow_circular_includes_from": [  ],
         "check_includes": true,
         "crate_name": "foo",
         "crate_root": "//foo/lib.rs",
         "deps": [  ],
         "externs": {

         },
         "metadata": {

         },
         "outputs": [ "//out/Debug/obj/foo/libbar.rlib" ],
         "public": "*",
         "sources": [ "//foo/lib.rs" ],
         "testonly": false,
         "toolchain": "",
         "type": "rust_library",
         "visibility": [ "*" ]
      }
   },
   "toolchains": {
      "//toolchain:default": {
         "alink": {
            "command": "ar {{output}} {{source}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}.a" ],
            "weak_framework_switch": "-weak_framework "
         },
         "cc": {
            "command": "cc {{source}} {{cflags}} {{cflags_c}} {{defines}} {{include_dirs}} -o {{output}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         },
         "compile_xcassets": {
            "command": "touch {{output}}"
         },
         "copy": {
            "command": "cp {{source}} {{output}}"
         },
         "copy_bundle_data": {
            "command": "cp {{source}} {{output}}"
         },
         "cxx": {
            "command": "c++ {{source}} {{cflags}} {{cflags_cc}} {{defines}} {{include_dirs}} -o {{output}}",
            "command_launcher": "launcher",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         },
         "link": {
            "command": "ld -o {{target_output_name}} {{source}} {{ldflags}} {{libs}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{root_out_dir}}/{{target_output_name}}" ],
            "weak_framework_switch": "-weak_framework "
         },
         "objc": {
            "command": "objcc {{source}} {{cflags}} {{cflags_objc}} {{defines}} {{include_dirs}} -o {{output}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         },
         "objcxx": {
            "command": "objcxx {{source}} {{cflags}} {{cflags_objcc}} {{defines}} {{include_dirs}} -o {{output}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         },
         "rust_bin": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "outputs": [ "{{root_out_dir}}/{{crate_name}}{{output_extension}}" ]
         },
         "rust_cdylib": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "rust_dylib": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "rust_macro": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "rust_rlib": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".rlib",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "rust_staticlib": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".a",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "solink": {
            "command": "ld -shared -o {{target_output_name}}.so {{inputs}} {{ldflags}} {{libs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "output_prefix": "lib",
            "outputs": [ "{{root_out_dir}}/{{target_output_name}}{{output_extension}}" ],
            "weak_framework_switch": "-weak_framework "
         },
         "solink_module": {
            "command": "ld -bundle -o {{target_output_name}}.so {{inputs}} {{ldflags}} {{libs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "output_prefix": "lib",
            "outputs": [ "{{root_out_dir}}/{{target_output_name}}{{output_extension}}" ],
            "weak_framework_switch": "-weak_framework "
         },
         "stamp": {
            "command": "touch {{output}}"
         },
         "swift": {
            "command": "swiftc --module-name {{module_name}} {{module_dirs}} {{inputs}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{target_out_dir}}/{{module_name}}.swiftmodule" ],
            "partial_outputs": [ "{{target_out_dir}}/{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         }
      }
   }
}
)_";
  EXPECT_EQ(expected_json, out) << out;
}

TEST_F(JSONWriter, ForEachWithResponseFile) {
  Err err;
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::ACTION_FOREACH);

  target.sources().push_back(SourceFile("//foo/input1.txt"));
  target.action_values().set_script(SourceFile("//foo/script.py"));

  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  // Make sure we get interesting substitutions for both the args and the
  // response file contents.
  target.action_values().args() = SubstitutionList::MakeForTest(
      "{{source}}", "{{source_file_part}}", "{{response_file_name}}");
  target.action_values().rsp_file_contents() =
      SubstitutionList::MakeForTest("-j", "{{source_name_part}}");
  target.action_values().outputs() =
      SubstitutionList::MakeForTest("//out/Debug/{{source_name_part}}.out");

  setup.build_settings()->set_python_path(
      base::FilePath(FILE_PATH_LITERAL("/usr/bin/python")));
  std::vector<const Target*> targets;
  targets.push_back(&target);
#if defined(OS_WIN)
  base::FilePath root_path =
      base::FilePath(FILE_PATH_LITERAL("c:/path/to/src"));
#else
  base::FilePath root_path = base::FilePath(FILE_PATH_LITERAL("/path/to/src"));
#endif
  setup.build_settings()->SetRootPath(root_path);
  g_scheduler->AddGenDependency(root_path.Append(FILE_PATH_LITERAL(".gn")));
  g_scheduler->AddGenDependency(
      root_path.Append(FILE_PATH_LITERAL("BUILD.gn")));
  std::string out =
      JSONProjectWriter::RenderJSON(setup.build_settings(), targets);
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif
  const char expected_json[] =
      R"_({
   "build_settings": {
      "build_dir": "//out/Debug/",
      "default_toolchain": "//toolchain:default",
      "gen_input_files": [ "//.gn", "//BUILD.gn" ],
)_"
#if defined(OS_WIN)
      "      \"root_path\": \"c:/path/to/src\"\n"
#else
      "      \"root_path\": \"/path/to/src\"\n"
#endif
      R"_(   },
   "targets": {
      "//foo:bar()": {
         "args": [ "{{source}}", "{{source_file_part}}", "{{response_file_name}}" ],
         "deps": [  ],
         "metadata": {

         },
         "output_patterns": [ "//out/Debug/{{source_name_part}}.out" ],
         "outputs": [ "//out/Debug/input1.out" ],
         "public": "*",
         "response_file_contents": [ "-j", "{{source_name_part}}" ],
         "script": "//foo/script.py",
         "source_outputs": {
            "//foo/input1.txt": [ "input1.out" ]
         },
         "sources": [ "//foo/input1.txt" ],
         "testonly": false,
         "toolchain": "",
         "type": "action_foreach",
         "visibility": [  ]
      }
   },
   "toolchains": {
      "//toolchain:default": {
         "alink": {
            "command": "ar {{output}} {{source}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}.a" ],
            "weak_framework_switch": "-weak_framework "
         },
         "cc": {
            "command": "cc {{source}} {{cflags}} {{cflags_c}} {{defines}} {{include_dirs}} -o {{output}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         },
         "compile_xcassets": {
            "command": "touch {{output}}"
         },
         "copy": {
            "command": "cp {{source}} {{output}}"
         },
         "copy_bundle_data": {
            "command": "cp {{source}} {{output}}"
         },
         "cxx": {
            "command": "c++ {{source}} {{cflags}} {{cflags_cc}} {{defines}} {{include_dirs}} -o {{output}}",
            "command_launcher": "launcher",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         },
         "link": {
            "command": "ld -o {{target_output_name}} {{source}} {{ldflags}} {{libs}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{root_out_dir}}/{{target_output_name}}" ],
            "weak_framework_switch": "-weak_framework "
         },
         "objc": {
            "command": "objcc {{source}} {{cflags}} {{cflags_objc}} {{defines}} {{include_dirs}} -o {{output}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         },
         "objcxx": {
            "command": "objcxx {{source}} {{cflags}} {{cflags_objcc}} {{defines}} {{include_dirs}} -o {{output}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         },
         "rust_bin": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "outputs": [ "{{root_out_dir}}/{{crate_name}}{{output_extension}}" ]
         },
         "rust_cdylib": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "rust_dylib": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "rust_macro": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "rust_rlib": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".rlib",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "rust_staticlib": {
            "command": "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} --crate-type {{crate_type}} {{rustflags}} -o {{output}} {{rustdeps}} {{externs}}",
            "default_output_extension": ".a",
            "framework_dir_switch": "-Lframework=",
            "framework_switch": "-lframework=",
            "lib_dir_switch": "-Lnative=",
            "lib_switch": "-l",
            "linker_arg": "-Clink-arg=",
            "output_prefix": "lib",
            "outputs": [ "{{target_out_dir}}/{{target_output_name}}{{output_extension}}" ]
         },
         "solink": {
            "command": "ld -shared -o {{target_output_name}}.so {{inputs}} {{ldflags}} {{libs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "output_prefix": "lib",
            "outputs": [ "{{root_out_dir}}/{{target_output_name}}{{output_extension}}" ],
            "weak_framework_switch": "-weak_framework "
         },
         "solink_module": {
            "command": "ld -bundle -o {{target_output_name}}.so {{inputs}} {{ldflags}} {{libs}}",
            "default_output_extension": ".so",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "output_prefix": "lib",
            "outputs": [ "{{root_out_dir}}/{{target_output_name}}{{output_extension}}" ],
            "weak_framework_switch": "-weak_framework "
         },
         "stamp": {
            "command": "touch {{output}}"
         },
         "swift": {
            "command": "swiftc --module-name {{module_name}} {{module_dirs}} {{inputs}}",
            "framework_dir_switch": "-F",
            "framework_switch": "-framework ",
            "lib_dir_switch": "-L",
            "lib_switch": "-l",
            "outputs": [ "{{target_out_dir}}/{{module_name}}.swiftmodule" ],
            "partial_outputs": [ "{{target_out_dir}}/{{source_name_part}}.o" ],
            "weak_framework_switch": "-weak_framework "
         }
      }
   }
}
)_";
  EXPECT_EQ(expected_json, out) << out;
}
