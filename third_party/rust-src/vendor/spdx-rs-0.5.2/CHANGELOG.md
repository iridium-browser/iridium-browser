<!--
SPDX-FileCopyrightText: 2021 HH Partners
 
SPDX-License-Identifier: MIT
 -->

# Changelog

## [0.5.2](https://github.com/doubleopen-project/spdx-rs/compare/v0.5.1...v0.5.2) (2022-12-01)


### Bug Fixes

* disable default features of chrono to avoid security vuln ([370c25c](https://github.com/doubleopen-project/spdx-rs/commit/370c25c4580b6d7da915f0ce6c38b34c07565c6f))
* remove needless lifetimes ([151a7c7](https://github.com/doubleopen-project/spdx-rs/commit/151a7c76a12c8553f204adfb37a3e2e7fa281160))
* replace deprecated chrono usage ([dc29e99](https://github.com/doubleopen-project/spdx-rs/commit/dc29e99902df10d493cc42fb5f3887e7a99d3fce))

## [0.5.1](https://github.com/doubleopen-project/spdx-rs/compare/v0.5.0...v0.5.1) (2022-07-05)


### Miscellaneous Chores

* release 0.5.1 ([3cda72b](https://github.com/doubleopen-project/spdx-rs/commit/3cda72b8264d3ffd6576f80febdc1d0845ab2650))

## [0.5.0] - 2022-04-13

### Changed

- **Breaking:** Change field `license_information_in_file` of `FileInformation` to be a
  `Vec<SimpleExpression>` instead of a `Vec<String>`.

## [0.4.1] - 2022-04-12

### Added

- Implement `PartialEq` for SPDX.

## [0.4.0] - 2022-04-12

### Changed

- **Breaking:** Change `SPDXExpression` to use the `spdx-expression` crate. Changes everything
  around the expression handling.

## [0.3.0] - 2021-10-21

### Changed

- **Breaking:** Refactor snippet byte range and line range.

### Removed

- **Breaking:** Moved the functionality for getting the SPDX License List to a utility crate. See
  [spdx-toolkit](https://github.com/doubleopen-project/spdx-toolkit).
- **Breaking:** Moved the functionality for for creating graphs from the relationships to a utility
  crate. See [spdx-toolkit](https://github.com/doubleopen-project/spdx-toolkit).

## [0.2.1] - 2021-10-14

### Fixed

- Accepts lowercase relationship types when parsing tag-value documents.

## [0.2.0] - 2021-10-13

### Changed

- Started following semantic versioning and keeping a changelog.

[0.5.0]: https://github.com/doubleopen-project/spdx-rs/compare/v0.4.1...v0.5.0
[0.4.1]: https://github.com/doubleopen-project/spdx-rs/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/doubleopen-project/spdx-rs/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/doubleopen-project/spdx-rs/compare/v0.2.1...v0.3.0
[0.2.1]: https://github.com/doubleopen-project/spdx-rs/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/doubleopen-project/spdx-rs/compare/v0.1.0...v0.2.0
