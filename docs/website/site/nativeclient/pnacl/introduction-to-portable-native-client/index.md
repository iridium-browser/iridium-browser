---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/pnacl
  - PNaCl
page_name: introduction-to-portable-native-client
title: Introduction to Portable Native Client
---

## Public Documentation

If you want to build a PNaCl application for Chrome, start with the [Native
Client developer documentation](https://developers.google.com/native-client/)

## Introduction

Native Client ([NaCl](https://developers.google.com/native-client/overview)) is
a secure sandbox for running untrusted native machine code in the Chrome
browser. NaCl programs have special restrictions on the generated code, which
are implemented by the compiler toolchain and statically verified at runtime by
the trusted NaCl loader. Chrome apps can embed NaCl modules into their pages,
which can perform computations and interact with the embedding page and the
system (e.g. to communicate with Javascript or render with OpenGL) via an API
called Pepper. Portable Native Client (PNaCl) defines a low-level stable
portable intermediate representation (based on the IR used by the open-source
LLVM compiler project) which is used as the wire format instead of x86 or ARM
machine code. PNaCl consists of a developer toolchain, a specification of the
stable portable IR format (along with compiler passes to transform ordinary LLVM
IR to the stable form), and a browser component (itself implemented as as NaCl
module) which translates the IR into x86-32, x86-64, or ARM machine code.

The developer toolchain compiles C and C++ applications to a subset of LLVM
bitcode.

These bitcode files (known as "portable executables", or "pexe" files) represent
an Native Client application

in a high-level format, which can be translated to native code for execution on
any processor supported

by NaCl.

The PNaCl toolchain is currently available for Windows, Mac, and Linux.

PNaCl currently supports code generation on 4 architectures:

*   X86-32
*   X86-64
*   ARM
*   MIPS32

## **How does PNaCl work?**

PNaCl is a cross-compiling toolchain, where the target architecture is bitcode.
PNaCl recognizes three types of bitcode files:

> .bc - LLVM Bitcode object file. (Analogous to an ELF object file)

> .a - Archive of LLVM bitcode objects. (Analogous to a static library archive)

> .pexe - LLVM or PNaCl Bitcode executable. (Analogous to an executable file)

PNaCl provides the standard tools:

```none
  pnacl-clang
  pnacl-clang++
  pnacl-finalize
  pnacl-ld
  pnacl-ar
  pnacl-nm
  pnacl-ranlib
  pnacl-strip
  pnacl-as
```

The pnacl-clang and pnacl-clang++ tools produce LLVM bc files. The pnacl-ld
linker tool produces a statically linked LLVM pexe. The pnacl-finalize tool
converts an LLVM pexe to a frozen PNaCl bitcode pexe. Chrome only runs the
frozen PNaCl bitcode format, not the standard LLVM bitcode pexe.

While chrome can load and translate a frozen pexe directly, there is an
additional tool `pnacl-translate `for generating native code from either LLVM or
PNaCl bitcode. That is useful for debugging (see the Debugging section below).

Installing the PNaCl Toolchain and Building Examples

### PNaCl is now available in the naclsdk updater (<https://developers.google.com/native-client/sdk/download>). It is available in pepper_30 and higher.

1.  Download the naclsdk updater, and run "naclsdk.bat update pepper_XX"
            (e.g., pepper_canary).
2.  Once downloaded, you can set your NACL_SDK_ROOT directory to
            pepper_XX.

    See pepper_canary/examples/ for various examples. The makefiles in each
    example directory should be configured to build pnacl pexes and generate
    manifest files for those pexes. Just set the "TOOLCHAIN" environment
    variable to be "pnacl". E.g.,

    "(cd pepper_canary/examples/hello_world; TOOLCHAIN=pnacl CONFIG=Release
    make)".

    With the PNaCl sdk, you can also try compiling some of the
    [naclports](http://code.google.com/p/naclports/) examples. Once you have set
    the NACL_SDK_ROOT environment variable to point at the pepper directory with
    pnacl installed, you can run "make" to build a naclports target by setting
    the environment variable NACL_ARCH to "pnacl". E.g., to build dosbox, run

    "NACL_ARCH=pnacl make dosbox".

    That will build the required libraries (e.g., SDL), and build dosbox as a
    pexe. Libraries will be installed directly into your SDK and the resulting
    application (pexe) will be published under out/publish/dosbox.

How to Run Examples in Chrome 31+

1.  Check chrome://nacl to see if the PNaCl Translator is already
            installed. The ~3-4MB download and install happens in the
            background.
    1.  If not already installed, it will be installed on-demand the
                first time you start a PNaCl application. A good example
                application is the "load progress" example [included
                here](https://chrome.google.com/webstore/detail/pnacl-examples/mblemkccghnfkjignlmgngmopopifacf).
    2.  An alternative is to go to chrome://components and click "Check
                for update".
    3.  For Chrome 31, the on-demand install can sometimes still fail.
                Restart the browser and try again if that happens.
2.  Once installed, visit a webpage with a [PNaCl
            NMF](/system/errors/NodeNotFound) file that points at your pexe
            application (the SDK has a create_nmf.py tool that will help you
            create one). For example, try loading the samples here:
            <http://gonativeclient.appspot.com/demo>.
3.  NOTE: In contrast to a NaCl application, the mime type must be
            application/x-pnacl (not application/x-nacl).

If you are having problems check the Chrome JavaScript console and see if there
are any error messages.

How to report bugs / feedback for PNaCl

1.  Go to <http://code.google.com/p/nativeclient/issues/list> and file a
            New Issue.
2.  Describe the issue.
3.  Tag the issue as Component-PNaCl, and Arch-All (or a specific Arch
            if turns out that it is arch-specific)

## PNaCl Developers

The section on developing PNaCl itself has moved to
<https://www.chromium.org/nativeclient/pnacl/developing-pnacl>
