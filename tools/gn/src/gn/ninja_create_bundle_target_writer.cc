// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/ninja_create_bundle_target_writer.h"

#include <iterator>

#include "base/strings/string_util.h"
#include "gn/filesystem_utils.h"
#include "gn/general_tool.h"
#include "gn/ninja_utils.h"
#include "gn/output_file.h"
#include "gn/scheduler.h"
#include "gn/substitution_writer.h"
#include "gn/target.h"
#include "gn/toolchain.h"

namespace {

bool TargetRequireAssetCatalogCompilation(const Target* target) {
  return !target->bundle_data().assets_catalog_sources().empty() ||
         !target->bundle_data().partial_info_plist().is_null();
}

void FailWithMissingToolError(const char* tool_name, const Target* target) {
  g_scheduler->FailWithError(
      Err(nullptr, std::string(tool_name) + " tool not defined",
          "The toolchain " +
              target->toolchain()->label().GetUserVisibleName(false) +
              "\n"
              "used by target " +
              target->label().GetUserVisibleName(false) +
              "\n"
              "doesn't define a \"" +
              tool_name + "\" tool."));
}

bool EnsureAllToolsAvailable(const Target* target) {
  const char* kRequiredTools[] = {
      GeneralTool::kGeneralToolCopyBundleData,
      GeneralTool::kGeneralToolStamp,
  };

  for (size_t i = 0; i < std::size(kRequiredTools); ++i) {
    if (!target->toolchain()->GetTool(kRequiredTools[i])) {
      FailWithMissingToolError(kRequiredTools[i], target);
      return false;
    }
  }

  // The compile_xcassets tool is only required if the target has asset
  // catalog resources to compile.
  if (TargetRequireAssetCatalogCompilation(target)) {
    if (!target->toolchain()->GetTool(
            GeneralTool::kGeneralToolCompileXCAssets)) {
      FailWithMissingToolError(GeneralTool::kGeneralToolCompileXCAssets,
                               target);
      return false;
    }
  }

  return true;
}

}  // namespace

NinjaCreateBundleTargetWriter::NinjaCreateBundleTargetWriter(
    const Target* target,
    std::ostream& out)
    : NinjaTargetWriter(target, out) {}

NinjaCreateBundleTargetWriter::~NinjaCreateBundleTargetWriter() = default;

void NinjaCreateBundleTargetWriter::Run() {
  if (!EnsureAllToolsAvailable(target_))
    return;

  // Stamp users are CopyBundleData, CompileAssetsCatalog, CodeSigning and
  // StampForTarget.
  size_t num_stamp_uses = 4;
  std::vector<OutputFile> order_only_deps = WriteInputDepsStampAndGetDep(
      std::vector<const Target*>(), num_stamp_uses);

  std::string code_signing_rule_name = WriteCodeSigningRuleDefinition();

  std::vector<OutputFile> output_files;
  WriteCopyBundleDataSteps(order_only_deps, &output_files);
  WriteCompileAssetsCatalogStep(order_only_deps, &output_files);
  WriteCodeSigningStep(code_signing_rule_name, order_only_deps, &output_files);

  for (const Target* data_dep : resolved().GetDataDeps(target_))
    order_only_deps.push_back(data_dep->dependency_output_file());
  WriteStampForTarget(output_files, order_only_deps);

  // Write a phony target for the outer bundle directory. This allows other
  // targets to treat the entire bundle as a single unit, even though it is
  // a directory, so that it can be depended upon as a discrete build edge.
  out_ << "build ";
  path_output_.WriteFile(
      out_,
      OutputFile(settings_->build_settings(),
                 target_->bundle_data().GetBundleRootDirOutput(settings_)));
  out_ << ": phony " << target_->dependency_output_file().value();
  out_ << std::endl;
}

std::string NinjaCreateBundleTargetWriter::WriteCodeSigningRuleDefinition() {
  if (target_->bundle_data().code_signing_script().is_null())
    return std::string();

  std::string target_label = target_->label().GetUserVisibleName(true);
  std::string custom_rule_name(target_label);
  base::ReplaceChars(custom_rule_name, ":/()", "_", &custom_rule_name);
  custom_rule_name.append("_code_signing_rule");

  out_ << "rule " << custom_rule_name << std::endl;
  out_ << "  command = ";
  path_output_.WriteFile(out_, settings_->build_settings()->python_path());
  out_ << " ";
  path_output_.WriteFile(out_, target_->bundle_data().code_signing_script());

  const SubstitutionList& args = target_->bundle_data().code_signing_args();
  EscapeOptions args_escape_options;
  args_escape_options.mode = ESCAPE_NINJA_COMMAND;

  for (const auto& arg : args.list()) {
    out_ << " ";
    SubstitutionWriter::WriteWithNinjaVariables(arg, args_escape_options, out_);
  }
  out_ << std::endl;
  out_ << "  description = CODE SIGNING " << target_label << std::endl;
  out_ << "  restat = 1" << std::endl;
  out_ << std::endl;

  return custom_rule_name;
}

void NinjaCreateBundleTargetWriter::WriteCopyBundleDataSteps(
    const std::vector<OutputFile>& order_only_deps,
    std::vector<OutputFile>* output_files) {
  for (const BundleFileRule& file_rule : target_->bundle_data().file_rules())
    WriteCopyBundleFileRuleSteps(file_rule, order_only_deps, output_files);
}

void NinjaCreateBundleTargetWriter::WriteCopyBundleFileRuleSteps(
    const BundleFileRule& file_rule,
    const std::vector<OutputFile>& order_only_deps,
    std::vector<OutputFile>* output_files) {
  // Note that we don't write implicit deps for copy steps. "copy_bundle_data"
  // steps as this is most likely implemented using hardlink in the common case.
  // See NinjaCopyTargetWriter::WriteCopyRules() for a detailed explanation.
  for (const SourceFile& source_file : file_rule.sources()) {
    // There is no need to check for errors here as the substitution will have
    // been performed when computing the list of output of the target during
    // the Target::OnResolved phase earlier.
    OutputFile expanded_output_file;
    file_rule.ApplyPatternToSourceAsOutputFile(
        settings_, target_, target_->bundle_data(), source_file,
        &expanded_output_file,
        /*err=*/nullptr);
    output_files->push_back(expanded_output_file);

    out_ << "build ";
    path_output_.WriteFile(out_, expanded_output_file);
    out_ << ": " << GetNinjaRulePrefixForToolchain(settings_)
         << GeneralTool::kGeneralToolCopyBundleData << " ";
    path_output_.WriteFile(out_, source_file);

    if (!order_only_deps.empty()) {
      out_ << " ||";
      path_output_.WriteFiles(out_, order_only_deps);
    }

    out_ << std::endl;
  }
}

void NinjaCreateBundleTargetWriter::WriteCompileAssetsCatalogStep(
    const std::vector<OutputFile>& order_only_deps,
    std::vector<OutputFile>* output_files) {
  if (!TargetRequireAssetCatalogCompilation(target_))
    return;

  OutputFile compiled_catalog;
  if (!target_->bundle_data().assets_catalog_sources().empty()) {
    compiled_catalog =
        OutputFile(settings_->build_settings(),
                   target_->bundle_data().GetCompiledAssetCatalogPath());
    output_files->push_back(compiled_catalog);
  }

  OutputFile partial_info_plist;
  if (!target_->bundle_data().partial_info_plist().is_null()) {
    partial_info_plist =
        OutputFile(settings_->build_settings(),
                   target_->bundle_data().partial_info_plist());

    output_files->push_back(partial_info_plist);
  }

  // If there are no asset catalog to compile but the "partial_info_plist" is
  // non-empty, then add a target to generate an empty file (to avoid breaking
  // code that depends on this file existence).
  if (target_->bundle_data().assets_catalog_sources().empty()) {
    DCHECK(!target_->bundle_data().partial_info_plist().is_null());

    out_ << "build ";
    path_output_.WriteFile(out_, partial_info_plist);
    out_ << ": " << GetNinjaRulePrefixForToolchain(settings_)
         << GeneralTool::kGeneralToolStamp;
    if (!order_only_deps.empty()) {
      out_ << " ||";
      path_output_.WriteFiles(out_, order_only_deps);
    }
    out_ << std::endl;
    return;
  }

  OutputFile input_dep = WriteCompileAssetsCatalogInputDepsStamp(
      target_->bundle_data().assets_catalog_deps());
  DCHECK(!input_dep.value().empty());

  out_ << "build ";
  path_output_.WriteFile(out_, compiled_catalog);
  if (partial_info_plist != OutputFile()) {
    // If "partial_info_plist" is non-empty, then add it to list of implicit
    // outputs of the asset catalog compilation, so that target can use it
    // without getting the ninja error "'foo', needed by 'bar', missing and
    // no known rule to make it".
    out_ << " | ";
    path_output_.WriteFile(out_, partial_info_plist);
  }

  out_ << ": " << GetNinjaRulePrefixForToolchain(settings_)
       << GeneralTool::kGeneralToolCompileXCAssets;

  SourceFileSet asset_catalog_bundles;
  for (const auto& source : target_->bundle_data().assets_catalog_sources()) {
    out_ << " ";
    path_output_.WriteFile(out_, source);
    asset_catalog_bundles.insert(source);
  }

  out_ << " | ";
  path_output_.WriteFile(out_, input_dep);

  if (!order_only_deps.empty()) {
    out_ << " ||";
    path_output_.WriteFiles(out_, order_only_deps);
  }

  out_ << std::endl;

  out_ << "  product_type = " << target_->bundle_data().product_type()
       << std::endl;

  if (partial_info_plist != OutputFile()) {
    out_ << "  partial_info_plist = ";
    path_output_.WriteFile(out_, partial_info_plist);
    out_ << std::endl;
  }

  const std::vector<SubstitutionPattern>& flags =
      target_->bundle_data().xcasset_compiler_flags().list();
  if (!flags.empty()) {
    out_ << "  " << SubstitutionXcassetsCompilerFlags.ninja_name << " =";
    EscapeOptions args_escape_options;
    args_escape_options.mode = ESCAPE_NINJA_COMMAND;
    for (const auto& flag : flags) {
      out_ << " ";
      SubstitutionWriter::WriteWithNinjaVariables(flag, args_escape_options,
                                                  out_);
    }
    out_ << std::endl;
  }
}

OutputFile
NinjaCreateBundleTargetWriter::WriteCompileAssetsCatalogInputDepsStamp(
    const std::vector<const Target*>& dependencies) {
  DCHECK(!dependencies.empty());
  if (dependencies.size() == 1)
    return dependencies[0]->dependency_output_file();

  OutputFile xcassets_input_stamp_file =
      GetBuildDirForTargetAsOutputFile(target_, BuildDirType::OBJ);
  xcassets_input_stamp_file.value().append(target_->label().name());
  xcassets_input_stamp_file.value().append(".xcassets.inputdeps.stamp");

  out_ << "build ";
  path_output_.WriteFile(out_, xcassets_input_stamp_file);
  out_ << ": " << GetNinjaRulePrefixForToolchain(settings_)
       << GeneralTool::kGeneralToolStamp;

  for (const Target* target : dependencies) {
    out_ << " ";
    path_output_.WriteFile(out_, target->dependency_output_file());
  }
  out_ << std::endl;
  return xcassets_input_stamp_file;
}

void NinjaCreateBundleTargetWriter::WriteCodeSigningStep(
    const std::string& code_signing_rule_name,
    const std::vector<OutputFile>& order_only_deps,
    std::vector<OutputFile>* output_files) {
  if (code_signing_rule_name.empty())
    return;

  OutputFile code_signing_input_stamp_file =
      WriteCodeSigningInputDepsStamp(order_only_deps, output_files);
  DCHECK(!code_signing_input_stamp_file.value().empty());

  out_ << "build";
  std::vector<OutputFile> code_signing_output_files;
  SubstitutionWriter::GetListAsOutputFiles(
      settings_, target_->bundle_data().code_signing_outputs(),
      &code_signing_output_files);
  path_output_.WriteFiles(out_, code_signing_output_files);

  // Since the code signature step depends on all the files from the bundle,
  // the create_bundle stamp can just depends on the output of the signature
  // script (dependencies are transitive).
  *output_files = std::move(code_signing_output_files);

  out_ << ": " << code_signing_rule_name;
  out_ << " | ";
  path_output_.WriteFile(out_, code_signing_input_stamp_file);
  out_ << std::endl;
}

OutputFile NinjaCreateBundleTargetWriter::WriteCodeSigningInputDepsStamp(
    const std::vector<OutputFile>& order_only_deps,
    std::vector<OutputFile>* output_files) {
  std::vector<SourceFile> code_signing_input_files;
  code_signing_input_files.push_back(
      target_->bundle_data().code_signing_script());
  code_signing_input_files.insert(
      code_signing_input_files.end(),
      target_->bundle_data().code_signing_sources().begin(),
      target_->bundle_data().code_signing_sources().end());
  for (const OutputFile& output_file : *output_files) {
    code_signing_input_files.push_back(
        output_file.AsSourceFile(settings_->build_settings()));
  }

  DCHECK(!code_signing_input_files.empty());
  if (code_signing_input_files.size() == 1 && order_only_deps.empty())
    return OutputFile(settings_->build_settings(), code_signing_input_files[0]);

  OutputFile code_signing_input_stamp_file =
      GetBuildDirForTargetAsOutputFile(target_, BuildDirType::OBJ);
  code_signing_input_stamp_file.value().append(target_->label().name());
  code_signing_input_stamp_file.value().append(".codesigning.inputdeps.stamp");

  out_ << "build ";
  path_output_.WriteFile(out_, code_signing_input_stamp_file);
  out_ << ": " << GetNinjaRulePrefixForToolchain(settings_)
       << GeneralTool::kGeneralToolStamp;

  for (const SourceFile& source : code_signing_input_files) {
    out_ << " ";
    path_output_.WriteFile(out_, source);
  }
  if (!order_only_deps.empty()) {
    out_ << " ||";
    path_output_.WriteFiles(out_, order_only_deps);
  }
  out_ << std::endl;
  return code_signing_input_stamp_file;
}
