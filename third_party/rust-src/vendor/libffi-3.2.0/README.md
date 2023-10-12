# libffi-rs: Rust bindings for [libffi](https://sourceware.org/libffi/)

[![GitHub Workflow Status](https://img.shields.io/github/workflow/status/tov/libffi-rs/Build%20&%20Test)](https://github.com/tov/libffi-rs/actions)
[![Documentation](https://img.shields.io/docsrs/libffi/latest)](https://docs.rs/libffi/latest/libffi/)
[![Crates.io](https://img.shields.io/crates/v/libffi.svg?maxAge=2592000)](https://crates.io/crates/libffi)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE-MIT)
[![License: Apache 2.0](https://img.shields.io/badge/license-Apache_2.0-blue.svg)](LICENSE-APACHE)

The C libffi library provides two main facilities: assembling calls
to functions dynamically, and creating closures that can be called
as ordinary C functions. In Rust, the latter means that we can turn
a Rust lambda (or any object implementing `Fn`/`FnMut`) into an
ordinary C function pointer that we can pass as a callback to C.

## Usage

Building `libffi` will build `lifbffi-sys`, which will in turn build the
libffi C library [from github](https://github.com/libffi/libffi), which
requires that you have a working make, C compiler, automake, and
autoconf first. It’s [on crates.io](https://crates.io/crates/libffi), so
you can add

```toml
[dependencies]
libffi = "3.2.0"
```

to your `Cargo.toml`.

This crate depends on [the `libffi-sys` crate], which by default
attempts to build its own version of the C libffi library. In order to
use your system’s C libffi instead, enable this crate’s `system`
feature in your `Cargo.toml`:

```toml
[features]
libffi = { version = "3.2.0", features = ["system"] }
```

See [the `libffi-sys` documentation] for more information about how it
finds C libffi.

This crate supports Rust version 1.48 and later.

### Examples

In this example, we convert a Rust lambda containing a free variable
into an ordinary C code pointer. The type of `fun` below is
`extern "C" fn(u64, u64) -> u64`.

```rust
use libffi::high::Closure2;

let x = 5u64;
let f = |y: u64, z: u64| x + y + z;

let closure = Closure2::new(&f);
let fun     = closure.code_ptr();

assert_eq!(18, fun(6, 7));
```

[the `libffi-sys` crate]: https://crates.io/crates/libffi-sys/
[the `libffi-sys` documentation]: https://docs.rs/libffi-sys/#usage
