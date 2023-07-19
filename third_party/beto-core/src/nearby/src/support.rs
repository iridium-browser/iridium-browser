// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use anyhow::{anyhow, Context as _};
use owo_colors::OwoColorize as _;
use std::{collections, env, ffi, io, io::BufRead, path, process, thread};

pub fn run_cmd_shell(
    dir: &path::Path,
    cmd: impl AsRef<ffi::OsStr>,
) -> anyhow::Result<SuccessOutput> {
    run_cmd_shell_with_color::<DefaultColors>(dir, cmd)
}

/// Run a shell command using shell arg parsing.
///
/// Removes all `*CARGO*` and `*RUSTUP*` env vars in case this was run with
/// `cargo run`. If they are left in, they confuse nested `cargo` invocations.
///
/// Return Ok if the process completed normally.
pub fn run_cmd_shell_with_color<C: TermColors>(
    dir: &path::Path,
    cmd: impl AsRef<ffi::OsStr>,
) -> anyhow::Result<SuccessOutput> {
    run::<C>(dir, process::Command::new("sh").current_dir(dir).args(["-c".as_ref(), cmd.as_ref()]))
}

/// Run a cmd with explicit args directly without a shell.
///
/// Removes all `*CARGO*` and `*RUSTUP*` env vars in case this was run with
/// `cargo run`.
///
/// Return Ok if the process completed normally.
#[allow(dead_code)]
pub fn run_cmd<C: TermColors, P, A, S>(
    dir: &path::Path,
    cmd: &P,
    args: A,
) -> anyhow::Result<SuccessOutput>
where
    P: AsRef<path::Path> + ?Sized,
    A: Clone + IntoIterator<Item = S>,
    S: AsRef<ffi::OsStr>,
{
    run::<C>(dir, process::Command::new(cmd.as_ref()).current_dir(dir).args(args))
}

/// Run the specified command.
///
/// `cmd_with_args` is used
fn run<C: TermColors>(
    dir: &path::Path,
    command: &mut process::Command,
) -> Result<SuccessOutput, anyhow::Error> {
    // approximately human readable version of the invocation for logging
    let cmd_with_args = command.get_args().fold(
        command.get_program().to_os_string(),
        |mut acc: ffi::OsString, s| {
            acc.push(" ");
            acc.push(shell_escape::escape(s.to_string_lossy()).as_ref());
            acc
        },
    );

    let context = format!("{} [{}]", cmd_with_args.to_string_lossy(), dir.to_string_lossy(),);
    println!("[{}] [{}]", cmd_with_args.to_string_lossy().green(), dir.to_string_lossy().blue());

    let mut child = command
        .env_clear()
        .envs(modified_cmd_env())
        .stdin(process::Stdio::null())
        .stdout(process::Stdio::piped())
        .stderr(process::Stdio::piped())
        .spawn()
        .context(context.clone())?;

    // If thread creation overhead becomes a problem, we could always use a shared context
    // that holds on to some channels.
    let stdout_thread = spawn_print_thread::<C::StdoutColor, _, _>(
        child.stdout.take().expect("stdout must be present"),
        io::stdout(),
    );
    let stderr_thread = spawn_print_thread::<C::StderrColor, _, _>(
        child.stderr.take().expect("stderr must be present"),
        io::stderr(),
    );

    let status = child.wait()?;

    let stdout = stdout_thread.join().expect("stdout thread panicked");
    stderr_thread.join().expect("stderr thread panicked");

    match status.code() {
        None => {
            eprintln!("Process terminated by signal");
            Err(anyhow!("Process terminated by signal"))
        }
        Some(0) => Ok(SuccessOutput { stdout }),
        Some(n) => {
            eprintln!("Exit code: {n}");
            Err(anyhow!("Exit code: {n}"))
        }
    }
    .context(context)
}

pub struct SuccessOutput {
    stdout: String,
}

impl SuccessOutput {
    pub fn stdout(&self) -> &str {
        &self.stdout
    }
}

/// Returns modified env vars that are suitable for use in child invocations.
fn modified_cmd_env() -> collections::HashMap<String, String> {
    env::vars()
        // Filter out `*CARGO*` or `*RUSTUP*` vars as those will confuse child invocations of `cargo`.
        .filter(|(k, _)| !(k.contains("CARGO") || k.contains("RUSTUP")))
        // We want colors in our cargo invocations
        .chain([(String::from("CARGO_TERM_COLOR"), String::from("always"))])
        .collect()
}

/// Trait for specifying the terminal text colors of the command output.
pub trait TermColors {
    /// Color for stdout. Use `owo_colors::colors::Default` to keep color codes from the command.
    type StdoutColor: owo_colors::Color;
    /// Color for stderr. Use `owo_colors::colors::Default` to keep color codes from the command.
    type StderrColor: owo_colors::Color;
}

/// Override only the stderr color to yellow.
#[non_exhaustive]
pub struct YellowStderr;

impl TermColors for YellowStderr {
    type StdoutColor = owo_colors::colors::Default;
    type StderrColor = owo_colors::colors::Yellow;
}

/// Keep the default colors from the command output. Typically used with `--color=always` or
/// equivalent env vars like `CARGO_TERM_COLOR` to keep the colors even though the output is not a
/// tty.
#[non_exhaustive]
pub struct DefaultColors;
impl TermColors for DefaultColors {
    type StdoutColor = owo_colors::colors::Default;
    type StderrColor = owo_colors::colors::Default;
}

/// Spawn a thread that will print any lines read from the input using the specified color on
/// the provided writer (intended to be `stdin`/`stdout`.
///
/// The thread accumulates all output lines and returns it
fn spawn_print_thread<C, R, W>(input: R, mut output: W) -> thread::JoinHandle<String>
where
    C: owo_colors::Color,
    R: io::Read + Send + 'static,
    W: io::Write + Send + 'static,
{
    thread::spawn(move || {
        let mut line = String::new();
        let mut all_output = String::new();
        let mut buf_read = io::BufReader::new(input);

        loop {
            line.clear();
            match buf_read.read_line(&mut line) {
                Ok(0) => break,
                Ok(_) => {
                    all_output.push_str(&line);
                    write!(output, "{}", line.fg::<C>()).expect("write to stdio failed");
                }
                // TODO do something smarter for non-UTF8 output
                Err(e) => eprintln!("{}: {:?}", "Could not read line".red(), e),
            }
        }

        all_output
    })
}
