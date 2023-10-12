// Copyright 2012-2014 The Rust Project Developers. See the COPYRIGHT
// file at the top-level directory of this distribution and at
// http://rust-lang.org/COPYRIGHT.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
pub use self::Mode::*;

use std::env;
use std::fmt;
use std::fs::{read_dir, remove_file};
use std::str::FromStr;
use std::path::PathBuf;
#[cfg(feature = "rustc")]
use rustc_session;

use test::ColorConfig;
use runtest::dylib_env_var;

#[derive(Clone, Copy, PartialEq, Debug)]
pub enum Mode {
    CompileFail,
    ParseFail,
    RunFail,
    RunPass,
    RunPassValgrind,
    Pretty,
    DebugInfoGdb,
    DebugInfoLldb,
    Codegen,
    Rustdoc,
    CodegenUnits,
    Incremental,
    RunMake,
    Ui,
    MirOpt,
    Assembly,
}

impl Mode {
    pub fn disambiguator(self) -> &'static str {
        // Run-pass and pretty run-pass tests could run concurrently, and if they do,
        // they need to keep their output segregated. Same is true for debuginfo tests that
        // can be run both on gdb and lldb.
        match self {
            Pretty => ".pretty",
            DebugInfoGdb => ".gdb",
            DebugInfoLldb => ".lldb",
            _ => "",
        }
    }
}

impl FromStr for Mode {
    type Err = ();
    fn from_str(s: &str) -> Result<Mode, ()> {
        match s {
            "compile-fail" => Ok(CompileFail),
            "parse-fail" => Ok(ParseFail),
            "run-fail" => Ok(RunFail),
            "run-pass" => Ok(RunPass),
            "run-pass-valgrind" => Ok(RunPassValgrind),
            "pretty" => Ok(Pretty),
            "debuginfo-lldb" => Ok(DebugInfoLldb),
            "debuginfo-gdb" => Ok(DebugInfoGdb),
            "codegen" => Ok(Codegen),
            "rustdoc" => Ok(Rustdoc),
            "codegen-units" => Ok(CodegenUnits),
            "incremental" => Ok(Incremental),
            "run-make" => Ok(RunMake),
            "ui" => Ok(Ui),
            "mir-opt" => Ok(MirOpt),
            "assembly" => Ok(Assembly),
            _ => Err(()),
        }
    }
}

impl fmt::Display for Mode {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        fmt::Display::fmt(match *self {
                              CompileFail => "compile-fail",
                              ParseFail => "parse-fail",
                              RunFail => "run-fail",
                              RunPass => "run-pass",
                              RunPassValgrind => "run-pass-valgrind",
                              Pretty => "pretty",
                              DebugInfoGdb => "debuginfo-gdb",
                              DebugInfoLldb => "debuginfo-lldb",
                              Codegen => "codegen",
                              Rustdoc => "rustdoc",
                              CodegenUnits => "codegen-units",
                              Incremental => "incremental",
                              RunMake => "run-make",
                              Ui => "ui",
                              MirOpt => "mir-opt",
                              Assembly => "assembly",
                          },
                          f)
    }
}

#[derive(Clone)]
pub struct Config {
    /// `true` to overwrite stderr/stdout/fixed files instead of complaining about changes in output.
    pub bless: bool,

    /// The library paths required for running the compiler
    pub compile_lib_path: PathBuf,

    /// The library paths required for running compiled programs
    pub run_lib_path: PathBuf,

    /// The rustc executable
    pub rustc_path: PathBuf,

    /// The rustdoc executable
    pub rustdoc_path: Option<PathBuf>,

    /// The python executable to use for LLDB
    pub lldb_python: String,

    /// The python executable to use for htmldocck
    pub docck_python: String,

    /// The llvm FileCheck binary path
    pub llvm_filecheck: Option<PathBuf>,

    /// The valgrind path
    pub valgrind_path: Option<String>,

    /// Whether to fail if we can't run run-pass-valgrind tests under valgrind
    /// (or, alternatively, to silently run them like regular run-pass tests).
    pub force_valgrind: bool,

    /// The directory containing the tests to run
    pub src_base: PathBuf,

    /// The directory where programs should be built
    pub build_base: PathBuf,

    /// The name of the stage being built (stage1, etc)
    pub stage_id: String,

    /// The test mode, compile-fail, run-fail, run-pass
    pub mode: Mode,

    /// Run ignored tests
    pub run_ignored: bool,

    /// Only run tests that match these filters
    pub filters: Vec<String>,

    /// Exactly match the filter, rather than a substring
    pub filter_exact: bool,

    /// Write out a parseable log of tests that were run
    pub logfile: Option<PathBuf>,

    /// A command line to prefix program execution with,
    /// for running under valgrind
    pub runtool: Option<String>,

    /// Flags to pass to the compiler when building for the host
    pub host_rustcflags: Option<String>,

    /// Flags to pass to the compiler when building for the target
    pub target_rustcflags: Option<String>,

    /// Target system to be tested
    pub target: String,

    /// Host triple for the compiler being invoked
    pub host: String,

    /// Path to / name of the GDB executable
    pub gdb: Option<String>,

    /// Version of GDB, encoded as ((major * 1000) + minor) * 1000 + patch
    pub gdb_version: Option<u32>,

    /// Whether GDB has native rust support
    pub gdb_native_rust: bool,

    /// Version of LLDB
    pub lldb_version: Option<String>,

    /// Version of LLVM
    pub llvm_version: Option<String>,

    /// Is LLVM a system LLVM
    pub system_llvm: bool,

    /// Path to the android tools
    pub android_cross_path: PathBuf,

    /// Extra parameter to run adb on arm-linux-androideabi
    pub adb_path: String,

    /// Extra parameter to run test suite on arm-linux-androideabi
    pub adb_test_dir: String,

    /// status whether android device available or not
    pub adb_device_status: bool,

    /// the path containing LLDB's Python module
    pub lldb_python_dir: Option<String>,

    /// Explain what's going on
    pub verbose: bool,

    /// Print one character per test instead of one line
    pub quiet: bool,

    /// Whether to use colors in test.
    pub color: ColorConfig,

    /// where to find the remote test client process, if we're using it
    pub remote_test_client: Option<PathBuf>,

    /// If true, this will generate a coverage file with UI test files that run `MachineApplicable`
    /// diagnostics but are missing `run-rustfix` annotations. The generated coverage file is
    /// created in `/<build_base>/rustfix_missing_coverage.txt`
    pub rustfix_coverage: bool,

    /// The default Rust edition
    pub edition: Option<String>,

    /// Whether parsing of headers uses `//@` and errors on malformed headers or
    /// just allows any comment to have headers and silently ignores things that don't parse
    /// as a header.
    pub strict_headers: bool,

    // Configuration for various run-make tests frobbing things like C compilers
    // or querying about various LLVM component information.
    pub cc: String,
    pub cxx: String,
    pub cflags: String,
    pub ar: String,
    pub linker: Option<String>,
    pub llvm_components: String,
    pub llvm_cxxflags: String,
    pub nodejs: Option<String>,
}

#[derive(Clone)]
pub struct TestPaths {
    pub file: PathBuf,         // e.g., compile-test/foo/bar/baz.rs
    pub base: PathBuf,         // e.g., compile-test, auxiliary
    pub relative_dir: PathBuf, // e.g., foo/bar
}

/// Used by `ui` tests to generate things like `foo.stderr` from `foo.rs`.
pub fn expected_output_path(
    testpaths: &TestPaths,
    revision: Option<&str>,
    kind: &str,
) -> PathBuf {
    assert!(UI_EXTENSIONS.contains(&kind));
    let mut parts = Vec::new();

    if let Some(x) = revision {
        parts.push(x);
    }
    parts.push(kind);

    let extension = parts.join(".");
    testpaths.file.with_extension(extension)
}

pub const UI_EXTENSIONS: &[&str] = &[UI_STDERR, UI_STDOUT, UI_FIXED];
pub const UI_STDERR: &str = "stderr";
pub const UI_STDOUT: &str = "stdout";
pub const UI_FIXED: &str = "fixed";

impl Config {
    /// Add rustc flags to link with the crate's dependencies in addition to the crate itself
    pub fn link_deps(&mut self) {
        let varname = dylib_env_var();

        // Dependencies can be found in the environment variable. Throw everything there into the
        // link flags
        let lib_paths = env::var(varname).unwrap_or_else(|err| match err {
            env::VarError::NotPresent => String::new(),
            err => panic!("can't get {} environment variable: {}", varname, err),
        });

        // Append to current flags if any are set, otherwise make new String
        let mut flags = self.target_rustcflags.take().unwrap_or_else(String::new);
        if !lib_paths.is_empty() {
            for p in env::split_paths(&lib_paths) {
                flags += " -L ";
                flags += p.to_str().unwrap(); // Can't fail. We already know this is unicode
            }
        }

        self.target_rustcflags = Some(flags);
    }

    /// Remove rmeta files from target `deps` directory
    ///
    /// These files are created by `cargo check`, and conflict with
    /// `cargo build` rlib files, causing E0464 for tests which use
    /// the parent crate.
    pub fn clean_rmeta(&self) {
        if self.target_rustcflags.is_some() {
            for directory in self.target_rustcflags
                .as_ref()
                .unwrap()
                .split_whitespace()
                .filter(|s| s.ends_with("/deps"))
            {
                if let Ok(mut entries) = read_dir(directory) {
                    while let Some(Ok(entry)) = entries.next() {
                        if entry.file_name().to_string_lossy().ends_with(".rmeta") {
                            let _ = remove_file(entry.path());
                        }
                    }
                }
            }
        }
    }

    #[cfg(feature = "tmp")]
    pub fn tempdir(mut self) -> ConfigWithTemp {
        let tmp = tempfile::Builder::new().prefix("compiletest").tempdir()
            .expect("failed to create temporary directory");
        self.build_base = tmp.path().to_owned();
        config_tempdir::ConfigWithTemp {
            config: self,
            tempdir: tmp,
        }
    }
}

#[cfg(feature = "tmp")]
mod config_tempdir {
    use tempfile;
    use std::ops;

    pub struct ConfigWithTemp {
        pub config: super::Config,
        pub tempdir: tempfile::TempDir,
    }

    impl ops::Deref for ConfigWithTemp {
        type Target = super::Config;

        fn deref(&self) -> &Self::Target {
            &self.config
        }
    }

    impl ops::DerefMut for ConfigWithTemp {
        fn deref_mut(&mut self) -> &mut Self::Target {
            &mut self.config
        }
    }
}

#[cfg(feature = "tmp")]
pub use self::config_tempdir::ConfigWithTemp;


impl Default for Config {
    fn default() -> Config {
        #[cfg(feature = "rustc")]
        let platform = rustc_session::config::host_triple().to_string();

        Config {
            bless: false,
            compile_lib_path: PathBuf::from(""),
            run_lib_path: PathBuf::from(""),
            rustc_path: PathBuf::from("rustc"),
            rustdoc_path: None,
            lldb_python: "python".to_owned(),
            docck_python: "docck-python".to_owned(),
            valgrind_path: None,
            force_valgrind: false,
            llvm_filecheck: None,
            src_base: PathBuf::from("tests/run-pass"),
            build_base: env::temp_dir(),
            stage_id: "stage-id".to_owned(),
            mode: Mode::RunPass,
            run_ignored: false,
            filters: vec![],
            filter_exact: false,
            logfile: None,
            runtool: None,
            host_rustcflags: None,
            target_rustcflags: None,
            #[cfg(feature = "rustc")]
            target: platform.clone(),
            #[cfg(not(feature = "rustc"))]
            target: env!("TARGET").to_string(),
            #[cfg(feature = "rustc")]
            host: platform.clone(),
            #[cfg(not(feature = "rustc"))]
            host: env!("HOST").to_string(),
            rustfix_coverage: false,
            gdb: None,
            gdb_version: None,
            gdb_native_rust: false,
            lldb_version: None,
            llvm_version: None,
            system_llvm: false,
            android_cross_path: PathBuf::from("android-cross-path"),
            adb_path: "adb-path".to_owned(),
            adb_test_dir: "adb-test-dir/target".to_owned(),
            adb_device_status: false,
            lldb_python_dir: None,
            verbose: false,
            quiet: false,
            color: ColorConfig::AutoColor,
            remote_test_client: None,
            cc: "cc".to_string(),
            cxx: "cxx".to_string(),
            cflags: "cflags".to_string(),
            ar: "ar".to_string(),
            linker: None,
            llvm_components: "llvm-components".to_string(),
            llvm_cxxflags: "llvm-cxxflags".to_string(),
            nodejs: None,
            edition: None,
            strict_headers: false,
        }
    }
}
