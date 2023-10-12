# rustc-build-sysroot

This crate offers the ability to build a rustc sysroot from source. You can think of it as a very
lightweight version of [xargo] (which was a useful source for information on how to do this), or a
version of [`cargo -Zbuild-std`] that builds a sysroot rather than building the standard library for
the current crate.

[xargo]: https://github.com/japaric/xargo/
[`cargo -Zbuild-std`]: https://github.com/rust-lang/wg-cargo-std-aware

Building the sysroot from source is useful for tools like [Miri] and [cargo-careful] that need the
standard library to be built with different flags. Building a sysroot from different sources is
*not* a goal of this crate.

[Miri]: https://github.com/rust-lang/miri
[cargo-careful]: https://github.com/RalfJung/cargo-careful
