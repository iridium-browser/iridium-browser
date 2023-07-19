# What is this?

A crypto provider that abstracts over different crypto implementations, mainly the Rust
implementations by [RustCrypto](https://github.com/RustCrypto), OpenSSL and BoringSSL.

## Project structure

### `crypto_provider`

Our own abstraction on top of crypto implementations, including functionalities
like AES, SHA2, X25519 and P256 ECDH, HKDF, HMAC, etc.

Two implementations are currently provided, `crypto_provider_rustcrypto` and
`crypto_provider_openssl`.

#### `crypto_provider::aes`
Abstraction on top plain AES, including AES-CTR and AES-CBC.

Since we know we'll have multiple AES implementations in practice (an embedded
device might want to use mbed, but a phone or server might use BoringSSL, etc),
it's nice to define our own minimal AES interface that exposes only what we need
and is easy to use from FFI (when we get to that point).

### `crypto_provider_rustcrypto`

Implementations of `crypto_provider` types using the convenient pure-Rust primitives
from [Rust Crypto](https://github.com/RustCrypto).

### `crypto_provider_openssl`

Implementations of `crypto_provider` types using the
[openSSL Rust crate](https://github.com/sfackler/rust-openssl), which is a Rust
wrapper for openSSL.

#### Using BoringSSL

`crypto_provider_openssl` can also be made to use BoringSSL via the `boringssl` feature. This
translates to using the `openssl` and `openssl-sys` crates' `unstable_boringssl` feature. Since the
depenedency `bssl-sys` is not on crates.io, to test the BoringSSL integration, you'll need to run
`cargo run -- build-boringssl`, which clones BoringSSL. Then, to use the Android version of
`rust-openssl`, run `cargo run -- prepare-rust-openssl`.

* Run `cargo run -- build-boringssl` to setup the workspace
* Run `cargo --config=.cargo/config-boringssl.toml test --features=boringssl` to test the crypto
  provider implementations.
* Run `cargo --config=.cargo/config-boringssl.toml run -p <package> --features=openssl,boringssl
  --no-default-features` on FFI, JNI, or shell targets to make them use BoringSSL.

## Setup

See `nearby/presence/README.md` for machine setup instructions.
