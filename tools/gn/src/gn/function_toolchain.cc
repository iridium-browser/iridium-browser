// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include "gn/c_tool.h"
#include "gn/err.h"
#include "gn/functions.h"
#include "gn/general_tool.h"
#include "gn/label.h"
#include "gn/label_ptr.h"
#include "gn/parse_tree.h"
#include "gn/scheduler.h"
#include "gn/scope.h"
#include "gn/settings.h"
#include "gn/tool.h"
#include "gn/toolchain.h"
#include "gn/value_extractors.h"
#include "gn/variables.h"

namespace functions {

namespace {

// This is just a unique value to take the address of to use as the key for
// the toolchain property on a scope.
const int kToolchainPropertyKey = 0;

}  // namespace

// toolchain -------------------------------------------------------------------

const char kToolchain[] = "toolchain";
const char kToolchain_HelpShort[] = "toolchain: Defines a toolchain.";
const char kToolchain_Help[] =
    R"*(toolchain: Defines a toolchain.

  A toolchain is a set of commands and build flags used to compile the source
  code. The toolchain() function defines these commands.

Toolchain overview

  You can have more than one toolchain in use at once in a build and a target
  can exist simultaneously in multiple toolchains. A build file is executed
  once for each toolchain it is referenced in so the GN code can vary all
  parameters of each target (or which targets exist) on a per-toolchain basis.

  When you have a simple build with only one toolchain, the build config file
  is loaded only once at the beginning of the build. It must call
  set_default_toolchain() (see "gn help set_default_toolchain") to tell GN the
  label of the toolchain definition to use. The "toolchain_args" section of the
  toolchain definition is ignored.

  When a target has a dependency on a target using different toolchain (see "gn
  help labels" for how to specify this), GN will start a build using that
  secondary toolchain to resolve the target. GN will load the build config file
  with the build arguments overridden as specified in the toolchain_args.
  Because the default toolchain is already known, calls to
  set_default_toolchain() are ignored.

  To load a file in an alternate toolchain, GN does the following:

    1. Loads the file with the toolchain definition in it (as determined by the
       toolchain label).
    2. Re-runs the master build configuration file, applying the arguments
       specified by the toolchain_args section of the toolchain definition.
    3. Loads the destination build file in the context of the configuration file
       in the previous step.

  The toolchain configuration is two-way. In the default toolchain (i.e. the
  main build target) the configuration flows from the build config file to the
  toolchain. The build config file looks at the state of the build (OS type,
  CPU architecture, etc.) and decides which toolchain to use (via
  set_default_toolchain()). In secondary toolchains, the configuration flows
  from the toolchain to the build config file: the "toolchain_args" in the
  toolchain definition specifies the arguments to re-invoke the build.

Functions and variables

  tool()
    The tool() function call specifies the commands to run for a given step. See
    "gn help tool".

  toolchain_args [scope]
    Overrides for build arguments to pass to the toolchain when invoking it.
    This is a variable of type "scope" where the variable names correspond to
    variables in declare_args() blocks.

    When you specify a target using an alternate toolchain, the master build
    configuration file is re-interpreted in the context of that toolchain.
    toolchain_args allows you to control the arguments passed into this
    alternate invocation of the build.

    Any default system arguments or arguments passed in via "gn args" will also
    be passed to the alternate invocation unless explicitly overridden by
    toolchain_args.

    The toolchain_args will be ignored when the toolchain being defined is the
    default. In this case, it's expected you want the default argument values.

    See also "gn help buildargs" for an overview of these arguments.

  propagates_configs [boolean, default=false]
    Determines whether public_configs and all_dependent_configs in this
    toolchain propagate to targets in other toolchains.

    When false (the default), this toolchain will not propagate any configs to
    targets in other toolchains that depend on it targets inside this
    toolchain. This matches the most common usage of toolchains where they
    represent different architectures or compilers and the settings that apply
    to one won't necessarily apply to others.

    When true, configs (public and all-dependent) will cross the boundary out
    of this toolchain as if the toolchain boundary wasn't there. This only
    affects one direction of dependencies: a toolchain can't control whether
    it accepts such configs, only whether it pushes them. The build is
    responsible for ensuring that any external targets depending on targets in
    this toolchain are compatible with the compiler flags, etc. that may be
    propagated.

  deps [string list]
    Dependencies of this toolchain. These dependencies will be resolved before
    any target in the toolchain is compiled. To avoid circular dependencies
    these must be targets defined in another toolchain.

    This is expressed as a list of targets, and generally these targets will
    always specify a toolchain:
      deps = [ "//foo/bar:baz(//build/toolchain:bootstrap)" ]

    This concept is somewhat inefficient to express in Ninja (it requires a lot
    of duplicate of rules) so should only be used when absolutely necessary.

Example of defining a toolchain

  toolchain("32") {
    tool("cc") {
      command = "gcc {{source}}"
      ...
    }

    toolchain_args = {
      use_doom_melon = true  # Doom melon always required for 32-bit builds.
      current_cpu = "x86"
    }
  }

  toolchain("64") {
    tool("cc") {
      command = "gcc {{source}}"
      ...
    }

    toolchain_args = {
      # use_doom_melon is not overridden here, it will take the default.
      current_cpu = "x64"
    }
  }

Example of cross-toolchain dependencies

  If a 64-bit target wants to depend on a 32-bit binary, it would specify a
  dependency using data_deps (data deps are like deps that are only needed at
  runtime and aren't linked, since you can't link a 32-bit and a 64-bit
  library).

    executable("my_program") {
      ...
      if (target_cpu == "x64") {
        # The 64-bit build needs this 32-bit helper.
        data_deps = [ ":helper(//toolchains:32)" ]
      }
    }

    if (target_cpu == "x86") {
      # Our helper library is only compiled in 32-bits.
      shared_library("helper") {
        ...
      }
    }
)*";

Value RunToolchain(Scope* scope,
                   const FunctionCallNode* function,
                   const std::vector<Value>& args,
                   BlockNode* block,
                   Err* err) {
  NonNestableBlock non_nestable(scope, function, "toolchain");
  if (!non_nestable.Enter(err))
    return Value();

  if (!EnsureNotProcessingImport(function, scope, err) ||
      !EnsureNotProcessingBuildConfig(function, scope, err))
    return Value();

  if (!EnsureSingleStringArg(function, args, err))
    return Value();

  // Note that we don't want to use MakeLabelForScope since that will include
  // the toolchain name in the label, and toolchain labels don't themselves
  // have toolchain names.
  const SourceDir& input_dir = scope->GetSourceDir();
  Label label(input_dir, args[0].string_value());
  if (g_scheduler->verbose_logging())
    g_scheduler->Log("Defining toolchain", label.GetUserVisibleName(false));

  // This object will actually be copied into the one owned by the toolchain
  // manager, but that has to be done in the lock.
  std::unique_ptr<Toolchain> toolchain = std::make_unique<Toolchain>(
      scope->settings(), label, scope->build_dependency_files());
  toolchain->set_defined_from(function);
  toolchain->visibility().SetPublic();

  Scope block_scope(scope);
  block_scope.SetProperty(&kToolchainPropertyKey, toolchain.get());
  block->Execute(&block_scope, err);
  block_scope.SetProperty(&kToolchainPropertyKey, nullptr);
  if (err->has_error())
    return Value();

  // Read deps (if any).
  const Value* deps_value = block_scope.GetValue(variables::kDeps, true);
  if (deps_value) {
    ExtractListOfLabels(scope->settings()->build_settings(), *deps_value,
                        block_scope.GetSourceDir(),
                        ToolchainLabelForScope(&block_scope),
                        &toolchain->deps(), err);
    if (err->has_error())
      return Value();
  }

  // Read toolchain args (if any).
  const Value* toolchain_args = block_scope.GetValue("toolchain_args", true);
  if (toolchain_args) {
    if (!toolchain_args->VerifyTypeIs(Value::SCOPE, err))
      return Value();

    Scope::KeyValueMap values;
    toolchain_args->scope_value()->GetCurrentScopeValues(&values);
    toolchain->args() = values;
  }

  // Read propagates_configs (if present).
  const Value* propagates_configs =
      block_scope.GetValue("propagates_configs", true);
  if (propagates_configs) {
    if (!propagates_configs->VerifyTypeIs(Value::BOOLEAN, err))
      return Value();
    toolchain->set_propagates_configs(propagates_configs->boolean_value());
  }

  if (!block_scope.CheckForUnusedVars(err))
    return Value();

  // Save this toolchain.
  toolchain->ToolchainSetupComplete();
  Scope::ItemVector* collector = scope->GetItemCollector();
  if (!collector) {
    *err = Err(function, "Can't define a toolchain in this context.");
    return Value();
  }
  collector->push_back(std::move(toolchain));
  return Value();
}

// tool ------------------------------------------------------------------------

const char kTool[] = "tool";
const char kTool_HelpShort[] = "tool: Specify arguments to a toolchain tool.";
const char kTool_Help[] =
    R"(tool: Specify arguments to a toolchain tool.

Usage

  tool(<tool type>) {
    <tool variables...>
  }

Tool types

    Compiler tools:
      "cc": C compiler
      "cxx": C++ compiler
      "cxx_module": C++ compiler used for Clang .modulemap files
      "objc": Objective C compiler
      "objcxx": Objective C++ compiler
      "rc": Resource compiler (Windows .rc files)
      "asm": Assembler
      "swift": Swift compiler driver

    Linker tools:
      "alink": Linker for static libraries (archives)
      "solink": Linker for shared libraries
      "link": Linker for executables

    Other tools:
      "stamp": Tool for creating stamp files
      "copy": Tool to copy files.
      "action": Defaults for actions

    Platform specific tools:
      "copy_bundle_data": [iOS, macOS] Tool to copy files in a bundle.
      "compile_xcassets": [iOS, macOS] Tool to compile asset catalogs.

    Rust tools:
      "rust_bin": Tool for compiling Rust binaries
      "rust_cdylib": Tool for compiling C-compatible dynamic libraries.
      "rust_dylib": Tool for compiling Rust dynamic libraries.
      "rust_macro": Tool for compiling Rust procedural macros.
      "rust_rlib": Tool for compiling Rust libraries.
      "rust_staticlib": Tool for compiling Rust static libraries.

Tool variables

    command  [string with substitutions]
        Valid for: all tools except "action" (required)

        The command to run.

    command_launcher  [string]
        Valid for: all tools except "action" (optional)

        The prefix with which to launch the command (e.g. the path to a Goma or
        CCache compiler launcher).

        Note that this prefix will not be included in the compilation database or
        IDE files generated from the build.

    default_output_dir  [string with substitutions]
        Valid for: linker tools

        Default directory name for the output file relative to the
        root_build_dir. It can contain other substitution patterns. This will
        be the default value for the {{output_dir}} expansion (discussed below)
        but will be overridden by the "output_dir" variable in a target, if one
        is specified.

        GN doesn't do anything with this string other than pass it along,
        potentially with target-specific overrides. It is the tool's job to use
        the expansion so that the files will be in the right place.

    default_output_extension  [string]
        Valid for: linker tools

        Extension for the main output of a linkable tool. It includes the
        leading dot. This will be the default value for the
        {{output_extension}} expansion (discussed below) but will be overridden
        by by the "output extension" variable in a target, if one is specified.
        Empty string means no extension.

        GN doesn't actually do anything with this extension other than pass it
        along, potentially with target-specific overrides. One would typically
        use the {{output_extension}} value in the "outputs" to read this value.

        Example: default_output_extension = ".exe"

    depfile  [string with substitutions]
        Valid for: compiler tools (optional)

        If the tool can write ".d" files, this specifies the name of the
        resulting file. These files are used to list header file dependencies
        (or other implicit input dependencies) that are discovered at build
        time. See also "depsformat".

        Example: depfile = "{{output}}.d"

    depsformat  [string]
        Valid for: compiler tools (when depfile is specified)

        Format for the deps outputs. This is either "gcc" or "msvc". See the
        ninja documentation for "deps" for more information.

        Example: depsformat = "gcc"

    description  [string with substitutions, optional]
        Valid for: all tools

        What to print when the command is run.

        Example: description = "Compiling {{source}}"

    exe_output_extension [string, optional, rust tools only]
    rlib_output_extension [string, optional, rust tools only]
    dylib_output_extension [string, optional, rust tools only]
    cdylib_output_extension [string, optional, rust tools only]
    rust_proc_macro_output_extension [string, optional, rust tools only]
        Valid for: Rust tools

        These specify the default tool output for each of the crate types.
        The default is empty for executables, shared, and static libraries and
        ".rlib" for rlibs. Note that the Rust compiler complains with an error
        if external crates do not take the form `lib<name>.rlib` or
        `lib<name>.<shared_extension>`, where `<shared_extension>` is `.so`,
        `.dylib`, or `.dll` as appropriate for the platform.

    lib_switch  [string, optional, link tools only]
    lib_dir_switch  [string, optional, link tools only]
        Valid for: Linker tools except "alink"

        These strings will be prepended to the libraries and library search
        directories, respectively, because linkers differ on how to specify
        them.

        If you specified:
          lib_switch = "-l"
          lib_dir_switch = "-L"
        then the "{{libs}}" expansion for
          [ "freetype", "expat" ]
        would be
          "-lfreetype -lexpat".

    framework_switch [string, optional, link tools only]
    weak_framework_switch [string, optional, link tools only]
    framework_dir_switch [string, optional, link tools only]
        Valid for: Linker tools

        These strings will be prepended to the frameworks and framework search
        path directories, respectively, because linkers differ on how to specify
        them.

        If you specified:
          framework_switch = "-framework "
          weak_framework_switch = "-weak_framework "
          framework_dir_switch = "-F"
        and:
          framework_dirs = [ "$root_out_dir" ]
          frameworks = [ "UIKit.framework", "Foo.framework" ]
          weak_frameworks = [ "MediaPlayer.framework" ]
        would be:
          "-F. -framework UIKit -framework Foo -weak_framework MediaPlayer"

    swiftmodule_switch [string, optional, link tools only]
        Valid for: Linker tools except "alink"

        The string will be prependend to the path to the .swiftmodule files
        that are embedded in the linker output.

        If you specified:
          swiftmodule_swift = "-Wl,-add_ast_path,"
        then the "{{swiftmodules}}" expansion for
          [ "obj/foo/Foo.swiftmodule" ]
        would be
          "-Wl,-add_ast_path,obj/foo/Foo.swiftmodule"

    outputs  [list of strings with substitutions]
        Valid for: Linker and compiler tools (required)

        An array of names for the output files the tool produces. These are
        relative to the build output directory. There must always be at least
        one output file. There can be more than one output (a linker might
        produce a library and an import library, for example).

        This array just declares to GN what files the tool will produce. It is
        your responsibility to specify the tool command that actually produces
        these files.

        If you specify more than one output for shared library links, you
        should consider setting link_output, depend_output, and
        runtime_outputs.

        Example for a compiler tool that produces .obj files:
          outputs = [
            "{{source_out_dir}}/{{source_name_part}}.obj"
          ]

        Example for a linker tool that produces a .dll and a .lib. The use of
        {{target_output_name}}, {{output_extension}} and {{output_dir}} allows
        the target to override these values.
          outputs = [
            "{{output_dir}}/{{target_output_name}}{{output_extension}}",
            "{{output_dir}}/{{target_output_name}}.lib",
          ]

    partial_outputs  [list of strings with substitutions]
        Valid for: "swift" only

        An array of names for the partial outputs the tool produces. These
        are relative to the build output directory. The expansion will be
        evaluated for each file listed in the "sources" of the target.

        This is used to deal with whole module optimization, allowing to
        list one object file per source file when whole module optimization
        is disabled.

    pool [label, optional]
        Valid for: all tools (optional)

        Label of the pool to use for the tool. Pools are used to limit the
        number of tasks that can execute concurrently during the build.

        See also "gn help pool".

    link_output  [string with substitutions]
    depend_output  [string with substitutions]
        Valid for: "solink" only (optional)

        These two files specify which of the outputs from the solink tool
        should be used for linking and dependency tracking. These should match
        entries in the "outputs". If unspecified, the first item in the
        "outputs" array will be used for all. See "Separate linking and
        dependencies for shared libraries" below for more.

        On Windows, where the tools produce a .dll shared library and a .lib
        import library, you will want the first two to be the import library
        and the third one to be the .dll file. On Linux, if you're not doing
        the separate linking/dependency optimization, all of these should be
        the .so output.

    output_prefix  [string]
        Valid for: Linker tools (optional)

        Prefix to use for the output name. Defaults to empty. This prefix will
        be prepended to the name of the target (or the output_name if one is
        manually specified for it) if the prefix is not already there. The
        result will show up in the {{output_name}} substitution pattern.

        Individual targets can opt-out of the output prefix by setting:
          output_prefix_override = true
        (see "gn help output_prefix_override").

        This is typically used to prepend "lib" to libraries on
        Posix systems:
          output_prefix = "lib"

    precompiled_header_type  [string]
        Valid for: "cc", "cxx", "objc", "objcxx"

        Type of precompiled headers. If undefined or the empty string,
        precompiled headers will not be used for this tool. Otherwise use "gcc"
        or "msvc".

        For precompiled headers to be used for a given target, the target (or a
        config applied to it) must also specify a "precompiled_header" and, for
        "msvc"-style headers, a "precompiled_source" value. If the type is
        "gcc", then both "precompiled_header" and "precompiled_source" must
        resolve to the same file, despite the different formats required for
        each."

        See "gn help precompiled_header" for more.

    restat  [boolean]
        Valid for: all tools (optional, defaults to false)

        Requests that Ninja check the file timestamp after this tool has run to
        determine if anything changed. Set this if your tool has the ability to
        skip writing output if the output file has not changed.

        Normally, Ninja will assume that when a tool runs the output be new and
        downstream dependents must be rebuild. When this is set to trye, Ninja
        can skip rebuilding downstream dependents for input changes that don't
        actually affect the output.

        Example:
          restat = true

    rspfile  [string with substitutions]
        Valid for: all tools except "action" (optional)

        Name of the response file. If empty, no response file will be
        used. See "rspfile_content".

    rspfile_content  [string with substitutions]
        Valid for: all tools except "action" (required when "rspfile" is used)

        The contents to be written to the response file. This may include all
        or part of the command to send to the tool which allows you to get
        around OS command-line length limits.

        This example adds the inputs and libraries to a response file, but
        passes the linker flags directly on the command line:
          tool("link") {
            command = "link -o {{output}} {{ldflags}} @{{output}}.rsp"
            rspfile = "{{output}}.rsp"
            rspfile_content = "{{inputs}} {{solibs}} {{libs}} {{rlibs}}"
          }

    runtime_outputs  [string list with substitutions]
        Valid for: linker tools

        If specified, this list is the subset of the outputs that should be
        added to runtime deps (see "gn help runtime_deps"). By default (if
        runtime_outputs is empty or unspecified), it will be the link_output.

    rust_sysroot
        Valid for: Rust tools

        A path relative to root_out_dir. This is not used in the build
        process, but may be used when generating metadata for rust-analyzer.
        (See --export-rust-project). It enables such metadata to include
        information about the Rust standard library.

)"  // String break to prevent overflowing the 16K max VC string length.
    R"(Expansions for tool variables

  All paths are relative to the root build directory, which is the current
  directory for running all tools. These expansions are available to all tools:

    {{label}}
        The label of the current target. This is typically used in the
        "description" field for link tools. The toolchain will be omitted from
        the label for targets in the default toolchain, and will be included
        for targets in other toolchains.

    {{label_name}}
        The short name of the label of the target. This is the part after the
        colon. For "//foo/bar:baz" this will be "baz". Unlike
        {{target_output_name}}, this is not affected by the "output_prefix" in
        the tool or the "output_name" set on the target.

    {{label_no_toolchain}}
        The label of the current target, never including the toolchain
        (otherwise, this is identical to {{label}}). This is used as the module
        name when using .modulemap files.

    {{output}}
        The relative path and name of the output(s) of the current build step.
        If there is more than one output, this will expand to a list of all of
        them. Example: "out/base/my_file.o"

    {{target_gen_dir}}
    {{target_out_dir}}
        The directory of the generated file and output directories,
        respectively, for the current target. There is no trailing slash. See
        also {{output_dir}} for linker tools. Example: "out/base/test"

    {{target_output_name}}
        The short name of the current target with no path information, or the
        value of the "output_name" variable if one is specified in the target.
        This will include the "output_prefix" if any. See also {{label_name}}.

        Example: "libfoo" for the target named "foo" and an output prefix for
        the linker tool of "lib".

  Compiler tools have the notion of a single input and a single output, along
  with a set of compiler-specific flags. The following expansions are
  available:

    {{asmflags}}
    {{cflags}}
    {{cflags_c}}
    {{cflags_cc}}
    {{cflags_objc}}
    {{cflags_objcc}}
    {{defines}}
    {{include_dirs}}
        Strings correspond that to the processed flags/defines/include
        directories specified for the target.
        Example: "--enable-foo --enable-bar"

        Defines will be prefixed by "-D" and include directories will be
        prefixed by "-I" (these work with Posix tools as well as Microsoft
        ones).

    {{module_deps}}
    {{module_deps_no_self}}
        Strings that correspond to the flags necessary to depend upon the Clang
        modules referenced by the current target. The "_no_self" version doesn't
        include the module for the current target, and can be used to compile
        the pcm itself.

    {{source}}
        The relative path and name of the current input file.
        Example: "../../base/my_file.cc"

    {{source_file_part}}
        The file part of the source including the extension (with no directory
        information).
        Example: "foo.cc"

    {{source_name_part}}
        The filename part of the source file with no directory or extension.
        Example: "foo"

    {{source_gen_dir}}
    {{source_out_dir}}
        The directory in the generated file and output directories,
        respectively, for the current input file. If the source file is in the
        same directory as the target is declared in, they will will be the same
        as the "target" versions above. Example: "gen/base/test"

  Linker tools have multiple inputs and (potentially) multiple outputs. The
  static library tool ("alink") is not considered a linker tool. The following
  expansions are available:

    {{inputs}}
    {{inputs_newline}}
        Expands to the inputs to the link step. This will be a list of object
        files and static libraries.
        Example: "obj/foo.o obj/bar.o obj/somelibrary.a"

        The "_newline" version will separate the input files with newlines
        instead of spaces. This is useful in response files: some linkers can
        take a "-filelist" flag which expects newline separated files, and some
        Microsoft tools have a fixed-sized buffer for parsing each line of a
        response file.

    {{ldflags}}
        Expands to the processed set of ldflags and library search paths
        specified for the target.
        Example: "-m64 -fPIC -pthread -L/usr/local/mylib"

    {{libs}}
        Expands to the list of system libraries to link to. Each will be
        prefixed by the "lib_switch".

        Example: "-lfoo -lbar"

    {{output_dir}}
        The value of the "output_dir" variable in the target, or the the value
        of the "default_output_dir" value in the tool if the target does not
        override the output directory. This will be relative to the
        root_build_dir and will not end in a slash. Will be "." for output to
        the root_build_dir.

        This is subtly different than {{target_out_dir}} which is defined by GN
        based on the target's path and not overridable. {{output_dir}} is for
        the final output, {{target_out_dir}} is generally for object files and
        other outputs.

        Usually {{output_dir}} would be defined in terms of either
        {{target_out_dir}} or {{root_out_dir}}

    {{output_extension}}
        The value of the "output_extension" variable in the target, or the
        value of the "default_output_extension" value in the tool if the target
        does not specify an output extension.
        Example: ".so"

    {{solibs}}
        Extra libraries from shared library dependencies not specified in the
        {{inputs}}. This is the list of link_output files from shared libraries
        (if the solink tool specifies a "link_output" variable separate from
        the "depend_output").

        These should generally be treated the same as libs by your tool.

        Example: "libfoo.so libbar.so"

    {{rlibs}}
        Any Rust .rlibs which need to be linked into a final C++ target.
        These should be treated as {{inputs}} except that sometimes
        they might have different linker directives applied.

        Example: "obj/foo/libfoo.rlib"

    {{frameworks}}
        Shared libraries packaged as framework bundle. This is principally
        used on Apple's platforms (macOS and iOS). All name must be ending
        with ".framework" suffix; the suffix will be stripped when expanding
        {{frameworks}} and each item will be preceded by "-framework" or
        "-weak_framework".

    {{swiftmodules}}
        Swift .swiftmodule files that needs to be embedded into the binary.
        This is necessary to correctly link with object generated by the
        Swift compiler (the .swiftmodule file cannot be embedded in object
        files directly). Those will be prefixed with "swiftmodule_switch"
        value.

)"  // String break to prevent overflowing the 16K max VC string length.
    R"(  The static library ("alink") tool allows {{arflags}} plus the common tool
  substitutions.

  The copy tool allows the common compiler/linker substitutions, plus
  {{source}} which is the source of the copy. The stamp tool allows only the
  common tool substitutions.

  The copy_bundle_data and compile_xcassets tools only allows the common tool
  substitutions. Both tools are required to create iOS/macOS bundles and need
  only be defined on those platforms.

  The copy_bundle_data tool will be called with one source and needs to copy
  (optionally optimizing the data representation) to its output. It may be
  called with a directory as input and it needs to be recursively copied.

  The compile_xcassets tool will be called with one or more source (each an
  asset catalog) that needs to be compiled to a single output. The following
  substitutions are available:

    {{inputs}}
        Expands to the list of .xcassets to use as input to compile the asset
        catalog.

    {{bundle_product_type}}
        Expands to the product_type of the bundle that will contain the
        compiled asset catalog. Usually corresponds to the product_type
        property of the corresponding create_bundle target.

    {{bundle_partial_info_plist}}
        Expands to the path to the partial Info.plist generated by the
        assets catalog compiler. Usually based on the target_name of
        the create_bundle target.

    {{xcasset_compiler_flags}}
        Expands to the list of flags specified in corresponding
        create_bundle target.

  The Swift tool has multiple input and outputs. It must have exactly one
  output of .swiftmodule type, but can have one or more object file outputs,
  in addition to other type of outputs. The following expansions are available:

    {{module_name}}
        Expands to the string representing the module name of target under
        compilation (see "module_name" variable).

    {{module_dirs}}
        Expands to the list of -I<path> for the target Swift module search
        path computed from target dependencies.

    {{swiftflags}}
        Expands to the list of strings representing Swift compiler flags.

  Rust tools have the notion of a single input and a single output, along
  with a set of compiler-specific flags. The following expansions are
  available:

    {{crate_name}}
        Expands to the string representing the crate name of target under
        compilation.

    {{crate_type}}
        Expands to the string representing the type of crate for the target
        under compilation.

    {{externs}}
        Expands to the list of --extern flags needed to include addition Rust
        libraries in this target. Includes any specified renamed dependencies.

    {{rustdeps}}
        Expands to the list of -Ldependency=<path> strings needed to compile
        this target.

    {{rustenv}}
        Expands to the list of environment variables.

    {{rustflags}}
        Expands to the list of strings representing Rust compiler flags.

Separate linking and dependencies for shared libraries

  Shared libraries are special in that not all changes to them require that
  dependent targets be re-linked. If the shared library is changed but no
  imports or exports are different, dependent code needn't be relinked, which
  can speed up the build.

  If your link step can output a list of exports from a shared library and
  writes the file only if the new one is different, the timestamp of this file
  can be used for triggering re-links, while the actual shared library would be
  used for linking.

  You will need to specify
    restat = true
  in the linker tool to make this work, so Ninja will detect if the timestamp
  of the dependency file has changed after linking (otherwise it will always
  assume that running a command updates the output):

    tool("solink") {
      command = "..."
      outputs = [
        "{{output_dir}}/{{target_output_name}}{{output_extension}}",
        "{{output_dir}}/{{target_output_name}}{{output_extension}}.TOC",
      ]
      link_output =
        "{{output_dir}}/{{target_output_name}}{{output_extension}}"
      depend_output =
        "{{output_dir}}/{{target_output_name}}{{output_extension}}.TOC"
      restat = true
    }

Example

  toolchain("my_toolchain") {
    # Put these at the top to apply to all tools below.
    lib_switch = "-l"
    lib_dir_switch = "-L"

    tool("cc") {
      command = "gcc {{source}} -o {{output}}"
      outputs = [ "{{source_out_dir}}/{{source_name_part}}.o" ]
      description = "GCC {{source}}"
    }
    tool("cxx") {
      command = "g++ {{source}} -o {{output}}"
      outputs = [ "{{source_out_dir}}/{{source_name_part}}.o" ]
      description = "G++ {{source}}"
    }
  };
)";

Value RunTool(Scope* scope,
              const FunctionCallNode* function,
              const std::vector<Value>& args,
              BlockNode* block,
              Err* err) {
  // Find the toolchain definition we're executing inside of. The toolchain
  // function will set a property pointing to it that we'll pick up.
  Toolchain* toolchain = reinterpret_cast<Toolchain*>(
      scope->GetProperty(&kToolchainPropertyKey, nullptr));
  if (!toolchain) {
    *err = Err(function->function(), "tool() called outside of toolchain().",
               "The tool() function can only be used inside a toolchain() "
               "definition.");
    return Value();
  }

  if (!EnsureSingleStringArg(function, args, err))
    return Value();
  const std::string& tool_name = args[0].string_value();

  // Run the tool block.
  Scope block_scope(scope);
  block->Execute(&block_scope, err);
  if (err->has_error())
    return Value();

  std::unique_ptr<Tool> tool =
      Tool::CreateTool(function, tool_name, &block_scope, toolchain, err);

  if (!tool) {
    return Value();
  }

  tool->set_defined_from(function);
  toolchain->SetTool(std::move(tool));

  // Make sure there weren't any vars set in this tool that were unused.
  if (!block_scope.CheckForUnusedVars(err))
    return Value();

  return Value();
}

}  // namespace functions
