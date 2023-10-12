# libffi-sys-rs: Low-level Rust bindings for [libffi]

[![GitHub Workflow Status](https://img.shields.io/github/workflow/status/tov/libffi-rs/Build%20&%20Test)](https://github.com/tov/libffi-rs/actions)
[![Documentation](https://img.shields.io/docsrs/libffi-sys/latest)](https://docs.rs/libffi-sys/latest/libffi_sys/)
[![Crates.io](https://img.shields.io/crates/v/libffi-sys.svg?maxAge=2592000)](https://crates.io/crates/libffi-sys)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE-MIT)
[![License: Apache 2.0](https://img.shields.io/badge/license-Apache_2.0-blue.svg)](LICENSE-APACHE)

The C libffi library provides two main facilities: assembling calls
to functions dynamically, and creating closures that can be called
as ordinary C functions. This is an undocumented wrapper, generated
by bindgen, intended as the basis for higher-level bindings.

If you clone this repository in order to build the library and you do
not plan to enable the `system` Cargo feature to build against your
system’s C libffi, then you should do a recursive clone, by default this
library builds C libffi from a Git submodule.

See [the `libffi` crate] for a higher-level API.

## Usage

`libffi-sys` can either build its own copy of the libffi C library [from
github][libffi github] or it can link against your
system’s C libffi. By default it builds its own because many systems
ship with an old C libffi; this requires that you have a working make,
C compiler, automake, and autoconf first. If your system libffi
is new enough (v3.2.1 as of October 2019), you can instead enable the
`system` feature flag to use that. If you want this crate to build
a C libffi for you, add

```toml
[dependencies]
libffi-sys = "2.3.0"
```

to your `Cargo.toml`. If you want to use your system C libffi, then

```toml
[dependencies.libffi-sys]
version = "2.3.0"
features = ["system"]
```

to your `Cargo.toml` instead.

This crate supports Rust version 1.32 and later.

[the `libffi` crate]: https://crates.io/crates/libffi/
[libffi]: https://sourceware.org/libffi/
[libffi github]: https://github.com/libffi/libffi
