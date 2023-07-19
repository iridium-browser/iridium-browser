// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/build_settings.h"
#include "gn/functions.h"
#include "gn/parse_tree.h"
#include "gn/settings.h"
#include "gn/substitution_writer.h"
#include "gn/target.h"
#include "gn/value.h"

namespace functions {

const char kGetTargetOutputs[] = "get_target_outputs";
const char kGetTargetOutputs_HelpShort[] =
    "get_target_outputs: [file list] Get the list of outputs from a target.";
const char kGetTargetOutputs_Help[] =
    R"(get_target_outputs: [file list] Get the list of outputs from a target.

  get_target_outputs(target_label)

  Returns a list of output files for the named target. The named target must
  have been previously defined in the current file before this function is
  called (it can't reference targets in other files because there isn't a
  defined execution order, and it obviously can't reference targets that are
  defined after the function call).

  Only copy, generated_file, and action targets are supported. The outputs from
  binary targets will depend on the toolchain definition which won't
  necessarily have been loaded by the time a given line of code has run, and
  source sets and groups have no useful output file.

Return value

  The names in the resulting list will be absolute file paths (normally like
  "//out/Debug/bar.exe", depending on the build directory).

  action, copy, and generated_file targets: this will just return the files
  specified in the "outputs" variable of the target.

  action_foreach targets: this will return the result of applying the output
  template to the sources (see "gn help source_expansion"). This will be the
  same result (though with guaranteed absolute file paths), as
  process_file_template will return for those inputs (see "gn help
  process_file_template").

  source sets and groups: this will return a list containing the path of the
  "stamp" file that Ninja will produce once all outputs are generated. This
  probably isn't very useful.

Example

  # Say this action generates a bunch of C source files.
  action_foreach("my_action") {
    sources = [ ... ]
    outputs = [ ... ]
  }

  # Compile the resulting source files into a source set.
  source_set("my_lib") {
    sources = get_target_outputs(":my_action")
  }
)";

Value RunGetTargetOutputs(Scope* scope,
                          const FunctionCallNode* function,
                          const std::vector<Value>& args,
                          Err* err) {
  if (args.size() != 1) {
    *err = Err(function, "Expected one argument.");
    return Value();
  }

  // Resolve the requested label.
  Label label =
      Label::Resolve(scope->GetSourceDir(),
                     scope->settings()->build_settings()->root_path_utf8(),
                     ToolchainLabelForScope(scope), args[0], err);
  if (label.is_null())
    return Value();

  // Find the referenced target. The targets previously encountered in this
  // scope will have been stashed in the item collector (they'll be dispatched
  // when this file is done running) so we can look through them.
  const Target* target = nullptr;
  Scope::ItemVector* collector = scope->GetItemCollector();
  if (!collector) {
    *err = Err(function, "No targets defined in this context.");
    return Value();
  }
  for (const auto& item : *collector) {
    if (item->label() != label)
      continue;

    const Target* as_target = item->AsTarget();
    if (!as_target) {
      *err = Err(function, "Label does not refer to a target.",
                 label.GetUserVisibleName(false) + "\nrefers to a " +
                     item->GetItemTypeName());
      return Value();
    }
    target = as_target;
    break;
  }

  if (!target) {
    bool with_toolchain = !scope->settings()->is_default();
    *err = Err(function, "Target not found in this context.",
               label.GetUserVisibleName(with_toolchain) +
                   "\nwas not found. get_target_outputs() can only be used for "
                   "targets\n"
                   "previously defined in the current file.");
    return Value();
  }

  // Range for GetOutputsAsSourceFiles to blame for errors.
  LocationRange arg_range;
  if (args[0].origin())
    arg_range = args[0].origin()->GetRange();

  std::vector<SourceFile> files;

  // The build is currently running so only non-binary targets (they don't
  // depend on the toolchain definition which may not have been loaded yet) can
  // be queried. Pass false for build_complete so it will flag such queries as
  // an error.
  if (!target->GetOutputsAsSourceFiles(arg_range, false, &files, err))
    return Value();

  // Convert to Values.
  Value ret(function, Value::LIST);
  ret.list_value().reserve(files.size());
  for (const auto& file : files)
    ret.list_value().push_back(Value(function, file.value()));

  return ret;
}

}  // namespace functions
