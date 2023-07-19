// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/functions.h"

#include "gn/config_values_generator.h"
#include "gn/err.h"
#include "gn/parse_tree.h"
#include "gn/scope.h"
#include "gn/target_generator.h"
#include "gn/template.h"
#include "gn/value.h"
#include "gn/variables.h"

#define DEPENDENT_CONFIG_VARS \
  "  Dependent configs: all_dependent_configs, public_configs\n"
#define DEPS_VARS \
  "  Deps: assert_no_deps, data_deps, deps, public_deps, runtime_deps,\n" \
  "        write_runtime_deps\n"
#define GENERAL_TARGET_VARS                                                \
  "  General: check_includes, configs, data, friend, inputs, metadata,\n"  \
  "           output_extension, output_name, public, sources, testonly,\n" \
  "           visibility\n"
#define RUST_VARS \
  "  Rust variables: aliased_deps, crate_root, crate_name\n"
#define RUST_SHARED_VARS                                                 \
  "  Rust variables: aliased_deps, crate_root, crate_name, crate_type\n"
#define ACTION_VARS \
  "  Action variables: args, bridge_header, configs, data, depfile,\n" \
  "                    framework_dirs, inputs, mnemonic, module_deps,\n" \
  "                    module_name, outputs*, pool, response_file_contents,\n" \
  "                    script*, sources\n"

namespace functions {

namespace {

Value ExecuteGenericTarget(const char* target_type,
                           Scope* scope,
                           const FunctionCallNode* function,
                           const std::vector<Value>& args,
                           BlockNode* block,
                           Err* err) {
  NonNestableBlock non_nestable(scope, function, "target");
  if (!non_nestable.Enter(err))
    return Value();

  if (!EnsureNotProcessingImport(function, scope, err) ||
      !EnsureNotProcessingBuildConfig(function, scope, err))
    return Value();
  Scope block_scope(scope);
  if (!FillTargetBlockScope(scope, function, target_type, block, args,
                            &block_scope, err))
    return Value();

  block->Execute(&block_scope, err);
  if (err->has_error())
    return Value();

  TargetGenerator::GenerateTarget(&block_scope, function, args, target_type,
                                  err);
  if (err->has_error())
    return Value();

  block_scope.CheckForUnusedVars(err);
  return Value();
}

}  // namespace

// action ----------------------------------------------------------------------

// Common help paragraph on script runtime execution directories.
#define SCRIPT_EXECUTION_CONTEXT                                              \
  "\n"                                                                        \
  "  The script will be executed with the given arguments with the current\n" \
  "  directory being that of the root build directory. If you pass files\n"   \
  "  to your script, see \"gn help rebase_path\" for how to convert\n"        \
  "  file names to be relative to the build directory (file names in the\n"   \
  "  sources, outputs, and inputs will be all treated as relative to the\n"   \
  "  current build file and converted as needed automatically).\n"            \
  "\n"                                                                        \
  "  GN sets Ninja's flag 'restat = 1` for all action commands. This means\n" \
  "  that Ninja will check the timestamp of the output after the action\n"    \
  "  completes. If output timestamp is unchanged, the step will be treated\n" \
  "  as if it never needed to be rebuilt, potentially eliminating some\n"     \
  "  downstream steps for incremental builds. Scripts can improve build\n"    \
  "  performance by taking care not to change the timstamp of the output\n"   \
  "  file(s) if the contents have not changed.\n"

// Common help paragraph on script output directories.
#define SCRIPT_EXECUTION_OUTPUTS                                           \
  "\n"                                                                     \
  "  All output files must be inside the output directory of the build.\n" \
  "  You would generally use |$target_out_dir| or |$target_gen_dir| to\n"  \
  "  reference the output or generated intermediate file directories,\n"   \
  "  respectively.\n"

#define ACTION_DEPS                                                           \
  "\n"                                                                        \
  "  The \"deps\" and \"public_deps\" for an action will always be\n"         \
  "  completed before any part of the action is run so it can depend on\n"    \
  "  the output of previous steps. The \"data_deps\" will be built if the\n"  \
  "  action is built, but may not have completed before all steps of the\n"   \
  "  action are started. This can give additional parallelism in the build\n" \
  "  for runtime-only dependencies.\n"

// Common help paragraph on targets that can use different languages.
#define LANGUAGE_HELP                                                     \
  "\n"                                                                    \
  "  The tools and commands used to create this target type will be\n"    \
  "  determined by the source files in its sources. Targets containing\n" \
  "  multiple compiler-incompatible languages are not allowed (e.g. a\n"  \
  "  target containing both C and C++ sources is acceptable, but a\n"     \
  "  target containing C and Rust sources is not).\n"

const char kAction[] = "action";
const char kAction_HelpShort[] =
    "action: Declare a target that runs a script a single time.";
const char kAction_Help[] =
    R"(action: Declare a target that runs a script a single time.

  This target type allows you to run a script a single time to produce one or
  more output files. If you want to run a script once for each of a set of
  input files, see "gn help action_foreach".

Inputs

  In an action the "sources" and "inputs" are treated the same: they're both
  input dependencies on script execution with no special handling. If you want
  to pass the sources to your script, you must do so explicitly by including
  them in the "args". Note also that this means there is no special handling of
  paths since GN doesn't know which of the args are paths and not. You will
  want to use rebase_path() to convert paths to be relative to the
  root_build_dir.

  You can dynamically write input dependencies (for incremental rebuilds if an
  input file changes) by writing a depfile when the script is run (see "gn help
  depfile"). This is more flexible than "inputs".

  If the command line length is very long, you can use response files to pass
  args to your script. See "gn help response_file_contents".

  It is recommended you put inputs to your script in the "sources" variable,
  and stuff like other Python files required to run your script in the "inputs"
  variable.

  Actions can take the configs and public_configs lists, as well as any of the
  configs variables (defines, include_dirs, etc.) set directly on the target.
  These behave exactly as they would on a binary target and can be accessed
  using substitution patterns in the script args (see "gn help args") to
  implement custom compiler-like tools.
)"

    ACTION_DEPS

    R"(
Outputs

  You should specify files created by your script by specifying them in the
  "outputs".
)"

    SCRIPT_EXECUTION_CONTEXT

    R"(
File name handling
)"

    SCRIPT_EXECUTION_OUTPUTS

    R"(
Variables

)" CONFIG_VALUES_VARS_HELP DEPENDENT_CONFIG_VARS DEPS_VARS GENERAL_TARGET_VARS
ACTION_VARS

R"(  * = required

Example

  action("run_this_guy_once") {
    script = "doprocessing.py"
    sources = [ "my_configuration.txt" ]
    outputs = [ "$target_gen_dir/insightful_output.txt" ]

    # Our script imports this Python file so we want to rebuild if it changes.
    inputs = [ "helper_library.py" ]

    # Note that we have to manually pass the sources to our script if the
    # script needs them as inputs.
    args = [ "--out", rebase_path(target_gen_dir, root_build_dir) ] +
           rebase_path(sources, root_build_dir)
  }
)";

Value RunAction(Scope* scope,
                const FunctionCallNode* function,
                const std::vector<Value>& args,
                BlockNode* block,
                Err* err) {
  return ExecuteGenericTarget(functions::kAction, scope, function, args, block,
                              err);
}

// action_foreach --------------------------------------------------------------

const char kActionForEach[] = "action_foreach";
const char kActionForEach_HelpShort[] =
    "action_foreach: Declare a target that runs a script over a set of files.";
const char kActionForEach_Help[] =
    R"(action_foreach: Declare a target that runs a script over a set of files.

  This target type allows you to run a script once-per-file over a set of
  sources. If you want to run a script once that takes many files as input, see
  "gn help action".

Inputs

  The script will be run once per file in the "sources" variable. The "outputs"
  variable should specify one or more files with a source expansion pattern in
  it (see "gn help source_expansion"). The output file(s) for each script
  invocation should be unique. Normally you use "{{source_name_part}}" in each
  output file.

  If your script takes additional data as input, such as a shared configuration
  file or a Python module it uses, those files should be listed in the "inputs"
  variable. These files are treated as dependencies of each script invocation.

  If the command line length is very long, you can use response files to pass
  args to your script. See "gn help response_file_contents".

  You can dynamically write input dependencies (for incremental rebuilds if an
  input file changes) by writing a depfile when the script is run (see "gn help
  depfile"). This is more flexible than "inputs".
)" ACTION_DEPS
    R"(
Outputs
)" SCRIPT_EXECUTION_CONTEXT
    R"(
File name handling
)" SCRIPT_EXECUTION_OUTPUTS
    R"(
Variables

)" CONFIG_VALUES_VARS_HELP DEPENDENT_CONFIG_VARS DEPS_VARS GENERAL_TARGET_VARS
ACTION_VARS

R"(  * = required

Example

  # Runs the script over each IDL file. The IDL script will generate both a .cc
  # and a .h file for each input.
  action_foreach("my_idl") {
    script = "idl_processor.py"
    sources = [ "foo.idl", "bar.idl" ]

    # Causes ninja to output "IDL <label>" rather than the default
    # "ACTION <label>" when building this action.
    mnemonic = "IDL"

    # Our script reads this file each time, so we need to list it as a
    # dependency so we can rebuild if it changes.
    inputs = [ "my_configuration.txt" ]

    # Transformation from source file name to output file names.
    outputs = [ "$target_gen_dir/{{source_name_part}}.h",
                "$target_gen_dir/{{source_name_part}}.cc" ]

    # Note that since "args" is opaque to GN, if you specify paths here, you
    # will need to convert it to be relative to the build directory using
    # rebase_path().
    args = [
      "{{source}}",
      "-o",
      rebase_path(target_gen_dir, root_build_dir) +
        "/{{source_name_part}}.h" ]
  }
)";

Value RunActionForEach(Scope* scope,
                       const FunctionCallNode* function,
                       const std::vector<Value>& args,
                       BlockNode* block,
                       Err* err) {
  return ExecuteGenericTarget(functions::kActionForEach, scope, function, args,
                              block, err);
}

// bundle_data -----------------------------------------------------------------

const char kBundleData[] = "bundle_data";
const char kBundleData_HelpShort[] =
    "bundle_data: [iOS/macOS] Declare a target without output.";
const char kBundleData_Help[] =
    R"(bundle_data: [iOS/macOS] Declare a target without output.

  This target type allows one to declare data that is required at runtime. It is
  used to inform "create_bundle" targets of the files to copy into generated
  bundle, see "gn help create_bundle" for help.

  The target must define a list of files as "sources" and a single "outputs".
  If there are multiple files, source expansions must be used to express the
  output. The output must reference a file inside of {{bundle_root_dir}}.

  This target can be used on all platforms though it is designed only to
  generate iOS/macOS bundle. In cross-platform projects, it is advised to put it
  behind iOS/macOS conditionals.

  See "gn help create_bundle" for more information.

Variables

)" DEPENDENT_CONFIG_VARS DEPS_VARS GENERAL_TARGET_VARS

R"(  Bundle-specific: sources*, outputs*
  * = required

Examples

  bundle_data("icudata") {
    sources = [ "sources/data/in/icudtl.dat" ]
    outputs = [ "{{bundle_resources_dir}}/{{source_file_part}}" ]
  }

  bundle_data("base_unittests_bundle_data]") {
    sources = [ "test/data" ]
    outputs = [
      "{{bundle_resources_dir}}/{{source_root_relative_dir}}/" +
          "{{source_file_part}}"
    ]
  }

  bundle_data("material_typography_bundle_data") {
    sources = [
      "src/MaterialTypography.bundle/Roboto-Bold.ttf",
      "src/MaterialTypography.bundle/Roboto-Italic.ttf",
      "src/MaterialTypography.bundle/Roboto-Regular.ttf",
      "src/MaterialTypography.bundle/Roboto-Thin.ttf",
    ]
    outputs = [
      "{{bundle_resources_dir}}/MaterialTypography.bundle/"
          "{{source_file_part}}"
    ]
  }
)";

Value RunBundleData(Scope* scope,
                    const FunctionCallNode* function,
                    const std::vector<Value>& args,
                    BlockNode* block,
                    Err* err) {
  return ExecuteGenericTarget(functions::kBundleData, scope, function, args,
                              block, err);
}

// create_bundle ---------------------------------------------------------------

const char kCreateBundle[] = "create_bundle";
const char kCreateBundle_HelpShort[] =
    "create_bundle: [iOS/macOS] Build an iOS or macOS bundle.";
const char kCreateBundle_Help[] =
    R"(create_bundle: [ios/macOS] Build an iOS or macOS bundle.

  This target generates an iOS or macOS bundle (which is a directory with a
  well-know structure). This target does not define any sources, instead they
  are computed from all "bundle_data" target this one depends on transitively
  (the recursion stops at "create_bundle" targets).

  The "bundle_*_dir" are be used for the expansion of {{bundle_*_dir}} rules in
  "bundle_data" outputs. The properties are optional but must be defined if any
  of the "bundle_data" target use them.

  This target can be used on all platforms though it is designed only to
  generate iOS or macOS bundle. In cross-platform projects, it is advised to put
  it behind iOS/macOS conditionals.

  If a create_bundle is specified as a data_deps for another target, the bundle
  is considered a leaf, and its public and private dependencies will not
  contribute to any data or data_deps. Required runtime dependencies should be
  placed in the bundle. A create_bundle can declare its own explicit data and
  data_deps, however.

Code signing

  Some bundle needs to be code signed as part of the build (on iOS all
  application needs to be code signed to run on a device). The code signature
  can be configured via the code_signing_script variable.

  If set, code_signing_script is the path of a script that invoked after all
  files have been moved into the bundle. The script must not change any file in
  the bundle, but may add new files.

  If code_signing_script is defined, then code_signing_outputs must also be
  defined and non-empty to inform when the script needs to be re-run. The
  code_signing_args will be passed as is to the script (so path have to be
  rebased) and additional inputs may be listed with the variable
  code_signing_sources.

Variables

)" DEPENDENT_CONFIG_VARS DEPS_VARS GENERAL_TARGET_VARS

R"(  Bundle vars: bundle_root_dir, bundle_contents_dir, bundle_resources_dir,
               bundle_executable_dir, bundle_deps_filter, product_type,
               code_signing_args, code_signing_script, code_signing_sources,
               code_signing_outputs, xcode_extra_attributes,
               xcode_test_application_name, partial_info_plist

Example

  # Defines a template to create an application. On most platform, this is just
  # an alias for an "executable" target, but on iOS/macOS, it builds an
  # application bundle.
  template("app") {
    if (!is_ios && !is_mac) {
      executable(target_name) {
        forward_variables_from(invoker, "*")
      }
    } else {
      app_name = target_name
      gen_path = target_gen_dir

      action("${app_name}_generate_info_plist") {
        script = [ "//build/ios/ios_gen_plist.py" ]
        sources = [ "templates/Info.plist" ]
        outputs = [ "$gen_path/Info.plist" ]
        args = rebase_path(sources, root_build_dir) +
               rebase_path(outputs, root_build_dir)
      }

      bundle_data("${app_name}_bundle_info_plist") {
        public_deps = [ ":${app_name}_generate_info_plist" ]
        sources = [ "$gen_path/Info.plist" ]
        outputs = [ "{{bundle_contents_dir}}/Info.plist" ]
      }

      executable("${app_name}_generate_executable") {
        forward_variables_from(invoker, "*", [
                                               "output_name",
                                               "visibility",
                                             ])
        output_name =
            rebase_path("$gen_path/$app_name", root_build_dir)
      }

      code_signing =
          defined(invoker.code_signing) && invoker.code_signing

      if (!is_ios || !code_signing) {
        bundle_data("${app_name}_bundle_executable") {
          public_deps = [ ":${app_name}_generate_executable" ]
          sources = [ "$gen_path/$app_name" ]
          outputs = [ "{{bundle_executable_dir}}/$app_name" ]
        }
      }

      create_bundle("$app_name.app") {
        product_type = "com.apple.product-type.application"

        if (is_ios) {
          bundle_root_dir = "$root_build_dir/$target_name"
          bundle_contents_dir = bundle_root_dir
          bundle_resources_dir = bundle_contents_dir
          bundle_executable_dir = bundle_contents_dir

          xcode_extra_attributes = {
            ONLY_ACTIVE_ARCH = "YES"
            DEBUG_INFORMATION_FORMAT = "dwarf"
          }
        } else {
          bundle_root_dir = "$root_build_dir/$target_name"
          bundle_contents_dir  = "$bundle_root_dir/Contents"
          bundle_resources_dir = "$bundle_contents_dir/Resources"
          bundle_executable_dir = "$bundle_contents_dir/MacOS"
        }
        deps = [ ":${app_name}_bundle_info_plist" ]
        if (is_ios && code_signing) {
          deps += [ ":${app_name}_generate_executable" ]
          code_signing_script = "//build/config/ios/codesign.py"
          code_signing_sources = [
            invoker.entitlements_path,
            "$target_gen_dir/$app_name",
          ]
          code_signing_outputs = [
            "$bundle_root_dir/$app_name",
            "$bundle_root_dir/_CodeSignature/CodeResources",
            "$bundle_root_dir/embedded.mobileprovision",
            "$target_gen_dir/$app_name.xcent",
          ]
          code_signing_args = [
            "-i=" + ios_code_signing_identity,
            "-b=" + rebase_path(
                "$target_gen_dir/$app_name", root_build_dir),
            "-e=" + rebase_path(
                invoker.entitlements_path, root_build_dir),
            "-e=" + rebase_path(
                "$target_gen_dir/$app_name.xcent", root_build_dir),
            rebase_path(bundle_root_dir, root_build_dir),
          ]
        } else {
          deps += [ ":${app_name}_bundle_executable" ]
        }
      }
    }
  }
)";

Value RunCreateBundle(Scope* scope,
                      const FunctionCallNode* function,
                      const std::vector<Value>& args,
                      BlockNode* block,
                      Err* err) {
  return ExecuteGenericTarget(functions::kCreateBundle, scope, function, args,
                              block, err);
}

// copy ------------------------------------------------------------------------

const char kCopy[] = "copy";
const char kCopy_HelpShort[] = "copy: Declare a target that copies files.";
const char kCopy_Help[] =
    R"(copy: Declare a target that copies files.

File name handling

  All output files must be inside the output directory of the build. You would
  generally use |$target_out_dir| or |$target_gen_dir| to reference the output
  or generated intermediate file directories, respectively.

  Both "sources" and "outputs" must be specified. Sources can include as many
  files as you want, but there can only be one item in the outputs list (plural
  is used for the name for consistency with other target types).

  If there is more than one source file, your output name should specify a
  mapping from each source file to an output file name using source expansion
  (see "gn help source_expansion"). The placeholders will look like
  "{{source_name_part}}", for example.

  If you want to copy the output of a previous build step, the target that
  generates the file to copy must be reachable from the deps or public_deps of
  the copy target.

Variables

)" DEPENDENT_CONFIG_VARS DEPS_VARS GENERAL_TARGET_VARS

R"(  Copy variables: sources*, outputs*
  * = required

Examples

  # Write a rule that copies a checked-in DLL to the output directory.
  copy("mydll") {
    sources = [ "mydll.dll" ]
    outputs = [ "$target_out_dir/mydll.dll" ]
  }

  # Write a rule to copy several files to the target generated files directory.
  copy("myfiles") {
    sources = [ "data1.dat", "data2.dat", "data3.dat" ]

    # Use source expansion to generate output files with the corresponding file
    # names in the gen dir. This will just copy each file.
    outputs = [ "$target_gen_dir/{{source_file_part}}" ]
  }

  # Copy the output of a generated executable.
  copy("package_melon") {
    # This example uses get_label_info() to compute the output directory of the
    # dependency. This allows the copy rule to work regardless of the toolchain.
    #
    # In some cases (particularly actions defined previously in the same file)
    # you can use get_target_outputs() to get the input file which can eliminate
    # the assumptions about the output file name of the dependency.

    input_dir = get_label_info("//src/tools/melon", "root_out_dir");
    sources = [ "$input_dir/melon" ]

    outputs = [ "$target_gen_dir/{{source_file_part}}" ]

    # Depend on the target to build the file before copying.
    deps = [ "//src/tools/melon" ]
  }
)";

Value RunCopy(const FunctionCallNode* function,
              const std::vector<Value>& args,
              Scope* scope,
              Err* err) {
  if (!EnsureNotProcessingImport(function, scope, err) ||
      !EnsureNotProcessingBuildConfig(function, scope, err))
    return Value();
  TargetGenerator::GenerateTarget(scope, function, args, functions::kCopy, err);
  return Value();
}

// executable ------------------------------------------------------------------

const char kExecutable[] = "executable";
const char kExecutable_HelpShort[] =
    "executable: Declare an executable target.";
const char kExecutable_Help[] =
    R"(executable: Declare an executable target.

Language and compilation
)" LANGUAGE_HELP
    R"(

Variables

)" CONFIG_VALUES_VARS_HELP DEPS_VARS DEPENDENT_CONFIG_VARS GENERAL_TARGET_VARS
        RUST_VARS;

Value RunExecutable(Scope* scope,
                    const FunctionCallNode* function,
                    const std::vector<Value>& args,
                    BlockNode* block,
                    Err* err) {
  return ExecuteGenericTarget(functions::kExecutable, scope, function, args,
                              block, err);
}

// group -----------------------------------------------------------------------

const char kGroup[] = "group";
const char kGroup_HelpShort[] = "group: Declare a named group of targets.";
const char kGroup_Help[] =
    R"(group: Declare a named group of targets.

  This target type allows you to create meta-targets that just collect a set of
  dependencies into one named target. Groups can additionally specify configs
  that apply to their dependents.

Variables

)" DEPS_VARS DEPENDENT_CONFIG_VARS GENERAL_TARGET_VARS

    R"(
Example

  group("all") {
    deps = [
      "//project:runner",
      "//project:unit_tests",
    ]
  }
)";

Value RunGroup(Scope* scope,
               const FunctionCallNode* function,
               const std::vector<Value>& args,
               BlockNode* block,
               Err* err) {
  return ExecuteGenericTarget(functions::kGroup, scope, function, args, block,
                              err);
}

// loadable_module -------------------------------------------------------------

const char kLoadableModule[] = "loadable_module";
const char kLoadableModule_HelpShort[] =
    "loadable_module: Declare a loadable module target.";
const char kLoadableModule_Help[] =
    R"(loadable_module: Declare a loadable module target.

  This target type allows you to create an object file that is (and can only
  be) loaded and unloaded at runtime.

  A loadable module will be specified on the linker line for targets listing
  the loadable module in its "deps". If you don't want this (if you don't need
  to dynamically load the library at runtime), then you should use a
  "shared_library" target type instead.

Language and compilation
)" LANGUAGE_HELP
    R"(

Variables

)" CONFIG_VALUES_VARS_HELP DEPS_VARS DEPENDENT_CONFIG_VARS GENERAL_TARGET_VARS
        RUST_SHARED_VARS;

Value RunLoadableModule(Scope* scope,
                        const FunctionCallNode* function,
                        const std::vector<Value>& args,
                        BlockNode* block,
                        Err* err) {
  return ExecuteGenericTarget(functions::kLoadableModule, scope, function, args,
                              block, err);
}

// rust_library ----------------------------------------------------------------

const char kRustLibrary[] = "rust_library";
const char kRustLibrary_HelpShort[] =
    "rust_library: Declare a Rust library target.";
const char kRustLibrary_Help[] =
    R"(rust_library: Declare a Rust library target.

  A Rust library is an archive containing additional rust-c provided metadata.
  These are the files produced by the rustc compiler with the `.rlib`
  extension, and are the intermediate step for most Rust-based binaries.

Language and compilation
)" LANGUAGE_HELP
    R"(

Variables

)" CONFIG_VALUES_VARS_HELP DEPS_VARS DEPENDENT_CONFIG_VARS GENERAL_TARGET_VARS
        RUST_VARS;
Value RunRustLibrary(Scope* scope,
                     const FunctionCallNode* function,
                     const std::vector<Value>& args,
                     BlockNode* block,
                     Err* err) {
  return ExecuteGenericTarget(functions::kRustLibrary, scope, function, args,
                              block, err);
}

// rust_proc_macro ----------------------------------------------------------------

const char kRustProcMacro[] = "rust_proc_macro";
const char kRustProcMacro_HelpShort[] =
    "rust_proc_macro: Declare a Rust procedural macro target.";
const char kRustProcMacro_Help[] =
    R"(rust_proc_macro: Declare a Rust procedural macro target.

  A Rust procedural macro allows creating syntax extensions as execution of a
  function. They are compiled as dynamic libraries and used by the compiler at
  runtime.

  Their use is the same as of other Rust libraries, but their build has some
  additional restrictions in terms of supported flags.

Language and compilation
)" LANGUAGE_HELP
    R"(

Variables

)" CONFIG_VALUES_VARS_HELP DEPS_VARS DEPENDENT_CONFIG_VARS GENERAL_TARGET_VARS
        RUST_VARS;
Value RunRustProcMacro(Scope* scope,
                   const FunctionCallNode* function,
                   const std::vector<Value>& args,
                   BlockNode* block,
                   Err* err) {
  return ExecuteGenericTarget(functions::kRustProcMacro, scope, function, args,
                              block, err);
}

// shared_library --------------------------------------------------------------

const char kSharedLibrary[] = "shared_library";
const char kSharedLibrary_HelpShort[] =
    "shared_library: Declare a shared library target.";
const char kSharedLibrary_Help[] =
    R"(shared_library: Declare a shared library target.

  A shared library will be specified on the linker line for targets listing the
  shared library in its "deps". If you don't want this (say you dynamically
  load the library at runtime), then you should depend on the shared library
  via "data_deps" or, on Darwin platforms, use a "loadable_module" target type
  instead.

Language and compilation
)" LANGUAGE_HELP
    R"(

Variables

)" CONFIG_VALUES_VARS_HELP DEPS_VARS DEPENDENT_CONFIG_VARS GENERAL_TARGET_VARS
        RUST_SHARED_VARS;

Value RunSharedLibrary(Scope* scope,
                       const FunctionCallNode* function,
                       const std::vector<Value>& args,
                       BlockNode* block,
                       Err* err) {
  return ExecuteGenericTarget(functions::kSharedLibrary, scope, function, args,
                              block, err);
}

// source_set ------------------------------------------------------------------

const char kSourceSet[] = "source_set";
const char kSourceSet_HelpShort[] = "source_set: Declare a source set target.";
const char kSourceSet_Help[] =
    R"(source_set: Declare a source set target.

  Only C-language source sets are supported at the moment.

C-language source_sets

  A source set is a collection of sources that get compiled, but are not linked
  to produce any kind of library. Instead, the resulting object files are
  implicitly added to the linker line of all targets that depend on the source
  set.

  In most cases, a source set will behave like a static library, except no
  actual library file will be produced. This will make the build go a little
  faster by skipping creation of a large static library, while maintaining the
  organizational benefits of focused build targets.

  The main difference between a source set and a static library is around
  handling of exported symbols. Most linkers assume declaring a function
  exported means exported from the static library. The linker can then do dead
  code elimination to delete code not reachable from exported functions.

  A source set will not do this code elimination since there is no link step.
  This allows you to link many source sets into a shared library and have the
  "exported symbol" notation indicate "export from the final shared library and
  not from the intermediate targets." There is no way to express this concept
  when linking multiple static libraries into a shared library.

Variables

)" CONFIG_VALUES_VARS_HELP DEPS_VARS DEPENDENT_CONFIG_VARS GENERAL_TARGET_VARS;

Value RunSourceSet(Scope* scope,
                   const FunctionCallNode* function,
                   const std::vector<Value>& args,
                   BlockNode* block,
                   Err* err) {
  return ExecuteGenericTarget(functions::kSourceSet, scope, function, args,
                              block, err);
}

// static_library --------------------------------------------------------------

const char kStaticLibrary[] = "static_library";
const char kStaticLibrary_HelpShort[] =
    "static_library: Declare a static library target.";
const char kStaticLibrary_Help[] =
    R"(static_library: Declare a static library target.

  Make a ".a" / ".lib" file.

  If you only need the static library for intermediate results in the build,
  you should consider a source_set instead since it will skip the (potentially
  slow) step of creating the intermediate library file.

Variables

  complete_static_lib
)" CONFIG_VALUES_VARS_HELP DEPS_VARS DEPENDENT_CONFIG_VARS GENERAL_TARGET_VARS
        RUST_VARS LANGUAGE_HELP;

Value RunStaticLibrary(Scope* scope,
                       const FunctionCallNode* function,
                       const std::vector<Value>& args,
                       BlockNode* block,
                       Err* err) {
  return ExecuteGenericTarget(functions::kStaticLibrary, scope, function, args,
                              block, err);
}

// target ---------------------------------------------------------------------

const char kTarget[] = "target";
const char kTarget_HelpShort[] =
    "target: Declare a target with the given programmatic type.";
const char kTarget_Help[] =
    R"(target: Declare a target with the given programmatic type.

  target(target_type_string, target_name_string) { ... }

  The target() function is a way to invoke a built-in target or template with a
  type determined at runtime. This is useful for cases where the type of a
  target might not be known statically.

  Only templates and built-in target functions are supported for the
  target_type_string parameter. Arbitrary functions, configs, and toolchains
  are not supported.

  The call:
    target("source_set", "doom_melon") {
  Is equivalent to:
    source_set("doom_melon") {

Common target variables

)" DEPS_VARS DEPENDENT_CONFIG_VARS GENERAL_TARGET_VARS

R"(
  Targets will also have variables specific to that type, see "gn help <type>"
  for more.

Example

  if (foo_build_as_shared) {
    my_type = "shared_library"
  } else {
    my_type = "source_set"
  }

  target(my_type, "foo") {
    ...
  }
)";
Value RunTarget(Scope* scope,
                const FunctionCallNode* function,
                const std::vector<Value>& args,
                BlockNode* block,
                Err* err) {
  if (args.size() != 2) {
    *err = Err(function, "Expected two arguments.", "Try \"gn help target\".");
    return Value();
  }

  // The first argument must be a string (the target type). Don't type-check
  // the second argument since the target-specific function will do that.
  if (!args[0].VerifyTypeIs(Value::STRING, err))
    return Value();
  const std::string& target_type = args[0].string_value();

  // The rest of the args are passed to the function.
  std::vector<Value> sub_args(args.begin() + 1, args.end());

  // Run a template if it is one.
  const Template* templ = scope->GetTemplate(target_type);
  if (templ)
    return templ->Invoke(scope, function, target_type, sub_args, block, err);

  // Otherwise, assume the target is a built-in target type.
  return ExecuteGenericTarget(target_type.c_str(), scope, function, sub_args,
                              block, err);
}

const char kGeneratedFile[] = "generated_file";
const char kGeneratedFile_HelpShort[] =
    "generated_file: Declare a generated_file target.";
const char kGeneratedFile_Help[] =
    R"(generated_file: Declare a generated_file target.

  Writes data value(s) to disk on resolution. This target type mirrors some
  functionality of the write_file() function, but also provides the ability to
  collect metadata from its dependencies on resolution rather than writing out
  at parse time.

  The `outputs` variable is required to be a list with a single element,
  specifying the intended location of the output file.

  The `output_conversion` variable specified the format to write the
  value. See `gn help io_conversion`.

  One of `contents` or `data_keys` must be specified; use of `contents` will
  write the contents of that value to file, while use of `data_keys` will
  trigger a metadata collection walk based on the dependencies of the target and
  the optional values of the `rebase` and `walk_keys` variables. See
  `gn help metadata`.

  Collected metadata, if specified, will be returned in postorder of
  dependencies. See the example for details.

Variables

)" DEPENDENT_CONFIG_VARS DEPS_VARS GENERAL_TARGET_VARS

R"(  Generated file: contents, data_keys, rebase, walk_keys, output_conversion

Example (metadata collection)

  Given the following targets defined in //base/BUILD.gn, where A depends on B
  and B depends on C and D:

    group("a") {
      metadata = {
        doom_melon = [ "enable" ]
        my_files = [ "foo.cpp" ]

        # Note: this is functionally equivalent to not defining `my_barrier`
        # at all in this target's metadata.
        my_barrier = [ "" ]
      }

      deps = [ ":b" ]
    }

    group("b") {
      metadata = {
        my_files = [ "bar.cpp" ]
        my_barrier = [ ":c" ]
      }

      deps = [ ":c", ":d" ]
    }

    group("c") {
      metadata = {
        doom_melon = [ "disable" ]
        my_files = [ "baz.cpp" ]
      }
    }

    group("d") {
      metadata = {
        my_files = [ "missing.cpp" ]
      }
    }

  If the following generated_file target is defined:

    generated_file("my_files_metadata") {
      outputs = [ "$root_build_dir/my_files.json" ]
      data_keys = [ "my_files" ]

      deps = [ "//base:a" ]
    }

  The following will be written to "$root_build_dir/my_files.json" (less the
  comments):
    [
      "baz.cpp",  // from //base:c via //base:b
      "missing.cpp"  // from //base:d via //base:b
      "bar.cpp",  // from //base:b via //base:a
      "foo.cpp",  // from //base:a
    ]

  Alternatively, as an example of using walk_keys, if the following
  generated_file target is defined:

  generated_file("my_files_metadata") {
    outputs = [ "$root_build_dir/my_files.json" ]
    data_keys = [ "my_files" ]
    walk_keys = [ "my_barrier" ]

    deps = [ "//base:a" ]
  }

  The following will be written to "$root_build_dir/my_files.json" (again less
  the comments):
    [
      "baz.cpp",  // from //base:c via //base:b
      "bar.cpp",  // from //base:b via //base:a
      "foo.cpp",  // from //base:a
    ]

  If `rebase` is used in the following generated_file target:

  generated_file("my_files_metadata") {
    outputs = [ "$root_build_dir/my_files.json" ]
    data_keys = [ "my_files" ]
    walk_keys = [ "my_barrier" ]
    rebase = root_build_dir

    deps = [ "//base:a" ]
  }

  The following will be written to "$root_build_dir/my_files.json" (again less
  the comments) (assuming root_build_dir = "//out"):
    [
      "../base/baz.cpp",  // from //base:c via //base:b
      "../base/bar.cpp",  // from //base:b via //base:a
      "../base/foo.cpp",  // from //base:a
    ]
)";

Value RunGeneratedFile(Scope* scope,
                       const FunctionCallNode* function,
                       const std::vector<Value>& args,
                       BlockNode* block,
                       Err* err) {
  return ExecuteGenericTarget(functions::kGeneratedFile, scope, function, args,
                              block, err);
}

}  // namespace functions
