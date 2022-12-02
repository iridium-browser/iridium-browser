---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/pnacl
  - PNaCl
page_name: building-pnacl-components-for-distribution-packagers
title: Building and using PNaCl components for Chromium distribution packagers
---

## Overview

Chromium's support for NaCl includes a runtime component called the IRT
(integrated runtime) that is built as untrusted NaCl code with a NaCl compiler.
Currently the PNaCl compiler is required for building this component on x86-64,
and is used on other architectures as well for consistency.

Chromium's automated builders download binary tarballs of the NaCl compilers, to
avoid having to rebuild them on every run. However distributors of Chromium may
be unwilling to do this and wish to build the toolchain from source (and
possibly even package the toolchain itself). Also, the toolchain binary is built
for the x86_64 architecture and won't run on systems with i686 kernels.

So this document describes how to build the PNaCl toolchain from source, (for
someone who doesn't necessarily want to modify it) and build Chromium using a
locally-built toolchain.

PNaCl toolchains are built using a set of scripts in the Native Client
repository, whose entry point is `toolchain_build/toolchain_build_pnacl.py`

This script is mostly optimized for use for day-to-day toolchain development and
for our automated builders, so some non-default flags are needed.

For now this script must also be used to fetch the PNaCl toolchain sources. In
the future we hope to provide source tarballs and/or patches against upstream
sources.

To see a full list of supported flags, use the -h flag. The toolchain consists
of many "packages" for different components such as the host LLVM and binutils
binaries, PNaCl driver scripts, and target libraries for each supported target.

TL;DR: You can build all of the required components from a checkout of the
native_client source tree (which could be the one DEPSed in by Chromium)

```none
$ toolchain_build/toolchain_build_pnacl.py --verbose --sync --clobber --gcc [--no-nacl-gcc] --no-use-cached-results --no-use-remote-cache --disable-git-cache --build-sbtc
$ build/package_version/package_version.py --packages pnacl_newlib --tar-dir toolchain_build/out/packages --dest-dir toolchain/ extract --skip-missing
$ build/package_version/package_version.py --packages pnacl_translator --tar-dir toolchain_build/out/packages --dest-dir toolchain/ extract --skip-missing
```

When you run Chromium's gyp to configure the build, add the following to your
gyp defines:

```none
pnacl_newlib_toolchain=</path/to/native_client>/toolchain/linux_x86/pnacl_newlib
pnacl_translator_dir=</path/to/native_client>/toolchain/linux_x86/pnacl_translator
```

When the gyp-generated ninja build prepares the toolchain to build the IRT, it
will copy the specified directory rather than extracting the binary tarballs.

## Explanation

The first command builds the host toolchain components. It fetches the sources
(--sync) into the toolchain_build/src directory without relying on tools
specific to Chromium's depot_tools package (--disable-git-cache). It clobbers
any existing build working directories (--clobber), builds the PNaCl host
toolchain using the system compiler (--gcc) and does not use our automated build
infrastructure for caching build artifacts
(--no-cache-results,--no-use-cached-results). Its output is an install directory
for each package in `toolchain_build/out/{packagename}_install`, a set of
tarballs in `toolchain_build/out/cache` and a set of JSON files that describe
the packages and their contents in `toolchain_build/out/packages`.

The contents of these install directories can be copied (or the tarballs can be
extracted together) into any directory, and the toolchain can be used from that
directory. This is what the second command does: it takes the JSON file
describing the pnacl_newlib package and extracts its tarballs into the
`toolchain/linux_x86/pnacl_newlib` directory.

One caveat is that nacl-clang uses gcc's exception handling runtime (libgcc_eh)
on x86 architectures. This in turn requires {i686,x86_64}-nacl-gcc to build.
This toolchain has a separate build script see (tools/Makefile) or can be
installed as a binary in the same way that the PNaCl compilers can.
Alternatively the --no-nacl-gcc flag can be used when building the PNaCl, which
will allow the build to complete but will result in a non-functional C++
exception handling runtime (this EH runtime is not needed to build any Chrome
components, so you'd only need to build it if you want to also create a NaCl SDK
for developers to use).

## PNaCl in-browser translator

If you want to enable PNaCl as well as NaCl in Chromium, the last component that
needs to be built is the in-browser PNaCl translator component. The third
command does that, and leaves its output in
`toolchain/linux_x86/pnacl_translator`. It consists of 2 NaCl modules and a few
NaCl object/archive files (per supported architecture), and these are packaged
(munging the filenames) into a directory called `pnacl` in Chromium's build
output directory by a python script that runs as part of Chromium's build (e.g.
in `/path/to/chromium/src/out/Release/pnacl/pnacl_public_foo`). For desktop
Google Chrome official builds, these artifacts are not packaged with Chrome's
installer, and are instead delivered after install by the component updater
code. However for locally-built Chromium binaries, distribution packages, and
ChromeOS (where the entire OS image is updated at once) the PNaCl translator in
this directory is used.

If you also want to avoid using the `package_version.py` tool after running
`toolchain_build_pnacl.py` you can manually copy the contents of all of the
`toolchain_build/out/<packagename>_install` directories into a location of your
choosing, and make the `pnacl_newlib_toolchain` gyp variable point to that
directory. If you do so, you will also have to create a stamp file in that
directory; by default it is called `pnacl_newlib.json` but that name can be
overridden with the `pnacl_newlib_stamp` gyp variable. Only the modified time of
the stamp file matters, not the content; if you change the contents of the
toolchain directory and want to rebuild Chromium, you will need to touch the
stamp file.

## Network Access

Sometimes distributions want to be able to build from locally-available sources
without requiring network access during the build. Ideally we would have source
tarballs or patches against upstream sources available for this purpose, but
today we do not. However the sources can be fetched separately from the build:
the `--sync-only` flag for `toolchain_build_pnacl.py` causes the sources to be
fetched into the `toolchain_build/src` directory without building anything. Then
the build can be done later by a separate invocation which does not require
network access.

```none
$ toolchain_build/toolchain_build_pnacl.py --verbose --sync --sync-only --disable-git-cache
$ toolchain_build/toolchain_build_pnacl.py --verbose --clobber --gcc --no-use-cached-results --no-use-remote-cache 
```

## Other flags

A listing of all the available package names and the supported flags for
`toolchain_build_pnacl.py` can be seen by using the `--help` flag. Other flags
that may be of interest in this context include `--output=`, `--source=`, and
`--cache=` which allow changing the default directories for the fetched sources
and build artifacts.

## More information

There is a tracking bug for work to help enable this use case at
<https://code.google.com/p/nativeclient/issues/detail?id=3954>
