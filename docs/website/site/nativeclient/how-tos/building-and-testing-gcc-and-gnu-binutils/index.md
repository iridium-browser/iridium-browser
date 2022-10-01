---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/how-tos
  - '2: How Tos'
page_name: building-and-testing-gcc-and-gnu-binutils
title: Building and Testing GCC and GNU binutils
---

[TOC]

## Introduction

This page describes the structure of source code of the GCC-based Native Client
toolchain. The sources are based on stable releases of external packages:
[Binutils](http://git.chromium.org/gitweb/?p=nacl-binutils.git;a=summary),
[GCC](http://git.chromium.org/gitweb/?p=nacl-gcc.git;a=summary),
[Newlib](http://git.chromium.org/gitweb/?p=nacl-newlib.git;a=summary),
[GDB](http://git.chromium.org/gitweb/?p=nacl-gdb.git;a=summary),
[GlibC](http://git.chromium.org/gitweb/?p=nacl-glibc.git;a=summary) plus a
subset of [linux kernel
headers](http://git.chromium.org/gitweb/?p=linux-headers-for-nacl.git;a=summary).
The rest of the page addresses the structure of the source repositories, the
ways to synchronize patches across them and the build script.

## Prerequisites

We recommend hacking the toolchain on Linux. After each commit the toolchain
tarballs are built on [builders](https://ci.chromium.org/p/nacl/g/main/console).
Using Windows is difficult since Chromium no longer supports Cygwin, and it's
too slow to be practical anyway (mostly due to the poor performance of fork()).

### Installing packages on your system

*   Refer to
            [HowToBuildPorts](http://code.google.com/p/nativeclient/wiki/HowToBuildPorts)
            for setting up the build environment.
*   optional: Git and Subversion (useful for development)

    ```none
    sudo apt-get install gitk subversion
    ```

Native Client Source is also required to build the toolchain (specifically,
libraries). Follow the instructions on the [Native Client
Source](http://code.google.com/p/nativeclient/wiki/Source) page to fetch the
sources.

### Setting up environment variables for Git

If you are going to contribute to the toolchain, you will need to use the Git
repository. Add similar lines to your .bashrc or a shell login script:

```none
export GIT_COMMITTER_NAME="John Doe"
export GIT_COMMITTER_EMAIL="doe@example.com"
export GIT_AUTHOR_NAME="Mary Major"
export GIT_AUTHOR_EMAIL="mary@example.com"
```

In the most common case author and committer strings should be equal.

## Obtaining the toolchain sources

```none
cd native_client/tools
make sync  # For more options about syncing and building read the topmost comment in the Makefile.
```

### Building the toolchain

Option 1: Build a *newlib*-based toolchain

```none
make clean build-with-newlib -j16
```

Option2:Build a *GlibC*-based toolchain.

```none
make clean build-with-glibc -j16
```

The toolchain binaries get installed to native_client/tools/out by default, you
bay override it by providing *TOOLCHAINLOC=&lt;path&gt; in make invocation.*

*note:* Although the build script is written as a Makefile, it does not support
incremental rebuilds (though it supports parallel builds).

*note*: To build in a 100% clean way, the TOOLCHAINLOC directory must be empty.

## Structure of the Git repositories

The Git repositories are hosted at [chromium git
repositories](http://git.chromium.org/). NaCl development happens in branch
master in each repository.

### Branches

The branch vendor-src keeps the sources from the original tarball unchanged.
From time to time the master branch should be **rebased** on top of newer
revisions of the vendor-src branch. Once the **master** branch is rebased, the
old branch should be kept from garbage collection so that old commits can be
found by hashes (required to be able to reproduce old builds).

## Code reviews

Follow the [Git Cookbook](http://code.google.com/p/chromium/wiki/GitCookbook)
for sending your commit for review. Recommendations: Method #1 is the easiest.
Instead of "git cl" you can use git-cl from **depot_tools**. In the simplest
case you may:

```none
git checkout -b my_hack origin/master # make your branch
# hack hack hack
git add your/file1 your/file2
git commit -m "do my hack with GCC"
git-cl upload --send-mail -r reviewer@chromium.org
# fix stuff during review
git commit --amend your/fixed/file1
git-cl upload
# you've got an LGTM
git-cl push
```

## Testing the toolchain (the GCC testsuite)

#### Currently it is only easy to run the tests in x86-64 mode, x86-32 testing is broken. To run all tests:

```none
make -j4 check SDKLOC=/path/to/your/location
```

To run one of the three testsuites (c++ testsuite in this case):

```none
make DEJAGNU=/your/native_client/tools/dejagnu/site.exp check-c++ RUNTESTFLAGS="SIM=/your/sel_ldr --target_board=nacl --outdir=your-directory-for-reports"
```

To run a subset of a testsuite governed by some '.exp' config file just add the
name of the file (without path to it) as additional parameter to runtest:

<pre><code>
cd your-native-client/native_client/tools/BUILD/build-gcc-nacl64/gcc
DEJAGNU=your-dejagnu/site.exp <b>runtest</b> --tool gcc --target_board=nacl --outdir=/tmp/your-temp-dir SIM=/your/sel_ldr builtins.exp
</code></pre>

You may limit running tests to a single file. Again, the file name should omit
the path part:

<pre><code>
cd your-native-client/native_client/tools/BUILD/build-gcc-nacl64/gcc
DEJAGNU=your-dejagnu/site.exp <b>runtest</b> --tool gcc --target_board=nacl --outdir=/tmp/your-temp-dir SIM=/your/sel_ldr builtins.exp<b>=strncat-chk.c</b>
</code></pre>
