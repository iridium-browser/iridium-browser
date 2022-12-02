# GN

GN is a meta-build system that generates build files for
[Ninja](https://ninja-build.org).

Related resources:

  * Documentation in [docs/](https://gn.googlesource.com/gn/+/main/docs/). In
    particular:
    *  [GN quick start guide](https://gn.googlesource.com/gn/+/main/docs/quick_start.md).
    *  [Frequently asked questions](https://gn.googlesource.com/gn/+/main/docs/faq.md)
    *  [Reference](https://gn.googlesource.com/gn/+/main/docs/reference.md)
       (all builtin help converted to a single file).
  * An introductory [presentation](https://docs.google.com/presentation/d/15Zwb53JcncHfEwHpnG_PoIbbzQ3GQi_cpujYwbpcbZo/edit?usp=sharing).
  * The [mailing list](https://groups.google.com/a/chromium.org/forum/#!forum/gn-dev).
  * The [bug database](https://bugs.chromium.org/p/gn/issues/list).

## What GN is for

GN is currently used as the build system for Chromium, Fuchsia, and related
projects. Some strengths of GN are:

  * It is designed for large projects and large teams. It scales efficiently to
    many thousands of build files and tens of thousands of source files.

  * It has a readable, clean syntax. Once a build is set-up, it is generally
    easy for people with no backround in GN to make basic edits to the build.

  * It is designed for multi-platform projects. It can cleanly express many
    complicated build variants across different platforms. A single build
    invocation can target multiple platforms.

  * It supports multiple parallel output directories, each with their own
    configuration. This allows a developer to maintain builds targeting debug,
    release, or different platforms in parallel without forced rebuilds when
    switching.

  * It has a focus on correctness. GN checks for the correct dependencies,
    inputs, and outputs to the extent possible, and has a number of tools to
    allow developers to ensure the build evolves as desired (for example, `gn
    check`, `testonly`, `assert_no_deps`).

  * It has comprehensive build-in help available from the command-line.

Although small projects successfully use GN, the focus on large projects has
some disadvanages:

  * GN has the goal of being minimally expressive. Although it can be quite
    flexible, a design goal is to direct members of a large team (who may not
    have much knowledge about the build) down an easy-to-understand, well-lit
    path. This isn't necessarily the correct trade-off for smaller projects.

  * The minimal build configuration is relatively heavyweight. There are several
    files required and the exact way all compilers and linkers are run must be
    specified in the configuration (see "Examples" below). There is no default
    compiler configuration.

  * It is not easily composable. GN is designed to compile a single large
    project with relatively uniform settings and rules. Projects like Chromium
    do bring together multiple repositories from multiple teams, but the
    projects must agree on some conventions in the build files to allow this to
    work.

  * GN is designed with the expectation that the developers building a project
    want to compile an identical configuration. So while builds can integrate
    with the user's environment like the CXX and CFLAGS variables if they want,
    this is not the default and most project's builds do not do this. The result
    is that many GN projects do not integrate well with other systems like
    ebuild.

  * There is no simple release scheme (see "Versioning and distribution" below).
    Projects are expected to manage the version of GN they require. Getting an
    appropriate GN binary can be a hurdle for new contributors to a project.
    Since GN is relatively uncommon, it can be more difficult to find
    information and examples.

GN can generate Ninja build files for C, C++, Rust, Objective C, and Swift
source on most popular platforms. Other languages can be compiled using the
general "action" rules which are executed by Python or another scripting
language (Google does this to compile Java and Go). But because this is not as
clean, generally GN is only used when the bulk of the build is in one of the
main built-in languages.

## Getting a binary

You can download the latest version of GN binary for
[Linux](https://chrome-infra-packages.appspot.com/dl/gn/gn/linux-amd64/+/latest),
[macOS](https://chrome-infra-packages.appspot.com/dl/gn/gn/mac-amd64/+/latest) and
[Windows](https://chrome-infra-packages.appspot.com/dl/gn/gn/windows-amd64/+/latest)
from Google's build infrastructure (see "Versioning and distribution" below for
how this is expected to work).

Alternatively, you can build GN from source with a C++17 compiler:

    git clone https://gn.googlesource.com/gn
    cd gn
    python build/gen.py # --allow-warning if you want to build with warnings.
    ninja -C out
    # To run tests:
    out/gn_unittests

On Windows, it is expected that `cl.exe`, `link.exe`, and `lib.exe` can be found
in `PATH`, so you'll want to run from a Visual Studio command prompt, or
similar.

On Linux, Mac and z/OS, the default compiler is `clang++`, a recent version is
expected to be found in `PATH`. This can be overridden by setting the `CC`, `CXX`,
and `AR` environment variables.

On z/OS, building GN requires [ZOSLIB](https://github.com/ibmruntimes/zoslib) to be
installed, as described at that URL. When building with `build/gen.py`, use the option
`--zoslib-dir` to specify the path to [ZOSLIB](https://github.com/ibmruntimes/zoslib):

    cd gn
    python build/gen.py --zoslib-dir /path/to/zoslib

By default, if you don't specify `--zoslib-dir`, `gn/build/gen.py` expects to find
`zoslib` directory under `gn/third_party/`.

## Examples

There is a simple example in [examples/simple_build](examples/simple_build)
directory that is a good place to get started with the minimal configuration.

To build and run the simple example with the default gcc compiler:

    cd examples/simple_build
    ../../out/gn gen -C out
    ninja -C out
    ./out/hello

For a maximal configuration see the Chromium setup:
  * [.gn](https://cs.chromium.org/chromium/src/.gn)
  * [BUILDCONFIG.gn](https://cs.chromium.org/chromium/src/build/config/BUILDCONFIG.gn)
  * [Toolchain setup](https://cs.chromium.org/chromium/src/build/toolchain/)
  * [Compiler setup](https://cs.chromium.org/chromium/src/build/config/compiler/BUILD.gn)

and the Fuchsia setup:
  * [.gn](https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/.gn)
  * [BUILDCONFIG.gn](https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/build/config/BUILDCONFIG.gn)
  * [Toolchain setup](https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/build/toolchain/)
  * [Compiler setup](https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/build/config/BUILD.gn)

## Reporting bugs

If you find a bug, you can see if it is known or report it in the [bug
database](https://bugs.chromium.org/p/gn/issues/list).

## Sending patches

GN uses [Gerrit](https://www.gerritcodereview.com/) for code review hosted at
[gn-review.googlesource.com](https://gn-review.googlesource.com/). The short
version of how to patch is:

    Register at https://gn-review.googlesource.com.

    ... edit code ...
    ninja -C out && out/gn_unittests

Then, to upload a change for review:

    git commit
    git push origin HEAD:refs/for/main

The first time you do this you'll get an error from the server about a missing
change-ID. Follow the directions in the error message to install the change-ID
hook and run `git commit --amend` to apply the hook to the current commit.

When revising a change, use:

    git commit --amend
    git push origin HEAD:refs/for/main

which will add the new changes to the existing code review, rather than creating
a new one.

We ask that all contributors
[sign Google's Contributor License Agreement](https://cla.developers.google.com/)
(either individual or corporate as appropriate, select 'any other Google
project').

## Community

You may ask questions and follow along with GN's development on Chromium's
[gn-dev@](https://groups.google.com/a/chromium.org/forum/#!forum/gn-dev)
Google Group.

## Versioning and distribution

Most open-source projects are designed to use the developer's computer's current
toolchain such as compiler, linker, and build tool. But the large
centrally controlled projects that GN is designed for typically want a more
hermetic environment. They will ensure that developers are using a specific
compatible toolchain that is versioned with the code.

As a result, GN expects that the project choose the appropriate version of GN
that will work with each version of the project. There is no "current stable
version" of GN that is expected to work for all projects.

As a result, the GN developers do not maintain any packages in any of the
various packaging systems (Debian, RedHat, HomeBrew, etc.). Some of these
systems to have GN packages, but they are maintained by third parties and you
should use them at your own risk. Instead, we recommend you refer your checkout
tooling to download binaries for a specific hash from [Google's build
infrastructure](https://chrome-infra-packages.appspot.com/p/gn/gn) or compile
your own.

GN does not guarantee the backwards-compatibility of new versions and has no
branches or versioning scheme beyond the sequence of commits to the main git
branch (which is expected to be stable).

In practice, however, GN is very backwards-compatible. The core functionality
has been stable for many years and there is enough GN code at Google alone to
make non-backwards-compatible changes very difficult, even if they were
desirable.

There have been discussions about adding a versioning scheme with some
guarantees about backwards-compatibility, but nothing has yet been implemented.
