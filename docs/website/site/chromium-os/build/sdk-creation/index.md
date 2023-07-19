---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/build
  - Chromium OS Build
page_name: sdk-creation
title: Chromium OS SDK Creation
---

[TOC]

## Introduction

The Chromium OS project has an SDK that provides a standalone environment for
building the target system. When you boil it down, it's simply a Gentoo/Linux
chroot with a lot of build scripts to simplify and automate the overall build
process. We use a chroot as it ends up being much more portable -- we don't have
to worry what distro you've decided to run and whether you've got all the right
packages (and are generally up-to-date). Most things run inside of the chroot,
and we fully control that environment, so Chromium OS developers have a lot less
to worry about. It does mean that you need root on the system, but
unfortunately, that cannot be avoided.

This document will cover how the SDK is actually created. It assumes you have a
full Chromium OS checkout already.

## Prebuilt Flow

When you run `cros_sdk` (found in
[chromite/bin/](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/scripts/cros_sdk.py))
for the first time, it automatically downloads the last known good sdk version
and unpacks it into the chroot/ directory (in the root of the Chromium OS source
checkout).

That version information is stored in
[src/third_party/chromiumos-overlay/chromeos/binhost/host/sdk_version.conf](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/chromeos/binhost/host/sdk_version.conf):

```
SDK_LATEST_VERSION="2022.03.17.105954"
```

This is used to look up the tarball in the
[chromiumos-sdk](https://storage.googleapis.com/chromiumos-sdk/) Google Storage
bucket. So with the information above, we know to fetch the file:

<https://storage.googleapis.com/chromiumos-sdk/cros-sdk-2022.03.17.105954.tar.xz>

## Bootstrap Flow

The question might arise: How is the prebuilt SDK tarball created in the first
place? The chromiumos-sdk builder (a.k.a. the “SDK builder”) builds each
new version of the SDK tarball for developers and other builders to use. The SDK
builder runs in a continuous loop to include any recent commits and takes ~24
hours for a successful run.

For a bootstrap starting point, to avoid the case where the SDK itself may have
been broken by a commit, the builder uses a pinned known "good" version from
which to build the next SDK version. This version is manually moved forward as
needed.

If the overall SDK generation fails, then the SDK is not refreshed and the
released files stay stable for developers.

### Overview

The overall process looks like:

*   Download bootstrap version of the SDK and setup chroot environment
*   Build the SDK (amd64-host) board
*   Build and install the cross-compiler toolchains
*   Generate standalone copies of the cross-compilers
*   Package the freshly built SDK creating and uploading a tarball of it
*   Verify the SDK by building/testing other boards with it
*   Upload binpkgs
*   Uprev the SDK to point to this latest version

### SDK bootstrap version

We download a copy of the SDK tarball to bootstrap with and setup the chroot
environment. This is just using the standard `cros_sdk` command with its
`--bootstrap` option.

As with the latest SDK version, the bootstrap version of the SDK to use is
stored in
[sdk_version.conf](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/chromeos/binhost/host/sdk_version.conf):

```
BOOTSTRAP_FROZEN_VERSION="2020.11.09.170314"
```

This is used to look up the tarball in the chromiumos-sdk Google Storage bucket.
So with the information above, `cros_sdk` knows to fetch the file:

<https://storage.googleapis.com/chromiumos-sdk/cros-sdk-2020.11.09.170314.tar.xz>

`cros_sdk` continues its normal process of running
[`update_chroot`](https://chromium.googlesource.com/chromiumos/platform/crosutils/+/HEAD/update_chroot)
and setting up the chroot SDK environment.

### Build the SDK (amd64-host) board

The first step of the creation of the new SDK is for the SDK builder to
[build the special amd64-host board](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/cbuildbot/stages/build_stages.py#554).
To leverage existing build scripts and infrastructure we treat building the SDK
as similar to a traditional board called "amd64-host."

This step is performed by the
[src/scripts/build_sdk_board](https://chromium.googlesource.com/chromiumos/platform/crosutils/+/HEAD/build_sdk_board)
script and is accomplished by just running:

```
./build_sdk_board --board amd64-host
```

It'll take quite a long time for this to finish (as it has to build a
few hundred packages from source). Everything will be written to
`/board/amd64-host`.

### Build and install the cross-compiler toolchains

To
[build and install the toolchains](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/cbuildbot/stages/sdk_stages.py#75),
the cbuildbot process will then run
[`cros_setup_toolchains`](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/scripts/cros_setup_toolchains.py)
which generates all toolchains marked for inclusion in the SDK (see the
toolchain.conf file for more details).

### Generate and upload standalone cross-compilers

Since our cross-compilers are pretty cool & useful, we want to be able to use
them all by themselves. In other words, without the overhead of the full
Chromium OS source checkout and the SDK. With a few tricks, we can accomplish
exactly that. It copies all the host libraries the toolchain itself uses,
generates wrapper scripts so that the local copies of libraries can be found,
and then takes care of munging all the paths to make them standalone.

The
[cros_setup_toolchains](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/scripts/cros_setup_toolchains.py)
script has all the logic to create the packages which are then uploaded to a
Google Storage bucket.

### Package The SDK

Now that the SDK is compiled and has everything we want, we
[create then upload the SDK tarball](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/cbuildbot/stages/sdk_stages.py#127)
to a Google Storage bucket.

### Test The SDK

We want to be sure that the SDK we just built is actually sane. This helps us
from releasing a toolchain update (like gcc) that is horribly broken (e.g. can't
properly compile or link things).
[To test this](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/cbuildbot/stages/sdk_stages.py#309),
we simply use the SDK tarball built earlier to create a new chroot. This also
verifies the normal developer workflow of creating a new chroot from this SDK.

Inside of the new chroot, we do a normal build flow for a couple of boards. At
the moment, that means one for each major architecture (i.e. amd64-generic,
arm-generic). We only run `cros build-packages` though; no unittests or
anything else (as current history as shown it to not be necessary).

### Upload the binary packages

Once all the tests pass, we go ahead and
[upload the binary packages](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/cbuildbot/stages/artifact_stages.py#645)
for the SDK itself.

### Uprev the SDK

Now that the new SDK version has been tested and the tarball and binary packages
uploaded, the SDK is ready to be
[upreved](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/cbuildbot/stages/sdk_stages.py#379)
to point to this latest version. This simply updates the `SDK_LATEST_VERSION` in
the
[sdk_version.conf](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/chromeos/binhost/host/sdk_version.conf)
file mentioned earlier.

The new SDK is now available for developers and builders to use.

## Running the SDK builder as a developer

Suppose as a developer you have changes (e.g. in
[chromite/lib](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/lib/))
that you wish to test with the SDK builder. To run the entire chromiumos-sdk
builder process described above, run as a tryjob:

```
cros tryjob -g <cl 1> [-g …] chromiumos-sdk-tryjob
```

where `<cl 1> .. <cl n>` are CLs to be run against.

It's likely that just building the SDK board locally would be sufficent for most
cases. To do that, from `~/chromiumos/src/scripts` run:

```
./build_sdk_board
```
