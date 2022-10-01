// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_XCODE_OBJECT_H_
#define TOOLS_GN_XCODE_OBJECT_H_

#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Helper classes to generate Xcode project files.
//
// This code is based on gyp xcodeproj_file.py generator. It does not support
// all features of Xcode project but instead just enough to implement a hybrid
// mode where Xcode uses external scripts to perform the compilation steps.
//
// See
// https://chromium.googlesource.com/external/gyp/+/master/pylib/gyp/xcodeproj_file.py
// for more information on Xcode project file format.

// PBXObjectClass -------------------------------------------------------------

enum PBXObjectClass {
  // Those values needs to stay sorted in alphabetic order.
  PBXAggregateTargetClass,
  PBXBuildFileClass,
  PBXContainerItemProxyClass,
  PBXFileReferenceClass,
  PBXFrameworksBuildPhaseClass,
  PBXGroupClass,
  PBXNativeTargetClass,
  PBXProjectClass,
  PBXResourcesBuildPhaseClass,
  PBXShellScriptBuildPhaseClass,
  PBXSourcesBuildPhaseClass,
  PBXTargetDependencyClass,
  XCBuildConfigurationClass,
  XCConfigurationListClass,
};

const char* ToString(PBXObjectClass cls);

// Forward-declarations -------------------------------------------------------

class PBXAggregateTarget;
class PBXBuildFile;
class PBXBuildPhase;
class PBXContainerItemProxy;
class PBXFileReference;
class PBXFrameworksBuildPhase;
class PBXGroup;
class PBXNativeTarget;
class PBXObject;
class PBXProject;
class PBXResourcesBuildPhase;
class PBXShellScriptBuildPhase;
class PBXSourcesBuildPhase;
class PBXTarget;
class PBXTargetDependency;
class XCBuildConfiguration;
class XCConfigurationList;

using PBXAttributes = std::map<std::string, std::string>;

// PBXObjectVisitor -----------------------------------------------------------

class PBXObjectVisitor {
 public:
  PBXObjectVisitor();
  virtual ~PBXObjectVisitor();
  virtual void Visit(PBXObject* object) = 0;

 private:
  PBXObjectVisitor(const PBXObjectVisitor&) = delete;
  PBXObjectVisitor& operator=(const PBXObjectVisitor&) = delete;
};

// PBXObjectVisitorConst ------------------------------------------------------

class PBXObjectVisitorConst {
 public:
  PBXObjectVisitorConst();
  virtual ~PBXObjectVisitorConst();
  virtual void Visit(const PBXObject* object) = 0;

 private:
  PBXObjectVisitorConst(const PBXObjectVisitorConst&) = delete;
  PBXObjectVisitorConst& operator=(const PBXObjectVisitorConst&) = delete;
};

// PBXObject ------------------------------------------------------------------

class PBXObject {
 public:
  PBXObject();
  virtual ~PBXObject();

  const std::string id() const { return id_; }
  void SetId(const std::string& id);

  std::string Reference() const;

  virtual PBXObjectClass Class() const = 0;
  virtual std::string Name() const = 0;
  virtual std::string Comment() const;
  virtual void Visit(PBXObjectVisitor& visitor);
  virtual void Visit(PBXObjectVisitorConst& visitor) const;
  virtual void Print(std::ostream& out, unsigned indent) const = 0;

 private:
  std::string id_;

  PBXObject(const PBXObject&) = delete;
  PBXObject& operator=(const PBXObject&) = delete;
};

// PBXBuildPhase --------------------------------------------------------------

class PBXBuildPhase : public PBXObject {
 public:
  PBXBuildPhase();
  ~PBXBuildPhase() override;

  void AddBuildFile(std::unique_ptr<PBXBuildFile> build_file);

  // PBXObject implementation.
  void Visit(PBXObjectVisitor& visitor) override;
  void Visit(PBXObjectVisitorConst& visitor) const override;

 protected:
  std::vector<std::unique_ptr<PBXBuildFile>> files_;

 private:
  PBXBuildPhase(const PBXBuildPhase&) = delete;
  PBXBuildPhase& operator=(const PBXBuildPhase&) = delete;
};

// PBXTarget ------------------------------------------------------------------

class PBXTarget : public PBXObject {
 public:
  PBXTarget(const std::string& name,
            const std::string& shell_script,
            const std::vector<std::string>& configs,
            const PBXAttributes& attributes);
  ~PBXTarget() override;

  void AddDependency(std::unique_ptr<PBXTargetDependency> dependency);

  // PBXObject implementation.
  std::string Name() const override;
  void Visit(PBXObjectVisitor& visitor) override;
  void Visit(PBXObjectVisitorConst& visitor) const override;

 protected:
  std::unique_ptr<XCConfigurationList> configurations_;
  std::vector<std::unique_ptr<PBXBuildPhase>> build_phases_;
  std::vector<std::unique_ptr<PBXTargetDependency>> dependencies_;
  PBXSourcesBuildPhase* source_build_phase_ = nullptr;
  PBXResourcesBuildPhase* resource_build_phase_ = nullptr;
  std::string name_;

 private:
  PBXTarget(const PBXTarget&) = delete;
  PBXTarget& operator=(const PBXTarget&) = delete;
};

// PBXAggregateTarget ---------------------------------------------------------

class PBXAggregateTarget : public PBXTarget {
 public:
  PBXAggregateTarget(const std::string& name,
                     const std::string& shell_script,
                     const std::vector<std::string>& configs,
                     const PBXAttributes& attributes);
  ~PBXAggregateTarget() override;

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  void Print(std::ostream& out, unsigned indent) const override;

 private:
  PBXAggregateTarget(const PBXAggregateTarget&) = delete;
  PBXAggregateTarget& operator=(const PBXAggregateTarget&) = delete;
};

// PBXBuildFile ---------------------------------------------------------------

class PBXBuildFile : public PBXObject {
 public:
  PBXBuildFile(const PBXFileReference* file_reference,
               const PBXBuildPhase* build_phase);
  ~PBXBuildFile() override;

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  std::string Name() const override;
  void Print(std::ostream& out, unsigned indent) const override;

 private:
  const PBXFileReference* file_reference_ = nullptr;
  const PBXBuildPhase* build_phase_ = nullptr;

  PBXBuildFile(const PBXBuildFile&) = delete;
  PBXBuildFile& operator=(const PBXBuildFile&) = delete;
};

// PBXContainerItemProxy ------------------------------------------------------
class PBXContainerItemProxy : public PBXObject {
 public:
  PBXContainerItemProxy(const PBXProject* project, const PBXTarget* target);
  ~PBXContainerItemProxy() override;

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  std::string Name() const override;
  void Print(std::ostream& out, unsigned indent) const override;

 private:
  const PBXProject* project_ = nullptr;
  const PBXTarget* target_ = nullptr;

  PBXContainerItemProxy(const PBXContainerItemProxy&) = delete;
  PBXContainerItemProxy& operator=(const PBXContainerItemProxy&) = delete;
};

// PBXFileReference -----------------------------------------------------------

class PBXFileReference : public PBXObject {
 public:
  PBXFileReference(const std::string& name,
                   const std::string& path,
                   const std::string& type);
  ~PBXFileReference() override;

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  std::string Name() const override;
  std::string Comment() const override;
  void Print(std::ostream& out, unsigned indent) const override;

  const std::string& path() const { return path_; }

 private:
  std::string name_;
  std::string path_;
  std::string type_;

  PBXFileReference(const PBXFileReference&) = delete;
  PBXFileReference& operator=(const PBXFileReference&) = delete;
};

// PBXFrameworksBuildPhase ----------------------------------------------------

class PBXFrameworksBuildPhase : public PBXBuildPhase {
 public:
  PBXFrameworksBuildPhase();
  ~PBXFrameworksBuildPhase() override;

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  std::string Name() const override;
  void Print(std::ostream& out, unsigned indent) const override;

 private:
  PBXFrameworksBuildPhase(const PBXFrameworksBuildPhase&) = delete;
  PBXFrameworksBuildPhase& operator=(const PBXFrameworksBuildPhase&) = delete;
};

// PBXGroup -------------------------------------------------------------------

class PBXGroup : public PBXObject {
 public:
  explicit PBXGroup(const std::string& path = std::string(),
                    const std::string& name = std::string());
  ~PBXGroup() override;

  const std::string& path() const { return path_; }
  const std::string& name() const { return name_; }

  PBXFileReference* AddSourceFile(const std::string& navigator_path,
                                  const std::string& source_path);

  template <typename T, typename... Args>
  T* CreateChild(Args&&... args) {
    return static_cast<T*>(
        AddChildImpl(std::make_unique<T>(std::forward<Args>(args)...)));
  }

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  std::string Name() const override;
  void Visit(PBXObjectVisitor& visitor) override;
  void Visit(PBXObjectVisitorConst& visitor) const override;
  void Print(std::ostream& out, unsigned indent) const override;

  // Returns whether the current PBXGroup should sort last when sorting
  // children of a PBXGroup. This should only be used for the "Products"
  // group which is hidden in Xcode UI when it is the last children of
  // the main PBXProject group.
  virtual bool SortLast() const;

 private:
  PBXObject* AddChildImpl(std::unique_ptr<PBXObject> child);

  std::vector<std::unique_ptr<PBXObject>> children_;
  std::string name_;
  std::string path_;

  PBXGroup(const PBXGroup&) = delete;
  PBXGroup& operator=(const PBXGroup&) = delete;
};

// PBXMainGroup ---------------------------------------------------------------

class PBXMainGroup : public PBXGroup {
 public:
  explicit PBXMainGroup(const std::string& source_path);
  ~PBXMainGroup() override;

  std::string Name() const override;
};

// PBXProductsGroup -----------------------------------------------------------

class PBXProductsGroup : public PBXGroup {
 public:
  explicit PBXProductsGroup();
  ~PBXProductsGroup() override;

  bool SortLast() const override;
};

// PBXNativeTarget ------------------------------------------------------------

class PBXNativeTarget : public PBXTarget {
 public:
  PBXNativeTarget(const std::string& name,
                  const std::string& shell_script,
                  const std::vector<std::string>& configs,
                  const PBXAttributes& attributes,
                  const std::string& product_type,
                  const std::string& product_name,
                  const PBXFileReference* product_reference);
  ~PBXNativeTarget() override;

  void AddResourceFile(const PBXFileReference* file_reference);

  void AddFileForIndexing(const PBXFileReference* file_reference);

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  void Print(std::ostream& out, unsigned indent) const override;

 private:
  const PBXFileReference* product_reference_ = nullptr;
  std::string product_type_;
  std::string product_name_;

  PBXNativeTarget(const PBXNativeTarget&) = delete;
  PBXNativeTarget& operator=(const PBXNativeTarget&) = delete;
};

// PBXProject -----------------------------------------------------------------

class PBXProject : public PBXObject {
 public:
  PBXProject(const std::string& name,
             std::vector<std::string> configs,
             const std::string& source_path,
             const PBXAttributes& attributes);
  ~PBXProject() override;

  void AddSourceFileToIndexingTarget(const std::string& navigator_path,
                                     const std::string& source_path);
  void AddSourceFile(const std::string& navigator_path,
                     const std::string& source_path,
                     PBXNativeTarget* target);
  void AddAggregateTarget(const std::string& name,
                          const std::string& output_dir,
                          const std::string& shell_script);
  void AddIndexingTarget();
  PBXNativeTarget* AddNativeTarget(
      const std::string& name,
      const std::string& type,
      const std::string& output_dir,
      const std::string& output_name,
      const std::string& output_type,
      const std::string& shell_script,
      const PBXAttributes& extra_attributes = PBXAttributes());

  void SetProjectDirPath(const std::string& project_dir_path);
  void SetProjectRoot(const std::string& project_root);
  void AddTarget(std::unique_ptr<PBXTarget> target);

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  std::string Name() const override;
  std::string Comment() const override;
  void Visit(PBXObjectVisitor& visitor) override;
  void Visit(PBXObjectVisitorConst& visitor) const override;
  void Print(std::ostream& out, unsigned indent) const override;

 private:
  PBXAttributes attributes_;
  std::unique_ptr<XCConfigurationList> configurations_;
  std::unique_ptr<PBXGroup> main_group_;
  std::string project_dir_path_;
  std::string project_root_;
  std::vector<std::unique_ptr<PBXTarget>> targets_;
  std::string name_;
  std::vector<std::string> configs_;

  PBXGroup* products_ = nullptr;
  PBXNativeTarget* target_for_indexing_ = nullptr;

  PBXProject(const PBXProject&) = delete;
  PBXProject& operator=(const PBXProject&) = delete;
};

// PBXResourcesBuildPhase -----------------------------------------------------

class PBXResourcesBuildPhase : public PBXBuildPhase {
 public:
  PBXResourcesBuildPhase();
  ~PBXResourcesBuildPhase() override;

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  std::string Name() const override;
  void Print(std::ostream& out, unsigned indent) const override;

 private:
  PBXResourcesBuildPhase(const PBXResourcesBuildPhase&) = delete;
  PBXResourcesBuildPhase& operator=(const PBXResourcesBuildPhase&) = delete;
};

// PBXShellScriptBuildPhase ---------------------------------------------------

class PBXShellScriptBuildPhase : public PBXBuildPhase {
 public:
  PBXShellScriptBuildPhase(const std::string& name,
                           const std::string& shell_script);
  ~PBXShellScriptBuildPhase() override;

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  std::string Name() const override;
  void Print(std::ostream& out, unsigned indent) const override;

 private:
  std::string name_;
  std::string shell_script_;

  PBXShellScriptBuildPhase(const PBXShellScriptBuildPhase&) = delete;
  PBXShellScriptBuildPhase& operator=(const PBXShellScriptBuildPhase&) = delete;
};

// PBXSourcesBuildPhase -------------------------------------------------------

class PBXSourcesBuildPhase : public PBXBuildPhase {
 public:
  PBXSourcesBuildPhase();
  ~PBXSourcesBuildPhase() override;

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  std::string Name() const override;
  void Print(std::ostream& out, unsigned indent) const override;

 private:
  PBXSourcesBuildPhase(const PBXSourcesBuildPhase&) = delete;
  PBXSourcesBuildPhase& operator=(const PBXSourcesBuildPhase&) = delete;
};

// PBXTargetDependency -----------------------------------------------------
class PBXTargetDependency : public PBXObject {
 public:
  PBXTargetDependency(
      const PBXTarget* target,
      std::unique_ptr<PBXContainerItemProxy> container_item_proxy);
  ~PBXTargetDependency() override;

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  std::string Name() const override;
  void Visit(PBXObjectVisitor& visitor) override;
  void Visit(PBXObjectVisitorConst& visitor) const override;
  void Print(std::ostream& out, unsigned indent) const override;

 private:
  const PBXTarget* target_ = nullptr;
  std::unique_ptr<PBXContainerItemProxy> container_item_proxy_;

  PBXTargetDependency(const PBXTargetDependency&) = delete;
  PBXTargetDependency& operator=(const PBXTargetDependency&) = delete;
};

// XCBuildConfiguration -------------------------------------------------------

class XCBuildConfiguration : public PBXObject {
 public:
  XCBuildConfiguration(const std::string& name,
                       const PBXAttributes& attributes);
  ~XCBuildConfiguration() override;

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  std::string Name() const override;
  void Print(std::ostream& out, unsigned indent) const override;

 private:
  PBXAttributes attributes_;
  std::string name_;

  XCBuildConfiguration(const XCBuildConfiguration&) = delete;
  XCBuildConfiguration& operator=(const XCBuildConfiguration&) = delete;
};

// XCConfigurationList --------------------------------------------------------

class XCConfigurationList : public PBXObject {
 public:
  XCConfigurationList(const std::vector<std::string>& configs,
                      const PBXAttributes& attributes,
                      const PBXObject* owner_reference);
  ~XCConfigurationList() override;

  // PBXObject implementation.
  PBXObjectClass Class() const override;
  std::string Name() const override;
  void Visit(PBXObjectVisitor& visitor) override;
  void Visit(PBXObjectVisitorConst& visitor) const override;
  void Print(std::ostream& out, unsigned indent) const override;

 private:
  std::vector<std::unique_ptr<XCBuildConfiguration>> configurations_;
  const PBXObject* owner_reference_ = nullptr;

  XCConfigurationList(const XCConfigurationList&) = delete;
  XCConfigurationList& operator=(const XCConfigurationList&) = delete;
};

#endif  // TOOLS_GN_XCODE_OBJECT_H_
