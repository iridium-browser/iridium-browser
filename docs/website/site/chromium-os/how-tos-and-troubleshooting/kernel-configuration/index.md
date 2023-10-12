---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: kernel-configuration
title: Kernel Configuration
---

[TOC]

## Overview

Kernel configuration is a little complicated in Chromium OS. There are a number
of different kernels based on family (chromeos, manatee, termina),
architecture (x86_64, arm64), variants (generic,
rockchip) and platforms (Seaboard, Mario). We want to split the config into a
sort of hierarchy so that the common options are shared as shown in this figure
(click to enlarge):

[<img alt="image"
src="/chromium-os/how-tos-and-troubleshooting/kernel-configuration/kernel_config_diag.png"
height=274
width=400>](/chromium-os/how-tos-and-troubleshooting/kernel-configuration/kernel_config_diag.png)

Shown here is one family (chromeos) with 6 flavours, three for ARM and three for
Intel (it is not clear why flavours is spelt the British way).

The sharing of common config options makes it easier to maintain the variants.

## Conversions

We need to be able to do the following:

*   create a .config file for a particular flavour - this is done simply
            by concatenating the three levels for that flavour. For
            example, we create the tegra2 flavour by concatenating `base.config`,
            `armel/common.config` and `armel/chromeos-tegra2.flavour.config`. This
            .config file is suitable for configuring the kernel.
*   split a .config into its constituent parts. This is a little more
            complicated and has to be done for all flavours at once. Given a
            .config file for each flavour we can look for items which are common
            to all flavours - these can be put into the family config. We also
            look for everything common within each architecture - these are put
            into the architecture files like config.common.arm. The splitconfig
            script does this.

## Scripts

Scripts to deal with the kernel configuration are in the chromeos/scripts
directory. A partial listing is shown below:

```none
$ ls chromeos/scripts/
kernelconfig  prepareconfig
```

### kernelconfig

This script operates on all flavours at once.
It builds the config for each flavour, performs the requested
operations, and splits the configs back into the proper
hierarchy. It supports four operations:

*   **oldconfig** - runs a 'make oldconfig' on each config
*   **olddefconfig** - like 'oldconfig', but it accepts the default
            selections automatically
*   **editconfig** - runs a 'make menuconfig' on each config
*   **genconfig** - like oldconfig, but it leaves the generated full config
            files lying around afterwards in the CONFIGS directory

There is a bit of a problem with kernelconfig. If you want to change a kernel
option which is common to all flavours, for example, then you must change it
once for each flavour using 'kernelconfig editconfig'. At the moment this is ~10
times! If you miss one, then the result may be no change, which can be very
confusing. See below for another approach.

### prepareconfig

This is used to prepare the config for a particular flavour. It assembles the
default config for the flavour. This is useful when starting development on a
board and you want to build in-tree:

```none
CHROMEOS_KERNEL_FAMILY=chromeos ./chromeos/scripts/prepareconfig chromeos-tegra2
export ARCH=arm
export CROSS_COMPILE=armv7a-cros-linux-gnueabi-
make olddefconfig    #(to pick up new options, if any)
make
...
```

prepareconfig requires an environment variable `CHROMEOS_KERNEL_FAMILY` to
correctly determine the split config. The above example uses `chromeos` as the
kernel family. Each supported kernel family's split config is maintained in
`chromeos/config/<family>/` directory.

This script is used in the emerge. An example resulting string is 'Ubuntu
2.6.32-0.1-chromeos-tegra2'.

## Using the kernelconfig script

The kernel config is split into several config files - generic, chrome-specific,
architecture-specific and board specific. Automatic tools split and combine
these configs.

If you want to edit the configuration for a particular board, then you need
to do something like:

```none
$ cd .../src/third_party/kernel/v$VER/
$ ./chromeos/scripts/kernelconfig editconfig
```

The value for $VER depends on the board. You can
find what kernel version a board uses by running `` `emerge-$BOARD -pv
virtual/linux-sources` ``.

The `editconfig` command runs `make menuconfig` for all existing
kernel configurations.  You need to figure out which part your config options
appear in, make the changes, save the file, and exit.

## Alternative Method to Edit Config

This is faster, if you know what you're doing:

*   Search the split configs for the kernel config option you want to edit.

    ```none
    $ grep -rs CONFIG_LEGACY_PTYS  chromeos/config/
    chromeos/config/chromeos/base.config:# CONFIG_LEGACY_PTYS is not set
    chromeos/config/termina/base.config:# CONFIG_LEGACY_PTYS is not set
    ```

*   Now assuming that we are working with chromeos kernel family, edit the file
    `chromeos/config/chromeos/base.config` to change the config like this:

    ```
    CONFIG_LEDS_CLASS=y
    <s># CONFIG_LEGACY_PTYS is not set</s>
    CONFIG_LEGACY_PTYS=y
    CONFIG_LKDTM=y
    ```

*   Now run this to recreate all the configs based on your changes:

    ```none
    $ chromeos/scripts/kernelconfig olddefconfig
    ```

*   This will accept the default choices for any new or otherwise
    unspecified options.

*   Use `git diff` to ensure that the result is as expected.  You may
    repeat editing the files and executing `kernelconfig` as needed.

If you find that during that last step, your configs are disappearing, then
you've likely enabled a config without enabling all of its dependencies. You'll
need to either 1) track down those dependencies and enable them as well, or 2)
Use the editconfig method mentioned above.

## Comparing in-tree config to split config

This might not be useful, but:

```none
# get full flavour config sorted, without leading #
$ FLAVOUR=chromeos-tegra2
$ cat chromeos/config/*/base.config \
   chromeos/config/*/armel/common.config \
   chromeos/config/*/armel/$FLAVOUR.flavour.config | \
   grep CONFIG_ | sed "s/.*CONFIG/CONFIG/" | sort -u >1.emerge
# purify .config in the same way
```

```none
make ARCH=${kernarch} savedefconfig
```

```none
# compare
diff 1.emerge defconfig
```

## Manual Building

Here is how to build the kernel yourself manually, from within the chroot.

Make sure you have done something like:

```none
export B="tegra2_seaboard"
export ARCH=arm
export CROSS_COMPILE=armv7a-cros-linux-gnueabi-
# indicate that you are making kernel changes
```

```none
cros_workon --board=$B start sys-kernel/chromeos-kernel-$VER
```

The exact value for $VER depends on your board as each one is different. You can
find that out by seeing what version by running `` `emerge-$BOARD -pv
virtual/linux-sources` ``.

As you probably know, the `cros_workon start` tells the build system to use your
local source install of the official source. Then set up the config:

```none
# set up the .config file
CHROMEOS_KERNEL_FAMILY=<kernel-family>  ./chromeos/scripts/prepareconfig chromeos-tegra2
make olddefconfig
# edit config
make menuconfig
# (make your changes)
```

Now, after you make a kernel change, you can simply type `make` as normal:

```none
# build the kernel and modules
make
# or maybe: build the uImage on an 8 core machine
make -j10 uImage
# or both!
make && make uImage
```

### Generate a new splitconfig after adding new Kconfig options

Run olddefconfig to regenerate the splitconfigs. The olddefconfig script will
run "make olddefconfig" on each flavor config.

```none
cd ~/trunk/src/third_party/kernel/v$VER
chromeos/scripts/kernelconfig olddefconfig # regenerate splitconfig
```

### Adding a new config

Copy the new config to the appropriate place in the chromeos/config and then run
oldconfig

```none
cd ~/trunk/src/third_party/kernel/v$VER
cp config.flavour.chromeos-new .config
make ARCH=${ARCH} olddefconfig
cp .config chromeos/config/<kernel-family>/<arch>/<flavour>.flavour.config
chromeos/scripts/kernelconfig olddefconfig # regenerate splitconfig
```

### How can I find the list of flavour configs

```none
cd ~/trunk/src/third_party/kernel/v$VER
find chromeos/config -name \*.flavour.config
```

### How to regenerate and validate splitconfigs from single config

```none
cd ~/trunk/src/third_party/kernel/v$VER
cp <single config> chromeos/config/<kernel-family>/<arch>/<flavour>.flavour.config
chromeos/scripts/kernelconfig olddefconfig
CHROMEOS_KERNEL_FAMILY=<kernel-family>  chromeos/scripts/prepareconfig <flavour>
make ARCH=${ARCH} oldconfig
diff .config <single config>
```
