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

**Note: This is an internal tools document and has nothing to do with building a
Chromium OS image that one can boot and test on. If you were linked here from an
article claiming otherwise, the author is mistaken.**

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

When you run \`cros_sdk\` (found in
[chromite/bin/](https://chromium.googlesource.com/chromiumos/chromite/+/master/scripts/cros_sdk.py))
for the first time, it automatically downloads the last known good sdk version
and unpacks it into the chroot/ directory (in the root of the Chromium OS source
checkout).

That version information is stored in:

```none
[src/chromiumos-overlay/chromeos/binhost/host/sdk_version.conf](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/master/chromeos/binhost/host)
SDK_LATEST_VERSION="2013.11.16.104134"
```

This is used to look up the tarball in the
[chromiumos-sdk](https://commondatastorage.googleapis.com/chromiumos-sdk/)
Google Storage bucket. So with the information above, we know to fetch the file:

```none
<https://commondatastorage.googleapis.com/chromiumos-sdk/cros-sdk-2013.11.16.104134.tar.xz>
```

## Bootstrap Flow

You might be wondering about the chicken & egg problem. How do you create a
prebuilt sdk tarball without using a prebuilt sdk tarball? We do this everyday
and is in fact a design feature: if the sdk itself were to be broken by a
commit, how would we recover easily? To avoid that, we have a chromiumos-sdk bot
config that does a full bootstrap and runs in the normal [Chromium OS
waterfall](http://build.chromium.org/p/chromiumos/builders/chromiumos%20sdk).

But we still need some bootstrap starting point. For that, we turn to
[Gentoo](http://www.gentoo.org/main/en/where.xml). Their release process
includes publishing what is called a stage3 tarball. It's a full (albeit basic)
Gentoo chroot that is used for installing Gentoo. We use it for creating the
Chromium OS SDK from scratch. Then we tar up the result and publish it in the
public Google Storage bucket for people.

If the overall SDK generation fails, then we don't refresh the SDK, and the
released files stay stable for developers.

### Overview

The overall process looks like:

*   download last known good Gentoo stage3
*   upgrade/install/remove development packages that we will need
            (things like git)
*   build all the cross-compilers we care about
*   build the board "amd64-host" (an old name for the sdk) from scratch
*   install all the cross-compilers into the new board root
*   generate standalone copies of the cross-compilers for use by the
            Simple Chrome workflow (among other things)
*   verify the freshly built sdk works by building a selection of boards
*   upload all the binary packages, cross-compilers, and the sdk itself

### Gentoo Stage3

We grab a copy of the Gentoo stage3 tarball and post it to the chromiumos-sdk
Google Storage bucket.

The version of the last known good stage3 is stored in:

```none
[src/third_party/chromiumos-overlay/chromeos/binhost/host/sdk_version.conf](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/master/chromeos/binhost/host/sdk_version.conf)
BOOTSTRAP_LATEST_VERSION="2013.01.30"
```

This is used to look up the tarball in the chromiumos-sdk Google Storage bucket.
So with the information above, we know to fetch the file:

```none
<https://commondatastorage.googleapis.com/chromiumos-sdk/stage3-amd64-2013.01.30.tar.xz>
```

We simply unpack it and then chroot into it using our build scripts.

### Update Base Packages

Now we run the
[make_chroot.sh](https://chromium.googlesource.com/chromiumos/platform/crosutils/+/master/sdk_lib/make_chroot.sh)
script (found in src/scripts/sdk_lib/) to handle installing/removing/upgrading
key packages. It is very careful in making sure to install, remove, and upgrade
packages in the right order to avoid conflicts and circular dependencies. We can
also add various intermediate hacks as the versions of packages we upgrade in
our overlays drift further away from the ones found in the stage3, although it's
preferable to just update the base stage3 version when possible. See the section
at the end of this page for more information.

It then runs the normal
[update_chroot](https://chromium.googlesource.com/chromiumos/platform/crosutils/+/master/update_chroot)
script which installs all of the packages that normally exist in the SDK (the
[virtual/target-sdk](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/master/virtual/target-sdk)
meta package).

### Build Cross-compilers

The cbuildbot process will then run
[cros_setup_toolchains](https://chromium.googlesource.com/chromiumos/chromite/+/master/scripts/cros_setup_toolchains.py)
to generate all toolchains marked for inclusion in the SDK (see the
toolchain.conf file for more details).

### Build amd64-host

Now that we have a chroot that pretty much looks and acts like a Chromium OS
SDK, we can go ahead and build the Chromium OS SDK from scratch. We do this to
verify the latest native toolchain is sane, and so that we can produce a
pristine SDK without random garbage accumulating over time (like all the
temp/generated files found in the current chroot).

This step is accomplished by just running:

```none
./setup_board --board=amd64-host --skip_chroot_upgrade --nousepkg --reuse_pkgs_from_local_boards
```

If you're used to the normal Chromium OS flow, be aware that this step is really
like running setup_board and build_packages in one go. Arguably it shouldn't be
doing this, but it doesn't matter enough today to get around to cleaning it up.

This means it'll take quite a long time for this to finish (as it has to build a
few hundred packages from source). Everything will be written to
`/board/amd64-host`.

### Package The SDK

Now that the SDK is compiled and has everything we want, we create the SDK
tarball. We don't upload it just yet though as it hasn't been tested.

### Generate Standalone Cross-compilers

Since our cross-compilers are pretty cool & useful, we want to be able to use
them all by themselves. In other words, without the overhead of the full
Chromium OS source checkout and the SDK. With a few tricks, we can accomplish
exactly that. It copies all the host libraries the toolchain itself uses,
generates wrapper scripts so that the local copies of libraries can be found,
and then takes care of munging all the paths to make them standalone.

The
[cros_setup_toolchains](https://chromium.googlesource.com/chromiumos/chromite/+/master/scripts/cros_setup_toolchains.py)
script has all the logic.

### Test The SDK

We want to be sure that the SDK we just built is actually sane. This helps us
from releasing a toolchain update (like gcc) that is horribly broken (like can't
properly compile or link things). To test this, we simply use the SDK tarball
built earlier to create a new chroot. This also verifies the normal developer
workflow of creating a new chroot from this SDK.

Inside of the new chroot, we do a normal build flow for a couple of boards. At
the moment, that means one for each major architecture (i.e. amd64-generic-full,
arm-generic-full, and x86-generic-full). We only run setup_board+build_packages
though; no unittests or anything else (as current history as shown it to not be
necessary).

### Publish The SDK

Once all the tests pass, we go ahead and upload the results:

*   the SDK tarball
*   the standalone cross-compiler tarballs
*   binary packages for the chroot itself

## Upgrading The Stage3

The stage3 needs to be refreshed from time to time. It's a largely mechanical
process. It boils down to:

*   select a new stage3 from [upstream
            Gentoo](http://www.gentoo.org/main/en/where.xml) (look at the amd64
            stage3s)
*   make sure it's compressed with xz and not bzip2
*   upload it to the [chromiumos-sdk
            bucket](https://cloud.google.com/console#/storage/chromiumos-sdk/)
*   update the bootstrap version in the
            [src/chromiumos-overlay/chromeos/binhost/host/sdk_version.conf](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/master/chromeos/binhost/host/sdk_version.conf)
            file
*   run some chromiumos-sdk remote trybot configs
*   review the InitSDK stage log and look for packages that get
            downgraded ("\[ebuild UD \]" lines)
*   [post package upgrades for all the relevant
            packages](/chromium-os/gentoo-package-upgrade-process)
*   update the
            [make_chroot.sh](https://chromium.googlesource.com/chromiumos/platform/crosutils/+/master/sdk_lib/make_chroot.sh)
            script as needed (clean out old hacks, add new ones, etc...)
*   repeat process until the InitSDK stage works
*   once the whole bot config passes, your works is done