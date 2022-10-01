---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/how-tos
  - '2: How Tos'
page_name: build-tcb
title: Building and Testing the Native Client Trusted Code Base
---

[TOC]

## What build system(s) is Native Client using?

The primary build system used by Native Client is [SCons](http://www.scons.org).
For historical reasons we are not using plain [SCons](http://www.scons.org/) but
an extension call Hammer.

The parts of the system shared with Chrome are also built using Chrome's build
system, GN.

We also have some Makefiles and some shell scripts for certain build tasks.

## Why is this such a complex mess?

The usual excuses:

*   Inherent complexity.
*   Historical reasons.
*   Entropy requires no maintenance.
*   ...

## Which files contain build system information?

For [SCons](http://www.scons.org/) it is: SConstruct, \*\*/build.scons,
\*\*/nacl.scons There are also relevant configuration files in
site_scons/site_tools/\*, and random Python scripts located here and there.

For GN it is: \*\*/BUILD.gn and \*\*/config.gni

## What is the difference between trusted and untrusted code?

"Trusted code" encompasses components like:

*   the browser plugin
*   service runtime (sel_ldr)

It is compiled using regular compilers. Bugs in trusted code can compromise
system security, hence the name. As far as the build system is concerned trusted
code is described in \*\*/build.scons files. Trusted code lives in
src/trusted/\*\*

"Untrusted code" encompasses components like:

*   quake and other examples of Native Client executables
*   libraries necessary to build those executables
*   The IRT

It is compiled using special sandboxing compilers. As far as the build system is
concerned, untrusted code is described in \*\*/nacl.scons files. Untrusted code
lives in src/untrusted/\*\* and also in tests/\*\*

Some code can be compiled either as trusted or shared code, e.g. libraries that
facilitate communication between trusted and untrusted code. Such code typically
lives in src/shared/\*\* and has both build.scons and nacl.scons files.

## How do you use the MODE= setting when invoking SCons?

The MODE= setting or its equivalent --mode is used to select whether you want to
compile trusted or untrusted code or both and how. Examples:

MODE=nacl

*   just build untrusted code
*   note that this doesn't build all of the untrusted code. If you don't
            specify a trusted platform (e.g. MODE=opt-linux,nacl) most of the
            tests will not be built.

MODE=opt-linux

*   just build (optimized) trusted code - you must be on a Linux system

MODE=nacl,dbg-win

*   build both (trusted code will be unoptimized) - you must be on a
            Windows system

NOTE: if you do not specify MODE, "opt-*&lt;your-system-os&gt;"* will be
assumed.

NOTE: if you want to run integration tests, you need to build both trusted and
untrusted code simultaneously, because those tests involve running untrusted
code under the control of trusted code.

What is the meaning of BUILD_ARCH, TARGET_ARCH, etc. ?

Just like any cross compilation environment, there are some hairy configuration
issues which are controlled by BUILD_ARCH, TARGET_ARCH, etc. to force
conditional compilation and linkage.

It helps to revisit the
[terminology](http://www.airs.com/ian/configure/configure_5.html) used by cross
compilers to better understand Native Client:

> BUILD_SYSTEM: The system on which the tools will be built (initially) is
> called the build system.

> HOST_SYSTEM: The system on which the tools will run is called the host system.

> TARGET_SYSTEM: The system for which the tools generate code is called the
> target system.

For [NaCl](/p/nativeclient/wiki/NaCl) we only have **two** of these, sadly they
have confusing names:

> BUILD_PLATFORM: The system on which the trusted code runs.

> TARGET_PLATFORM: The sandbox system that is being enforced by the trusted
> code.

The BUILD_PLATFORM is closest in nature to the HOST_SYSTEM, the TARGET_PLATFORM
is closest to the TARGET_SYSTEM. We do not have an equivalent to BUILD_SYSTEM
since we just assume the build system is x86 (either a 32- or 64-bit system).

## What kind of BUILD_PLATFORM/TARGET_PLATFORM configurations are supported?

Conceptually we have

> BUILD_PLATFORM = (BUILD_ARCH, BUILD_SUBARCH, BUILD_OS)

and

> TARGET_PLATFORM = (TARGET_ARCH. TARGET_SUBARCH)

NOTE:

There is no TARGET_OS, since Native Client executables are OS independent.

The BUILD_OS is usually tested for using [SCons](http://www.scons.org/)
expressions like "env.Bit('windows')". You cannot really control it as it is
inherited from the system you are building on, the BUILD_SYSTEM in cross
compiler speak.

Enumeration of all BUILD_PLATFORMs:

(x86, 32, linux) (x86, 32, windows) (x86, 32, mac) (arm, 32, linux) // the 32 is
implicit as there is no 64bit arm

(x86, 64, linux) (x86, 64, windows)

***Special note for Windows users:** The Windows command-line build currently
relies on vcvarsXX.bat being called to set up the environment. The compiler's
target subarchitecture (32,64) is selected by the version of vcvars that you
called (vcvars32/vcvars64). If you call vcvars32 and then build with
platform=x86-64, you will get "target mismatch" errors.*

Enumeration of all TARGET_PLATFORMs: (x86, 32) (x86, 64) (arm, 32) // the 32 is
implicit as there is no 64bit arm

Usually BUILD_ARCH == TARGET_ARCH and BUILD_SUBARCH == TARGET_SUBARCH

There is ONLY ONE exception, you can build the ARM validator like so:

> BUILD_ARCH = x86, BUILD_SUBARCH=**, TARGET_ARCH=arm TARGET_SUBARCH=32**

**In particular it is NOT possible to use different SUBARCHs for BUILD and
TARGET.**

## What is the relationship between TARGET_PLATFORM and untrusted code?

The flavor of the untrusted code is derived from the TARGET_PLATFORM

## Why are *BUILD and ARCH* used inconsistently?

Usually BUILD_ARCH == TARGET_ARCH and BUILD_SUBARCH == TARGET_SUBARCH so
mistakes have no consequences.

## So how do I build something? (Finally!)

\[Note: scons --download has been deprecated as of Aug 17, 2010\]

The first time you build, you will need to get the latest toolchain build. Do
this using gclient hooks:

```none
fetch nacl
gclient sync
cd native_client
```

~~The first time you build something, you use the --download switch to download
the platform's toolchain.~~

    ~~./scons --download MODE=opt-linux,nacl~~

    ~~./scons --download MODE=opt-mac,nacl~~

    ~~.\\scons --download MODE=opt-win,nacl~~

~~Subsequent builds can omit the --download option. You should use --download
anytime you want to update your toolchain.~~

The default value for MODE is dbg-*&lt;your-system-os&gt;* so these two commands
are identical

*   ./scons MODE=dbg-*&lt;your-system-os&gt;*
*   ./scons

## How do I run the unittests after the build completes?

To run all unittests:

    ./scons MODE=opt-*&lt;your-system-os&gt;*,nacl run_all_tests

To run specific sets of tests:

    ./scons MODE=opt-*&lt;your-system-os&gt;*,nacl small_tests

    ./scons MODE=opt-*&lt;your-system-os&gt;*,nacl medium_tests

    ./scons MODE=opt-*&lt;your-system-os&gt;*,nacl large_tests

    ./scons MODE=opt-*&lt;your-system-os&gt;*,nacl browser_tests

To run a single test:

*   A trusted test
    *   ./scons MODE=opt-*&lt;your-system-os&gt;* run_format_string_test
        *   run_format_string_test is defined in
                    src/trusted/service-runtime/build.scons
        *   Other trusted unittest targets exist in other build.scons
                    files
        *   Note that you do not need to specify nacl as a mode for a
                    trusted test
*   An untrusted test
    *   ./scons MODE=opt-*&lt;your-system-os&gt;*,nacl run_thread_test
        *   run_thread_test is defined in tests/threads/nacl.scons
        *   Other untrusted unittest targets exist in other nacl.scons
                    files

## Are there any other cool Scons options?

There are some other scons options which are useful. Note the confusing syntax
differences for option words (platform=), single minus options (-c), and double
minus options (--clang). Sorry.

*   --clang
    *   Use the Clang toolchain downloaded by the DEPS hooks instead of
                gcc on Linux. Probably should be the default.
*   -c
    *   clean before building
*   -jN
    *   Split the build into N processes. A good choice for N is often
                the number of processors on the machine doing the build
*   MODE=*&lt;build-mode-list&gt;* (or its equivalent
            --mode=*&lt;build-mode-list&gt;*)
    *   choices are opt-linux, dbg-linux, opt-mac, dbg-mac, opt-win, and
                dbg-win (for the type of trusted code to build) and nacl (to
                build untrusted code)
    *   Usually the *&lt;build-mode-list&gt;* will contain one trusted
                code choice and the untrusted code name, like: opt-linux,nacl
*   platform=TARGET_ARCH
    *   Cross-compile for TARGET_ARCH
    *   platform=x86-64 is required in order to build for 64-bit
*   sdl=none
    *   Do not use SDL

## What about 64-bit?

The build defaults to a 32-bit build even if the machine running the build is a
64-bit machine. To build for 64-bit:

*   add: platform=x86-64

*Also see the **Special note for Windows users** in the **What kind of
BUILD_PLATFORM/TARGET_PLATFORM configurations are supported?** section above.*

## Where is the source code?

The source code is divided into these main areas:

*   src/trusted: Code that runs only as part of the trusted portion of
            Native Client
*   src/untrusted: Code that runs only as part of a user-created Native
            Client program
*   src/shared: Code that can be used in both the trusted portion and
            the user-created portion of Native Client
