## elsa

[![Build Status](https://travis-ci.org/Manishearth/elsa.svg?branch=master)](https://travis-ci.org/Manishearth/elsa)
[![Current Version](https://img.shields.io/crates/v/elsa.svg)](https://crates.io/crates/elsa)
[![License: MIT/Apache-2.0](https://img.shields.io/crates/l/elsa.svg)](#license)

_🎵 Immutability never bothered me anyway 🎶_

This crate provides various "frozen" collections.

These are append-only collections where references to entries can be held on to even across insertions. This is safe because these collections only support storing data that's present behind some indirection -- i.e. `String`, `Vec<T>`, `Box<T>`, etc, and they only yield references to the data behind the allocation (`&str`, `&[T]`, and `&T` respectively)

The typical use case is having a global cache of strings or other data which the rest of the program borrows from.

### Running all examples

```bash
cargo test --examples --features indexmap
```
