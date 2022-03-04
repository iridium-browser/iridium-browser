---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: kernel-configuration
title: Kernel Configuration
---

## [TOC]

## Overview

Kernel configuration is a little complicated in Chromium OS. There are a number
of different kernels based on architecture (x86_64, arm64) variants (generic,
rockchip) and platforms (Seaboard, Mario). We want to split the config into a
sort of hierarchy so that the common options are shared between the variants
(click to enlarge):

[<img alt="image"
src="/chromium-os/how-tos-and-troubleshooting/kernel-configuration/kernel_config_diag.png"
height=274
width=400>](/chromium-os/how-tos-and-troubleshooting/kernel-configuration/kernel_config_diag.png)

Shown here are 6 flavours, three for ARM and three for Intel (it is not clear
why flavours is spelt the British way).

## Conversions

We need to be able to do the following:

*   create a .config file for a particular flavour - this is done simply
            by concatenating the three levels for a particular flavour. For
            example we create the tegra2 flavour by concatenating base.config,
            armel/common.config and armel/chromeos-tegra2.flavour.config. This
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

This script deals with each separate flavour in turn, performing the selected
operations. It starts by building the config for each flavour, does the
operations, and afterwards it splits the flavour configs back into the proper
hierarchy. It supports four operations

*   oldconfig - does a 'make oldconfig' on each config
*   olddefconfig - like 'oldconfig', but it accepts the default
            selections automatically
*   editconfig - does a 'make menuconfig' on each config
*   genconfig - like oldconfig, but it leaves the generated full config
            files lying around afterwards in the CONFIGS directory

There is a bit of a problem with kernelconfig. If you want to change a kernel
option which is common to all flavours, for example, then you must change it
once for each flavour using 'kernelconfig editconfig'. At the moment this is ~10
times! If you miss one, then the result may be no change which can be very
confusing. See below for another approach.

### prepareconfig

This is used to prepare the config for a particular flavour. It assembles the
default config for the flavour. This is useful when starting development on a
board and you want to build in-tree:

```none
./chromeos/scripts/prepareconfig chromeos-tegra2
export ARCH=arm
export CROSS_COMPILE=armv7a-cros-linux-gnueabi-
make olddefconfig    #(to pick up new options, if any)
make
...
```

This script is used in the emerge. An example resulting string is 'Ubuntu
2.6.32-0.1-chromeos-tegra2'.

## Using the kernelconfig script

The kernel config is split into several config files - generic, chrome-specific,
architecture-specific and board specific. Automatic tools split and combine
these configs.

If you want to edit the configuration for a particular board etc. then you need
to do something like:

```none
$ cd .../src/third_party/kernel/v$VER/
$ ./chromeos/scripts/kernelconfig editconfig
```

The exact value for $VER depends on your board as each one is different. You can
find that out by seeing what version by running `` `emerge-$BOARD -pv
virtual/linux-sources` ``.

This will run 'make menuconfig' for each part of the config.

You need to figure out which part your config options appear in, and then make
the changes when that config comes up. When it says 'press a key', you have to
press &lt;enter&gt;.

## Make a change to the config

Run editconfig to modify each config appropriately using menuconfig and then
regenerate the splitconfigs. The editconfig script will run "make menuconfig" on
each flavor config. Please make appropriate changes for each menuconfig run and
the script will generate the correct splitconfig.

```none
cd /path/to/kernel
chromeos/scripts/kernelconfig editconfig # regenerate splitconfig
```

## Alternative Method to Edit Config

This is a little faster if you know what you're doing:

*   Look for the kernel config option you want to edit

```none
$ grep -rs CONFIG_NETWORK_FILESYSTEMS chromeos/config/
chromeos/config/base.config:# CONFIG_NETWORK_FILESYSTEMS is not set
```

*   Now edit that file `chromeos/config/base.config` to change the
            config like this:

<pre><code>
# CONFIG_NETPOLL_TRAP is not set
<s># CONFIG_NETWORK_FILESYSTEMS is not set</s>
CONFIG_NETWORK_FILESYSTEMS=y
CONFIG_NETWORK_SECMARK=y
# CONFIG_NET_9P is not set
</code></pre>

*   Now run this to recreate all the configs based on your changes

```none
$ chromeos/scripts/kernelconfig olddefconfig
```

*   This will accept the default choices for any new or otherwise
            unspecified options. If needed you can change them by editing the
            file on your next run around this method.
*   Rinse and repeat

If you find that during that last step, your configs are disappearing, then
you've likely enabled a config without enabling all of its dependencies. You'll
need to either 1) track down those dependencies and enable them as well, or 2)
Use the editconfig method mentioned above.

## Comparing in-tree config to split config

This might not be useful, but:

```none
# get full flavour config sorted, without leading #
FLAVOUR=chromeos-tegra2 cat chromeos/config/base.config \
   chromeos/config/armel/common.config \
   chromeos/config/armel/$FLAVOUR.flavour.config | \
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
./chromeos/scripts/prepareconfig chromeos-tegra2
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
cp .config chromeos/config/<arch>/<flavour>.flavour.config
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
cp <single config> chromeos/config/<arch>/<flavour>.flavour.config
chromeos/scripts/kernelconfig olddefconfig
chromeos/scripts/prepareconfig <flavour>
make ARCH=${ARCH} oldconfig
diff .config <single config>
```