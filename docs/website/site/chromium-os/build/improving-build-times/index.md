---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/build
  - Chromium OS Build
page_name: improving-build-times
title: Improving Build Times
---

[TOC]

A Chromium-based OS can take a long time to build. However, you can perform many
optimizations to speed up this process. Currently, the Chromium OS build system
is driven by bash scripts that don't detect what items have changed. If you
blindly use these scripts, you will be overbuilding and wasting precious time.

This page goes through each of the bash(or python) scripts, explaining what they
do and when you have to run them. Another source of information is the -h option
for each script.

## cros_sdk

This script creates the local chroot environment using the a prebuilt SDK chroot
and prebuilt packages. Use the `--replace` option to replace a previous chroot
environment (re-running this command will not delete your previous chroot). Once
it has created the local chroot environment, `cros_sdk` will also chroot into
the SDK and set up basic enviroment. If you prefer to just build and not enter,
please use the --download option. In terms of what you see, this is the
development environment that all packages see when they are building. This
script resides in chromite, but should be accessible in your PATH if you have
recent enough depot_tools.

## build_chrome.sh

This script builds your Chromium source for use with Chromium OS (special flags
and environment changes are necessary for this to happen correctly). It then
packages Chromium into a ZIP file that is used in build_image.sh.

This script must be run outside of the chroot environment and needs to be re-run
whenever major Chromium changes happen. Since building Chromium takes a long
time, unless you are making changes to Chromium, avoid syncing and rebuilding
whenever possible. This script is based around the build process of Chromium and
so it is more intelligent and will only build what is necessary. You can also
test the resulting executable without running it on a Chromium OS system.

## build_platform_packages.sh

This script does most of the building. It runs through all the directories in
third_party and under platform and runs their make_pkg.sh script. make_pkg.sh
builds the .deb file for that package to be installed into the final Chromium OS
image.

If you only make changes to one package, you only need to call its make_pkg.sh
(from within chroot of course) and copy/install that deb to your Chromium OS
device. This will save you a lot of time and prevent from having to rebuild the
image every time. You will need to modify this script if you add any packages
yourself.

## build_kernel.sh

This scripts builds the kernel.

## build_image.sh

Builds the Chromium OS image. This script goes through all the packages in the
~/src/build/x86/local_packages and all the other packages listed in the prod
text file in package_repo to create the Chromium OS image. The image is then
placed under ~/src/build/images. From here you can convert the image to a VMWare
image or install it to a USB drive.