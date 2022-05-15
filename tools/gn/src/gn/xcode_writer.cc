// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/xcode_writer.h"

#include <iomanip>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include "base/environment.h"
#include "base/logging.h"
#include "base/sha1.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "gn/args.h"
#include "gn/build_settings.h"
#include "gn/builder.h"
#include "gn/commands.h"
#include "gn/deps_iterator.h"
#include "gn/filesystem_utils.h"
#include "gn/item.h"
#include "gn/loader.h"
#include "gn/scheduler.h"
#include "gn/settings.h"
#include "gn/source_file.h"
#include "gn/string_output_buffer.h"
#include "gn/substitution_writer.h"
#include "gn/target.h"
#include "gn/value.h"
#include "gn/variables.h"
#include "gn/xcode_object.h"

namespace {

enum TargetOsType {
  WRITER_TARGET_OS_IOS,
  WRITER_TARGET_OS_MACOS,
};

const char* kXCTestFileSuffixes[] = {
    "egtest.m",     "egtest.mm", "egtest.swift", "xctest.m",      "xctest.mm",
    "xctest.swift", "UITests.m", "UITests.mm",   "UITests.swift",
};

const char kXCTestModuleTargetNamePostfix[] = "_module";
const char kXCUITestRunnerTargetNamePostfix[] = "_runner";

struct SafeEnvironmentVariableInfo {
  const char* name;
  bool capture_at_generation;
};

SafeEnvironmentVariableInfo kSafeEnvironmentVariables[] = {
    {"HOME", true},
    {"LANG", true},
    {"PATH", true},
    {"USER", true},
    {"TMPDIR", false},
    {"ICECC_VERSION", true},
    {"ICECC_CLANG_REMOTE_CPP", true}};

TargetOsType GetTargetOs(const Args& args) {
  const Value* target_os_value = args.GetArgOverride(variables::kTargetOs);
  if (target_os_value) {
    if (target_os_value->type() == Value::STRING) {
      if (target_os_value->string_value() == "ios")
        return WRITER_TARGET_OS_IOS;
    }
  }
  return WRITER_TARGET_OS_MACOS;
}

std::string GetNinjaExecutable(const std::string& ninja_executable) {
  return ninja_executable.empty() ? "ninja" : ninja_executable;
}

std::string ComputeScriptEnviron(base::Environment* environment) {
  std::stringstream buffer;
  for (const auto& variable : kSafeEnvironmentVariables) {
    buffer << variable.name << "=";
    if (variable.capture_at_generation) {
      std::string value;
      environment->GetVar(variable.name, &value);
      buffer << "'" << value << "'";
    } else {
      buffer << "\"${" << variable.name << "}\"";
    }
    buffer << " ";
  }
  return buffer.str();
}

std::string GetBuildScript(const std::string& target_name,
                           const std::string& ninja_executable,
                           base::Environment* environment) {
  // Launch ninja with a sanitized environment (Xcode sets many environment
  // variables overridding settings, including the SDK, thus breaking hermetic
  // build).
  std::stringstream buffer;
  buffer << "exec env -i " << ComputeScriptEnviron(environment);
  buffer << GetNinjaExecutable(ninja_executable) << " -C .";
  if (!target_name.empty()) {
    buffer << " '" << target_name << "'";
  }
  return buffer.str();
}

bool IsApplicationTarget(const Target* target) {
  return target->output_type() == Target::CREATE_BUNDLE &&
         target->bundle_data().product_type() ==
             "com.apple.product-type.application";
}

bool IsXCUITestRunnerTarget(const Target* target) {
  return IsApplicationTarget(target) &&
         base::EndsWith(target->label().name(),
                        kXCUITestRunnerTargetNamePostfix,
                        base::CompareCase::SENSITIVE);
}

bool IsXCTestModuleTarget(const Target* target) {
  return target->output_type() == Target::CREATE_BUNDLE &&
         target->bundle_data().product_type() ==
             "com.apple.product-type.bundle.unit-test" &&
         base::EndsWith(target->label().name(), kXCTestModuleTargetNamePostfix,
                        base::CompareCase::SENSITIVE);
}

bool IsXCUITestModuleTarget(const Target* target) {
  return target->output_type() == Target::CREATE_BUNDLE &&
         target->bundle_data().product_type() ==
             "com.apple.product-type.bundle.ui-testing" &&
         base::EndsWith(target->label().name(), kXCTestModuleTargetNamePostfix,
                        base::CompareCase::SENSITIVE);
}

bool IsXCTestFile(const SourceFile& file) {
  std::string file_name = file.GetName();
  for (size_t i = 0; i < std::size(kXCTestFileSuffixes); ++i) {
    if (base::EndsWith(file_name, kXCTestFileSuffixes[i],
                       base::CompareCase::SENSITIVE)) {
      return true;
    }
  }

  return false;
}

// Finds the application target from its target name.
std::optional<std::pair<const Target*, PBXNativeTarget*>>
FindApplicationTargetByName(
    const ParseNode* node,
    const std::string& target_name,
    const std::map<const Target*, PBXNativeTarget*>& targets,
    Err* err) {
  for (auto& pair : targets) {
    const Target* target = pair.first;
    if (target->label().name() == target_name) {
      if (!IsApplicationTarget(target)) {
        *err = Err(node, "host application target \"" + target_name +
                             "\" not an application bundle");
        return std::nullopt;
      }
      DCHECK(pair.first);
      DCHECK(pair.second);
      return pair;
    }
  }
  *err =
      Err(node, "cannot find host application bundle \"" + target_name + "\"");
  return std::nullopt;
}

// Adds |base_pbxtarget| as a dependency of |dependent_pbxtarget| in the
// generated Xcode project.
void AddPBXTargetDependency(const PBXTarget* base_pbxtarget,
                            PBXTarget* dependent_pbxtarget,
                            const PBXProject* project) {
  auto container_item_proxy =
      std::make_unique<PBXContainerItemProxy>(project, base_pbxtarget);
  auto dependency = std::make_unique<PBXTargetDependency>(
      base_pbxtarget, std::move(container_item_proxy));

  dependent_pbxtarget->AddDependency(std::move(dependency));
}

// Helper class to resolve list of XCTest files per target.
//
// Uses a cache of file found per intermediate targets to reduce the need
// to shared targets multiple times. It is recommended to reuse the same
// object to resolve all the targets for a project.
class XCTestFilesResolver {
 public:
  XCTestFilesResolver();
  ~XCTestFilesResolver();

  // Returns a set of all XCTest files for |target|. The returned reference
  // may be invalidated the next time this method is called.
  const SourceFileSet& SearchFilesForTarget(const Target* target);

 private:
  std::map<const Target*, SourceFileSet> cache_;
};

XCTestFilesResolver::XCTestFilesResolver() = default;

XCTestFilesResolver::~XCTestFilesResolver() = default;

const SourceFileSet& XCTestFilesResolver::SearchFilesForTarget(
    const Target* target) {
  // Early return if already visited and processed.
  auto iter = cache_.find(target);
  if (iter != cache_.end())
    return iter->second;

  SourceFileSet xctest_files;
  for (const SourceFile& file : target->sources()) {
    if (IsXCTestFile(file)) {
      xctest_files.insert(file);
    }
  }

  // Call recursively on public and private deps.
  for (const auto& t : target->public_deps()) {
    const SourceFileSet& deps_xctest_files = SearchFilesForTarget(t.ptr);
    xctest_files.insert(deps_xctest_files.begin(), deps_xctest_files.end());
  }

  for (const auto& t : target->private_deps()) {
    const SourceFileSet& deps_xctest_files = SearchFilesForTarget(t.ptr);
    xctest_files.insert(deps_xctest_files.begin(), deps_xctest_files.end());
  }

  auto insert = cache_.insert(std::make_pair(target, xctest_files));
  DCHECK(insert.second);
  return insert.first->second;
}

// Add xctest files to the "Compiler Sources" of corresponding test module
// native targets.
void AddXCTestFilesToTestModuleTarget(const std::vector<SourceFile>& sources,
                                      PBXNativeTarget* native_target,
                                      PBXProject* project,
                                      SourceDir source_dir,
                                      const BuildSettings* build_settings) {
  for (const SourceFile& source : sources) {
    const std::string source_path = RebasePath(
        source.value(), source_dir, build_settings->root_path_utf8());
    project->AddSourceFile(source_path, source_path, native_target);
  }
}

// Helper class to collect all PBXObject per class.
class CollectPBXObjectsPerClassHelper : public PBXObjectVisitorConst {
 public:
  CollectPBXObjectsPerClassHelper() = default;

  void Visit(const PBXObject* object) override {
    DCHECK(object);
    objects_per_class_[object->Class()].push_back(object);
  }

  const std::map<PBXObjectClass, std::vector<const PBXObject*>>&
  objects_per_class() const {
    return objects_per_class_;
  }

 private:
  std::map<PBXObjectClass, std::vector<const PBXObject*>> objects_per_class_;

  CollectPBXObjectsPerClassHelper(const CollectPBXObjectsPerClassHelper&) =
      delete;
  CollectPBXObjectsPerClassHelper& operator=(
      const CollectPBXObjectsPerClassHelper&) = delete;
};

std::map<PBXObjectClass, std::vector<const PBXObject*>>
CollectPBXObjectsPerClass(const PBXProject* project) {
  CollectPBXObjectsPerClassHelper visitor;
  project->Visit(visitor);
  return visitor.objects_per_class();
}

// Helper class to assign unique ids to all PBXObject.
class RecursivelyAssignIdsHelper : public PBXObjectVisitor {
 public:
  RecursivelyAssignIdsHelper(const std::string& seed)
      : seed_(seed), counter_(0) {}

  void Visit(PBXObject* object) override {
    std::stringstream buffer;
    buffer << seed_ << " " << object->Name() << " " << counter_;
    std::string hash = base::SHA1HashString(buffer.str());
    DCHECK_EQ(hash.size() % 4, 0u);

    uint32_t id[3] = {0, 0, 0};
    const uint32_t* ptr = reinterpret_cast<const uint32_t*>(hash.data());
    for (size_t i = 0; i < hash.size() / 4; i++)
      id[i % 3] ^= ptr[i];

    object->SetId(base::HexEncode(id, sizeof(id)));
    ++counter_;
  }

 private:
  std::string seed_;
  int64_t counter_;

  RecursivelyAssignIdsHelper(const RecursivelyAssignIdsHelper&) = delete;
  RecursivelyAssignIdsHelper& operator=(const RecursivelyAssignIdsHelper&) =
      delete;
};

void RecursivelyAssignIds(PBXProject* project) {
  RecursivelyAssignIdsHelper visitor(project->Name());
  project->Visit(visitor);
}

// Returns a configuration name derived from the build directory. This gives
// standard names if using the Xcode convention of naming the build directory
// out/$configuration-$platform (e.g. out/Debug-iphonesimulator).
std::string ConfigNameFromBuildSettings(const BuildSettings* build_settings) {
  std::string config_name = FilePathToUTF8(build_settings->build_dir()
                                               .Resolve(base::FilePath())
                                               .StripTrailingSeparators()
                                               .BaseName());

  std::string::size_type separator = config_name.find('-');
  if (separator != std::string::npos)
    config_name = config_name.substr(0, separator);

  DCHECK(!config_name.empty());
  return config_name;
}

// Returns the path to root_src_dir from settings.
std::string SourcePathFromBuildSettings(const BuildSettings* build_settings) {
  return RebasePath("//", build_settings->build_dir());
}

// Returns the default attributes for the project from settings.
PBXAttributes ProjectAttributesFromBuildSettings(
    const BuildSettings* build_settings) {
  const TargetOsType target_os = GetTargetOs(build_settings->build_args());

  PBXAttributes attributes;
  switch (target_os) {
    case WRITER_TARGET_OS_IOS:
      attributes["SDKROOT"] = "iphoneos";
      attributes["TARGETED_DEVICE_FAMILY"] = "1,2";
      break;
    case WRITER_TARGET_OS_MACOS:
      attributes["SDKROOT"] = "macosx";
      break;
  }

  // Xcode complains that the project needs to be upgraded if those keys are
  // not set. Since the generated Xcode project is only used for debugging
  // and the source of truth for build settings is the .gn files themselves,
  // we can safely set them in the project as they won't be used by "ninja".
  attributes["ALWAYS_SEARCH_USER_PATHS"] = "NO";
  attributes["CLANG_ANALYZER_LOCALIZABILITY_NONLOCALIZED"] = "YES";
  attributes["CLANG_WARN__DUPLICATE_METHOD_MATCH"] = "YES";
  attributes["CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING"] = "YES";
  attributes["CLANG_WARN_BOOL_CONVERSION"] = "YES";
  attributes["CLANG_WARN_COMMA"] = "YES";
  attributes["CLANG_WARN_CONSTANT_CONVERSION"] = "YES";
  attributes["CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS"] = "YES";
  attributes["CLANG_WARN_EMPTY_BODY"] = "YES";
  attributes["CLANG_WARN_ENUM_CONVERSION"] = "YES";
  attributes["CLANG_WARN_INFINITE_RECURSION"] = "YES";
  attributes["CLANG_WARN_INT_CONVERSION"] = "YES";
  attributes["CLANG_WARN_NON_LITERAL_NULL_CONVERSION"] = "YES";
  attributes["CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF"] = "YES";
  attributes["CLANG_WARN_OBJC_LITERAL_CONVERSION"] = "YES";
  attributes["CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER"] = "YES";
  attributes["CLANG_WARN_RANGE_LOOP_ANALYSIS"] = "YES";
  attributes["CLANG_WARN_STRICT_PROTOTYPES"] = "YES";
  attributes["CLANG_WARN_SUSPICIOUS_MOVE"] = "YES";
  attributes["CLANG_WARN_UNREACHABLE_CODE"] = "YES";
  attributes["ENABLE_STRICT_OBJC_MSGSEND"] = "YES";
  attributes["ENABLE_TESTABILITY"] = "YES";
  attributes["GCC_NO_COMMON_BLOCKS"] = "YES";
  attributes["GCC_WARN_64_TO_32_BIT_CONVERSION"] = "YES";
  attributes["GCC_WARN_ABOUT_RETURN_TYPE"] = "YES";
  attributes["GCC_WARN_UNDECLARED_SELECTOR"] = "YES";
  attributes["GCC_WARN_UNINITIALIZED_AUTOS"] = "YES";
  attributes["GCC_WARN_UNUSED_FUNCTION"] = "YES";
  attributes["GCC_WARN_UNUSED_VARIABLE"] = "YES";
  attributes["ONLY_ACTIVE_ARCH"] = "YES";

  return attributes;
}

}  // namespace

// Class representing the workspace embedded in an xcodeproj file used to
// configure the build settings shared by all targets in the project (used
// to configure the build system).
class XcodeWorkspace {
 public:
  XcodeWorkspace(const BuildSettings* build_settings,
                 XcodeWriter::Options options);
  ~XcodeWorkspace();

  XcodeWorkspace(const XcodeWorkspace&) = delete;
  XcodeWorkspace& operator=(const XcodeWorkspace&) = delete;

  // Generates the .xcworkspace files to disk.
  bool WriteWorkspace(const std::string& name, Err* err) const;

 private:
  // Writes the workspace data file.
  bool WriteWorkspaceDataFile(const std::string& name, Err* err) const;

  // Writes the settings file.
  bool WriteSettingsFile(const std::string& name, Err* err) const;

  const BuildSettings* build_settings_ = nullptr;
  XcodeWriter::Options options_;
};

XcodeWorkspace::XcodeWorkspace(const BuildSettings* build_settings,
                               XcodeWriter::Options options)
    : build_settings_(build_settings), options_(options) {}

XcodeWorkspace::~XcodeWorkspace() = default;

bool XcodeWorkspace::WriteWorkspace(const std::string& name, Err* err) const {
  return WriteWorkspaceDataFile(name, err) && WriteSettingsFile(name, err);
}

bool XcodeWorkspace::WriteWorkspaceDataFile(const std::string& name,
                                            Err* err) const {
  const SourceFile source_file =
      build_settings_->build_dir().ResolveRelativeFile(
          Value(nullptr, name + "/contents.xcworkspacedata"), err);
  if (source_file.is_null())
    return false;

  StringOutputBuffer storage;
  std::ostream out(&storage);
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      << "<Workspace\n"
      << "   version = \"1.0\">\n"
      << "   <FileRef\n"
      << "      location = \"self:\">\n"
      << "   </FileRef>\n"
      << "</Workspace>\n";

  return storage.WriteToFileIfChanged(build_settings_->GetFullPath(source_file),
                                      err);
}

bool XcodeWorkspace::WriteSettingsFile(const std::string& name,
                                       Err* err) const {
  const SourceFile source_file =
      build_settings_->build_dir().ResolveRelativeFile(
          Value(nullptr, name + "/xcshareddata/WorkspaceSettings.xcsettings"),
          err);
  if (source_file.is_null())
    return false;

  StringOutputBuffer storage;
  std::ostream out(&storage);
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
      << "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
      << "<plist version=\"1.0\">\n"
      << "<dict>\n";

  switch (options_.build_system) {
    case XcodeBuildSystem::kLegacy:
      out << "\t<key>BuildSystemType</key>\n"
          << "\t<string>Original</string>\n";
      break;
    case XcodeBuildSystem::kNew:
      break;
  }

  out << "</dict>\n"
      << "</plist>\n";

  return storage.WriteToFileIfChanged(build_settings_->GetFullPath(source_file),
                                      err);
}

// Class responsible for constructing and writing the .xcodeproj from the
// targets known to gn. It currently requires using the "Legacy build system"
// so it will embed an .xcworkspace file to force the setting.
class XcodeProject {
 public:
  XcodeProject(const BuildSettings* build_settings,
               XcodeWriter::Options options);
  ~XcodeProject();

  // Recursively finds "source" files from |builder| and adds them to the
  // project (this includes more than just text source files, e.g. images
  // in resources, ...).
  bool AddSourcesFromBuilder(const Builder& builder, Err* err);

  // Recursively finds targets from |builder| and adds them to the project.
  // Only targets of type CREATE_BUNDLE or EXECUTABLE are kept since they
  // are the only one that can be run and thus debugged from Xcode.
  bool AddTargetsFromBuilder(const Builder& builder, Err* err);

  // Assigns ids to all PBXObject that were added to the project. Must be
  // called before calling WriteFile().
  bool AssignIds(Err* err);

  // Generates the project file and the .xcodeproj file to disk if updated
  // (i.e. if the generated project is identical to the currently existing
  // one, it is not overwritten).
  bool WriteFile(Err* err) const;

 private:
  // Finds all targets that needs to be generated for the project (applies
  // the filter passed via |options|).
  std::optional<std::vector<const Target*>> GetTargetsFromBuilder(
      const Builder& builder,
      Err* err) const;

  // Adds a target of type EXECUTABLE to the project.
  PBXNativeTarget* AddBinaryTarget(const Target* target,
                                   base::Environment* env,
                                   Err* err);

  // Adds a target of type CREATE_BUNDLE to the project.
  PBXNativeTarget* AddBundleTarget(const Target* target,
                                   base::Environment* env,
                                   Err* err);

  // Adds the XCTest source files for all test xctest or xcuitest module target
  // to allow Xcode to index the list of tests (thus allowing to run individual
  // tests from Xcode UI).
  bool AddCXTestSourceFilesForTestModuleTargets(
      const std::map<const Target*, PBXNativeTarget*>& bundle_targets,
      Err* err);

  // Adds the corresponding test application target as dependency of xctest or
  // xcuitest module target in the generated Xcode project.
  bool AddDependencyTargetsForTestModuleTargets(
      const std::map<const Target*, PBXNativeTarget*>& bundle_targets,
      Err* err);

  // Generates the content of the .xcodeproj file into |out|.
  void WriteFileContent(std::ostream& out) const;

  // Returns whether the file should be added to the project.
  bool ShouldIncludeFileInProject(const SourceFile& source) const;

  const BuildSettings* build_settings_;
  XcodeWriter::Options options_;
  PBXProject project_;
};

XcodeProject::XcodeProject(const BuildSettings* build_settings,
                           XcodeWriter::Options options)
    : build_settings_(build_settings),
      options_(options),
      project_(options.project_name,
               ConfigNameFromBuildSettings(build_settings),
               SourcePathFromBuildSettings(build_settings),
               ProjectAttributesFromBuildSettings(build_settings)) {}

XcodeProject::~XcodeProject() = default;

bool XcodeProject::ShouldIncludeFileInProject(const SourceFile& source) const {
  if (IsStringInOutputDir(build_settings_->build_dir(), source.value()))
    return false;

  if (IsPathAbsolute(source.value()))
    return false;

  return true;
}

bool XcodeProject::AddSourcesFromBuilder(const Builder& builder, Err* err) {
  SourceFileSet sources;

  // Add sources from all targets.
  for (const Target* target : builder.GetAllResolvedTargets()) {
    for (const SourceFile& source : target->sources()) {
      if (ShouldIncludeFileInProject(source))
        sources.insert(source);
    }

    for (const SourceFile& source : target->config_values().inputs()) {
      if (ShouldIncludeFileInProject(source))
        sources.insert(source);
    }

    for (const SourceFile& source : target->public_headers()) {
      if (ShouldIncludeFileInProject(source))
        sources.insert(source);
    }

    const SourceFile& bridge_header = target->swift_values().bridge_header();
    if (!bridge_header.is_null() && ShouldIncludeFileInProject(bridge_header)) {
      sources.insert(bridge_header);
    }

    if (target->output_type() == Target::ACTION ||
        target->output_type() == Target::ACTION_FOREACH) {
      if (ShouldIncludeFileInProject(target->action_values().script()))
        sources.insert(target->action_values().script());
    }
  }

  // Add BUILD.gn and *.gni for targets, configs and toolchains.
  for (const Item* item : builder.GetAllResolvedItems()) {
    if (!item->AsConfig() && !item->AsTarget() && !item->AsToolchain())
      continue;

    const SourceFile build = builder.loader()->BuildFileForLabel(item->label());
    if (ShouldIncludeFileInProject(build))
      sources.insert(build);

    for (const SourceFile& source :
         item->settings()->import_manager().GetImportedFiles()) {
      if (ShouldIncludeFileInProject(source))
        sources.insert(source);
    }
  }

  // Add other files read by gn (the main dotfile, exec_script scripts, ...).
  for (const auto& path : g_scheduler->GetGenDependencies()) {
    if (!build_settings_->root_path().IsParent(path))
      continue;

    const std::string as8bit = path.As8Bit();
    const SourceFile source(
        "//" + as8bit.substr(build_settings_->root_path().value().size() + 1));

    if (ShouldIncludeFileInProject(source))
      sources.insert(source);
  }

  // Sort files to ensure deterministic generation of the project file (and
  // nicely sorted file list in Xcode).
  std::vector<SourceFile> sorted_sources(sources.begin(), sources.end());
  std::sort(sorted_sources.begin(), sorted_sources.end());

  const SourceDir source_dir("//");
  for (const SourceFile& source : sorted_sources) {
    const std::string source_file = RebasePath(
        source.value(), source_dir, build_settings_->root_path_utf8());
    project_.AddSourceFileToIndexingTarget(source_file, source_file);
  }

  return true;
}

bool XcodeProject::AddTargetsFromBuilder(const Builder& builder, Err* err) {
  std::unique_ptr<base::Environment> env(base::Environment::Create());

  project_.AddAggregateTarget(
      "All", GetBuildScript(options_.root_target_name,
                            options_.ninja_executable, env.get()));

  const std::optional<std::vector<const Target*>> targets =
      GetTargetsFromBuilder(builder, err);
  if (!targets)
    return false;

  std::map<const Target*, PBXNativeTarget*> bundle_targets;

  const TargetOsType target_os = GetTargetOs(build_settings_->build_args());

  for (const Target* target : *targets) {
    PBXNativeTarget* native_target = nullptr;
    switch (target->output_type()) {
      case Target::EXECUTABLE:
        if (target_os == WRITER_TARGET_OS_IOS)
          continue;

        native_target = AddBinaryTarget(target, env.get(), err);
        if (!native_target)
          return false;

        break;

      case Target::CREATE_BUNDLE: {
        if (target->bundle_data().product_type().empty())
          continue;

        // For XCUITest, two CREATE_BUNDLE targets are generated:
        // ${target_name}_runner and ${target_name}_module, however, Xcode
        // requires only one target named ${target_name} to run tests.
        if (IsXCUITestRunnerTarget(target))
          continue;

        native_target = AddBundleTarget(target, env.get(), err);
        if (!native_target)
          return false;

        bundle_targets.insert(std::make_pair(target, native_target));
        break;
      }

      default:
        break;
    }
  }

  if (!AddCXTestSourceFilesForTestModuleTargets(bundle_targets, err))
    return false;

  // Adding the corresponding test application target as a dependency of xctest
  // or xcuitest module target in the generated Xcode project so that the
  // application target is re-compiled when compiling the test module target.
  if (!AddDependencyTargetsForTestModuleTargets(bundle_targets, err))
    return false;

  return true;
}

bool XcodeProject::AddCXTestSourceFilesForTestModuleTargets(
    const std::map<const Target*, PBXNativeTarget*>& bundle_targets,
    Err* err) {
  const SourceDir source_dir("//");

  // Needs to search for xctest files under the application targets, and this
  // variable is used to store the results of visited targets, thus making the
  // search more efficient.
  XCTestFilesResolver resolver;

  for (const auto& pair : bundle_targets) {
    const Target* target = pair.first;
    if (!IsXCTestModuleTarget(target) && !IsXCUITestModuleTarget(target))
      continue;

    // For XCTest, test files are compiled into the application bundle.
    // For XCUITest, test files are compiled into the test module bundle.
    const Target* target_with_xctest_files = nullptr;
    if (IsXCTestModuleTarget(target)) {
      auto app_pair = FindApplicationTargetByName(
          target->defined_from(),
          target->bundle_data().xcode_test_application_name(), bundle_targets,
          err);
      if (!app_pair)
        return false;
      target_with_xctest_files = app_pair.value().first;
    } else {
      DCHECK(IsXCUITestModuleTarget(target));
      target_with_xctest_files = target;
    }

    const SourceFileSet& sources =
        resolver.SearchFilesForTarget(target_with_xctest_files);

    // Sort files to ensure deterministic generation of the project file (and
    // nicely sorted file list in Xcode).
    std::vector<SourceFile> sorted_sources(sources.begin(), sources.end());
    std::sort(sorted_sources.begin(), sorted_sources.end());

    // Add xctest files to the "Compiler Sources" of corresponding xctest
    // and xcuitest native targets for proper indexing and for discovery of
    // tests function.
    AddXCTestFilesToTestModuleTarget(sorted_sources, pair.second, &project_,
                                     source_dir, build_settings_);
  }

  return true;
}

bool XcodeProject::AddDependencyTargetsForTestModuleTargets(
    const std::map<const Target*, PBXNativeTarget*>& bundle_targets,
    Err* err) {
  for (const auto& pair : bundle_targets) {
    const Target* target = pair.first;
    if (!IsXCTestModuleTarget(target) && !IsXCUITestModuleTarget(target))
      continue;

    auto app_pair = FindApplicationTargetByName(
        target->defined_from(),
        target->bundle_data().xcode_test_application_name(), bundle_targets,
        err);
    if (!app_pair)
      return false;

    AddPBXTargetDependency(app_pair.value().second, pair.second, &project_);
  }

  return true;
}

bool XcodeProject::AssignIds(Err* err) {
  RecursivelyAssignIds(&project_);
  return true;
}

bool XcodeProject::WriteFile(Err* err) const {
  DCHECK(!project_.id().empty());

  SourceFile pbxproj_file = build_settings_->build_dir().ResolveRelativeFile(
      Value(nullptr, project_.Name() + ".xcodeproj/project.pbxproj"), err);
  if (pbxproj_file.is_null())
    return false;

  StringOutputBuffer storage;
  std::ostream pbxproj_string_out(&storage);
  WriteFileContent(pbxproj_string_out);

  if (!storage.WriteToFileIfChanged(build_settings_->GetFullPath(pbxproj_file),
                                    err)) {
    return false;
  }

  XcodeWorkspace workspace(build_settings_, options_);
  return workspace.WriteWorkspace(
      project_.Name() + ".xcodeproj/project.xcworkspace", err);
}

std::optional<std::vector<const Target*>> XcodeProject::GetTargetsFromBuilder(
    const Builder& builder,
    Err* err) const {
  std::vector<const Target*> all_targets = builder.GetAllResolvedTargets();

  // Filter targets according to the dir_filters_string if defined.
  if (!options_.dir_filters_string.empty()) {
    std::vector<LabelPattern> filters;
    if (!commands::FilterPatternsFromString(
            build_settings_, options_.dir_filters_string, &filters, err)) {
      return std::nullopt;
    }

    std::vector<const Target*> unfiltered_targets;
    std::swap(unfiltered_targets, all_targets);

    commands::FilterTargetsByPatterns(unfiltered_targets, filters,
                                      &all_targets);
  }

  // Filter out all target of type EXECUTABLE that are direct dependency of
  // a BUNDLE_DATA target (under the assumption that they will be part of a
  // CREATE_BUNDLE target generating an application bundle).
  TargetSet targets(all_targets.begin(), all_targets.end());
  for (const Target* target : all_targets) {
    if (!target->settings()->is_default())
      continue;

    if (target->output_type() != Target::BUNDLE_DATA)
      continue;

    for (const auto& pair : target->GetDeps(Target::DEPS_LINKED)) {
      if (pair.ptr->output_type() != Target::EXECUTABLE)
        continue;

      targets.erase(pair.ptr);
    }
  }

  // Sort the list of targets per-label to get a consistent ordering of them
  // in the generated Xcode project (and thus stability of the file generated).
  std::vector<const Target*> sorted_targets(targets.begin(), targets.end());
  std::sort(sorted_targets.begin(), sorted_targets.end(),
            [](const Target* lhs, const Target* rhs) {
              return lhs->label() < rhs->label();
            });

  return sorted_targets;
}

PBXNativeTarget* XcodeProject::AddBinaryTarget(const Target* target,
                                               base::Environment* env,
                                               Err* err) {
  DCHECK_EQ(target->output_type(), Target::EXECUTABLE);

  std::string output_dir = target->output_dir().value();
  if (output_dir.empty()) {
    const Tool* tool = target->toolchain()->GetToolForTargetFinalOutput(target);
    if (!tool) {
      std::string tool_name = Tool::GetToolTypeForTargetFinalOutput(target);
      *err = Err(nullptr, tool_name + " tool not defined",
                 "The toolchain " +
                     target->toolchain()->label().GetUserVisibleName(false) +
                     " used by target " +
                     target->label().GetUserVisibleName(false) +
                     " doesn't define a \"" + tool_name + "\" tool.");
      return nullptr;
    }
    output_dir = SubstitutionWriter::ApplyPatternToLinkerAsOutputFile(
                     target, tool, tool->default_output_dir())
                     .value();
  } else {
    output_dir = RebasePath(output_dir, build_settings_->build_dir());
  }

  return project_.AddNativeTarget(
      target->label().name(), "compiled.mach-o.executable",
      target->output_name().empty() ? target->label().name()
                                    : target->output_name(),
      "com.apple.product-type.tool", output_dir,
      GetBuildScript(target->label().name(), options_.ninja_executable, env));
}

PBXNativeTarget* XcodeProject::AddBundleTarget(const Target* target,
                                               base::Environment* env,
                                               Err* err) {
  DCHECK_EQ(target->output_type(), Target::CREATE_BUNDLE);

  std::string pbxtarget_name = target->label().name();
  if (IsXCUITestModuleTarget(target)) {
    std::string target_name = target->label().name();
    pbxtarget_name = target_name.substr(
        0, target_name.rfind(kXCTestModuleTargetNamePostfix));
  }

  PBXAttributes xcode_extra_attributes =
      target->bundle_data().xcode_extra_attributes();
  if (options_.build_system == XcodeBuildSystem::kLegacy) {
    xcode_extra_attributes["CODE_SIGN_IDENTITY"] = "";
  }

  const std::string& target_output_name = RebasePath(
      target->bundle_data().GetBundleRootDirOutput(target->settings()).value(),
      build_settings_->build_dir());
  const std::string output_dir =
      RebasePath(target->bundle_data().GetBundleDir(target->settings()).value(),
                 build_settings_->build_dir());
  return project_.AddNativeTarget(
      pbxtarget_name, std::string(), target_output_name,
      target->bundle_data().product_type(), output_dir,
      GetBuildScript(pbxtarget_name, options_.ninja_executable, env),
      xcode_extra_attributes);
}

void XcodeProject::WriteFileContent(std::ostream& out) const {
  out << "// !$*UTF8*$!\n"
      << "{\n"
      << "\tarchiveVersion = 1;\n"
      << "\tclasses = {\n"
      << "\t};\n"
      << "\tobjectVersion = 46;\n"
      << "\tobjects = {\n";

  for (auto& pair : CollectPBXObjectsPerClass(&project_)) {
    out << "\n"
        << "/* Begin " << ToString(pair.first) << " section */\n";
    std::sort(pair.second.begin(), pair.second.end(),
              [](const PBXObject* a, const PBXObject* b) {
                return a->id() < b->id();
              });
    for (auto* object : pair.second) {
      object->Print(out, 2);
    }
    out << "/* End " << ToString(pair.first) << " section */\n";
  }

  out << "\t};\n"
      << "\trootObject = " << project_.Reference() << ";\n"
      << "}\n";
}

// static
bool XcodeWriter::RunAndWriteFiles(const BuildSettings* build_settings,
                                   const Builder& builder,
                                   Options options,
                                   Err* err) {
  XcodeProject project(build_settings, options);
  if (!project.AddSourcesFromBuilder(builder, err))
    return false;

  if (!project.AddTargetsFromBuilder(builder, err))
    return false;

  if (!project.AssignIds(err))
    return false;

  if (!project.WriteFile(err))
    return false;

  return true;
}
