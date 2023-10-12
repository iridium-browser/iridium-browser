compiletest-rs
==============

This project is an attempt at extracting [the `compiletest` utility][upstream]
from the Rust compiler.

The `compiletest` utility is useful for library and plugin developers, who want
to include test programs that should fail to compile, issue warnings or
otherwise produce compile-time output.

To use in your project
----------------------
To use `compiletest-rs` in your application, add the following to `Cargo.toml`

```toml
[dev-dependencies]
compiletest_rs = "0.9"
```

By default, `compiletest-rs` should be able to run on both stable, beta and
nightly channels of rust. We use the [`tester`][tester] fork of Rust's builtin
`test` crate, so that we don't have require nightly. If you are running nightly
and want to use Rust's `test` crate directly, you need to have the rustc development
libraries install (which you can get by running `rustup component add rustc-dev
--toolchain nightly`). Once you have the rustc development libraries installed, you
can use the `rustc` feature to make compiletest use them instead of the `tester`
crate.

```toml
[dev-dependencies]
compiletest_rs = { version = "0.9", features = [ "rustc" ] }
```

Create a `tests` folder in the root folder of your project. Create a test file
with something like the following:

```rust
extern crate compiletest_rs as compiletest;

use std::path::PathBuf;

fn run_mode(mode: &'static str) {
    let mut config = compiletest::Config::default();

    config.mode = mode.parse().expect("Invalid mode");
    config.src_base = PathBuf::from(format!("tests/{}", mode));
    config.link_deps(); // Populate config.target_rustcflags with dependencies on the path
    config.clean_rmeta(); // If your tests import the parent crate, this helps with E0464

    compiletest::run_tests(&config);
}

#[test]
fn compile_test() {
    run_mode("compile-fail");
    run_mode("run-pass");
}

```

Each mode corresponds to a folder with the same name in the `tests` folder. That
is for the `compile-fail` mode the test runner looks for the
`tests/compile-fail` folder.

Adding flags to the Rust compiler is a matter of assigning the correct field in
the config. The most common flag to populate is the
`target_rustcflags` to include the link dependencies on the path.

```rust
// NOTE! This is the manual way of adding flags
config.target_rustcflags = Some("-L target/debug".to_string());
```

This is useful (and necessary) for library development. Note that other
secondary library dependencies may have their build artifacts placed in
different (non-obvious) locations and these locations must also be
added.

For convenience, `Config` provides a `link_deps()` method that
populates `target_rustcflags` with all the dependencies found in the
`PATH` variable (which is OS specific). For most cases, it should be
sufficient to do:

```rust
let mut config = compiletest::Config::default();
config.link_deps();
```

Note that `link_deps()` should not be used if any of the added paths contain
spaces, as these are currently not handled correctly.

Example
-------
See the `test-project` folder for a complete working example using the
`compiletest-rs` utility. Simply `cd test-project` and `cargo test` to see the
tests run. The annotation syntax is documented in the [rustc-guide][tests].

TODO
----
 - The `run-pass` mode is strictly not necessary since it's baked right into
   Cargo, but I haven't bothered to take it out

Contributing
------------

Thank you for your interest in improving this utility! Please consider
submitting your patch to [the upstream source][src] instead, as it will
be incorporated into this repo in due time. Still, there are some supporting
files that are specific to this repo, for example:

- src/lib.rs
- src/uidiff.rs
- test-project/

If you are unsure, open a pull request anyway and we would be glad to help!


[upstream]: https://github.com/rust-lang/rust/tree/master/src/tools/compiletest
[src]: https://github.com/rust-lang/rust/tree/master/src/tools/compiletest/src
[tests]: https://rustc-dev-guide.rust-lang.org/tests/adding.html#header-commands-configuring-rustc
[tester]: https://crates.io/crates/tester
