// Copyright 2012-2014 The Rust Project Developers. See the COPYRIGHT
// file at the top-level directory of this distribution and at
// http://rust-lang.org/COPYRIGHT.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#![crate_type = "lib"]
#![cfg_attr(feature = "rustc", feature(rustc_private))]
#![cfg_attr(feature = "rustc", feature(test))]
#![deny(unused_imports)]

#[cfg(feature = "rustc")]
extern crate rustc_session;

#[cfg(unix)]
extern crate libc;
#[cfg(feature = "rustc")]
extern crate test;
#[cfg(not(feature = "rustc"))]
extern crate tester as test;

#[cfg(feature = "tmp")]
extern crate tempfile;

#[macro_use]
extern crate log;
extern crate diff;
extern crate filetime;
extern crate regex;
extern crate serde_json;
#[macro_use]
extern crate serde_derive;
extern crate rustfix;

use common::{DebugInfoGdb, DebugInfoLldb, Pretty};
use common::{Mode, TestPaths};
use std::env;
use std::ffi::OsString;
use std::fs;
use std::io;
use std::path::{Path, PathBuf};

use self::header::EarlyProps;

pub mod common;
pub mod errors;
pub mod header;
mod json;
mod read2;
pub mod runtest;
pub mod uidiff;
pub mod util;

pub use common::Config;

pub fn run_tests(config: &Config) {
    if config.target.contains("android") {
        if let DebugInfoGdb = config.mode {
            println!(
                "{} debug-info test uses tcp 5039 port.\
                     please reserve it",
                config.target
            );
        }

        // android debug-info test uses remote debugger
        // so, we test 1 thread at once.
        // also trying to isolate problems with adb_run_wrapper.sh ilooping
        env::set_var("RUST_TEST_THREADS", "1");
    }

    if let DebugInfoLldb = config.mode {
        // Some older versions of LLDB seem to have problems with multiple
        // instances running in parallel, so only run one test task at a
        // time.
        env::set_var("RUST_TEST_TASKS", "1");
    }

    // If we want to collect rustfix coverage information,
    // we first make sure that the coverage file does not exist.
    // It will be created later on.
    if config.rustfix_coverage {
        let mut coverage_file_path = config.build_base.clone();
        coverage_file_path.push("rustfix_missing_coverage.txt");
        if coverage_file_path.exists() {
            if let Err(e) = fs::remove_file(&coverage_file_path) {
                panic!(
                    "Could not delete {} due to {}",
                    coverage_file_path.display(),
                    e
                )
            }
        }
    }
    let opts = test_opts(config);
    let tests = make_tests(config);
    // sadly osx needs some file descriptor limits raised for running tests in
    // parallel (especially when we have lots and lots of child processes).
    // For context, see #8904
    // unsafe { raise_fd_limit::raise_fd_limit(); }
    // Prevent issue #21352 UAC blocking .exe containing 'patch' etc. on Windows
    // If #11207 is resolved (adding manifest to .exe) this becomes unnecessary
    env::set_var("__COMPAT_LAYER", "RunAsInvoker");
    let res = test::run_tests_console(&opts, tests.into_iter().collect());
    match res {
        Ok(true) => {}
        Ok(false) => panic!("Some tests failed"),
        Err(e) => {
            println!("I/O failure during tests: {:?}", e);
        }
    }
}

pub fn test_opts(config: &Config) -> test::TestOpts {
    test::TestOpts {
        filters: config.filters.clone(),
        filter_exact: config.filter_exact,
        exclude_should_panic: false,
        force_run_in_process: false,
        run_ignored: if config.run_ignored {
            test::RunIgnored::Yes
        } else {
            test::RunIgnored::No
        },
        format: if config.quiet {
            test::OutputFormat::Terse
        } else {
            test::OutputFormat::Pretty
        },
        logfile: config.logfile.clone(),
        run_tests: true,
        bench_benchmarks: true,
        nocapture: match env::var("RUST_TEST_NOCAPTURE") {
            Ok(val) => &val != "0",
            Err(_) => false,
        },
        color: test::AutoColor,
        test_threads: None,
        skip: vec![],
        list: false,
        options: test::Options::new(),
        time_options: None,
        #[cfg(feature = "rustc")]
        shuffle: false,
        #[cfg(feature = "rustc")]
        shuffle_seed: None,
    }
}

pub fn make_tests(config: &Config) -> Vec<test::TestDescAndFn> {
    debug!("making tests from {:?}", config.src_base.display());
    let mut tests = Vec::new();
    collect_tests_from_dir(
        config,
        &config.src_base,
        &config.src_base,
        &PathBuf::new(),
        &mut tests,
    )
    .unwrap();
    tests
}

fn collect_tests_from_dir(
    config: &Config,
    base: &Path,
    dir: &Path,
    relative_dir_path: &Path,
    tests: &mut Vec<test::TestDescAndFn>,
) -> io::Result<()> {
    // Ignore directories that contain a file
    // `compiletest-ignore-dir`.
    for file in fs::read_dir(dir)? {
        let file = file?;
        let name = file.file_name();
        if name == *"compiletest-ignore-dir" {
            return Ok(());
        }
        if name == *"Makefile" && config.mode == Mode::RunMake {
            let paths = TestPaths {
                file: dir.to_path_buf(),
                base: base.to_path_buf(),
                relative_dir: relative_dir_path.parent().unwrap().to_path_buf(),
            };
            tests.push(make_test(config, &paths));
            return Ok(());
        }
    }

    // If we find a test foo/bar.rs, we have to build the
    // output directory `$build/foo` so we can write
    // `$build/foo/bar` into it. We do this *now* in this
    // sequential loop because otherwise, if we do it in the
    // tests themselves, they race for the privilege of
    // creating the directories and sometimes fail randomly.
    let build_dir = config.build_base.join(&relative_dir_path);
    fs::create_dir_all(&build_dir).unwrap();

    // Add each `.rs` file as a test, and recurse further on any
    // subdirectories we find, except for `aux` directories.
    let dirs = fs::read_dir(dir)?;
    for file in dirs {
        let file = file?;
        let file_path = file.path();
        let file_name = file.file_name();
        if is_test(&file_name) {
            debug!("found test file: {:?}", file_path.display());
            // output directory `$build/foo` so we can write
            // `$build/foo/bar` into it. We do this *now* in this
            // sequential loop because otherwise, if we do it in the
            // tests themselves, they race for the privilege of
            // creating the directories and sometimes fail randomly.
            let build_dir = config.build_base.join(&relative_dir_path);
            fs::create_dir_all(&build_dir).unwrap();

            let paths = TestPaths {
                file: file_path,
                base: base.to_path_buf(),
                relative_dir: relative_dir_path.to_path_buf(),
            };
            tests.push(make_test(config, &paths))
        } else if file_path.is_dir() {
            let relative_file_path = relative_dir_path.join(file.file_name());
            if &file_name == "auxiliary" {
                // `aux` directories contain other crates used for
                // cross-crate tests. Don't search them for tests, but
                // do create a directory in the build dir for them,
                // since we will dump intermediate output in there
                // sometimes.
                let build_dir = config.build_base.join(&relative_file_path);
                fs::create_dir_all(&build_dir).unwrap();
            } else {
                debug!("found directory: {:?}", file_path.display());
                collect_tests_from_dir(config, base, &file_path, &relative_file_path, tests)?;
            }
        } else {
            debug!("found other file/directory: {:?}", file_path.display());
        }
    }
    Ok(())
}

pub fn is_test(file_name: &OsString) -> bool {
    let file_name = file_name.to_str().unwrap();

    if !file_name.ends_with(".rs") {
        return false;
    }

    // `.`, `#`, and `~` are common temp-file prefixes.
    let invalid_prefixes = &[".", "#", "~"];
    !invalid_prefixes.iter().any(|p| file_name.starts_with(p))
}

pub fn make_test(config: &Config, testpaths: &TestPaths) -> test::TestDescAndFn {
    let early_props = EarlyProps::from_file(config, &testpaths.file);

    // The `should-fail` annotation doesn't apply to pretty tests,
    // since we run the pretty printer across all tests by default.
    // If desired, we could add a `should-fail-pretty` annotation.
    let should_panic = match config.mode {
        Pretty => test::ShouldPanic::No,
        _ => {
            if early_props.should_fail {
                test::ShouldPanic::Yes
            } else {
                test::ShouldPanic::No
            }
        }
    };

    test::TestDescAndFn {
        desc: test::TestDesc {
            name: make_test_name(config, testpaths),
            ignore: early_props.ignore,
            should_panic: should_panic,
            #[cfg(not(feature = "rustc"))]
            allow_fail: false,
            #[cfg(feature = "rustc")]
            compile_fail: false,
            #[cfg(feature = "rustc")]
            no_run: false,
            test_type: test::TestType::IntegrationTest,
            #[cfg(feature = "rustc")]
            ignore_message: None,
        },
        testfn: make_test_closure(config, testpaths),
    }
}

fn stamp(config: &Config, testpaths: &TestPaths) -> PathBuf {
    let stamp_name = format!(
        "{}-{}.stamp",
        testpaths.file.file_name().unwrap().to_str().unwrap(),
        config.stage_id
    );
    config
        .build_base
        .canonicalize()
        .unwrap_or_else(|_| config.build_base.clone())
        .join(stamp_name)
}

pub fn make_test_name(config: &Config, testpaths: &TestPaths) -> test::TestName {
    // Convert a complete path to something like
    //
    //    run-pass/foo/bar/baz.rs
    let path = PathBuf::from(config.src_base.file_name().unwrap())
        .join(&testpaths.relative_dir)
        .join(&testpaths.file.file_name().unwrap());
    test::DynTestName(format!("[{}] {}", config.mode, path.display()))
}

pub fn make_test_closure(config: &Config, testpaths: &TestPaths) -> test::TestFn {
    let config = config.clone();
    let testpaths = testpaths.clone();
    test::DynTestFn(Box::new(move || {
        let config = config.clone(); // FIXME: why is this needed?
        runtest::run(config, &testpaths)
    }))
}

fn extract_gdb_version(full_version_line: &str) -> Option<u32> {
    let full_version_line = full_version_line.trim();

    // GDB versions look like this: "major.minor.patch?.yyyymmdd?", with both
    // of the ? sections being optional

    // We will parse up to 3 digits for minor and patch, ignoring the date
    // We limit major to 1 digit, otherwise, on openSUSE, we parse the openSUSE version

    // don't start parsing in the middle of a number
    let mut prev_was_digit = false;
    for (pos, c) in full_version_line.char_indices() {
        if prev_was_digit || !c.is_digit(10) {
            prev_was_digit = c.is_digit(10);
            continue;
        }

        prev_was_digit = true;

        let line = &full_version_line[pos..];

        let next_split = match line.find(|c: char| !c.is_digit(10)) {
            Some(idx) => idx,
            None => continue, // no minor version
        };

        if line.as_bytes()[next_split] != b'.' {
            continue; // no minor version
        }

        let major = &line[..next_split];
        let line = &line[next_split + 1..];

        let (minor, patch) = match line.find(|c: char| !c.is_digit(10)) {
            Some(idx) => {
                if line.as_bytes()[idx] == b'.' {
                    let patch = &line[idx + 1..];

                    let patch_len = patch
                        .find(|c: char| !c.is_digit(10))
                        .unwrap_or_else(|| patch.len());
                    let patch = &patch[..patch_len];
                    let patch = if patch_len > 3 || patch_len == 0 {
                        None
                    } else {
                        Some(patch)
                    };

                    (&line[..idx], patch)
                } else {
                    (&line[..idx], None)
                }
            }
            None => (line, None),
        };

        if major.len() != 1 || minor.is_empty() {
            continue;
        }

        let major: u32 = major.parse().unwrap();
        let minor: u32 = minor.parse().unwrap();
        let patch: u32 = patch.unwrap_or("0").parse().unwrap();

        return Some(((major * 1000) + minor) * 1000 + patch);
    }

    None
}

#[allow(dead_code)]
fn extract_lldb_version(full_version_line: Option<String>) -> Option<String> {
    // Extract the major LLDB version from the given version string.
    // LLDB version strings are different for Apple and non-Apple platforms.
    // At the moment, this function only supports the Apple variant, which looks
    // like this:
    //
    // LLDB-179.5 (older versions)
    // lldb-300.2.51 (new versions)
    //
    // We are only interested in the major version number, so this function
    // will return `Some("179")` and `Some("300")` respectively.

    if let Some(ref full_version_line) = full_version_line {
        if !full_version_line.trim().is_empty() {
            let full_version_line = full_version_line.trim();

            for (pos, l) in full_version_line.char_indices() {
                if l != 'l' && l != 'L' {
                    continue;
                }
                if pos + 5 >= full_version_line.len() {
                    continue;
                }
                let l = full_version_line[pos + 1..].chars().next().unwrap();
                if l != 'l' && l != 'L' {
                    continue;
                }
                let d = full_version_line[pos + 2..].chars().next().unwrap();
                if d != 'd' && d != 'D' {
                    continue;
                }
                let b = full_version_line[pos + 3..].chars().next().unwrap();
                if b != 'b' && b != 'B' {
                    continue;
                }
                let dash = full_version_line[pos + 4..].chars().next().unwrap();
                if dash != '-' {
                    continue;
                }

                let vers = full_version_line[pos + 5..]
                    .chars()
                    .take_while(|c| c.is_digit(10))
                    .collect::<String>();
                if !vers.is_empty() {
                    return Some(vers);
                }
            }
            println!(
                "Could not extract LLDB version from line '{}'",
                full_version_line
            );
        }
    }
    None
}

#[allow(dead_code)]
fn is_blacklisted_lldb_version(version: &str) -> bool {
    version == "350"
}
