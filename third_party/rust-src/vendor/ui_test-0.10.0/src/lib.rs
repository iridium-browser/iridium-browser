#![allow(
    clippy::enum_variant_names,
    clippy::useless_format,
    clippy::too_many_arguments,
    rustc::internal
)]
#![deny(missing_docs)]

//! A crate to run the Rust compiler (or other binaries) and test their command line output.

use bstr::ByteSlice;
pub use color_eyre;
use color_eyre::eyre::{eyre, Result};
use crossbeam_channel::unbounded;
use parser::{ErrorMatch, Pattern, Revisioned};
use regex::bytes::Regex;
use rustc_stderr::{Diagnostics, Level, Message};
use status_emitter::StatusEmitter;
use std::borrow::Cow;
use std::collections::{HashSet, VecDeque};
use std::ffi::OsString;
use std::fmt::Display;
use std::num::NonZeroUsize;
use std::path::{Path, PathBuf};
use std::process::{Command, ExitStatus};
use std::thread;

use crate::dependencies::build_dependencies;
use crate::parser::{Comments, Condition};

mod dependencies;
mod diff;
pub mod github_actions;
mod parser;
mod rustc_stderr;
pub mod status_emitter;
#[cfg(test)]
mod tests;

#[derive(Debug, Clone)]
/// Central datastructure containing all information to run the tests.
pub struct Config {
    /// Arguments passed to the binary that is executed.
    /// These arguments are passed *after* the args inserted via `//@compile-flags:`.
    pub trailing_args: Vec<OsString>,
    /// Host triple; usually will be auto-detected.
    pub host: Option<String>,
    /// `None` to run on the host, otherwise a target triple
    pub target: Option<String>,
    /// Filters applied to stderr output before processing it.
    /// By default contains a filter for replacing backslashes with regular slashes.
    /// On windows, contains a filter to replace `\n` with `\r\n`.
    pub stderr_filters: Filter,
    /// Filters applied to stdout output before processing it.
    /// On windows, contains a filter to replace `\n` with `\r\n`.
    pub stdout_filters: Filter,
    /// The folder in which to start searching for .rs files
    pub root_dir: PathBuf,
    /// The mode in which to run the tests.
    pub mode: Mode,
    /// The binary to actually execute.
    pub program: CommandBuilder,
    /// The command to run to obtain the cfgs that the output is supposed to
    pub cfgs: CommandBuilder,
    /// What to do in case the stdout/stderr output differs from the expected one.
    pub output_conflict_handling: OutputConflictHandling,
    /// Only run tests with one of these strings in their path/name
    pub path_filter: Vec<String>,
    /// Path to a `Cargo.toml` that describes which dependencies the tests can access.
    pub dependencies_crate_manifest_path: Option<PathBuf>,
    /// The command to run can be changed from `cargo` to any custom command to build the
    /// dependencies in `dependencies_crate_manifest_path`
    pub dependency_builder: CommandBuilder,
    /// Print one character per test instead of one line
    pub quiet: bool,
    /// How many threads to use for running tests. Defaults to number of cores
    pub num_test_threads: NonZeroUsize,
    /// Where to dump files like the binaries compiled from tests.
    pub out_dir: Option<PathBuf>,
    /// The default edition to use on all tests
    pub edition: Option<String>,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            trailing_args: vec![],
            host: None,
            target: None,
            stderr_filters: vec![
                (Match::Exact(vec![b'\\']), b"/"),
                #[cfg(windows)]
                (Match::Exact(vec![b'\r']), b""),
            ],
            stdout_filters: vec![
                #[cfg(windows)]
                (Match::Exact(vec![b'\r']), b""),
            ],
            root_dir: PathBuf::new(),
            mode: Mode::Fail {
                require_patterns: true,
            },
            program: CommandBuilder::rustc(),
            cfgs: CommandBuilder::cfgs(),
            output_conflict_handling: OutputConflictHandling::Error,
            path_filter: vec![],
            dependencies_crate_manifest_path: None,
            dependency_builder: CommandBuilder::cargo(),
            quiet: false,
            num_test_threads: std::thread::available_parallelism().unwrap(),
            out_dir: None,
            edition: Some("2021".into()),
        }
    }
}

impl Config {
    /// Replace all occurrences of a path in stderr with a byte string.
    pub fn path_stderr_filter(
        &mut self,
        path: &Path,
        replacement: &'static (impl AsRef<[u8]> + ?Sized),
    ) {
        let pattern = path.canonicalize().unwrap();
        self.stderr_filters
            .push((pattern.parent().unwrap().into(), replacement.as_ref()));
    }

    /// Replace all occurrences of a regex pattern in stderr with a byte string.
    pub fn stderr_filter(
        &mut self,
        pattern: &str,
        replacement: &'static (impl AsRef<[u8]> + ?Sized),
    ) {
        self.stderr_filters
            .push((Regex::new(pattern).unwrap().into(), replacement.as_ref()));
    }

    /// Replace all occurrences of a regex pattern in stdout with a byte string.
    pub fn stdout_filter(
        &mut self,
        pattern: &str,
        replacement: &'static (impl AsRef<[u8]> + ?Sized),
    ) {
        self.stdout_filters
            .push((Regex::new(pattern).unwrap().into(), replacement.as_ref()));
    }

    fn build_dependencies_and_link_them(&mut self) -> Result<()> {
        let dependencies = build_dependencies(self)?;
        for (name, artifacts) in dependencies.dependencies {
            for dependency in artifacts {
                self.program.args.push("--extern".into());
                let mut dep = OsString::from(&name);
                dep.push("=");
                dep.push(dependency);
                self.program.args.push(dep);
            }
        }
        for import_path in dependencies.import_paths {
            self.program.args.push("-L".into());
            self.program.args.push(import_path.into());
        }
        Ok(())
    }

    /// Make sure we have the host and target triples.
    pub fn fill_host_and_target(&mut self) -> Result<()> {
        if self.host.is_none() {
            self.host = Some(
                rustc_version::VersionMeta::for_command(std::process::Command::new(
                    &self.program.program,
                ))
                .map_err(|err| {
                    color_eyre::eyre::Report::new(err).wrap_err(format!(
                        "failed to parse rustc version info: {}",
                        self.program.display()
                    ))
                })?
                .host,
            );
        }
        if self.target.is_none() {
            self.target = Some(self.host.clone().unwrap());
        }
        Ok(())
    }

    fn has_asm_support(&self) -> bool {
        static ASM_SUPPORTED_ARCHS: &[&str] = &[
            "x86", "x86_64", "arm", "aarch64", "riscv32",
            "riscv64",
            // These targets require an additional asm_experimental_arch feature.
            // "nvptx64", "hexagon", "mips", "mips64", "spirv", "wasm32",
        ];
        ASM_SUPPORTED_ARCHS
            .iter()
            .any(|arch| self.target.as_ref().unwrap().contains(arch))
    }
}

#[derive(Debug, Clone)]
/// A command, its args and its environment. Used for
/// the main command, the dependency builder and the cfg-reader.
pub struct CommandBuilder {
    /// Path to the binary.
    pub program: PathBuf,
    /// Arguments to the binary.
    pub args: Vec<OsString>,
    /// Environment variables passed to the binary that is executed.
    /// The environment variable is removed if the second tuple field is `None`
    pub envs: Vec<(OsString, Option<OsString>)>,
}

impl CommandBuilder {
    /// Uses the `CARGO` env var or just a program named `cargo` and the argument `build`.
    pub fn cargo() -> Self {
        Self {
            program: PathBuf::from(std::env::var_os("CARGO").unwrap_or_else(|| "cargo".into())),
            args: vec!["build".into()],
            envs: vec![],
        }
    }

    /// Uses the `RUSTC` env var or just a program named `rustc` and the argument `--error-format=json`.
    ///
    /// Take care to only append unless you actually meant to overwrite the defaults.
    /// Overwriting the defaults may make `//~ ERROR` style comments stop working.
    pub fn rustc() -> Self {
        Self {
            program: PathBuf::from(std::env::var_os("RUSTC").unwrap_or_else(|| "rustc".into())),
            args: vec!["--error-format=json".into()],
            envs: vec![],
        }
    }

    /// Same as [`rustc`], but with arguments for obtaining the cfgs.
    pub fn cfgs() -> Self {
        Self {
            args: vec!["--print".into(), "cfg".into()],
            ..Self::rustc()
        }
    }

    /// Build a `CommandBuilder` for a command without any argumemnts.
    /// You can still add arguments later.
    pub fn cmd(cmd: impl Into<PathBuf>) -> Self {
        Self {
            program: cmd.into(),
            args: vec![],
            envs: vec![],
        }
    }

    /// Render the command like you'd use it on a command line.
    pub fn display(&self) -> impl std::fmt::Display + '_ {
        struct Display<'a>(&'a CommandBuilder);
        impl std::fmt::Display for Display<'_> {
            fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                self.0.program.display().fmt(f)?;
                for arg in &self.0.args {
                    write!(f, " {arg:?}")?;
                }
                Ok(())
            }
        }
        Display(self)
    }

    /// Create a command with the given settings.
    pub fn build(&self) -> Command {
        let mut cmd = Command::new(&self.program);
        cmd.args(self.args.iter());
        self.apply_env(&mut cmd);
        cmd
    }

    fn apply_env(&self, cmd: &mut Command) {
        for (var, val) in self.envs.iter() {
            if let Some(val) = val {
                cmd.env(var, val);
            } else {
                cmd.env_remove(var);
            }
        }
    }
}

#[derive(Debug, Copy, Clone)]
/// The different options for what to do when stdout/stderr files differ from the actual output.
pub enum OutputConflictHandling {
    /// The default: emit a diff of the expected/actual output.
    Error,
    /// Ignore mismatches in the stderr/stdout files.
    Ignore,
    /// Instead of erroring if the stderr/stdout differs from the expected
    /// automatically replace it with the found output (after applying filters).
    Bless,
}

/// A filter's match rule.
#[derive(Clone, Debug)]
pub enum Match {
    /// If the regex matches, the filter applies
    Regex(Regex),
    /// If the exact byte sequence is found, the filter applies
    Exact(Vec<u8>),
}
impl Match {
    fn replace_all<'a>(&self, text: &'a [u8], replacement: &[u8]) -> Cow<'a, [u8]> {
        match self {
            Match::Regex(regex) => regex.replace_all(text, replacement),
            Match::Exact(needle) => text.replace(needle, replacement).into(),
        }
    }
}

impl From<&'_ Path> for Match {
    fn from(v: &Path) -> Self {
        let mut v = v.display().to_string();
        // Normalize away windows canonicalized paths.
        if v.starts_with(r#"\\?\"#) {
            v.drain(0..4);
        }
        let mut v = v.into_bytes();
        // Normalize paths on windows to use slashes instead of backslashes,
        // So that paths are rendered the same on all systems.
        for c in &mut v {
            if *c == b'\\' {
                *c = b'/';
            }
        }
        Self::Exact(v)
    }
}

impl From<Regex> for Match {
    fn from(v: Regex) -> Self {
        Self::Regex(v)
    }
}

/// Replacements to apply to output files.
pub type Filter = Vec<(Match, &'static [u8])>;

/// Run all tests as described in the config argument.
pub fn run_tests(config: Config) -> Result<()> {
    eprintln!("   Compiler: {}", config.program.display());

    run_tests_generic(
        config,
        |path| path.extension().map(|ext| ext == "rs").unwrap_or(false),
        |_, _| None,
        status_emitter::TextAndGha,
    )
}

/// Create a command for running a single file, with the settings from the `config` argument.
/// Ignores various settings from `Config` that relate to finding test files.
pub fn test_command(mut config: Config, path: &Path) -> Result<Command> {
    config.build_dependencies_and_link_them()?;

    let comments =
        Comments::parse_file(path)?.map_err(|errors| color_eyre::eyre::eyre!("{errors:#?}"))?;
    let mut errors = vec![];
    let result = build_command(
        path,
        &config,
        "",
        &comments,
        config.out_dir.as_deref(),
        &mut errors,
    );
    assert!(errors.is_empty(), "{errors:#?}");
    Ok(result)
}

#[allow(clippy::large_enum_variant)]
/// The possible results a single test can have.
pub enum TestResult {
    /// The test passed
    Ok,
    /// The test was ignored due to a rule (`//@only-*` or `//@ignore-*`)
    Ignored,
    /// The test was filtered with the `file_filter` argument.
    Filtered,
    /// The test failed.
    Errored {
        /// Command that failed
        command: Command,
        /// The errors that were encountered.
        errors: Vec<Error>,
        /// The full stderr of the test run.
        stderr: Vec<u8>,
    },
}

struct TestRun {
    result: TestResult,
    path: PathBuf,
    revision: String,
}

/// A version of `run_tests` that allows more fine-grained control over running tests.
pub fn run_tests_generic(
    mut config: Config,
    file_filter: impl Fn(&Path) -> bool + Sync,
    per_file_config: impl Fn(&Config, &Path) -> Option<Config> + Sync,
    status_emitter: impl StatusEmitter,
) -> Result<()> {
    config.fill_host_and_target()?;

    config.build_dependencies_and_link_them()?;

    // A channel for files to process
    let (submit, receive) = unbounded();

    let mut results = vec![];

    thread::scope(|s| -> Result<()> {
        // Create a thread that is in charge of walking the directory and submitting jobs.
        // It closes the channel when it is done.
        s.spawn(|| {
            let mut todo = VecDeque::new();
            todo.push_back(config.root_dir.clone());
            while let Some(path) = todo.pop_front() {
                if path.is_dir() {
                    if path.file_name().unwrap() == "auxiliary" {
                        continue;
                    }
                    // Enqueue everything inside this directory.
                    // We want it sorted, to have some control over scheduling of slow tests.
                    let mut entries = std::fs::read_dir(path)
                        .unwrap()
                        .collect::<Result<Vec<_>, _>>()
                        .unwrap();
                    entries.sort_by_key(|e| e.file_name());
                    for entry in entries {
                        todo.push_back(entry.path());
                    }
                } else if file_filter(&path) {
                    // Forward .rs files to the test workers.
                    submit.send(path).unwrap();
                }
            }
            // There will be no more jobs. This signals the workers to quit.
            // (This also ensures `submit` is moved into this closure.)
            drop(submit);
        });

        // A channel for the messages emitted by the individual test threads.
        // Used to produce live updates while running the tests.
        let (finished_files_sender, finished_files_recv) = unbounded::<TestRun>();

        s.spawn(|| {
            let mut status_emitter = status_emitter.run_tests(&config);
            for run in finished_files_recv {
                status_emitter.test_result(&run.path, &run.revision, &run.result);

                results.push(run);
            }
        });

        let mut threads = vec![];

        // Create N worker threads that receive files to test.
        for _ in 0..config.num_test_threads.get() {
            let finished_files_sender = finished_files_sender.clone();
            threads.push(s.spawn(|| -> Result<()> {
                let finished_files_sender = finished_files_sender;
                for path in &receive {
                    let maybe_config;
                    let config = match per_file_config(&config, &path) {
                        None => &config,
                        Some(config) => {
                            maybe_config = config;
                            &maybe_config
                        }
                    };
                    let result =
                        match std::panic::catch_unwind(|| parse_and_test_file(&path, config)) {
                            Ok(res) => res,
                            Err(err) => {
                                finished_files_sender.send(TestRun {
                                    result: TestResult::Errored {
                                        command: Command::new("<unknown>"),
                                        errors: vec![Error::Bug(
                                            *Box::<dyn std::any::Any + Send + 'static>::downcast::<
                                                String,
                                            >(err)
                                            .unwrap(),
                                        )],
                                        stderr: vec![],
                                    },
                                    path,
                                    revision: String::new(),
                                })?;
                                continue;
                            }
                        };
                    for result in result {
                        finished_files_sender.send(result)?;
                    }
                }
                Ok(())
            }));
        }

        for thread in threads {
            thread.join().unwrap()?;
        }
        Ok(())
    })?;

    let mut failures = vec![];
    let mut succeeded = 0;
    let mut ignored = 0;
    let mut filtered = 0;

    for run in results {
        match run.result {
            TestResult::Ok => succeeded += 1,
            TestResult::Ignored => ignored += 1,
            TestResult::Filtered => filtered += 1,
            TestResult::Errored {
                command,
                errors,
                stderr,
            } => failures.push((run.path, command, run.revision, errors, stderr)),
        }
    }

    let mut failure_emitter = status_emitter.finalize(failures.len(), succeeded, ignored, filtered);
    for (path, command, revision, errors, stderr) in &failures {
        let _guard = status_emitter.failed_test(revision, path, command, stderr);
        failure_emitter.test_failure(path, revision, errors);
    }

    if failures.is_empty() {
        Ok(())
    } else {
        Err(eyre!("tests failed"))
    }
}

fn parse_and_test_file(path: &Path, config: &Config) -> Vec<TestRun> {
    if !config.path_filter.is_empty() {
        let path_display = path.display().to_string();
        if !config
            .path_filter
            .iter()
            .any(|filter| path_display.contains(filter))
        {
            return vec![TestRun {
                result: TestResult::Filtered,
                path: path.into(),
                revision: "".into(),
            }];
        }
    }
    let comments = match parse_comments_in_file(path) {
        Ok(comments) => comments,
        Err((stderr, errors)) => {
            return vec![TestRun {
                result: TestResult::Errored {
                    command: Command::new("parse comments"),
                    errors,
                    stderr,
                },
                path: path.into(),
                revision: "".into(),
            }]
        }
    };
    // Run the test for all revisions
    comments
        .revisions
        .clone()
        .unwrap_or_else(|| vec![String::new()])
        .into_iter()
        .map(|revision| {
            // Ignore file if only/ignore rules do (not) apply
            if !test_file_conditions(&comments, config, &revision) {
                return TestRun {
                    result: TestResult::Ignored,
                    path: path.into(),
                    revision,
                };
            }
            let (command, errors, stderr) = run_test(path, config, &revision, &comments);
            let result = if errors.is_empty() {
                TestResult::Ok
            } else {
                TestResult::Errored {
                    command,
                    errors,
                    stderr,
                }
            };
            TestRun {
                result,
                revision,
                path: path.into(),
            }
        })
        .collect()
}

fn parse_comments_in_file(path: &Path) -> Result<Comments, (Vec<u8>, Vec<Error>)> {
    match Comments::parse_file(path) {
        Ok(Ok(comments)) => Ok(comments),
        Ok(Err(errors)) => Err((vec![], errors)),
        Err(err) => Err((format!("{err:?}").into(), vec![])),
    }
}

/// All the ways in which a test can fail.
#[derive(Debug)]
pub enum Error {
    /// Got an invalid exit status for the given mode.
    ExitStatus {
        /// The expected mode.
        mode: Mode,
        /// The exit status of the command.
        status: ExitStatus,
        /// The expected exit status as set in the file or derived from the mode.
        expected: i32,
    },
    /// A pattern was declared but had no matching error.
    PatternNotFound {
        /// The pattern that was missing an error
        pattern: Pattern,
        /// The line in which the pattern was defined.
        definition_line: usize,
    },
    /// A ui test checking for failure does not have any failure patterns
    NoPatternsFound,
    /// A ui test checking for success has failure patterns
    PatternFoundInPassTest,
    /// Stderr/Stdout differed from the `.stderr`/`.stdout` file present.
    OutputDiffers {
        /// The file containing the expected output that differs from the actual output.
        path: PathBuf,
        /// The output from the command.
        actual: Vec<u8>,
        /// The contents of the file.
        expected: Vec<u8>,
    },
    /// There were errors that don't have a pattern.
    ErrorsWithoutPattern {
        /// The main message of the error.
        msgs: Vec<Message>,
        /// File and line information of the error.
        path: Option<(PathBuf, usize)>,
    },
    /// A comment failed to parse.
    InvalidComment {
        /// The comment
        msg: String,
        /// THe line in which it was defined.
        line: usize,
    },
    /// A subcommand (e.g. rustfix) of a test failed.
    Command {
        /// The name of the subcommand (e.g. "rustfix").
        kind: String,
        /// The exit status of the command.
        status: ExitStatus,
    },
    /// This catches crashes of ui tests and reports them along the failed test.
    Bug(String),
    /// An auxiliary build failed with its own set of errors.
    Aux {
        /// Path to the aux file.
        path: PathBuf,
        /// The errors that occurred during the build of the aux file.
        errors: Vec<Error>,
        /// The line in which the aux file was requested to be built.
        line: usize,
    },
}

type Errors = Vec<Error>;

fn build_command(
    path: &Path,
    config: &Config,
    revision: &str,
    comments: &Comments,
    out_dir: Option<&Path>,
    errors: &mut Vec<Error>,
) -> Command {
    let mut cmd = config.program.build();
    if let Some(out_dir) = out_dir {
        cmd.arg("--out-dir");
        cmd.arg(out_dir);
    }
    cmd.arg(path);
    if !revision.is_empty() {
        cmd.arg(format!("--cfg={revision}"));
    }
    for arg in comments
        .for_revision(revision)
        .flat_map(|r| r.compile_flags.iter())
    {
        cmd.arg(arg);
    }
    let edition = comments.edition(errors, revision, config);
    if let Some((edition, _)) = edition {
        cmd.arg("--edition").arg(edition);
    }
    cmd.args(config.trailing_args.iter());
    cmd.envs(
        comments
            .for_revision(revision)
            .flat_map(|r| r.env_vars.iter())
            .map(|(k, v)| (k, v)),
    );

    cmd
}

fn build_aux(
    aux_file: &Path,
    path: &Path,
    config: &Config,
    revision: &str,
    comments: &Comments,
    kind: &str,
    aux: &Path,
    extra_args: &mut Vec<String>,
) -> std::result::Result<(), (Command, Vec<Error>, Vec<u8>)> {
    let comments = match parse_comments_in_file(aux_file) {
        Ok(comments) => comments,
        Err((msg, mut errors)) => {
            return Err((
                build_command(path, config, revision, comments, None, &mut errors),
                errors,
                msg,
            ))
        }
    };
    assert_eq!(comments.revisions, None);

    // Put aux builds into a separate directory per test so that
    // tests running in parallel but building the same aux build don't conflict.
    // FIXME: put aux builds into the regular build queue.
    let out_dir = config
        .out_dir
        .clone()
        .unwrap_or_default()
        .join(path.with_extension(""));

    let mut errors = vec![];

    let mut aux_cmd = build_command(
        aux_file,
        config,
        revision,
        &comments,
        Some(&out_dir),
        &mut errors,
    );

    if !errors.is_empty() {
        return Err((aux_cmd, errors, vec![]));
    }

    let current_extra_args =
        build_aux_files(aux_file, aux_file.parent().unwrap(), &comments, "", config)?;
    // Make sure we see our dependencies
    aux_cmd.args(current_extra_args.iter());
    // Make sure our dependents also see our dependencies.
    extra_args.extend(current_extra_args);

    aux_cmd.arg("--crate-type").arg(kind);
    aux_cmd.arg("--emit=link");
    let filename = aux.file_stem().unwrap().to_str().unwrap();
    let output = aux_cmd.output().unwrap();
    if !output.status.success() {
        let error = Error::Command {
            kind: "compilation of aux build failed".to_string(),
            status: output.status,
        };
        return Err((
            aux_cmd,
            vec![error],
            rustc_stderr::process(path, &output.stderr).rendered,
        ));
    }

    // Now run the command again to fetch the output filenames
    aux_cmd.arg("--print").arg("file-names");
    let output = aux_cmd.output().unwrap();
    assert!(output.status.success());

    for file in output.stdout.lines() {
        let file = std::str::from_utf8(file).unwrap();
        let crate_name = filename.replace('-', "_");
        let path = out_dir.join(file);
        extra_args.push("--extern".into());
        extra_args.push(format!("{crate_name}={}", path.display()));
        // Help cargo find the crates added with `--extern`.
        extra_args.push("-L".into());
        extra_args.push(out_dir.display().to_string());
    }
    Ok(())
}

fn run_test(
    path: &Path,
    config: &Config,
    revision: &str,
    comments: &Comments,
) -> (Command, Errors, Vec<u8>) {
    let extra_args = match build_aux_files(
        path,
        &path.parent().unwrap().join("auxiliary"),
        comments,
        revision,
        config,
    ) {
        Ok(value) => value,
        Err(value) => return value,
    };

    let mut errors = vec![];

    let mut cmd = build_command(
        path,
        config,
        revision,
        comments,
        config.out_dir.as_deref(),
        &mut errors,
    );
    cmd.args(&extra_args);

    let output = cmd
        .output()
        .unwrap_or_else(|_| panic!("could not execute {cmd:?}"));
    let mode = config.mode.maybe_override(comments, revision, &mut errors);
    let status_check = mode.ok(output.status);
    if status_check.is_empty() && matches!(mode, Mode::Run { .. }) {
        let cmd = run_test_binary(mode, path, revision, comments, cmd, config, &mut errors);
        return (cmd, errors, vec![]);
    }
    errors.extend(status_check);
    if output.status.code() == Some(101) && !matches!(config.mode, Mode::Panic | Mode::Yolo) {
        let stderr = String::from_utf8_lossy(&output.stderr);
        let stdout = String::from_utf8_lossy(&output.stdout);
        errors.push(Error::Bug(format!(
            "test panicked: stderr:\n{stderr}\nstdout:\n{stdout}",
        )));
        return (cmd, errors, vec![]);
    }
    // Always remove annotation comments from stderr.
    let diagnostics = rustc_stderr::process(path, &output.stderr);
    let rustfixed = matches!(mode, Mode::Fix).then(|| {
        run_rustfix(
            &output.stderr,
            path,
            comments,
            revision,
            config,
            extra_args,
            &mut errors,
        )
    });
    let stderr = check_test_result(
        path,
        config,
        revision,
        comments,
        &mut errors,
        &output.stdout,
        diagnostics,
    );
    if let Some((mut rustfix, rustfix_path)) = rustfixed {
        // picking the crate name from the file name is problematic when `.revision_name` is inserted
        rustfix.arg("--crate-name").arg(
            path.file_stem()
                .unwrap()
                .to_str()
                .unwrap()
                .replace('-', "_"),
        );
        let output = rustfix.output().unwrap();
        if !output.status.success() {
            errors.push(Error::Command {
                kind: "rustfix".into(),
                status: output.status,
            });
            return (
                rustfix,
                errors,
                rustc_stderr::process(&rustfix_path, &output.stderr).rendered,
            );
        }
    }
    (cmd, errors, stderr)
}

fn build_aux_files(
    path: &Path,
    aux_dir: &Path,
    comments: &Comments,
    revision: &str,
    config: &Config,
) -> Result<Vec<String>, (Command, Vec<Error>, Vec<u8>)> {
    let mut extra_args = vec![];
    for rev in comments.for_revision(revision) {
        for (aux, kind, line) in &rev.aux_builds {
            let aux_file = if aux.starts_with("..") {
                aux_dir.parent().unwrap().join(aux)
            } else {
                aux_dir.join(aux)
            };
            if let Err((command, errors, msg)) = build_aux(
                &aux_file,
                path,
                config,
                revision,
                comments,
                kind,
                aux,
                &mut extra_args,
            ) {
                return Err((
                    command,
                    vec![Error::Aux {
                        path: aux_file,
                        errors,
                        line: *line,
                    }],
                    msg,
                ));
            }
        }
    }
    Ok(extra_args)
}

fn run_test_binary(
    mode: Mode,
    path: &Path,
    revision: &str,
    comments: &Comments,
    mut cmd: Command,
    config: &Config,
    errors: &mut Vec<Error>,
) -> Command {
    let out_dir = config.out_dir.as_deref();
    cmd.arg("--print").arg("file-names");
    let output = cmd.output().unwrap();
    assert!(output.status.success());

    let mut files = output.stdout.lines();
    let file = files.next().unwrap();
    assert_eq!(files.next(), None);
    let file = std::str::from_utf8(file).unwrap();
    let exe = match out_dir {
        None => PathBuf::from(file),
        Some(path) => path.join(file),
    };
    let mut exe = Command::new(exe);
    let output = exe.output().unwrap();

    check_test_output(
        path,
        errors,
        revision,
        config,
        comments,
        &output.stdout,
        &output.stderr,
    );

    errors.extend(mode.ok(output.status));

    exe
}

fn run_rustfix(
    stderr: &[u8],
    path: &Path,
    comments: &Comments,
    revision: &str,
    config: &Config,
    extra_args: Vec<String>,
    errors: &mut Vec<Error>,
) -> (Command, PathBuf) {
    let input = std::str::from_utf8(stderr).unwrap();
    let suggestions = rustfix::get_suggestions_from_json(
        input,
        &HashSet::new(),
        if let Mode::Yolo = config.mode {
            rustfix::Filter::Everything
        } else {
            rustfix::Filter::MachineApplicableOnly
        },
    )
    .unwrap_or_else(|err| {
        panic!("could not deserialize diagnostics json for rustfix {err}:{input}")
    });
    let fixed_code =
        rustfix::apply_suggestions(&std::fs::read_to_string(path).unwrap(), &suggestions)
            .unwrap_or_else(|e| {
                panic!(
                    "failed to apply suggestions for {:?} with rustfix: {e}",
                    path.display()
                )
            });
    let edition = comments.edition(errors, revision, config);
    let rustfix_comments = Comments {
        revisions: None,
        revisioned: std::iter::once((
            vec![],
            Revisioned {
                ignore: vec![],
                only: vec![],
                stderr_per_bitwidth: false,
                compile_flags: comments
                    .for_revision(revision)
                    .flat_map(|r| r.compile_flags.iter().cloned())
                    .collect(),
                env_vars: comments
                    .for_revision(revision)
                    .flat_map(|r| r.env_vars.iter().cloned())
                    .collect(),
                normalize_stderr: vec![],
                error_in_other_files: vec![],
                error_matches: vec![],
                require_annotations_for_level: None,
                aux_builds: comments
                    .for_revision(revision)
                    .flat_map(|r| r.aux_builds.iter().cloned())
                    .collect(),
                edition,
                mode: Some((Mode::Pass, 0)),
                needs_asm_support: false,
            },
        ))
        .collect(),
    };
    let path = check_output(
        fixed_code.as_bytes(),
        path,
        errors,
        revised(revision, "fixed"),
        &Filter::default(),
        config,
        &rustfix_comments,
        revision,
    );

    let mut cmd = build_command(
        &path,
        config,
        revision,
        &rustfix_comments,
        config.out_dir.as_deref(),
        errors,
    );
    cmd.args(extra_args);
    (cmd, path)
}

fn revised(revision: &str, extension: &str) -> String {
    if revision.is_empty() {
        extension.to_string()
    } else {
        format!("{revision}.{extension}")
    }
}

fn check_test_result(
    path: &Path,
    config: &Config,
    revision: &str,
    comments: &Comments,
    errors: &mut Errors,
    stdout: &[u8],
    diagnostics: Diagnostics,
) -> Vec<u8> {
    check_test_output(
        path,
        errors,
        revision,
        config,
        comments,
        stdout,
        &diagnostics.rendered,
    );
    // Check error annotations in the source against output
    check_annotations(
        diagnostics.messages,
        diagnostics.messages_from_unknown_file_or_line,
        path,
        errors,
        config,
        revision,
        comments,
    );
    diagnostics.rendered
}

fn check_test_output(
    path: &Path,
    errors: &mut Vec<Error>,
    revision: &str,
    config: &Config,
    comments: &Comments,
    stdout: &[u8],
    stderr: &[u8],
) {
    // Check output files (if any)
    // Check output files against actual output
    check_output(
        stderr,
        path,
        errors,
        revised(revision, "stderr"),
        &config.stderr_filters,
        config,
        comments,
        revision,
    );
    check_output(
        stdout,
        path,
        errors,
        revised(revision, "stdout"),
        &config.stdout_filters,
        config,
        comments,
        revision,
    );
}

fn check_annotations(
    mut messages: Vec<Vec<Message>>,
    mut messages_from_unknown_file_or_line: Vec<Message>,
    path: &Path,
    errors: &mut Errors,
    config: &Config,
    revision: &str,
    comments: &Comments,
) {
    let error_patterns = comments
        .for_revision(revision)
        .flat_map(|r| r.error_in_other_files.iter());

    let mut seen_error_match = false;
    for (error_pattern, definition_line) in error_patterns {
        seen_error_match = true;
        // first check the diagnostics messages outside of our file. We check this first, so that
        // you can mix in-file annotations with //@error-in-other-file annotations, even if there is overlap
        // in the messages.
        if let Some(i) = messages_from_unknown_file_or_line
            .iter()
            .position(|msg| error_pattern.matches(&msg.message))
        {
            messages_from_unknown_file_or_line.remove(i);
        } else {
            errors.push(Error::PatternNotFound {
                pattern: error_pattern.clone(),
                definition_line: *definition_line,
            });
        }
    }

    // The order on `Level` is such that `Error` is the highest level.
    // We will ensure that *all* diagnostics of level at least `lowest_annotation_level`
    // are matched.
    let mut lowest_annotation_level = Level::Error;
    for &ErrorMatch {
        ref pattern,
        definition_line,
        line,
        level,
    } in comments
        .for_revision(revision)
        .flat_map(|r| r.error_matches.iter())
    {
        seen_error_match = true;
        // If we found a diagnostic with a level annotation, make sure that all
        // diagnostics of that level have annotations, even if we don't end up finding a matching diagnostic
        // for this pattern.
        lowest_annotation_level = std::cmp::min(lowest_annotation_level, level);

        if let Some(msgs) = messages.get_mut(line) {
            let found = msgs
                .iter()
                .position(|msg| pattern.matches(&msg.message) && msg.level == level);
            if let Some(found) = found {
                msgs.remove(found);
                continue;
            }
        }

        errors.push(Error::PatternNotFound {
            pattern: pattern.clone(),
            definition_line,
        });
    }

    let required_annotation_level = comments
        .find_one_for_revision(
            revision,
            |r| r.require_annotations_for_level,
            |_| {
                errors.push(Error::InvalidComment {
                    msg: "`require_annotations_for_level` specified twice for same revision".into(),
                    line: 0,
                })
            },
        )
        .unwrap_or(lowest_annotation_level);
    let filter = |mut msgs: Vec<Message>| -> Vec<_> {
        msgs.retain(|msg| msg.level >= required_annotation_level);
        msgs
    };

    let mode = config.mode.maybe_override(comments, revision, errors);

    if !matches!(config.mode, Mode::Yolo) {
        let messages_from_unknown_file_or_line = filter(messages_from_unknown_file_or_line);
        if !messages_from_unknown_file_or_line.is_empty() {
            errors.push(Error::ErrorsWithoutPattern {
                path: None,
                msgs: messages_from_unknown_file_or_line,
            });
        }

        for (line, msgs) in messages.into_iter().enumerate() {
            let msgs = filter(msgs);
            if !msgs.is_empty() {
                errors.push(Error::ErrorsWithoutPattern {
                    path: Some((path.to_path_buf(), line)),
                    msgs,
                });
            }
        }
    }

    match (mode, seen_error_match) {
        (Mode::Pass, true) | (Mode::Panic, true) => errors.push(Error::PatternFoundInPassTest),
        (
            Mode::Fail {
                require_patterns: true,
            },
            false,
        ) => errors.push(Error::NoPatternsFound),
        _ => {}
    }
}

fn check_output(
    output: &[u8],
    path: &Path,
    errors: &mut Errors,
    kind: String,
    filters: &Filter,
    config: &Config,
    comments: &Comments,
    revision: &str,
) -> PathBuf {
    let target = config.target.as_ref().unwrap();
    let output = normalize(path, output, filters, comments, revision);
    let path = output_path(path, comments, kind, target, revision);
    match config.output_conflict_handling {
        OutputConflictHandling::Bless => {
            if output.is_empty() {
                let _ = std::fs::remove_file(&path);
            } else {
                std::fs::write(&path, &output).unwrap();
            }
        }
        OutputConflictHandling::Error => {
            let expected_output = std::fs::read(&path).unwrap_or_default();
            if output != expected_output {
                errors.push(Error::OutputDiffers {
                    path: path.clone(),
                    actual: output,
                    expected: expected_output,
                });
            }
        }
        OutputConflictHandling::Ignore => {}
    }
    path
}

fn output_path(
    path: &Path,
    comments: &Comments,
    kind: String,
    target: &str,
    revision: &str,
) -> PathBuf {
    if comments
        .for_revision(revision)
        .any(|r| r.stderr_per_bitwidth)
    {
        return path.with_extension(format!("{}bit.{kind}", get_pointer_width(target)));
    }
    path.with_extension(kind)
}

fn test_condition(condition: &Condition, config: &Config) -> bool {
    let target = config.target.as_ref().unwrap();
    match condition {
        Condition::Bitwidth(bits) => get_pointer_width(target) == *bits,
        Condition::Target(t) => target.contains(t),
        Condition::Host(t) => config.host.as_ref().unwrap().contains(t),
        Condition::OnHost => target == config.host.as_ref().unwrap(),
    }
}

/// Returns whether according to the in-file conditions, this file should be run.
fn test_file_conditions(comments: &Comments, config: &Config, revision: &str) -> bool {
    if comments
        .for_revision(revision)
        .flat_map(|r| r.ignore.iter())
        .any(|c| test_condition(c, config))
    {
        return false;
    }
    if comments
        .for_revision(revision)
        .any(|r| r.needs_asm_support && !config.has_asm_support())
    {
        return false;
    }
    comments
        .for_revision(revision)
        .flat_map(|r| r.only.iter())
        .all(|c| test_condition(c, config))
}

// Taken 1:1 from compiletest-rs
fn get_pointer_width(triple: &str) -> u8 {
    if (triple.contains("64") && !triple.ends_with("gnux32") && !triple.ends_with("gnu_ilp32"))
        || triple.starts_with("s390x")
    {
        64
    } else if triple.starts_with("avr") {
        16
    } else {
        32
    }
}

fn normalize(
    path: &Path,
    text: &[u8],
    filters: &Filter,
    comments: &Comments,
    revision: &str,
) -> Vec<u8> {
    // Useless paths
    let path_filter = (Match::from(path.parent().unwrap()), b"$DIR" as &[u8]);
    let filters = filters.iter().chain(std::iter::once(&path_filter));
    let mut text = text.to_owned();
    if let Some(lib_path) = option_env!("RUSTC_LIB_PATH") {
        text = text.replace(lib_path, "RUSTLIB");
    }

    for (regex, replacement) in filters {
        text = regex.replace_all(&text, replacement).into_owned();
    }

    for (from, to) in comments
        .for_revision(revision)
        .flat_map(|r| r.normalize_stderr.iter())
    {
        text = from.replace_all(&text, to).into_owned();
    }
    text
}

#[derive(Copy, Clone, Debug)]
/// Decides what is expected of each test's exit status.
pub enum Mode {
    /// The test fails with an error, but passes after running rustfix
    Fix,
    /// The test passes a full execution of the rustc driver
    Pass,
    /// The test produces an executable binary that can get executed on the host
    Run {
        /// The expected exit code
        exit_code: i32,
    },
    /// The rustc driver panicked
    Panic,
    /// The rustc driver emitted an error
    Fail {
        /// Whether failing tests must have error patterns. Set to false if you just care about .stderr output.
        require_patterns: bool,
    },
    /// Run the tests, but always pass them as long as all annotations are satisfied and stderr files match.
    Yolo,
}

impl Mode {
    fn ok(self, status: ExitStatus) -> Errors {
        let expected = match self {
            Mode::Run { exit_code } => exit_code,
            Mode::Pass => 0,
            Mode::Panic => 101,
            Mode::Fail { .. } => 1,
            Mode::Fix | Mode::Yolo => return vec![],
        };
        if status.code() == Some(expected) {
            vec![]
        } else {
            vec![Error::ExitStatus {
                mode: self,
                status,
                expected,
            }]
        }
    }
    fn maybe_override(self, comments: &Comments, revision: &str, errors: &mut Vec<Error>) -> Self {
        comments
            .find_one_for_revision(
                revision,
                |r| r.mode.as_ref(),
                |&(_, line)| {
                    errors.push(Error::InvalidComment {
                        msg: "multiple mode changes found".into(),
                        line,
                    })
                },
            )
            .map(|&(mode, _)| mode)
            .unwrap_or(self)
    }
}

impl Display for Mode {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Mode::Run { exit_code } => write!(f, "run({exit_code})"),
            Mode::Pass => write!(f, "pass"),
            Mode::Panic => write!(f, "panic"),
            Mode::Fail {
                require_patterns: _,
            } => write!(f, "fail"),
            Mode::Yolo => write!(f, "yolo"),
            Mode::Fix => write!(f, "fix"),
        }
    }
}
