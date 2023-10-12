# What is this?

Implementations of XTS and LDT for Nearby Presence "v0" advertisements.

See the appendix below for more details on XTS and LDT.

## Project structure

*Note all new crates follow the convention of using underscore `_` instead of
hyphen `-` in crate names

### `ldt`

An implementation
of [`LDT`](https://luca-giuzzi.unibs.it/corsi/Support/papers-cryptography/1619-2007-NIST-Submission.pdf)
which can use `xts-aes` as its tweakable block cipher.

### `ldt_tbc`

The Tweakable Block Cipher traits for use in LDT. These traits have
implementations in the `xts_aes`

### `ldt_np_adv`

Higher-level wrapper around the core LDT algorithm that does key derivation and
payload validation the way Nearby Presence advertisements need.

### `ldt_np_adv_ffi`

C API for rust library, currently exposes C/C++ clients the needed API's to use
the NP specific LDT rust implementation.
For an example of how to integrate with these API's see program
in `ldt_np_c_sample`

### `ldt_np_c_sample`

Sample c program which provides its own OpenSSL based AES implementation to
encrypt data through the LDT rust implementation
An example of how to interface with the `ldt_np_adv_ffi` API's

### `np_hkdf`

The Key Derivation functions used for creating keys used by nearby presence from
a key_seed

### `xts_aes`

An implementation
of [`XTS-AES`](https://luca-giuzzi.unibs.it/corsi/Support/papers-cryptography/1619-2007-NIST-Submission.pdf)

## Setup for MacOS local development

Dependencies:

```
brew install protobuf rapidjson google-benchmark
```

We depend on OpenSSL of version at least 3.0.5 being installed on your machine
to build the fuzzers, for macOS run:

```
brew install openssl@3
```

Your build system may still be picking up an older version so you will have to
symlink to the brew installed version:

```
brew link --force openssl
```

The in-box version of Clang which comes from XCode developer tools does not have
a fuzzer runtime so we will have to use our own

```
brew install llvm
```

then to override the default version it needs to come before it in $PATH. first
find your path:

```
$(brew --prefix llvm)/bin
```

then add this to the beginning of your path

```
echo 'export PATH="/opt/homebrew/opt/llvm/bin:$PATH"' >> ~/.bash_profile
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
```

verify success with:

```
clang --version
```

it should display the path to the homebrew version and not the xcode version.

Some other dependencies you may need include:

```
brew install ninja bindgen
```

## Examples

Examples use [clap](https://docs.rs/clap/latest/clap/) for nice CLIs, so try
running with `--help` to see all args.

*Note:* the examples are in the `ldt` crate, so `cd` into that first.

### `ldt_prp`

Confirm that LDT is, in fact, behaving as a PRP. That is, flipping one bit in
the ciphertext is on average going to flip half of the bits in the decrypted
plaintext, and that a change to the first `n` bytes of plaintext is increasingly
likely to be detected as `n` increases.

```
cargo run --release --example ldt_prp -- \
    --trials 1000000 \
    --check-leading-bytes 16
```

### `ldt_benchmark`

For interactive exploration of LDT performance looking for a needle in a
haystack of ciphertexts.

```
cargo run --release --example ldt_benchmark -- \
    --trials 500 \
    --keys 1000
```

### `ldt_np_c_sample`

From the root directory run the following commands to build and run the C
sample.

```
mkdir -p cmake-build && cd cmake-build
cmake ..
make
./ldt_np_c_sample/sample
```

### `ldt_np_c_sample/tests`

Test cases for the ldt_np_adv_ffi C API which are built alongside the sample,
use the following commands to run the tests, from root of repo:

```
mkdir -p cmake-build && cd cmake-build
cmake .. -DENABLE_TESTS=TRUE
make
cd ldt_np_c_sample/tests && ctest
```

you can then view the output of the tests
in `ldt_np_c_sample/tests/Testing/Temporary/LastTest.log`

To run the benchmarks:

`ldt_np_c_sample/tests/benchmarks`

## Fuzzing

To build all the fuzzers, run `scripts/build-fuzzers.sh`.

### Rust

Crates with fuzzers: `ldt`, `ldt_np_adv`, `xts_aes`

- `cd` to a crate's directory
- `cargo fuzz list` to list available fuzzers
- `cargo fuzz run [fuzzer name]` to run a fuzzer

### C

Build cmake project with `-DENABLE_FUZZ=true`<br>
Fuzz targets will be output to the build dir for:<br>

- `ldt_np_adv_ffi_fuzz`<br>
- `np_cpp_ffi/fuzz`
    - To run `fuzzer_np_cpp_deserialize`
      use: `./fuzzer_np_cpp_deserialize -max_len=255 corpus`
    - The `corpus` directory provides seed data to help the fuzzer generate
      more relevant data to input

## Cross-compilation for Android

- Add the 64bit ARM target to the stable and nightly toolchains:
    - `rustup target add aarch64-linux-android`
    - `rustup target add aarch64-linux-android --toolchain nightly`

- Install the v22 NDK that still links against `libgcc` the way rust's stdlib
  expects.
    - Newer NDKs use `libunwind` instead, which can be used just fine if you
      build your own rust stdlib, but for our purposes there's no problem with
      just using NDK 22
    - `./sdkmanager --install platform-tools 'ndk;22.0.7026061'`

- Configure the linker used for the ARMv8 Android target to be the NDK's linker.
    - `export CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER=$ANDROID_HOME/ndk/22.0.7026061/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android30-clang`
    - `export PKG_CONFIG_SYSROOT_DIR=$ANDROID_HOME/ndk/22.0.7026061/toolchains/llvm/prebuilt/linux-x86_64/sysroot`

- See if everything builds, using the nightly toolchain for the moment to
  convince the `aes` crate to use intrinsics on aarch64
    - `cargo +nightly build --workspace --all-targets --target aarch64-linux-android`
    - `cargo +nightly bench --workspace --no-run --target aarch64-linux-android`

- Prepare a place for the benchmark to be on the phone

    - `adb shell`
    - then
    - `mkdir -p /data/local/tmp/np && cd /data/local/tmp/np`
    - Leave the shell on the phone open so you can use it to run the benchmark.

- Find the benchmark binary in the build products

    - Use whatever directory you configured as the `target-dir` in
      `.cargo/config.toml` initially, and look for the file without the
      trailing `.d`.
    - `find TARGET_DIR -name 'ldt_scan*' | grep android`
    - Copy the file to the phone
    - `adb push FILE_FOUND_ABOVE /data/local/tmp/np/`

- In your `adb shell`, run the benchmark

    - `./ldt_scan-... --bench`

### Building min-sized release cross-compiled for Android

- Copy and paste the following into your `~/.cargo/config.toml`, replacing with
  a path to your NDK and Host OS

```
[target.aarch64-linux-android]
# Replace this with a path to your ndk version and the prebuilt toolchain for your Host OS
linker = "Library/Android/sdk/ndk/23.2.8568313/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android21-clang"
```

- then run:
  `cargo +nightly build -Z build-std=core,alloc -Z build-std-features=panic_immediate_abort --target aarch64-linux-android --profile release-min-size`

## Appendix

### XTS

XTS-AES has a NIST
spec: [The XTS-AES Tweakable Block Cipher - An Extract from IEEE Std 1619-2007](https://luca-giuzzi.unibs.it/corsi/Support/papers-cryptography/1619-2007-NIST-Submission.pdf)

XTS is a scheme for turning a block cipher (AES in this case) into a _tweakable_
block cipher. Tweakable block ciphers incorporate a _tweak_ which is cheaper to
change than the key, with the assumption being that the tweak will change with
each block or sequence of blocks. XTS-AES in particular is used in disk
encryption, where the sector number or the like might be incorporated into the
tweak to prevent the same data in different places on the disk being encrypted
into the same ciphertext.

### LDT

LDT is the current state of the art in length
doublers: [Efficient Length Doubling From Tweakable Block Ciphers](https://eprint.iacr.org/2017/841.pdf)
. It builds on top of a tweakable block cipher, which is why we also have an XTS
implementation.

A length doubler is a way of adapting a block cipher to act as a secure PRP (
pseudo random permutation) on data of lengths in `[block size, 2 * block size)`.
For comparison, block ciphers act as PRPs on one block at a time rather than the
whole message. Wide block modes would also work, but have higher overhead.

We use a length doubler in Nearby Presence so that changing any ciphertext bit
should flip each bit in the decrypted plaintext with 50% probability for each
bit, making it possible to detect changes anywhere because it is very unlikely
for none of the bit flips to affect the metadata key (which has a known digest).
