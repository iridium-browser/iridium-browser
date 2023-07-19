// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/test_with_scope.h"

#include <memory>
#include <utility>

#include "gn/parser.h"
#include "gn/tokenizer.h"

namespace {

BuildSettings CreateBuildSettingsForTest() {
  BuildSettings build_settings;
  build_settings.SetBuildDir(SourceDir("//out/Debug/"));
  return build_settings;
}

}  // namespace

TestWithScope::TestWithScope()
    : build_settings_(CreateBuildSettingsForTest()),
      settings_(&build_settings_, std::string()),
      toolchain_(&settings_, Label(SourceDir("//toolchain/"), "default")),
      scope_(&settings_),
      scope_progammatic_provider_(&scope_, true) {
  build_settings_.set_print_callback(
      [this](const std::string& str) { AppendPrintOutput(str); });

  settings_.set_toolchain_label(toolchain_.label());
  settings_.set_default_toolchain_label(toolchain_.label());

  SetupToolchain(&toolchain_);
  scope_.set_item_collector(&items_);
}

TestWithScope::~TestWithScope() = default;

Label TestWithScope::ParseLabel(const std::string& str) const {
  Err err;
  Label result = Label::Resolve(SourceDir("//"), std::string_view(),
                                toolchain_.label(), Value(nullptr, str), &err);
  CHECK(!err.has_error());
  return result;
}

bool TestWithScope::ExecuteSnippet(const std::string& str, Err* err) {
  TestParseInput input(str);
  if (input.has_error()) {
    *err = input.parse_err();
    return false;
  }

  size_t first_item = items_.size();
  input.parsed()->Execute(&scope_, err);
  if (err->has_error())
    return false;

  for (size_t i = first_item; i < items_.size(); ++i) {
    CHECK(items_[i]->AsTarget() != nullptr)
        << "Only targets are supported in ExecuteSnippet()";
    items_[i]->AsTarget()->SetToolchain(&toolchain_);
    if (!items_[i]->OnResolved(err))
      return false;
  }
  return true;
}

Value TestWithScope::ExecuteExpression(const std::string& expr, Err* err) {
  InputFile input_file(SourceFile("//test"));
  input_file.SetContents(expr);

  std::vector<Token> tokens = Tokenizer::Tokenize(&input_file, err);
  if (err->has_error()) {
    return Value();
  }
  std::unique_ptr<ParseNode> node = Parser::ParseExpression(tokens, err);
  if (err->has_error()) {
    return Value();
  }

  return node->Execute(&scope_, err);
}

// static
void TestWithScope::SetupToolchain(Toolchain* toolchain, bool use_toc) {
  Err err;

  // CC
  std::unique_ptr<Tool> cc_tool = Tool::CreateTool(CTool::kCToolCc);
  SetCommandForTool(
      "cc {{source}} {{cflags}} {{cflags_c}} {{defines}} {{include_dirs}} "
      "-o {{output}}",
      cc_tool.get());
  cc_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o"));
  toolchain->SetTool(std::move(cc_tool));

  // CXX
  std::unique_ptr<Tool> cxx_tool = Tool::CreateTool(CTool::kCToolCxx);
  SetCommandForTool(
      "c++ {{source}} {{cflags}} {{cflags_cc}} {{defines}} {{include_dirs}} "
      "-o {{output}}",
      cxx_tool.get());
  cxx_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o"));
  cxx_tool->set_command_launcher("launcher");
  toolchain->SetTool(std::move(cxx_tool));

  // OBJC
  std::unique_ptr<Tool> objc_tool = Tool::CreateTool(CTool::kCToolObjC);
  SetCommandForTool(
      "objcc {{source}} {{cflags}} {{cflags_objc}} {{defines}} "
      "{{include_dirs}} -o {{output}}",
      objc_tool.get());
  objc_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o"));
  toolchain->SetTool(std::move(objc_tool));

  // OBJC
  std::unique_ptr<Tool> objcxx_tool = Tool::CreateTool(CTool::kCToolObjCxx);
  SetCommandForTool(
      "objcxx {{source}} {{cflags}} {{cflags_objcc}} {{defines}} "
      "{{include_dirs}} -o {{output}}",
      objcxx_tool.get());
  objcxx_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o"));
  toolchain->SetTool(std::move(objcxx_tool));

  // Don't use RC and ASM tools in unit tests yet. Add here if needed.

  // ALINK
  std::unique_ptr<Tool> alink = Tool::CreateTool(CTool::kCToolAlink);
  CTool* alink_tool = alink->AsC();
  SetCommandForTool("ar {{output}} {{source}}", alink_tool);
  alink_tool->set_lib_switch("-l");
  alink_tool->set_lib_dir_switch("-L");
  alink_tool->set_output_prefix("lib");
  alink_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{target_out_dir}}/{{target_output_name}}.a"));
  toolchain->SetTool(std::move(alink));

  // SOLINK
  std::unique_ptr<Tool> solink = Tool::CreateTool(CTool::kCToolSolink);
  CTool* solink_tool = solink->AsC();
  SetCommandForTool(
      "ld -shared -o {{target_output_name}}.so {{inputs}} "
      "{{ldflags}} {{libs}}",
      solink_tool);
  solink_tool->set_lib_switch("-l");
  solink_tool->set_lib_dir_switch("-L");
  solink_tool->set_output_prefix("lib");
  solink_tool->set_default_output_extension(".so");
  if (use_toc) {
    solink_tool->set_outputs(SubstitutionList::MakeForTest(
        "{{root_out_dir}}/{{target_output_name}}{{output_extension}}.TOC",
        "{{root_out_dir}}/{{target_output_name}}{{output_extension}}"));
    solink_tool->set_link_output(SubstitutionPattern::MakeForTest(
        "{{root_out_dir}}/{{target_output_name}}{{output_extension}}"));
    solink_tool->set_depend_output(SubstitutionPattern::MakeForTest(
        "{{root_out_dir}}/{{target_output_name}}{{output_extension}}.TOC"));
  } else {
    solink_tool->set_outputs(SubstitutionList::MakeForTest(
        "{{root_out_dir}}/{{target_output_name}}{{output_extension}}"));
  }
  toolchain->SetTool(std::move(solink));

  // SOLINK_MODULE
  std::unique_ptr<Tool> solink_module =
      Tool::CreateTool(CTool::kCToolSolinkModule);
  CTool* solink_module_tool = solink_module->AsC();
  SetCommandForTool(
      "ld -bundle -o {{target_output_name}}.so {{inputs}} "
      "{{ldflags}} {{libs}}",
      solink_module_tool);
  solink_module_tool->set_lib_switch("-l");
  solink_module_tool->set_lib_dir_switch("-L");
  solink_module_tool->set_output_prefix("lib");
  solink_module_tool->set_default_output_extension(".so");
  solink_module_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{root_out_dir}}/{{target_output_name}}{{output_extension}}"));
  toolchain->SetTool(std::move(solink_module));

  // LINK
  std::unique_ptr<Tool> link = Tool::CreateTool(CTool::kCToolLink);
  CTool* link_tool = link->AsC();
  SetCommandForTool(
      "ld -o {{target_output_name}} {{source}} "
      "{{ldflags}} {{libs}}",
      link_tool);
  link_tool->set_lib_switch("-l");
  link_tool->set_lib_dir_switch("-L");
  link_tool->set_outputs(
      SubstitutionList::MakeForTest("{{root_out_dir}}/{{target_output_name}}"));
  toolchain->SetTool(std::move(link));

  // STAMP
  std::unique_ptr<Tool> stamp_tool =
      Tool::CreateTool(GeneralTool::kGeneralToolStamp);
  SetCommandForTool("touch {{output}}", stamp_tool.get());
  toolchain->SetTool(std::move(stamp_tool));

  // COPY
  std::unique_ptr<Tool> copy_tool =
      Tool::CreateTool(GeneralTool::kGeneralToolCopy);
  SetCommandForTool("cp {{source}} {{output}}", copy_tool.get());
  toolchain->SetTool(std::move(copy_tool));

  // COPY_BUNDLE_DATA
  std::unique_ptr<Tool> copy_bundle_data_tool =
      Tool::CreateTool(GeneralTool::kGeneralToolCopyBundleData);
  SetCommandForTool("cp {{source}} {{output}}", copy_bundle_data_tool.get());
  toolchain->SetTool(std::move(copy_bundle_data_tool));

  // COMPILE_XCASSETS
  std::unique_ptr<Tool> compile_xcassets_tool =
      Tool::CreateTool(GeneralTool::kGeneralToolCompileXCAssets);
  SetCommandForTool("touch {{output}}", compile_xcassets_tool.get());
  toolchain->SetTool(std::move(compile_xcassets_tool));

  // RUST
  std::unique_ptr<Tool> rustc_tool = Tool::CreateTool(RustTool::kRsToolBin);
  SetCommandForTool(
      "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} "
      "--crate-type {{crate_type}} {{rustflags}} -o {{output}} "
      "{{rustdeps}} {{externs}}",
      rustc_tool.get());
  rustc_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{root_out_dir}}/{{crate_name}}{{output_extension}}"));
  toolchain->SetTool(std::move(rustc_tool));

  // SWIFT
  std::unique_ptr<Tool> swift_tool = Tool::CreateTool(CTool::kCToolSwift);
  SetCommandForTool(
      "swiftc --module-name {{module_name}} {{module_dirs}} {{inputs}}",
      swift_tool.get());
  swift_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{target_out_dir}}/{{module_name}}.swiftmodule"));
  swift_tool->set_partial_outputs(SubstitutionList::MakeForTest(
      "{{target_out_dir}}/{{source_name_part}}.o"));
  toolchain->SetTool(std::move(swift_tool));

  // RUST CDYLIB
  std::unique_ptr<Tool> cdylib_tool = Tool::CreateTool(RustTool::kRsToolCDylib);
  SetCommandForTool(
      "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} "
      "--crate-type {{crate_type}} {{rustflags}} -o {{output}} "
      "{{rustdeps}} {{externs}}",
      cdylib_tool.get());
  cdylib_tool->set_output_prefix("lib");
  cdylib_tool->set_default_output_extension(".so");
  cdylib_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{target_out_dir}}/{{target_output_name}}{{output_extension}}"));
  toolchain->SetTool(std::move(cdylib_tool));

  // RUST DYLIB
  std::unique_ptr<Tool> dylib_tool = Tool::CreateTool(RustTool::kRsToolDylib);
  SetCommandForTool(
      "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} "
      "--crate-type {{crate_type}} {{rustflags}} -o {{output}} "
      "{{rustdeps}} {{externs}}",
      dylib_tool.get());
  dylib_tool->set_output_prefix("lib");
  dylib_tool->set_default_output_extension(".so");
  dylib_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{target_out_dir}}/{{target_output_name}}{{output_extension}}"));
  toolchain->SetTool(std::move(dylib_tool));

  // RUST_PROC_MACRO
  std::unique_ptr<Tool> rust_proc_macro_tool =
      Tool::CreateTool(RustTool::kRsToolMacro);
  SetCommandForTool(
      "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} "
      "--crate-type {{crate_type}} {{rustflags}} -o {{output}} "
      "{{rustdeps}} {{externs}}",
      rust_proc_macro_tool.get());
  rust_proc_macro_tool->set_output_prefix("lib");
  rust_proc_macro_tool->set_default_output_extension(".so");
  rust_proc_macro_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{target_out_dir}}/{{target_output_name}}{{output_extension}}"));
  toolchain->SetTool(std::move(rust_proc_macro_tool));

  // RLIB
  std::unique_ptr<Tool> rlib_tool = Tool::CreateTool(RustTool::kRsToolRlib);
  SetCommandForTool(
      "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} "
      "--crate-type {{crate_type}} {{rustflags}} -o {{output}} "
      "{{rustdeps}} {{externs}}",
      rlib_tool.get());
  rlib_tool->set_output_prefix("lib");
  rlib_tool->set_default_output_extension(".rlib");
  rlib_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{target_out_dir}}/{{target_output_name}}{{output_extension}}"));
  toolchain->SetTool(std::move(rlib_tool));

  // RUST STATICLIB
  std::unique_ptr<Tool> staticlib_tool =
      Tool::CreateTool(RustTool::kRsToolStaticlib);
  SetCommandForTool(
      "{{rustenv}} rustc --crate-name {{crate_name}} {{source}} "
      "--crate-type {{crate_type}} {{rustflags}} -o {{output}} "
      "{{rustdeps}} {{externs}}",
      staticlib_tool.get());
  staticlib_tool->set_output_prefix("lib");
  staticlib_tool->set_default_output_extension(".a");
  staticlib_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{target_out_dir}}/{{target_output_name}}{{output_extension}}"));
  static_cast<RustTool*>(staticlib_tool.get())->set_dynamic_link_switch("-Clink-arg=-Balternative-dynamic");
  toolchain->SetTool(std::move(staticlib_tool));

  toolchain->ToolchainSetupComplete();
}

// static
void TestWithScope::SetCommandForTool(const std::string& cmd, Tool* tool) {
  Err err;
  SubstitutionPattern command;
  command.Parse(cmd, nullptr, &err);
  CHECK(!err.has_error()) << "Couldn't parse \"" << cmd << "\", "
                          << "got " << err.message();
  tool->set_command(command);
}

void TestWithScope::AppendPrintOutput(const std::string& str) {
  print_output_.append(str);
}

TestParseInput::TestParseInput(const std::string& input)
    : input_file_(SourceFile("//test")) {
  input_file_.SetContents(input);

  tokens_ = Tokenizer::Tokenize(&input_file_, &parse_err_);
  if (!parse_err_.has_error())
    parsed_ = Parser::Parse(tokens_, &parse_err_);
}

TestParseInput::~TestParseInput() = default;

TestTarget::TestTarget(const TestWithScope& setup,
                       const std::string& label_string,
                       Target::OutputType type)
    : Target(setup.settings(), setup.ParseLabel(label_string)) {
  visibility().SetPublic();
  set_output_type(type);
  SetToolchain(setup.toolchain());
}

TestTarget::~TestTarget() = default;
