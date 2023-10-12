---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: quick-start-guide
title: Quick Start Guide
---

## Introduction

Welcome to Chromium OS. This document serves as a quick start guide to
installing your own Chromium OS image on a device. For more detailed
information, or if steps in this quick start guide don't work for you, please
refer to the [Chromium OS Developer's
Guide](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/developer_guide.md).

### Prerequisites

You should have

*   [Ubuntu](http://www.ubuntu.com/) Linux (version == 16.04 - Xenial)
    *   This is the only officially support distro, but building
                Chromium OS should work fine on any x86_64 Linux distro running
                a 2.6.16+ kernel

*   a 64-bit system for performing the build

*   an account with sudo access

1.  Install the git and subversion revision control systems, the curl
            download helper, and lvm tools. On Ubuntu, the magic incantation to
            do this is:

    ```none
    sudo apt-get install git-core gitk git-gui subversion curl lvm2 thin-provisioning-tools python-pkg-resources python-virtualenv python-oauth2client
    ```

2.  You must also [install
            depot_tools](/developers/how-tos/install-depot-tools). This step is
            required so that you can use the repo command to get/sync the source
            code.
3.  You must also [tweak your sudoers configuration](http://www.chromium.org/chromium-os/tips-and-tricks-for-chromium-os-developers#TOC-Making-sudo-a-little-more-permissive). This is required for using cros_sdk.
4.  **NOTE**: Do not run any of the commands listed in this document as
            root – the commands themselves will run sudo to get root access when
            needed.

Get the Source ([full version](/chromium-os/developer-guide#TOC-Get-the-Source))

Create a directory to hold the source, "`${SOURCE_REPO}`". This **should not**
be installed on a remote NFS directory.

```none
cd ${SOURCE_REPO}
repo init -u https://chromium.googlesource.com/chromiumos/manifest.git
# Optional: Make any changes to .repo/local_manifests/local_manifest.xml before syncing
repo sync
```

Build Chromium OS ([full
version](http://www.chromium.org/chromium-os/developer-guide#TOC-Building-Chromium-OS))

At this point, you’ll have to know the `${BOARD}` you would like to build on. If
you don't have a specific target in mind, amd64-generic is a good starter board
as it’s compatible with most 64-bit x86_64 systems. cros_sdk works from any path
with `${SOURCE_REPO}` so make sure you are within the source tree before running
these commands.

```none
export BOARD=amd64-generic
cros build-packages --board=${BOARD}
```

This will take a long time the first time as it sets up your build environment.
Subsequent invocations will take much less time.

Finally, with all the packages built, we are ready to build an image that can be
installed on your device. To do so run:

```none
cros build-image --board=${BOARD}
```

Now copy this image onto a usb drive. Insert the usb stick you’d like to use and
run:

```none
cros flash --board=${BOARD} usb://
```

This will prompt you for which usb device you’d like to use. (Note that
auto-mounting of USB devices should be turned off as it may corrupt the disk
image while it's being written.)

## Install Chromium OS on your Device ([full version](http://www.chromium.org/chromium-os/developer-guide#TOC-Installing-Chromium-OS-on-your-Device))

Now you’re ready to install this image on your device. You’ll need to setup your
device to boot from USB.

*   On a non-chromebook, set your system to boot from a usb drive using
            instructions specific to your device.
*   On a chromebook, [enter developer-mode for your specific type of
            hardware](/chromium-os/developer-information-for-chrome-os-devices).
            For Samsung / Acer devices, you can now boot from your usb image
            using Ctrl+U on the developer mode screen. For the CR-48, you’ll
            have to follow the instructions to [build a recovery
            image](/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information)
            and install from it using those same instructions.

With that done, hit Ctrl+Alt+Back (F2). At the prompt type chronos and install
using the following command:

```none
/usr/sbin/chromeos-install
```

#### Next steps

*   Read the full [Chromium OS Developer's
            Guide](/chromium-os/developer-guide).
*   Errors during quick start? Send an email to
            [chromium-os-dev@chromium.org](mailto:chromium-os-dev@chromium.org)
            or start a dialogue on our IRC channel
            [irc.freenode.net/#chromium-os-users](http://irc.freenode.net/#chromium-os-users)
