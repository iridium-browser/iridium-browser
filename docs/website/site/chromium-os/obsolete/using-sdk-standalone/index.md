---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/obsolete
  - Obsolete Documentation
page_name: using-sdk-standalone
title: Using Chromium OS SDK as standalone, DIY
---

[TOC]

This page describes how to use the Chromium OS SDK as standalone package,
without anything on top.

## Target audience

*   Developers who want to experiment with the Chromium OS binary
            enviroment, but not build any of Chromium OS
*   Developers who want just the toolchain
*   People who need the chroot, but do not want to download any sources

For the most part, you should know what you're doing to go this way.

## Provided by the SDK

*   Complete system tools, just like in the normal SDK chroot
*   Complete toolchain packages for:
    *   host (no prefix or x86_64-cros-linux-gnu-)
    *   all currently supported targets
        *   armv7a-cros-linux-gnueabi-
        *   x86_64-cros-linux-gnu-
        *   i686-pc-linux-gnu-
    *   gold and bfd ld

NOTE: It is not possible to switch toolchains or install new ones, as well as
generally update the system in any way. If you want a newer version, delete all
and start over.

## Installation instructions

### Getting the SDK

The latest version of SDK can be located in chromiumos-overlay.git repository,
in
[sdk_version.conf](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/chromeos/binhost/host/sdk_version.conf)

The individual SDK tarballs can be found in the Google Storage mirror:
<http://commondatastorage.googleapis.com/chromiumos-sdk/>

In order to download the latest SDK automatically, you can execute:

```none
eval $(curl -s https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/chromeos/binhost/host/sdk_version.conf?format=TEXT | base64 -d | grep '^SDK_LATEST_VERSION=')
wget "https://commondatastorage.googleapis.com/chromiumos-sdk/cros-sdk-${SDK_LATEST_VERSION}.tar.xz"
```

### Unzipping & entering SDK

Unzip the SDK to a folder of your choice, for example "./sdk":

```none
mkdir sdk
cd sdk
tar xvf ../cros-sdk-${SDK_LATEST_VERSION}.tar.xz
```

Set up some mounts:

```none
mount -t proc proc ./proc
mount -t sysfs sysfs ./sys
```

Enter by using chroot:

```none
sudo chroot . /bin/bash
```

### Optional: setting up various environment bits prior to chroot

These are mostly optional and depend on what do you want to do inside the
chroot. For merely running gcc, you should need neither.

1.  Mimic host network configuration:
    `sudo cp /etc/resolv.conf /etc/hosts sdk/etc/`
    This usually needs to be done only once unless your host's network is
    dynamic.
2.  Bind /dev, if you're going to be working with any devices:
    `sudo mount -o bind /dev sdk/dev`
3.  Make a copy or bind-mount of any configuration/data you will need
            inside the chroot.

Remember to clean up the chroot (especially) from mounts after you stop using
it.
