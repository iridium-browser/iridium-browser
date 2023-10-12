# Nearby Rust

## Folder Structure

Root repo of the nearby Rust components, folder structure:

`/connections` nearby connections rust components

`/crypto` shared crypto components

`/presence` nearby presence rust components

## Setup

### Toolchain

If you don't already have a Rust toolchain, see [rustup.rs](https://rustup.rs/).

### Cargo

Install [`cargo-deny`](https://github.com/EmbarkStudios/cargo-deny)
and [`cargo-fuzz`](https://github.com/rust-fuzz/cargo-fuzz):

```
cargo install --locked cargo-deny
cargo install cargo-fuzz
```

### Setting up a Docker dev environment

Our project requires specific versions of system dependencies like OpenSSL and
protobuf in order to build correctly. To make the setup of this easier you can
use Docker to handle setting up the environment in a container.

First install Docker then build and run the image:

```
sudo docker build -t nearby_rust:v1.0 ..
sudo docker run --rm -it nearby_rust:v1.0
```

Building the image creates a snapshot of the environment that has all of the
system dependencies needed to start building and running all of the artifacts in
the codebase.

Running the image runs `cargo run -- check-everything` to verify all of the targets can
successfully build and all of the tests pass in your new container environment.

To open a bash shell from the container environment:

```
sudo docker run -it nearby_rust:v1.0 bash
```

You can also setup VSCode
to [develop in a Docker container on a remote host](https://code.visualstudio.com/remote/advancedcontainers/develop-remote-host)
that way you can make code changes and test them in the same environment without
having to re-build the image.

## Common tasks

See `cargo run -- --help` for all the available subcommands.

Check everything:

```
cargo run -- check-everything
```

Build everything:

```
cargo build --workspace --all-targets
```

Run tests:

```
cargo test --workspace
```

Generate Docs:

```
cargo doc --no-deps --workspace --open
```

Run linter on dependencies as configured in `deny.toml` <br>
This will make sure all of our dependencies are using valid licenses and check
for known existing security
vulnerabilities, bugs and deprecated versions

```
cargo deny --workspace check
```

Update dependencies in `Cargo.lock` to their latest in the currently specified
version ranges (i.e. transitive deps):

```
cargo update
```

Check for outdated dependencies
with [cargo-outdated](https://github.com/kbknapp/cargo-outdated):

```
cargo outdated
```

## Benchmarks

Benchmarks are in `benches`, and use
[Criterion](https://bheisler.github.io/criterion.rs/book/getting_started.html) .

```
cargo bench --workspace
```
