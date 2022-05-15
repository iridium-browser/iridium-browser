---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
- - /Home/chromium-security/boringssl
  - BoringSSL
page_name: contributing
title: Contributing to BoringSSL
---

## Location of the code

The [BoringSSL](/Home/chromium-security/boringssl) code lives at
<https://boringssl.googlesource.com/boringssl.git>.

It is mapped into the Chromium tree via
[src/DEPS](https://chromium.googlesource.com/chromium/src/+/HEAD/DEPS) to
[src/third_party/boringssl](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/boringssl/&sq=package:chromium)

## Filing bugs

Bugs are filed under the [BoringSSL issue
tracker](https://bugs.chromium.org/p/boringssl/issues/list).

## Building

Refer to
[BUILDING.md](https://boringssl.googlesource.com/boringssl/+/HEAD/BUILDING.md)
for the canonical build instructions. Assuming those haven't changed, set things
up to build through ninja by executing:

```none
cd src/third_party/boringssl/src
cmake -GNinja -B build
ninja -C build
```

Once the ninja files are generated you can re-build from other directories
using:

```none
ninja -C <path-to-boringssl-src-build>
```

## Running the tests

See
[instructions](https://boringssl.googlesource.com/boringssl/+/HEAD/BUILDING.md#Running-tests)
in BUILDING.md to run the tests. From a Chromium checkout, this incantation may
be used:

```none
cd src/third_party/boringssl/src
ninja -C build run_tests
```

## Uploading changes for review

See
[CONTRIBUTING.md](https://boringssl.googlesource.com/boringssl/+/HEAD/CONTRIBUTING.md)
in the BoringSSL repository.

## Rolling DEPS into Chromium

Because BoringSSL lives in a separate repository, it must be "rolled" into
Chromium to get the updates.

To roll BoringSSL create a changelist in the Chromium repository that modifies
[src/DEPS](https://chromium.googlesource.com/chromium/src/+/HEAD/DEPS) and
re-generates the gn and asm files:

*   Simple example: <https://codereview.chromium.org/866213002> (Just
            modifies DEPS):
*   More complicated example:
            <https://codereview.chromium.org/693893006> (ASM changed and test
            added)

There is a script to automate all these steps:

```none
python3 third_party/boringssl/roll_boringssl.py
```
