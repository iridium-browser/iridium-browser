---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/ec-development
  - Chromium Embedded Controller (EC) Development
page_name: getting-started-building-ec-images-quickly
title: Get Started Building EC Images (Quickly)
---

The [Chromium OS Developer Guide](/chromium-os/developer-guide) and [EC
Development
Guide](https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/README.md)
walk through the steps needed to fetch and build Chromium OS source. These steps
can be followed to retrieve and build EC source as well. On the other hand, if
your sole interest is building an EC image, the general developer guide contains
some extra unneeded steps.

The fastest possible way to build an EC image is to skip the Chromium OS chroot
install entirely. The following steps have been tested on an Ubuntu 15.10 (Wily
Werewolf) 64-bit host machine. Other distros / versions may be used, but
toolchain incompatibilities may require extra debug.

1.  Install build / dev tools: *sudo apt-get install git libftdi-dev
            libusb-dev libncurses5-dev gcc-arm-none-eabi*
2.  Sync the cros-ec git repo: *git clone
            https://chromium.googlesource.com/chromiumos/platform/ec*
3.  Build your EC image: **HOSTCC=x86_64-linux-gnu-gcc* make
            BOARD=$board*

Most boards are buildable, but some will fail due to dependencies on external
binaries (such as futility **\[1\]**). Also, some related tools (such as
flash_ec and servod) must be run from the Chromium OS chroot. Here is a set of
steps to setup a minimal development environment to build EC images from the
Chromium OS chroot:

1.  Create a folder for your chroot (ex. *mkdir cros-src; cd cros-src*)
2.  Run *repo init -u
            https://chromium.googlesource.com/chromiumos/manifest.git --repo-url
            https://chromium.googlesource.com/external/repo.git -g minilayout*
3.  Edit .repo/manifest.xml, and add *groups="minilayout"* to the
            platform/ec project, so the line becomes *&lt;project
            path="src/platform/ec" name="chromiumos/platform/ec"
            groups="minilayout" /&gt;*
4.  *repo sync -j &lt;number of cores on your workstatsion&gt;*
5.  *./chromite/bin/cros_sdk* and enter your password for sudo if
            prompted
6.  `cros build-packages --board=$BOARD chromeos-ec`
7.  Now, EC images for any board can be built with
    `cd ~/chromiumos/src/platform/ec; make BOARD=$board -j`

---

**\[1\]** if you want to build the 'futility' host tool outside the normal
Chrome OS chroot self-contained environment, you can try the following:

1.  install futility build dependencies: *`sudo apt-get install uuid-dev
            liblzma-dev libyaml-dev libssl-dev`*
2.  get the vboot reference sources: *`git clone
            https://chromium.googlesource.com/chromiumos/platform/vboot_reference`*
3.  build it: *`cd vboot_reference ; make`*
4.  install it in /usr/local/bin: *`sudo make install`*
5.  add /usr/local/bin to your default PATH: *`export
            PATH="${PATH}:/usr/local/bin"`*
