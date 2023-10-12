# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog] and this project adheres to
[Semantic Versioning].

[Keep a Changelog]: http://keepachangelog.com/en/1.0.0/
[Semantic Versioning]: http://semver.org/spec/v2.0.0.html

## [3.2.0] - 2023-03-28

- Handle return type promotion in the high layer: https://github.com/tov/libffi-rs/pull/69
- Fix return type in call example code: https://github.com/tov/libffi-rs/pull/68

## [3.1.0] - 2023-01-02

- Bump version requirement of libffi-sys to 2.1.0
- Copy size and alignment of structures on clone: https://github.com/tov/libffi-rs/pull/62

## [3.0.1] - 2022-09-02

- Removed dependency on abort_on_panic: https://github.com/tov/libffi-rs/pull/58

## [3.0.0] - 2022-03-07

- Improve cross-compilation support: https://github.com/tov/libffi-rs/pull/53
- Rust 1.48 or newer is now required

## [2.0.1] - 2022-02-13

### Changed

- Fix linker errors for aarch64: https://github.com/tov/libffi-rs/pull/51

## [2.0.0] - 2021-08-17

### Fixed

- Fixed soundness issues with the "high" interface when using callbacks. See
  0a094088fde8f6e4e382c987ee189c58929a33d1 for more information.

### Changed

- Remove outdated documentation links

## [1.0.1] - 2021-05-06

- Fixed used after free bug in ffi_type_array_create

### Added
- Added support for ARMv7

## [1.0.0] - 2020-10-25

### Changed
- Use libffi-sys-rs 1.0.0
- Update libc dependency to a more recent version

## [0.9.0] - 2019-12-07

### Added
- Methods `middle::Closure::instantiate_code_ptr` and
  `middle::ClosureOnce::instantiate_code_ptr`.

### Changed
- Updated `libffi-sys` dependency version to `"0.9.0"` from `"0.8.0"`.
- Updated to Rust edition to 2018.
- Updated oldest supported rustc version to 1.36.0.

## [0.8.0] - 2019-10-24

### Added
- Added `system` Cargo feature, which passes the same feature to `libffi-sys`,
  which causes it to use the system C libffi instead of building its own.

### Changed
- Updated `libc` dependency version to `"0.2.65"` from
  `"0.2.11"`.
- Updated `libffi-sys` dependency version to `"0.8.0"` from
  `"0.7.0"`.

## [0.7.0] - 2019-05-12

### Removed
- Broken `"unique"` feature.

### Changed
- Updated `abort_on_panic` dependency version to `"2.0.0"` from
  `"1.0.0"`.
- Updated `libffi-sys` dependency version to `"0.7.0"` from
  `"0.6.0"`.

### Added
- Setting `doc(html_root_url)` for inter-crate docs linking.
- Testing on Rust 1.31.0 now, as oldest supported version.

## [0.6.3] - 2018-03-05

### Fixed
- Heading in docs.

## [0.6.2] - 2017-11-13

### Changed
- Upgraded to `libffi-sys` 0.6.0, which uses an upgraded bindgen.

## [0.6.0] - 2017-05-14

### Fixed
- Marked `Unique::new` as `unsafe`.

### Added
- Mentions dependencies in build instructions.

### Changed
- Constructors and factories that need sequences now take `IntoIterator`
instead of `Iterator` or `FixedSizeIterator`.

## [0.5.3] - 2017-04-15

### Fixed
- `Closure[0-9]` and `ClosureMut[0-9]` now abort on panic rather than
attempting to unwind past an FFI boundary. (Thanks, ngkz!)

## [0.5.2] - 2017-04-14

### Changed
- Now depends on `libffi-sys` 0.5.0.
