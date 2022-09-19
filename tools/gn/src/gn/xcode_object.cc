// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/xcode_object.h"

#include <iomanip>
#include <iterator>
#include <memory>
#include <sstream>
#include <utility>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "gn/filesystem_utils.h"

// Helper methods -------------------------------------------------------------

namespace {
struct IndentRules {
  bool one_line;
  unsigned level;
};

std::vector<std::unique_ptr<PBXObject>> EmptyPBXObjectVector() {
  return std::vector<std::unique_ptr<PBXObject>>();
}

bool CharNeedEscaping(char c) {
  if (base::IsAsciiAlpha(c) || base::IsAsciiDigit(c))
    return false;
  if (c == '$' || c == '.' || c == '/' || c == '_')
    return false;
  return true;
}

bool StringNeedEscaping(const std::string& string) {
  if (string.empty())
    return true;
  if (string.find("___") != std::string::npos)
    return true;

  for (char c : string) {
    if (CharNeedEscaping(c))
      return true;
  }
  return false;
}

std::string EncodeString(const std::string& string) {
  if (!StringNeedEscaping(string))
    return string;

  std::stringstream buffer;
  buffer << '"';
  for (char c : string) {
    if (c <= 31) {
      switch (c) {
        case '\a':
          buffer << "\\a";
          break;
        case '\b':
          buffer << "\\b";
          break;
        case '\t':
          buffer << "\\t";
          break;
        case '\n':
        case '\r':
          buffer << "\\n";
          break;
        case '\v':
          buffer << "\\v";
          break;
        case '\f':
          buffer << "\\f";
          break;
        default:
          buffer << std::hex << std::setw(4) << std::left << "\\U"
                 << static_cast<unsigned>(c);
          break;
      }
    } else {
      if (c == '"' || c == '\\')
        buffer << '\\';
      buffer << c;
    }
  }
  buffer << '"';
  return buffer.str();
}

struct SourceTypeForExt {
  const char* ext;
  const char* source_type;
};

const SourceTypeForExt kSourceTypeForExt[] = {
    {"a", "archive.ar"},
    {"app", "wrapper.application"},
    {"appex", "wrapper.app-extension"},
    {"bdic", "file"},
    {"bundle", "wrapper.cfbundle"},
    {"c", "sourcecode.c.c"},
    {"cc", "sourcecode.cpp.cpp"},
    {"cpp", "sourcecode.cpp.cpp"},
    {"css", "text.css"},
    {"cxx", "sourcecode.cpp.cpp"},
    {"dart", "sourcecode"},
    {"dylib", "compiled.mach-o.dylib"},
    {"framework", "wrapper.framework"},
    {"h", "sourcecode.c.h"},
    {"hxx", "sourcecode.cpp.h"},
    {"icns", "image.icns"},
    {"java", "sourcecode.java"},
    {"js", "sourcecode.javascript"},
    {"kext", "wrapper.kext"},
    {"m", "sourcecode.c.objc"},
    {"md", "net.daringfireball.markdown"},
    {"mm", "sourcecode.cpp.objcpp"},
    {"nib", "wrapper.nib"},
    {"o", "compiled.mach-o.objfile"},
    {"pdf", "image.pdf"},
    {"pl", "text.script.perl"},
    {"plist", "text.plist.xml"},
    {"pm", "text.script.perl"},
    {"png", "image.png"},
    {"py", "text.script.python"},
    {"r", "sourcecode.rez"},
    {"rez", "sourcecode.rez"},
    {"s", "sourcecode.asm"},
    {"storyboard", "file.storyboard"},
    {"strings", "text.plist.strings"},
    {"swift", "sourcecode.swift"},
    {"ts", "sourcecode.javascript"},
    {"ttf", "file"},
    {"xcassets", "folder.assetcatalog"},
    {"xcconfig", "text.xcconfig"},
    {"xcdatamodel", "wrapper.xcdatamodel"},
    {"xcdatamodeld", "wrapper.xcdatamodeld"},
    {"xctest", "wrapper.cfbundle"},
    {"xib", "file.xib"},
    {"xpc", "wrapper.xpc-service"},
    {"y", "sourcecode.yacc"},
};

const char* GetSourceType(std::string_view ext) {
  for (size_t i = 0; i < std::size(kSourceTypeForExt); ++i) {
    if (kSourceTypeForExt[i].ext == ext)
      return kSourceTypeForExt[i].source_type;
  }

  return "text";
}

bool HasExplicitFileType(std::string_view ext) {
  return ext == "dart" || ext == "ts";
}

bool IsSourceFileForIndexing(std::string_view ext) {
  return ext == "c" || ext == "cc" || ext == "cpp" || ext == "cxx" ||
         ext == "m" || ext == "mm";
}

// Wrapper around a const PBXObject* allowing to print just the object
// identifier instead of a reference (i.e. identitifer and name). This
// is used in a few place where Xcode uses the short identifier only.
struct NoReference {
  const PBXObject* value;

  explicit NoReference(const PBXObject* value) : value(value) {}
};

void PrintValue(std::ostream& out, IndentRules rules, unsigned value) {
  out << value;
}

void PrintValue(std::ostream& out, IndentRules rules, const char* value) {
  out << EncodeString(value);
}

void PrintValue(std::ostream& out,
                IndentRules rules,
                const std::string& value) {
  out << EncodeString(value);
}

void PrintValue(std::ostream& out, IndentRules rules, const NoReference& obj) {
  out << obj.value->id();
}

void PrintValue(std::ostream& out, IndentRules rules, const PBXObject* value) {
  out << value->Reference();
}

template <typename ObjectClass>
void PrintValue(std::ostream& out,
                IndentRules rules,
                const std::unique_ptr<ObjectClass>& value) {
  PrintValue(out, rules, value.get());
}

template <typename ValueType>
void PrintValue(std::ostream& out,
                IndentRules rules,
                const std::vector<ValueType>& values) {
  IndentRules sub_rule{rules.one_line, rules.level + 1};
  out << "(" << (rules.one_line ? " " : "\n");
  for (const auto& value : values) {
    if (!sub_rule.one_line)
      out << std::string(sub_rule.level, '\t');

    PrintValue(out, sub_rule, value);
    out << "," << (rules.one_line ? " " : "\n");
  }

  if (!rules.one_line && rules.level)
    out << std::string(rules.level, '\t');
  out << ")";
}

template <typename ValueType>
void PrintValue(std::ostream& out,
                IndentRules rules,
                const std::map<std::string, ValueType>& values) {
  IndentRules sub_rule{rules.one_line, rules.level + 1};
  out << "{" << (rules.one_line ? " " : "\n");
  for (const auto& pair : values) {
    if (!sub_rule.one_line)
      out << std::string(sub_rule.level, '\t');

    out << pair.first << " = ";
    PrintValue(out, sub_rule, pair.second);
    out << ";" << (rules.one_line ? " " : "\n");
  }

  if (!rules.one_line && rules.level)
    out << std::string(rules.level, '\t');
  out << "}";
}

template <typename ValueType>
void PrintProperty(std::ostream& out,
                   IndentRules rules,
                   const char* name,
                   ValueType&& value) {
  if (!rules.one_line && rules.level)
    out << std::string(rules.level, '\t');

  out << name << " = ";
  PrintValue(out, rules, std::forward<ValueType>(value));
  out << ";" << (rules.one_line ? " " : "\n");
}

struct PBXGroupComparator {
  using PBXObjectPtr = std::unique_ptr<PBXObject>;
  bool operator()(const PBXObjectPtr& lhs, const PBXObjectPtr& rhs) {
    if (lhs.get() == rhs.get())
      return false;

    // Ensure that PBXGroup that should sort last are sorted last.
    const bool lhs_sort_last = SortLast(lhs);
    const bool rhs_sort_last = SortLast(rhs);
    if (lhs_sort_last != rhs_sort_last)
      return rhs_sort_last;

    if (lhs->Class() != rhs->Class())
      return rhs->Class() < lhs->Class();

    return lhs->Name() < rhs->Name();
  }

  bool SortLast(const PBXObjectPtr& ptr) {
    if (ptr->Class() != PBXGroupClass)
      return false;

    return static_cast<PBXGroup*>(ptr.get())->SortLast();
  }
};
}  // namespace

// PBXObjectClass -------------------------------------------------------------

const char* ToString(PBXObjectClass cls) {
  switch (cls) {
    case PBXAggregateTargetClass:
      return "PBXAggregateTarget";
    case PBXBuildFileClass:
      return "PBXBuildFile";
    case PBXContainerItemProxyClass:
      return "PBXContainerItemProxy";
    case PBXFileReferenceClass:
      return "PBXFileReference";
    case PBXFrameworksBuildPhaseClass:
      return "PBXFrameworksBuildPhase";
    case PBXGroupClass:
      return "PBXGroup";
    case PBXNativeTargetClass:
      return "PBXNativeTarget";
    case PBXProjectClass:
      return "PBXProject";
    case PBXResourcesBuildPhaseClass:
      return "PBXResourcesBuildPhase";
    case PBXShellScriptBuildPhaseClass:
      return "PBXShellScriptBuildPhase";
    case PBXSourcesBuildPhaseClass:
      return "PBXSourcesBuildPhase";
    case PBXTargetDependencyClass:
      return "PBXTargetDependency";
    case XCBuildConfigurationClass:
      return "XCBuildConfiguration";
    case XCConfigurationListClass:
      return "XCConfigurationList";
  }
  NOTREACHED();
  return nullptr;
}

// PBXObjectVisitor -----------------------------------------------------------

PBXObjectVisitor::PBXObjectVisitor() = default;

PBXObjectVisitor::~PBXObjectVisitor() = default;

// PBXObjectVisitorConst ------------------------------------------------------

PBXObjectVisitorConst::PBXObjectVisitorConst() = default;

PBXObjectVisitorConst::~PBXObjectVisitorConst() = default;

// PBXObject ------------------------------------------------------------------

PBXObject::PBXObject() = default;

PBXObject::~PBXObject() = default;

void PBXObject::SetId(const std::string& id) {
  DCHECK(id_.empty());
  DCHECK(!id.empty());
  id_.assign(id);
}

std::string PBXObject::Reference() const {
  std::string comment = Comment();
  if (comment.empty())
    return id_;

  return id_ + " /* " + comment + " */";
}

std::string PBXObject::Comment() const {
  return Name();
}

void PBXObject::Visit(PBXObjectVisitor& visitor) {
  visitor.Visit(this);
}

void PBXObject::Visit(PBXObjectVisitorConst& visitor) const {
  visitor.Visit(this);
}

// PBXBuildPhase --------------------------------------------------------------

PBXBuildPhase::PBXBuildPhase() = default;

PBXBuildPhase::~PBXBuildPhase() = default;

void PBXBuildPhase::AddBuildFile(std::unique_ptr<PBXBuildFile> build_file) {
  DCHECK(build_file);
  files_.push_back(std::move(build_file));
}

void PBXBuildPhase::Visit(PBXObjectVisitor& visitor) {
  PBXObject::Visit(visitor);
  for (const auto& file : files_) {
    file->Visit(visitor);
  }
}

void PBXBuildPhase::Visit(PBXObjectVisitorConst& visitor) const {
  PBXObject::Visit(visitor);
  for (const auto& file : files_) {
    file->Visit(visitor);
  }
}

// PBXTarget ------------------------------------------------------------------

PBXTarget::PBXTarget(const std::string& name,
                     const std::string& shell_script,
                     const std::vector<std::string>& configs,
                     const PBXAttributes& attributes)
    : configurations_(
          std::make_unique<XCConfigurationList>(configs, attributes, this)),
      name_(name) {
  if (!shell_script.empty()) {
    build_phases_.push_back(
        std::make_unique<PBXShellScriptBuildPhase>(name, shell_script));
  }
}

PBXTarget::~PBXTarget() = default;

void PBXTarget::AddDependency(std::unique_ptr<PBXTargetDependency> dependency) {
  DCHECK(dependency);
  dependencies_.push_back(std::move(dependency));
}

std::string PBXTarget::Name() const {
  return name_;
}

void PBXTarget::Visit(PBXObjectVisitor& visitor) {
  PBXObject::Visit(visitor);
  configurations_->Visit(visitor);
  for (const auto& dependency : dependencies_)
    dependency->Visit(visitor);
  for (const auto& build_phase : build_phases_)
    build_phase->Visit(visitor);
}

void PBXTarget::Visit(PBXObjectVisitorConst& visitor) const {
  PBXObject::Visit(visitor);
  configurations_->Visit(visitor);
  for (const auto& dependency : dependencies_)
    dependency->Visit(visitor);
  for (const auto& build_phase : build_phases_)
    build_phase->Visit(visitor);
}

// PBXAggregateTarget ---------------------------------------------------------

PBXAggregateTarget::PBXAggregateTarget(const std::string& name,
                                       const std::string& shell_script,
                                       const std::vector<std::string>& configs,
                                       const PBXAttributes& attributes)
    : PBXTarget(name, shell_script, configs, attributes) {}

PBXAggregateTarget::~PBXAggregateTarget() = default;

PBXObjectClass PBXAggregateTarget::Class() const {
  return PBXAggregateTargetClass;
}

void PBXAggregateTarget::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {false, indent + 1};
  out << indent_str << Reference() << " = {\n";
  PrintProperty(out, rules, "isa", ToString(Class()));
  PrintProperty(out, rules, "buildConfigurationList", configurations_);
  PrintProperty(out, rules, "buildPhases", build_phases_);
  PrintProperty(out, rules, "dependencies", EmptyPBXObjectVector());
  PrintProperty(out, rules, "name", name_);
  PrintProperty(out, rules, "productName", name_);
  out << indent_str << "};\n";
}

// PBXBuildFile ---------------------------------------------------------------

PBXBuildFile::PBXBuildFile(const PBXFileReference* file_reference,
                           const PBXBuildPhase* build_phase)
    : file_reference_(file_reference), build_phase_(build_phase) {
  DCHECK(file_reference_);
  DCHECK(build_phase_);
}

PBXBuildFile::~PBXBuildFile() = default;

PBXObjectClass PBXBuildFile::Class() const {
  return PBXBuildFileClass;
}

std::string PBXBuildFile::Name() const {
  return file_reference_->Name() + " in " + build_phase_->Name();
}

void PBXBuildFile::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {true, 0};
  out << indent_str << Reference() << " = {";
  PrintProperty(out, rules, "isa", ToString(Class()));
  PrintProperty(out, rules, "fileRef", file_reference_);
  out << "};\n";
}

// PBXContainerItemProxy ------------------------------------------------------
PBXContainerItemProxy::PBXContainerItemProxy(const PBXProject* project,
                                             const PBXTarget* target)
    : project_(project), target_(target) {}

PBXContainerItemProxy::~PBXContainerItemProxy() = default;

PBXObjectClass PBXContainerItemProxy::Class() const {
  return PBXContainerItemProxyClass;
}

std::string PBXContainerItemProxy::Name() const {
  return "PBXContainerItemProxy";
}

void PBXContainerItemProxy::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {false, indent + 1};
  out << indent_str << Reference() << " = {\n";
  PrintProperty(out, rules, "isa", ToString(Class()));
  PrintProperty(out, rules, "containerPortal", project_);
  PrintProperty(out, rules, "proxyType", 1u);
  PrintProperty(out, rules, "remoteGlobalIDString", NoReference(target_));
  PrintProperty(out, rules, "remoteInfo", target_->Name());
  out << indent_str << "};\n";
}

// PBXFileReference -----------------------------------------------------------

PBXFileReference::PBXFileReference(const std::string& name,
                                   const std::string& path,
                                   const std::string& type)
    : name_(name), path_(path), type_(type) {}

PBXFileReference::~PBXFileReference() = default;

PBXObjectClass PBXFileReference::Class() const {
  return PBXFileReferenceClass;
}

std::string PBXFileReference::Name() const {
  return name_;
}

std::string PBXFileReference::Comment() const {
  return !name_.empty() ? name_ : path_;
}

void PBXFileReference::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {true, 0};
  out << indent_str << Reference() << " = {";
  PrintProperty(out, rules, "isa", ToString(Class()));

  if (!type_.empty()) {
    PrintProperty(out, rules, "explicitFileType", type_);
    PrintProperty(out, rules, "includeInIndex", 0u);
  } else {
    std::string_view ext = FindExtension(&name_);
    const char* prop_name =
        HasExplicitFileType(ext) ? "explicitFileType" : "lastKnownFileType";
    PrintProperty(out, rules, prop_name, GetSourceType(ext));
  }

  if (!name_.empty() && name_ != path_)
    PrintProperty(out, rules, "name", name_);

  DCHECK(!path_.empty());
  PrintProperty(out, rules, "path", path_);
  PrintProperty(out, rules, "sourceTree",
                type_.empty() ? "<group>" : "BUILT_PRODUCTS_DIR");
  out << "};\n";
}

// PBXFrameworksBuildPhase ----------------------------------------------------

PBXFrameworksBuildPhase::PBXFrameworksBuildPhase() = default;

PBXFrameworksBuildPhase::~PBXFrameworksBuildPhase() = default;

PBXObjectClass PBXFrameworksBuildPhase::Class() const {
  return PBXFrameworksBuildPhaseClass;
}

std::string PBXFrameworksBuildPhase::Name() const {
  return "Frameworks";
}

void PBXFrameworksBuildPhase::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {false, indent + 1};
  out << indent_str << Reference() << " = {\n";
  PrintProperty(out, rules, "isa", ToString(Class()));
  PrintProperty(out, rules, "buildActionMask", 0x7fffffffu);
  PrintProperty(out, rules, "files", files_);
  PrintProperty(out, rules, "runOnlyForDeploymentPostprocessing", 0u);
  out << indent_str << "};\n";
}

// PBXGroup -------------------------------------------------------------------

PBXGroup::PBXGroup(const std::string& path, const std::string& name)
    : name_(name), path_(path) {}

PBXGroup::~PBXGroup() = default;

PBXFileReference* PBXGroup::AddSourceFile(const std::string& navigator_path,
                                          const std::string& source_path) {
  DCHECK(!navigator_path.empty());
  DCHECK(!source_path.empty());

  std::string::size_type sep = navigator_path.find("/");
  if (sep == std::string::npos) {
    // Prevent same file reference being created and added multiple times.
    for (const auto& child : children_) {
      if (child->Class() != PBXFileReferenceClass)
        continue;

      PBXFileReference* child_as_file_reference =
          static_cast<PBXFileReference*>(child.get());
      if (child_as_file_reference->Name() == navigator_path &&
          child_as_file_reference->path() == navigator_path) {
        return child_as_file_reference;
      }
    }

    return CreateChild<PBXFileReference>(navigator_path, navigator_path,
                                         std::string());
  }

  PBXGroup* group = nullptr;
  std::string_view component(navigator_path.data(), sep);
  for (const auto& child : children_) {
    if (child->Class() != PBXGroupClass)
      continue;

    PBXGroup* child_as_group = static_cast<PBXGroup*>(child.get());
    if (child_as_group->name_ == component) {
      group = child_as_group;
      break;
    }
  }

  if (!group) {
    group =
        CreateChild<PBXGroup>(std::string(component), std::string(component));
  }

  DCHECK(group);
  DCHECK(group->name_ == component);
  return group->AddSourceFile(navigator_path.substr(sep + 1), source_path);
}

PBXObjectClass PBXGroup::Class() const {
  return PBXGroupClass;
}

std::string PBXGroup::Name() const {
  if (!name_.empty())
    return name_;
  if (!path_.empty())
    return path_;
  return std::string();
}

void PBXGroup::Visit(PBXObjectVisitor& visitor) {
  PBXObject::Visit(visitor);
  for (const auto& child : children_) {
    child->Visit(visitor);
  }
}

void PBXGroup::Visit(PBXObjectVisitorConst& visitor) const {
  PBXObject::Visit(visitor);
  for (const auto& child : children_) {
    child->Visit(visitor);
  }
}

void PBXGroup::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {false, indent + 1};
  out << indent_str << Reference() << " = {\n";
  PrintProperty(out, rules, "isa", ToString(Class()));
  PrintProperty(out, rules, "children", children_);
  if (!name_.empty() && name_ != path_)
    PrintProperty(out, rules, "name", name_);
  if (!path_.empty())
    PrintProperty(out, rules, "path", path_);
  PrintProperty(out, rules, "sourceTree", "<group>");
  out << indent_str << "};\n";
}

bool PBXGroup::SortLast() const {
  return false;
}

PBXObject* PBXGroup::AddChildImpl(std::unique_ptr<PBXObject> child) {
  DCHECK(child);
  DCHECK(child->Class() == PBXGroupClass ||
         child->Class() == PBXFileReferenceClass);

  auto iter = std::lower_bound(children_.begin(), children_.end(), child,
                               PBXGroupComparator());
  return children_.insert(iter, std::move(child))->get();
}

// PBXMainGroup ---------------------------------------------------------------

PBXMainGroup::PBXMainGroup(const std::string& source_path)
    : PBXGroup(source_path, std::string()) {}

PBXMainGroup::~PBXMainGroup() = default;

std::string PBXMainGroup::Name() const {
  return std::string();
}

// PBXProductsGroup -----------------------------------------------------------

PBXProductsGroup::PBXProductsGroup() : PBXGroup(std::string(), "Products") {}

PBXProductsGroup::~PBXProductsGroup() = default;

bool PBXProductsGroup::SortLast() const {
  return true;
}

// PBXNativeTarget ------------------------------------------------------------

PBXNativeTarget::PBXNativeTarget(const std::string& name,
                                 const std::string& shell_script,
                                 const std::vector<std::string>& configs,
                                 const PBXAttributes& attributes,
                                 const std::string& product_type,
                                 const std::string& product_name,
                                 const PBXFileReference* product_reference)
    : PBXTarget(name, shell_script, configs, attributes),
      product_reference_(product_reference),
      product_type_(product_type),
      product_name_(product_name) {
  DCHECK(product_reference_);
  build_phases_.push_back(std::make_unique<PBXSourcesBuildPhase>());
  source_build_phase_ =
      static_cast<PBXSourcesBuildPhase*>(build_phases_.back().get());

  build_phases_.push_back(std::make_unique<PBXFrameworksBuildPhase>());
  build_phases_.push_back(std::make_unique<PBXResourcesBuildPhase>());
  resource_build_phase_ =
      static_cast<PBXResourcesBuildPhase*>(build_phases_.back().get());
}

PBXNativeTarget::~PBXNativeTarget() = default;

void PBXNativeTarget::AddResourceFile(const PBXFileReference* file_reference) {
  DCHECK(file_reference);
  resource_build_phase_->AddBuildFile(
      std::make_unique<PBXBuildFile>(file_reference, resource_build_phase_));
}

void PBXNativeTarget::AddFileForIndexing(
    const PBXFileReference* file_reference) {
  DCHECK(file_reference);
  source_build_phase_->AddBuildFile(
      std::make_unique<PBXBuildFile>(file_reference, source_build_phase_));
}

PBXObjectClass PBXNativeTarget::Class() const {
  return PBXNativeTargetClass;
}

void PBXNativeTarget::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {false, indent + 1};
  out << indent_str << Reference() << " = {\n";
  PrintProperty(out, rules, "isa", ToString(Class()));
  PrintProperty(out, rules, "buildConfigurationList", configurations_);
  PrintProperty(out, rules, "buildPhases", build_phases_);
  PrintProperty(out, rules, "buildRules", EmptyPBXObjectVector());
  PrintProperty(out, rules, "dependencies", dependencies_);
  PrintProperty(out, rules, "name", name_);
  PrintProperty(out, rules, "productName", product_name_);
  PrintProperty(out, rules, "productReference", product_reference_);
  PrintProperty(out, rules, "productType", product_type_);
  out << indent_str << "};\n";
}

// PBXProject -----------------------------------------------------------------

PBXProject::PBXProject(const std::string& name,
                       std::vector<std::string> configs,
                       const std::string& source_path,
                       const PBXAttributes& attributes)
    : name_(name), configs_(std::move(configs)), target_for_indexing_(nullptr) {
  main_group_ = std::make_unique<PBXMainGroup>(source_path);
  products_ = main_group_->CreateChild<PBXProductsGroup>();

  configurations_ =
      std::make_unique<XCConfigurationList>(configs_, attributes, this);
}

PBXProject::~PBXProject() = default;

void PBXProject::AddSourceFileToIndexingTarget(
    const std::string& navigator_path,
    const std::string& source_path) {
  if (!target_for_indexing_) {
    AddIndexingTarget();
  }
  AddSourceFile(navigator_path, source_path, target_for_indexing_);
}

void PBXProject::AddSourceFile(const std::string& navigator_path,
                               const std::string& source_path,
                               PBXNativeTarget* target) {
  PBXFileReference* file_reference =
      main_group_->AddSourceFile(navigator_path, source_path);
  std::string_view ext = FindExtension(&source_path);
  if (!IsSourceFileForIndexing(ext))
    return;

  DCHECK(target);
  target->AddFileForIndexing(file_reference);
}

void PBXProject::AddAggregateTarget(const std::string& name,
                                    const std::string& output_dir,
                                    const std::string& shell_script) {
  PBXAttributes attributes;
  attributes["CLANG_ENABLE_OBJC_WEAK"] = "YES";
  attributes["CODE_SIGNING_REQUIRED"] = "NO";
  attributes["CONFIGURATION_BUILD_DIR"] = output_dir;
  attributes["PRODUCT_NAME"] = name;

  targets_.push_back(std::make_unique<PBXAggregateTarget>(
      name, shell_script, configs_, attributes));
}

void PBXProject::AddIndexingTarget() {
  DCHECK(!target_for_indexing_);
  PBXAttributes attributes;
  attributes["CLANG_ENABLE_OBJC_WEAK"] = "YES";
  attributes["CODE_SIGNING_REQUIRED"] = "NO";
  attributes["EXECUTABLE_PREFIX"] = "";
  attributes["HEADER_SEARCH_PATHS"] = main_group_->path();
  attributes["PRODUCT_NAME"] = "sources";

  PBXFileReference* product_reference =
      products_->CreateChild<PBXFileReference>(std::string(), "sources",
                                               "compiled.mach-o.executable");

  const char product_type[] = "com.apple.product-type.tool";
  targets_.push_back(std::make_unique<PBXNativeTarget>(
      "sources", std::string(), configs_, attributes, product_type, "sources",
      product_reference));
  target_for_indexing_ = static_cast<PBXNativeTarget*>(targets_.back().get());
}

PBXNativeTarget* PBXProject::AddNativeTarget(
    const std::string& name,
    const std::string& type,
    const std::string& output_name,
    const std::string& output_type,
    const std::string& output_dir,
    const std::string& shell_script,
    const PBXAttributes& extra_attributes) {
  std::string_view ext = FindExtension(&output_name);
  PBXFileReference* product = products_->CreateChild<PBXFileReference>(
      std::string(), output_name, type.empty() ? GetSourceType(ext) : type);

  // Per Xcode build settings documentation: Product Name (PRODUCT_NAME) should
  // the basename of the product generated by the target.
  // Therefore, take the basename of output name without file extension as the
  // "PRODUCT_NAME".
  size_t basename_offset = FindFilenameOffset(output_name);
  std::string output_basename = basename_offset != std::string::npos
                                    ? output_name.substr(basename_offset)
                                    : output_name;
  size_t ext_offset = FindExtensionOffset(output_basename);
  std::string product_name = ext_offset != std::string::npos
                                 ? output_basename.substr(0, ext_offset - 1)
                                 : output_basename;

  PBXAttributes attributes = extra_attributes;
  attributes["CLANG_ENABLE_OBJC_WEAK"] = "YES";
  attributes["CODE_SIGNING_REQUIRED"] = "NO";
  attributes["CONFIGURATION_BUILD_DIR"] = output_dir;
  attributes["PRODUCT_NAME"] = product_name;
  attributes["EXCLUDED_SOURCE_FILE_NAMES"] = "*.*";

  targets_.push_back(std::make_unique<PBXNativeTarget>(
      name, shell_script, configs_, attributes, output_type, product_name,
      product));
  return static_cast<PBXNativeTarget*>(targets_.back().get());
}

void PBXProject::SetProjectDirPath(const std::string& project_dir_path) {
  DCHECK(!project_dir_path.empty());
  project_dir_path_.assign(project_dir_path);
}

void PBXProject::SetProjectRoot(const std::string& project_root) {
  DCHECK(!project_root.empty());
  project_root_.assign(project_root);
}

void PBXProject::AddTarget(std::unique_ptr<PBXTarget> target) {
  DCHECK(target);
  targets_.push_back(std::move(target));
}

PBXObjectClass PBXProject::Class() const {
  return PBXProjectClass;
}

std::string PBXProject::Name() const {
  return name_;
}

std::string PBXProject::Comment() const {
  return "Project object";
}

void PBXProject::Visit(PBXObjectVisitor& visitor) {
  PBXObject::Visit(visitor);
  configurations_->Visit(visitor);
  main_group_->Visit(visitor);
  for (const auto& target : targets_) {
    target->Visit(visitor);
  }
}

void PBXProject::Visit(PBXObjectVisitorConst& visitor) const {
  PBXObject::Visit(visitor);
  configurations_->Visit(visitor);
  main_group_->Visit(visitor);
  for (const auto& target : targets_) {
    target->Visit(visitor);
  }
}
void PBXProject::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {false, indent + 1};
  out << indent_str << Reference() << " = {\n";
  PrintProperty(out, rules, "isa", ToString(Class()));
  PrintProperty(out, rules, "attributes", attributes_);
  PrintProperty(out, rules, "buildConfigurationList", configurations_);
  PrintProperty(out, rules, "compatibilityVersion", "Xcode 3.2");
  PrintProperty(out, rules, "developmentRegion", "en");
  PrintProperty(out, rules, "hasScannedForEncodings", 1u);
  PrintProperty(out, rules, "knownRegions",
                std::vector<std::string>({"en", "Base"}));
  PrintProperty(out, rules, "mainGroup", main_group_);
  PrintProperty(out, rules, "productRefGroup", products_);
  PrintProperty(out, rules, "projectDirPath", project_dir_path_);
  PrintProperty(out, rules, "projectRoot", project_root_);
  PrintProperty(out, rules, "targets", targets_);
  out << indent_str << "};\n";
}

// PBXResourcesBuildPhase -----------------------------------------------------

PBXResourcesBuildPhase::PBXResourcesBuildPhase() = default;

PBXResourcesBuildPhase::~PBXResourcesBuildPhase() = default;

PBXObjectClass PBXResourcesBuildPhase::Class() const {
  return PBXResourcesBuildPhaseClass;
}

std::string PBXResourcesBuildPhase::Name() const {
  return "Resources";
}

void PBXResourcesBuildPhase::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {false, indent + 1};
  out << indent_str << Reference() << " = {\n";
  PrintProperty(out, rules, "isa", ToString(Class()));
  PrintProperty(out, rules, "buildActionMask", 0x7fffffffu);
  PrintProperty(out, rules, "files", files_);
  PrintProperty(out, rules, "runOnlyForDeploymentPostprocessing", 0u);
  out << indent_str << "};\n";
}

// PBXShellScriptBuildPhase ---------------------------------------------------

PBXShellScriptBuildPhase::PBXShellScriptBuildPhase(
    const std::string& name,
    const std::string& shell_script)
    : name_("Action \"Compile and copy " + name + " via ninja\""),
      shell_script_(shell_script) {}

PBXShellScriptBuildPhase::~PBXShellScriptBuildPhase() = default;

PBXObjectClass PBXShellScriptBuildPhase::Class() const {
  return PBXShellScriptBuildPhaseClass;
}

std::string PBXShellScriptBuildPhase::Name() const {
  return name_;
}

void PBXShellScriptBuildPhase::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {false, indent + 1};
  out << indent_str << Reference() << " = {\n";
  PrintProperty(out, rules, "isa", ToString(Class()));
  PrintProperty(out, rules, "alwaysOutOfDate", 1u);
  PrintProperty(out, rules, "buildActionMask", 0x7fffffffu);
  PrintProperty(out, rules, "files", files_);
  PrintProperty(out, rules, "inputPaths", EmptyPBXObjectVector());
  PrintProperty(out, rules, "name", name_);
  PrintProperty(out, rules, "outputPaths", EmptyPBXObjectVector());
  PrintProperty(out, rules, "runOnlyForDeploymentPostprocessing", 0u);
  PrintProperty(out, rules, "shellPath", "/bin/sh");
  PrintProperty(out, rules, "shellScript", shell_script_);
  PrintProperty(out, rules, "showEnvVarsInLog", 0u);
  out << indent_str << "};\n";
}

// PBXSourcesBuildPhase -------------------------------------------------------

PBXSourcesBuildPhase::PBXSourcesBuildPhase() = default;

PBXSourcesBuildPhase::~PBXSourcesBuildPhase() = default;

PBXObjectClass PBXSourcesBuildPhase::Class() const {
  return PBXSourcesBuildPhaseClass;
}

std::string PBXSourcesBuildPhase::Name() const {
  return "Sources";
}

void PBXSourcesBuildPhase::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {false, indent + 1};
  out << indent_str << Reference() << " = {\n";
  PrintProperty(out, rules, "isa", ToString(Class()));
  PrintProperty(out, rules, "buildActionMask", 0x7fffffffu);
  PrintProperty(out, rules, "files", files_);
  PrintProperty(out, rules, "runOnlyForDeploymentPostprocessing", 0u);
  out << indent_str << "};\n";
}

PBXTargetDependency::PBXTargetDependency(
    const PBXTarget* target,
    std::unique_ptr<PBXContainerItemProxy> container_item_proxy)
    : target_(target), container_item_proxy_(std::move(container_item_proxy)) {}

PBXTargetDependency::~PBXTargetDependency() = default;

PBXObjectClass PBXTargetDependency::Class() const {
  return PBXTargetDependencyClass;
}

std::string PBXTargetDependency::Name() const {
  return "PBXTargetDependency";
}

void PBXTargetDependency::Visit(PBXObjectVisitor& visitor) {
  PBXObject::Visit(visitor);
  container_item_proxy_->Visit(visitor);
}

void PBXTargetDependency::Visit(PBXObjectVisitorConst& visitor) const {
  PBXObject::Visit(visitor);
  container_item_proxy_->Visit(visitor);
}

void PBXTargetDependency::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {false, indent + 1};
  out << indent_str << Reference() << " = {\n";
  PrintProperty(out, rules, "isa", ToString(Class()));
  PrintProperty(out, rules, "target", target_);
  PrintProperty(out, rules, "targetProxy", container_item_proxy_);
  out << indent_str << "};\n";
}

// XCBuildConfiguration -------------------------------------------------------

XCBuildConfiguration::XCBuildConfiguration(const std::string& name,
                                           const PBXAttributes& attributes)
    : attributes_(attributes), name_(name) {}

XCBuildConfiguration::~XCBuildConfiguration() = default;

PBXObjectClass XCBuildConfiguration::Class() const {
  return XCBuildConfigurationClass;
}

std::string XCBuildConfiguration::Name() const {
  return name_;
}

void XCBuildConfiguration::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {false, indent + 1};
  out << indent_str << Reference() << " = {\n";
  PrintProperty(out, rules, "isa", ToString(Class()));
  PrintProperty(out, rules, "buildSettings", attributes_);
  PrintProperty(out, rules, "name", name_);
  out << indent_str << "};\n";
}

// XCConfigurationList --------------------------------------------------------

XCConfigurationList::XCConfigurationList(
    const std::vector<std::string>& configs,
    const PBXAttributes& attributes,
    const PBXObject* owner_reference)
    : owner_reference_(owner_reference) {
  DCHECK(owner_reference_);
  for (const std::string& config_name : configs) {
    configurations_.push_back(
        std::make_unique<XCBuildConfiguration>(config_name, attributes));
  }
}

XCConfigurationList::~XCConfigurationList() = default;

PBXObjectClass XCConfigurationList::Class() const {
  return XCConfigurationListClass;
}

std::string XCConfigurationList::Name() const {
  std::stringstream buffer;
  buffer << "Build configuration list for "
         << ToString(owner_reference_->Class()) << " \""
         << owner_reference_->Name() << "\"";
  return buffer.str();
}

void XCConfigurationList::Visit(PBXObjectVisitor& visitor) {
  PBXObject::Visit(visitor);
  for (const auto& configuration : configurations_) {
    configuration->Visit(visitor);
  }
}

void XCConfigurationList::Visit(PBXObjectVisitorConst& visitor) const {
  PBXObject::Visit(visitor);
  for (const auto& configuration : configurations_) {
    configuration->Visit(visitor);
  }
}

void XCConfigurationList::Print(std::ostream& out, unsigned indent) const {
  const std::string indent_str(indent, '\t');
  const IndentRules rules = {false, indent + 1};
  out << indent_str << Reference() << " = {\n";
  PrintProperty(out, rules, "isa", ToString(Class()));
  PrintProperty(out, rules, "buildConfigurations", configurations_);
  PrintProperty(out, rules, "defaultConfigurationIsVisible", 0u);
  PrintProperty(out, rules, "defaultConfigurationName",
                configurations_[0]->Name());
  out << indent_str << "};\n";
}
