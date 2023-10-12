// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/ninja_action_target_writer.h"

#include <stddef.h>

#include "base/strings/string_util.h"
#include "gn/deps_iterator.h"
#include "gn/err.h"
#include "gn/general_tool.h"
#include "gn/pool.h"
#include "gn/settings.h"
#include "gn/string_utils.h"
#include "gn/substitution_writer.h"
#include "gn/target.h"

NinjaActionTargetWriter::NinjaActionTargetWriter(const Target* target,
                                                 std::ostream& out)
    : NinjaTargetWriter(target, out),
      path_output_no_escaping_(
          target->settings()->build_settings()->build_dir(),
          target->settings()->build_settings()->root_path_utf8(),
          ESCAPE_NONE) {}

NinjaActionTargetWriter::~NinjaActionTargetWriter() = default;

void NinjaActionTargetWriter::Run() {
  std::string custom_rule_name = WriteRuleDefinition();

  // Collect our deps to pass as additional "hard dependencies" for input deps.
  // This will force all of the action's dependencies to be completed before
  // the action is run. Usually, if an action has a dependency, it will be
  // operating on the result of that previous step, so we need to be sure to
  // serialize these.
  std::vector<const Target*> additional_hard_deps;
  std::vector<OutputFile> order_only_deps;
  const auto& target_deps = resolved().GetTargetDeps(target_);

  for (const Target* dep : target_deps.linked_deps()) {
    if (dep->IsDataOnly()) {
      order_only_deps.push_back(dep->dependency_output_file());
    } else {
      additional_hard_deps.push_back(dep);
    }
  }

  // Add all data-deps to the order-only-deps for the action.  The data_deps
  // field is used to implement different use-cases, including:
  //
  //  - Files needed at only runtime by the outputs of the action, and therefore
  //    need be built if ninja is building the action's outputs.  But they do
  //    not "dirty" the action's outputs if the data_deps alone are "dirty".
  //    If ninja had the concept of "weak" dependencies, that would be used
  //    instead, but that isn't available, so order-only dependencies are used.
  //
  //  - Files that _may_ need to be used to perform the action, and a depfile
  //    will be used to promote these order-only deps to implicit dependencies,
  //    and on an incremental build, if the now-implicit dependencies are
  //    'dirty', this action will be considered 'dirty' as well.
  //
  for (const Target* data_dep : target_deps.data_deps())
    order_only_deps.push_back(data_dep->dependency_output_file());

  // For ACTIONs, the input deps appear only once in the generated ninja
  // file, so WriteInputDepsStampAndGetDep() won't create a stamp file
  // and the action will just depend on all the input deps directly.
  size_t num_stamp_uses =
      target_->output_type() == Target::ACTION ? 1u : target_->sources().size();
  std::vector<OutputFile> input_deps =
      WriteInputDepsStampAndGetDep(additional_hard_deps, num_stamp_uses);
  out_ << std::endl;

  // Collects all output files for writing below.
  std::vector<OutputFile> output_files;

  if (target_->output_type() == Target::ACTION_FOREACH) {
    // Write separate build lines for each input source file.
    WriteSourceRules(custom_rule_name, input_deps, order_only_deps,
                     &output_files);
  } else {
    DCHECK(target_->output_type() == Target::ACTION);

    // Write a rule that invokes the script once with the outputs as outputs,
    // and the data as inputs. It does not depend on the sources.
    out_ << "build";
    SubstitutionWriter::GetListAsOutputFiles(
        settings_, target_->action_values().outputs(), &output_files);
    path_output_.WriteFiles(out_, output_files);

    out_ << ": " << custom_rule_name;
    if (!input_deps.empty()) {
      // As in WriteSourceRules, we want to force this target to rebuild any
      // time any of its dependencies change.
      out_ << " |";
      path_output_.WriteFiles(out_, input_deps);
    }
    if (!order_only_deps.empty()) {
      // Write any order-only deps out for actions just like they are for
      // binaries.
      out_ << " ||";
      path_output_.WriteFiles(out_, order_only_deps);
    }

    out_ << std::endl;
    if (target_->action_values().has_depfile()) {
      WriteDepfile(SourceFile());
    }

    WriteNinjaVariablesForAction();

    if (target_->pool().ptr) {
      out_ << "  pool = ";
      out_ << target_->pool().ptr->GetNinjaName(
          settings_->default_toolchain_label());
      out_ << std::endl;
    }
  }
  out_ << std::endl;

  // Write the stamp, which doesn't need to depend on the data deps because they
  // have been added as order-only deps of the action output itself.
  //
  // TODO(thakis): If the action has just a single output, make things depend
  // on that output directly without writing a stamp file.
  std::vector<OutputFile> stamp_file_order_only_deps;
  WriteStampForTarget(output_files, stamp_file_order_only_deps);
}

std::string NinjaActionTargetWriter::WriteRuleDefinition() {
  // Make a unique name for this rule.
  //
  // Use a unique name for the response file when there are multiple build
  // steps so that they don't stomp on each other. When there are no sources,
  // there will be only one invocation so we can use a simple name.
  std::string target_label = target_->label().GetUserVisibleName(true);
  std::string custom_rule_name(target_label);
  base::ReplaceChars(custom_rule_name, ":/()+", "_", &custom_rule_name);
  custom_rule_name.append("_rule");

  const SubstitutionList& args = target_->action_values().args();
  EscapeOptions args_escape_options;
  args_escape_options.mode = ESCAPE_NINJA_COMMAND;

  out_ << "rule " << custom_rule_name << std::endl;

  if (target_->action_values().uses_rsp_file()) {
    // Needs a response file. The unique_name part is for action_foreach so
    // each invocation of the rule gets a different response file. This isn't
    // strictly necessary for regular one-shot actions, but it's easier to
    // just always define unique_name.
    std::string rspfile = custom_rule_name;
    if (!target_->sources().empty())
      rspfile += ".$unique_name";
    rspfile += ".rsp";
    out_ << "  rspfile = " << rspfile << std::endl;

    // Response file contents.
    out_ << "  rspfile_content =";
    for (const auto& arg :
         target_->action_values().rsp_file_contents().list()) {
      out_ << " ";
      SubstitutionWriter::WriteWithNinjaVariables(arg, args_escape_options,
                                                  out_);
    }
    out_ << std::endl;
  }

  // The command line requires shell escaping to properly handle filenames
  // with spaces.
  PathOutput command_output(path_output_.current_dir(),
                            settings_->build_settings()->root_path_utf8(),
                            ESCAPE_NINJA_COMMAND);

  out_ << "  command = ";
  command_output.WriteFile(out_, settings_->build_settings()->python_path());
  out_ << " ";
  command_output.WriteFile(out_, target_->action_values().script());
  for (const auto& arg : args.list()) {
    out_ << " ";
    SubstitutionWriter::WriteWithNinjaVariables(arg, args_escape_options, out_);
  }
  out_ << std::endl;
  auto mnemonic = target_->action_values().mnemonic();
  if (mnemonic.empty())
    mnemonic = "ACTION";
  out_ << "  description = " << mnemonic << " " << target_label << std::endl;
  out_ << "  restat = 1" << std::endl;
  const Tool* tool =
      target_->toolchain()->GetTool(GeneralTool::kGeneralToolAction);
  if (tool && tool->pool().ptr) {
    out_ << "  pool = ";
    out_ << tool->pool().ptr->GetNinjaName(
        settings_->default_toolchain_label());
    out_ << std::endl;
  }

  return custom_rule_name;
}

void NinjaActionTargetWriter::WriteSourceRules(
    const std::string& custom_rule_name,
    const std::vector<OutputFile>& input_deps,
    const std::vector<OutputFile>& order_only_deps,
    std::vector<OutputFile>* output_files) {
  EscapeOptions args_escape_options;
  args_escape_options.mode = ESCAPE_NINJA_COMMAND;
  // We're writing the substitution values, these should not be quoted since
  // they will get pasted into the real command line.
  args_escape_options.inhibit_quoting = true;

  const Target::FileList& sources = target_->sources();
  for (size_t i = 0; i < sources.size(); i++) {
    out_ << "build";
    WriteOutputFilesForBuildLine(sources[i], output_files);

    out_ << ": " << custom_rule_name << " ";
    path_output_.WriteFile(out_, sources[i]);
    if (!input_deps.empty()) {
      // Using "|" for the dependencies forces all implicit dependencies to be
      // fully up to date before running the action, and will re-run this
      // action if any input dependencies change. This is important because
      // this action may consume the outputs of previous steps.
      out_ << " |";
      path_output_.WriteFiles(out_, input_deps);
    }
    if (!order_only_deps.empty()) {
      // Write any order-only deps out for actions just like they are written
      // out for binaries.
      out_ << " ||";
      path_output_.WriteFiles(out_, order_only_deps);
    }
    out_ << std::endl;

    // Response files require a unique name be defined.
    if (target_->action_values().uses_rsp_file())
      out_ << "  unique_name = " << i << std::endl;

    // The required types is the union of the args and response file. This
    // might theoretically duplicate a definition if the same substitution is
    // used in both the args and the response file. However, this should be
    // very unusual (normally the substitutions will go in one place or the
    // other) and the redundant assignment won't bother Ninja.
    SubstitutionWriter::WriteNinjaVariablesForSource(
        target_, settings_, sources[i],
        target_->action_values().args().required_types(), args_escape_options,
        out_);
    SubstitutionWriter::WriteNinjaVariablesForSource(
        target_, settings_, sources[i],
        target_->action_values().rsp_file_contents().required_types(),
        args_escape_options, out_);
    WriteNinjaVariablesForAction();

    if (target_->action_values().has_depfile()) {
      WriteDepfile(sources[i]);
    }
    if (target_->pool().ptr) {
      out_ << "  pool = ";
      out_ << target_->pool().ptr->GetNinjaName(
          settings_->default_toolchain_label());
      out_ << std::endl;
    }
  }
}

void NinjaActionTargetWriter::WriteOutputFilesForBuildLine(
    const SourceFile& source,
    std::vector<OutputFile>* output_files) {
  size_t first_output_index = output_files->size();

  SubstitutionWriter::ApplyListToSourceAsOutputFile(
      target_, settings_, target_->action_values().outputs(), source,
      output_files);

  for (size_t i = first_output_index; i < output_files->size(); i++) {
    out_ << " ";
    path_output_.WriteFile(out_, (*output_files)[i]);
  }
}

void NinjaActionTargetWriter::WriteDepfile(const SourceFile& source) {
  out_ << "  depfile = ";
  path_output_.WriteFile(
      out_,
      SubstitutionWriter::ApplyPatternToSourceAsOutputFile(
          target_, settings_, target_->action_values().depfile(), source));
  out_ << std::endl;
  // Using "deps = gcc" allows Ninja to read and store the depfile content in
  // its internal database which improves performance, especially for large
  // depfiles. The use of this feature with depfiles that contain multiple
  // outputs require Ninja version 1.9.0 or newer.
  if (settings_->build_settings()->ninja_required_version() >=
      Version{1, 9, 0}) {
    out_ << "  deps = gcc" << std::endl;
  }
}

void NinjaActionTargetWriter::WriteNinjaVariablesForAction() {
  SubstitutionBits subst;
  target_->action_values().args().FillRequiredTypes(&subst);
  WriteRustCompilerVars(subst, /*indent=*/true, /*always_write=*/false);
  WriteCCompilerVars(subst, /*indent=*/true, /*respect_source_types=*/false);
}
