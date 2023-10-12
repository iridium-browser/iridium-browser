<!--
SPDX-FileCopyrightText: 2021 HH Partners
 
SPDX-License-Identifier: MIT
 -->

# SPDX Documents in Rust

[![crates.io-icon]][crates.io] [![docs.rs-icon]][docs.rs]

[crates.io-icon]: https://img.shields.io/crates/v/spdx-rs
[crates.io]: https://crates.io/crates/spdx-rs
[docs.rs-icon]: https://img.shields.io/docsrs/spdx-rs
[docs.rs]: https://docs.rs/spdx-rs

`spdx-rs` parses [SPDX documents] in multiple data formats to Rust structs.

## Data formats

The library has been designed for working with SPDX documents in JSON. This is achieved with
[Serde], so any data format supported by Serde should work, as long as the naming is consistent with
that used in JSON SPDX documents.

In addition to serializing and deserializing with Serde, deserializing documents in tag value format
is supported with a custom parser.

## Usage

Simple usage examples for parsing documents from JSON and tag-value formats can be found in the
[integration tests].

[SPDX documents]: https://spdx.github.io/spdx-spec/
[Serde]: https://serde.rs/
[integration tests]: https://github.com/doubleopen-project/spdx-rs/tree/main/tests/integration.rs

## License

The project is licensed under the [MIT License][LICENSE.txt].

[LICENSE.txt]: license.txt
