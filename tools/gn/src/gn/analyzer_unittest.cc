// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/analyzer.h"

#include "gn/c_tool.h"
#include "gn/build_settings.h"
#include "gn/builder.h"
#include "gn/config.h"
#include "gn/general_tool.h"
#include "gn/loader.h"
#include "gn/pool.h"
#include "gn/settings.h"
#include "gn/source_file.h"
#include "gn/substitution_list.h"
#include "gn/target.h"
#include "gn/tool.h"
#include "gn/toolchain.h"
#include "util/test/test.h"

namespace gn_analyzer_unittest {

class MockLoader : public Loader {
 public:
  MockLoader() = default;

  void Load(const SourceFile& file,
            const LocationRange& origin,
            const Label& toolchain_name) override {}
  void ToolchainLoaded(const Toolchain* toolchain) override {}
  Label GetDefaultToolchain() const override {
    return Label(SourceDir("//tc/"), "default");
  }
  const Settings* GetToolchainSettings(const Label& label) const override {
    return nullptr;
  }
  SourceFile BuildFileForLabel(const Label& label) const override {
    return SourceFile(label.dir().value() + "BUILD.gn");
  }

 private:
  ~MockLoader() override = default;
};

class AnalyzerTest : public testing::Test {
 public:
  AnalyzerTest()
      : loader_(new MockLoader),
        builder_(loader_.get()),
        settings_(&build_settings_, std::string()),
        other_settings_(&build_settings_, std::string()) {
    build_settings_.SetBuildDir(SourceDir("//out/"));

    settings_.set_toolchain_label(Label(SourceDir("//tc/"), "default"));
    settings_.set_default_toolchain_label(settings_.toolchain_label());
    tc_dir_ = settings_.toolchain_label().dir();
    tc_name_ = settings_.toolchain_label().name();

    other_settings_.set_toolchain_label(Label(SourceDir("//other/"), "tc"));
    other_settings_.set_default_toolchain_label(
        other_settings_.toolchain_label());
    tc_other_dir_ = other_settings_.toolchain_label().dir();
    tc_other_name_ = other_settings_.toolchain_label().name();
  }

  std::unique_ptr<Target> MakeTarget(const std::string& dir,
                                     const std::string& name) {
    Label label(SourceDir(dir), name, tc_dir_, tc_name_);
    return std::make_unique<Target>(&settings_, label);
  }

  std::unique_ptr<Target> MakeTargetOtherToolchain(const std::string& dir,
                                                   const std::string& name) {
    Label label(SourceDir(dir), name, tc_other_dir_, tc_other_name_);
    return std::make_unique<Target>(&other_settings_, label);
  }

  std::unique_ptr<Config> MakeConfig(const std::string& dir,
                                     const std::string& name) {
    Label label(SourceDir(dir), name, tc_dir_, tc_name_);
    return std::make_unique<Config>(&settings_, label);
  }

  std::unique_ptr<Pool> MakePool(const std::string& dir,
                                 const std::string& name) {
    Label label(SourceDir(dir), name, tc_dir_, tc_name_);
    return std::make_unique<Pool>(&settings_, label);
  }

  void RunAnalyzerTest(const std::string& input,
                       const std::string& expected_output) {
    Analyzer analyzer(builder_, SourceFile("//build/config/BUILDCONFIG.gn"),
                      SourceFile("//.gn"),
                      {SourceFile("//out/debug/args.gn"),
                       SourceFile("//build/default_args.gn")});
    Err err;
    std::string actual_output = analyzer.Analyze(input, &err);
    EXPECT_EQ(err.has_error(), false);
    EXPECT_EQ(expected_output, actual_output);
  }

 protected:
  scoped_refptr<MockLoader> loader_;
  Builder builder_;
  BuildSettings build_settings_;

  Settings settings_;
  SourceDir tc_dir_;
  std::string tc_name_;

  Settings other_settings_;
  SourceDir tc_other_dir_;
  std::string tc_other_name_;
};

// Tests that a target is marked as affected if its sources are modified.
TEST_F(AnalyzerTest, TargetRefersToSources) {
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  Target* t_raw = t.get();
  builder_.ItemDefined(std::move(t));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/file_name.cc" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t_raw->sources().push_back(SourceFile("//dir/file_name.cc"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/file_name.cc" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that a target is marked as affected if its public headers are modified.
TEST_F(AnalyzerTest, TargetRefersToPublicHeaders) {
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  Target* t_raw = t.get();
  builder_.ItemDefined(std::move(t));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/header_name.h" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t_raw->public_headers().push_back(SourceFile("//dir/header_name.h"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/header_name.h" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that a target is marked as affected if its inputs are modified.
TEST_F(AnalyzerTest, TargetRefersToInputs) {
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  Target* t_raw = t.get();
  builder_.ItemDefined(std::move(t));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/extra_input.cc" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  SourceFile extra_input(SourceFile("//dir/extra_input.cc"));
  t_raw->config_values().inputs().push_back(extra_input);
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/extra_input.cc" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");

  t_raw->config_values().inputs().clear();
  std::unique_ptr<Config> c = MakeConfig("//dir", "config_name");
  c->own_values().inputs().push_back(extra_input);
  t_raw->configs().push_back(LabelConfigPair(c.get()));
  builder_.ItemDefined(std::move(c));

  RunAnalyzerTest(
      R"({
       "files": [ "//dir/extra_input.cc" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that a target is marked as affected if a sub-config is modified.
//
// This test uses two levels of sub-configs to ensure the config hierarchy
// is completely traversed.
TEST_F(AnalyzerTest, SubConfigIsModified) {
  std::unique_ptr<Config> ssc = MakeConfig("//dir3", "subsubconfig_name");
  ssc->build_dependency_files().insert(SourceFile("//dir3/BUILD.gn"));

  std::unique_ptr<Config> sc = MakeConfig("//dir2", "subconfig_name");
  sc->configs().push_back(LabelConfigPair(ssc->label()));

  std::unique_ptr<Config> c = MakeConfig("//dir", "config_name");
  c->configs().push_back(LabelConfigPair(sc->label()));

  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  t->configs().push_back(LabelConfigPair(c.get()));

  builder_.ItemDefined(std::move(ssc));
  builder_.ItemDefined(std::move(sc));
  builder_.ItemDefined(std::move(c));
  builder_.ItemDefined(std::move(t));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir3/BUILD.gn" ],
       "additional_compile_targets": [],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that a target is marked as affected if its data are modified.
TEST_F(AnalyzerTest, TargetRefersToData) {
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  Target* t_raw = t.get();
  builder_.ItemDefined(std::move(t));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/data.html" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t_raw->data().push_back("//dir/data.html");
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/data.html" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that a target is marked as affected if the target is an action and its
// action script is modified.
TEST_F(AnalyzerTest, TargetRefersToActionScript) {
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  Target* t_raw = t.get();
  t->set_output_type(Target::ACTION);
  builder_.ItemDefined(std::move(t));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/script.py" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t_raw->action_values().set_script(SourceFile("//dir/script.py"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/script.py" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that a target is marked as affected if its build dependency files are
// modified.
TEST_F(AnalyzerTest, TargetRefersToBuildDependencyFiles) {
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  Target* t_raw = t.get();
  builder_.ItemDefined(std::move(t));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t_raw->build_dependency_files().insert(SourceFile("//dir/BUILD.gn"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that if a target is marked as affected, then it propagates to dependent
// test_targets.
TEST_F(AnalyzerTest, AffectedTargetpropagatesToDependentTargets) {
  std::unique_ptr<Target> t1 = MakeTarget("//dir", "target_name1");
  std::unique_ptr<Target> t2 = MakeTarget("//dir", "target_name2");
  std::unique_ptr<Target> t3 = MakeTarget("//dir", "target_name3");
  t1->private_deps().push_back(LabelTargetPair(t2.get()));
  t2->private_deps().push_back(LabelTargetPair(t3.get()));
  builder_.ItemDefined(std::move(t1));
  builder_.ItemDefined(std::move(t2));

  Target* t3_raw = t3.get();
  builder_.ItemDefined(std::move(t3));

  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name1", "//dir:target_name2" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t3_raw->build_dependency_files().insert(SourceFile("//dir/BUILD.gn"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name1", "//dir:target_name2" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name1","//dir:target_name2"])"
      "}");
}

// Tests that if a config is marked as affected, then it propagates to dependent
// test_targets.
TEST_F(AnalyzerTest, AffectedConfigpropagatesToDependentTargets) {
  std::unique_ptr<Config> c = MakeConfig("//dir", "config_name");
  Config* c_raw = c.get();
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  t->configs().push_back(LabelConfigPair(c.get()));
  builder_.ItemDefined(std::move(t));
  builder_.ItemDefined(std::move(c));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  c_raw->build_dependency_files().insert(SourceFile("//dir/BUILD.gn"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that if toolchain is marked as affected, then it propagates to
// dependent test_targets.
TEST_F(AnalyzerTest, AffectedToolchainpropagatesToDependentTargets) {
  std::unique_ptr<Target> target = MakeTarget("//dir", "target_name");
  target->set_output_type(Target::EXECUTABLE);
  Toolchain* toolchain = new Toolchain(&settings_, settings_.toolchain_label());

  // The tool is not used anywhere, but is required to construct the dependency
  // between a target and the toolchain.
  std::unique_ptr<Tool> fake_tool = Tool::CreateTool(CTool::kCToolLink);
  fake_tool->set_outputs(
      SubstitutionList::MakeForTest("//out/debug/output.txt"));
  toolchain->SetTool(std::move(fake_tool));
  builder_.ItemDefined(std::move(target));
  builder_.ItemDefined(std::unique_ptr<Item>(toolchain));

  RunAnalyzerTest(
      R"({
         "files": [ "//tc/BUILD.gn" ],
         "additional_compile_targets": [ "all" ],
         "test_targets": [ "//dir:target_name" ]
         })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  toolchain->build_dependency_files().insert(SourceFile("//tc/BUILD.gn"));
  RunAnalyzerTest(
      R"({
       "files": [ "//tc/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that if a pool is marked as affected, then it propagates to dependent
// targets.
TEST_F(AnalyzerTest, AffectedPoolpropagatesToDependentTargets) {
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  t->set_output_type(Target::ACTION);
  std::unique_ptr<Pool> p = MakePool("//dir", "pool_name");
  Pool* p_raw = p.get();
  t->set_pool(LabelPtrPair<Pool>(p.get()));

  builder_.ItemDefined(std::move(t));
  builder_.ItemDefined(std::move(p));

  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  p_raw->build_dependency_files().insert(SourceFile("//dir/BUILD.gn"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that when dependency was found, the "compile_targets" in the output is
// not "all".
TEST_F(AnalyzerTest, CompileTargetsAllWasPruned) {
  std::unique_ptr<Target> t1 = MakeTarget("//dir", "target_name1");
  std::unique_ptr<Target> t2 = MakeTarget("//dir", "target_name2");
  t2->build_dependency_files().insert(SourceFile("//dir/BUILD.gn"));
  builder_.ItemDefined(std::move(t1));
  builder_.ItemDefined(std::move(t2));

  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": []
       })",
      "{"
      R"("compile_targets":["//dir:target_name2"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":[])"
      "}");
}

// Tests that output is "No dependency" when no dependency is found.
TEST_F(AnalyzerTest, NoDependency) {
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::move(t));

  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": []
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");
}

// Tests that output is "No dependency" when no files or targets are provided.
TEST_F(AnalyzerTest, NoFilesNoTargets) {
  RunAnalyzerTest(
      R"({
       "files": [],
       "additional_compile_targets": [],
       "test_targets": []
      })",
      "{"
      R"("compile_targets":[],)"
      R"("status":"No dependency",)"
      R"("test_targets":[])"
      "}");
}

// Tests that output displays proper error message when given files aren't
// source-absolute or absolute path.
TEST_F(AnalyzerTest, FilesArentSourceAbsolute) {
  RunAnalyzerTest(
      R"({
       "files": [ "a.cc" ],
       "additional_compile_targets": [],
       "test_targets": [ "//dir:target_name" ]
      })",
      "{"
      R"("error":)"
      R"("\"a.cc\" is not a source-absolute or absolute path.",)"
      R"("invalid_targets":[])"
      "}");
}

// Tests that output displays proper error message when input is ill-formed.
TEST_F(AnalyzerTest, WrongInputFields) {
  RunAnalyzerTest(
      R"({
       "files": [ "//a.cc" ],
       "compile_targets": [],
       "test_targets": [ "//dir:target_name" ]
      })",
      "{"
      R"("error":)"
      R"("Unknown analyze input key \"compile_targets\".",)"
      R"("invalid_targets":[])"
      "}");
}

// Bails out early with "Found dependency (all)" if dot file is modified.
TEST_F(AnalyzerTest, DotFileWasModified) {
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::move(t));

  RunAnalyzerTest(
      R"({
       "files": [ "//.gn" ],
       "additional_compile_targets": [],
       "test_targets": [ "//dir:target_name" ]
      })",
      "{"
      R"("compile_targets":["//dir:target_name"],)"
      R"/("status":"Found dependency (all)",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Bails out early with "Found dependency (all)" if master build config file is
// modified.
TEST_F(AnalyzerTest, BuildConfigFileWasModified) {
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::move(t));

  RunAnalyzerTest(
      R"({
       "files": [ "//build/config/BUILDCONFIG.gn" ],
       "additional_compile_targets": [],
       "test_targets": [ "//dir:target_name" ]
      })",
      "{"
      R"("compile_targets":["//dir:target_name"],)"
      R"/("status":"Found dependency (all)",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Bails out early with "Found dependency (all)" if a build args dependency file
// is modified.
TEST_F(AnalyzerTest, BuildArgsDependencyFileWasModified) {
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::move(t));

  RunAnalyzerTest(
      R"({
       "files": [ "//build/default_args.gn" ],
       "additional_compile_targets": [],
       "test_targets": [ "//dir:target_name" ]
      })",
      "{"
      R"("compile_targets":["//dir:target_name"],)"
      R"/("status":"Found dependency (all)",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that targets in explicitly labelled with the default toolchain are
// included when their sources change.
// change.
TEST_F(AnalyzerTest, TargetToolchainSpecifiedRefersToSources) {
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  Target* t_raw = t.get();
  builder_.ItemDefined(std::move(t));

  RunAnalyzerTest(
      R"/({
       "files": [ "//dir/file_name.cc" ],
       "additional_compile_targets": ["all"],
       "test_targets": [ "//dir:target_name(//tc:default)" ]
       })/",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t_raw->sources().push_back(SourceFile("//dir/file_name.cc"));

  RunAnalyzerTest(
      R"*({
       "files": [ "//dir/file_name.cc" ],
       "additional_compile_targets": [],
       "test_targets": [ "//dir:target_name(//tc:default)" ]
       })*",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"Found dependency",)/"
      R"/("test_targets":["//dir:target_name"])/"
      "}");
}

// Tests that targets in alternate toolchains are affected when their sources
// change.
TEST_F(AnalyzerTest, TargetAlternateToolchainRefersToSources) {
  std::unique_ptr<Target> t = MakeTarget("//dir", "target_name");
  std::unique_ptr<Target> t_alt =
      MakeTargetOtherToolchain("//dir", "target_name");
  Target* t_raw = t.get();
  Target* t_alt_raw = t_alt.get();
  builder_.ItemDefined(std::move(t));
  builder_.ItemDefined(std::move(t_alt));

  RunAnalyzerTest(
      R"/({
       "files": [ "//dir/file_name.cc" ],
       "additional_compile_targets": ["all"],
       "test_targets": [ "//dir:target_name", "//dir:target_name(//other:tc)" ]
       })/",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t_raw->sources().push_back(SourceFile("//dir/file_name.cc"));
  t_alt_raw->sources().push_back(SourceFile("//dir/alt_file_name.cc"));

  RunAnalyzerTest(
      R"*({
       "files": [ "//dir/file_name.cc" ],
       "additional_compile_targets": [],
       "test_targets": [ "//dir:target_name", "//dir:target_name(//other:tc)" ]
       })*",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");

  RunAnalyzerTest(
      R"*({
       "files": [ "//dir/alt_file_name.cc" ],
       "additional_compile_targets": [],
       "test_targets": [ "//dir:target_name", "//dir:target_name(//other:tc)" ]
       })*",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"Found dependency",)/"
      R"/("test_targets":["//dir:target_name(//other:tc)"])/"
      "}");

  RunAnalyzerTest(
      R"*({
       "files": [ "//dir/file_name.cc", "//dir/alt_file_name.cc" ],
       "additional_compile_targets": [],
       "test_targets": [ "//dir:target_name", "//dir:target_name(//other:tc)" ]
       })*",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"Found dependency",)/"
      R"/("test_targets":["//dir:target_name","//dir:target_name(//other:tc)"])/"
      "}");
}

}  // namespace gn_analyzer_unittest
