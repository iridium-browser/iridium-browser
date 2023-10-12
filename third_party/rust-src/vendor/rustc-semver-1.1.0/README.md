[![Coverage](https://img.shields.io/badge/dynamic/json?color=brightgreen&label=coverage&query=%24.data%5B0%5D.totals.lines.percent&suffix=%25&url=https%3A%2F%2Fraw.githubusercontent.com%2Fflip1995%2Frustc-semver%2Fgh-pages%2Fcov.json)](https://flip1995.github.io/rustc-semver/)
[![Tests](https://github.com/flip1995/rustc-semver/workflows/Tests/badge.svg)](https://github.com/flip1995/rustc-semver/actions?query=branch%3Amaster+event%3Apush+workflow%3ATests)

# Rustc Semver

This crate provides a minimalistic parser for Rust versions.

## Description

The parser will only accept Versions in the form

```text
<major>.<minor>.<patch>
```

and 3 special versions:

- `1.0.0-alpha`
- `1.0.0-alpha.2`
- `1.0.0-beta`

This covers every version of `rustc` that were released to date.

## Usage

There are 2 functions to create a `RustcVersion`:

1. `const RustcVersion::new(u32, u32, u32)`: This is mainly used to create
   constants
2. `RustcVersion::parse(&str)`: Usually you want to parse a version with this
   function

If you have a `RustcVersion` you can compare them, like you would expect:

```rust
assert!(RustcVersion::parse("1.42.0")? < RustcVersion::parse("1.43")?);
```

If you want to check whether one version meets another version according to the
[Caret Requirements], there is the method `RustcVersion::meets`:

```rust
assert!(RustcVersion::new(1, 48, 0).meets(RustcVersion::parse("1.42")?));
```

[Caret Requirements]: https://doc.rust-lang.org/cargo/reference/specifying-dependencies.html#caret-requirements

## Code of Conduct

This repository adopts the [Contributor Covenant Code of
Conduct](https://www.contributor-covenant.org/version/1/4/code-of-conduct/)

## License

Copyright 2020 Philipp Krones

Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
https://www.apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
https://opensource.org/licenses/MIT>, at your option. Files in the project may
not be copied, modified, or distributed except according to those terms.
