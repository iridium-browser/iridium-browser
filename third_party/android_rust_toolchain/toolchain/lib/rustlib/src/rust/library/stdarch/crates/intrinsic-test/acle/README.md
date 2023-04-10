<div align="center">
   <img src="Arm_logo_blue_RGB.svg" />
</div>

<!-- ALL-CONTRIBUTORS-BADGE:START - Do not remove or modify this section -->
[![All Contributors](https://img.shields.io/badge/all_contributors-3-orange.svg?style=flat-square)](#contributors-)
<!-- ALL-CONTRIBUTORS-BADGE:END -->
![Continuous Integration](https://github.com/ARM-software/acle/actions/workflows/ci.yml/badge.svg)

# Arm C Language Extensions

This repository contains the source material form which the
specifications for the Arm C Language Extensions (ACLE) is derived.
For the published documents please see the [Arm developer ACLE
page](https://developer.arm.com/architectures/system-architectures/software-standards/acle)

## Contributing

The branch ``main`` is the release branch, whose tip contains the
latest release. Therefore, please submit your PR against the branch
``next-release``.

## Defect reports

Please report defects in or enhancements to the specifications in this folder to
the [issue tracker page on
GitHub](https://github.com/ARM-software/acle/issues).

For reporting defects or enhancements to documents that currenlty are not yet
included in this repo and are thus only hosted on developer.arm.com, please send
an email to arm.acle@arm.com.

## Transitioning the ACLE specifications

This is a work in progress. The document added to the repository will
be referenced in this README file when added.

## List of documents

Document sources                                                           | Latest official release
---                                                                        | ---
[Arm C Language Extensions](main/acle.rst)                                 | [2021Q2](https://github.com/ARM-software/acle/releases/latest)
[Morello Supplement to the Arm C Language Extensions](morello/morello.rst) | [01alpha](https://github.com/ARM-software/acle/releases/latest)
[Arm MVE Intrinsics](mve_intrinsics/mve.rst)                               | [2021Q2](https://github.com/ARM-software/acle/releases/latest)
[Arm Neon Intrinsics Reference](neon_intrinsics/advsimd.rst)               | [2021Q2](https://github.com/ARM-software/acle/releases/latest)

# License

All the ACLE documents themselves are not dependent on any assets
outside of their own directory and all have their own license file
included in the directory. Currently all the ACLE documents are
licenced under the Creative Commons Attribution-ShareAlike 4.0
International License + grant of Patent License. Contributions to
these files are accepted under the same license.

The files in the sub-directories of the tools directory are provided
under the Apache 2.0 license. Contributions to these files are
accepted under the same license.

## Contributors ✨

The list below represents the list of (wonderful!) people that have
been contributing since July 2021 ([emoji
key](https://allcontributors.org/docs/en/emoji-key)), when the ACLE
specifications were released with an open source license.

Before that time many other people contributed to ACLE.

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<table>
  <tr>
    <td align="center"><a href="http://tubafranz.me/"><img src="https://avatars.githubusercontent.com/u/25690309?v=4?s=100" width="100px;" alt=""/><br /><sub><b>Francesco Petrogalli</b></sub></a><br /><a href="https://github.com/ARM-software/acle/commits?author=fpetrogalli" title="Code">💻</a> <a href="https://github.com/ARM-software/acle/pulls?q=is%3Apr+reviewed-by%3Afpetrogalli" title="Reviewed Pull Requests">👀</a></td>
    <td align="center"><a href="https://github.com/ktkachov-arm"><img src="https://avatars.githubusercontent.com/u/74917949?v=4?s=100" width="100px;" alt=""/><br /><sub><b>ktkachov-arm</b></sub></a><br /><a href="https://github.com/ARM-software/acle/commits?author=ktkachov-arm" title="Code">💻</a> <a href="#content-ktkachov-arm" title="Content">🖋</a> <a href="#infra-ktkachov-arm" title="Infrastructure (Hosting, Build-Tools, etc)">🚇</a></td>
    <td align="center"><a href="https://github.com/sallyarmneale"><img src="https://avatars.githubusercontent.com/u/56446080?v=4?s=100" width="100px;" alt=""/><br /><sub><b>sallyarmneale</b></sub></a><br /><a href="https://github.com/ARM-software/acle/pulls?q=is%3Apr+reviewed-by%3Asallyarmneale" title="Reviewed Pull Requests">👀</a></td>
  </tr>
</table>

<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END -->

This project follows the [all-contributors](https://github.com/all-contributors/all-contributors) specification. Contributions of any kind welcome!