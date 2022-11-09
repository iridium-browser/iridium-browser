---
breadcrumbs:
- - /developers
  - For Developers
page_name: u-boot
title: U-Boot
---

[TOC]

### Introduction

Das U-Boot is the boot loader used by Chromium OS on ARM. This page provide some
basic information about our use of it. See the [official U-Boot
page](http://www.denx.de/wiki/U-Boot) for more information.

### Variants

Chrome OS uses a single variant of U-Boot for all Tegra2 boards and all
purposes. The same applies for Exynos, although this is a different image from
Tegra2. This is a departure from the previous approach of having different
variants for each purpose (stub, developer, recovery, normal, flasher) and each
board (Seaboard, Aebl, Kaen).
Both of these changes are enabled by run-time configuration in U-Boot.
Traditionally, U-Boot's configuration has been set by `CONFIG_` options. We
still use `CONFIG_` options to select which functionality / drivers is included
in an image. However, we avoid using `CONFIG_` options that simply configure
that functionality. For example, `CONFIG_USB_EHCI` is used to include the EHCI
functionality for USB, but we do not use `CONFIG_TEGRA2_USB0`to specify the
physical address of the first USB port.
We use *flattened device tree* (FDT) to hold this run-time configuration. FDT
consists of a number of nodes each with a list of properties. It is easy to add
new nodes and properties, and a **libfdt** library exists for reading and
writing them. For more information about FDT, [see the specification
document](https://www.power.org/resources/downloads/Power_ePAPR_APPROVED_v1.0.pdf)
before you continue reading here. You might also find the U-Boot
[doc/README.fdt-control](http://git.denx.de/cgi-bin/gitweb.cgi?p=u-boot.git;a=blob;f=doc/README.fdt-control;h=85bda035043497d8d23aed8a436e24d630c60937;hb=2c734cd932b53b46e9f89e4f5db9360afc459ae6)
file useful.
***Note*:** as of March 2012 we still use '**seaboard**' (rather than
'**generic**') as the name of our generic Tegra2 board. This may change in the
future.

### FDTs

We have various types of nodes in the **FDT** and they can be categorized as
follows:

*   **SOC peripherals** - describe an SOC peripheral
    *   includes its physical address and other properties needed by the
                driver.
    *   For example, *usb@0xc5000000* is a USB peripheral located at
                0xc5000000, which includes a 'host-mode' property indicating
                that this port should be configured to run in host mode.
*   **Board peripherals** - describe a chip on the board, in a similar
            way to an SOC peripheral
    *   Normally, these are linked to an SOC peripheral using a phandle.
    *   For example, the *lcd* node describes the LCD attached to the
                board (its resolution, pixel clock, timings, backlight GPIO,
                etc.) and then links to the display peripheral (e.g.,
                *display1*).
*   **Aliases** - assign a new name to a peripheral instance
    *   Some peripherals have multiple instances. It is often useful to
                assign an order to the instances. This is done by numbering the
                nodes within the aliases section.
    *   For example, *usb0* and *usb1* each point to a USB node - this
                tells U-Boot that *usb0* (which might point to */usb@c5008000*)
                is the first USB port, and *usb1* is the second.
    *   Another example is the *console* alias which points to the UART
                to be used as the U-Boot console.
*   **config** - generic machine configuration information
    *   For example, we use it to hold the Linux machine ID.
*   **chromeos-config** - this is Chrome OS-specific configuration.
    *   We store the start address of U-Boot (*textbase*) and the type
                of verified boot we are using (*twostop*).

FDT-based configuration is enabled with the `CONFIG_OF_CONTROL` option. FDT
files are stored in board/nvidia/dts or board/samsung/dts.
FDT can be packaged with U-Boot in two ways:

*   `CONFIG_OF_EMBED` (only for debugging / development) - it is built
            as an object file and linked to U-Boot
*   `CONFIG_OF_SEPARATE` - it is built separately and then appended to
            the U-boot binary (u-boot.bin) using cat or similar. U-Boot locates
            it at start-up.

We use the second approach for Chrome OS, since it allows cros_bundle_firmware
to create an image for any board without needing to look around inside U-Boot to
change the FDT.

### fdtdec

Initially, we used a library called fdt_decode to decode FDT blobs on behalf of
different classes of drivers. That approach may become partially useful in
future, but we decided to drop it for upstream. There is a still an fdtdec
library which helps a lot, but drivers must decode their own nodes.

### Header Files

U-Boot uses header files for configuration. Since the FDT change this has become
very simple. We have:

*   *chromeos_seaboard_config* - top-level file for verified boot. It
            simply includes *seaboard.h* and *chromeos.h*.
*   *chromeos_daisy_config* - equivalent to the above for exynos.
*   *seaboard.h* - Seaboard-specific config, but, in fact, is used for
            all Tegra2 boards which use FDT for config. It includes
            *tegra2-common.h*. Most of this file is hidden behind '`#ifdef
            CONFIG_OF_CONTROL`' since it is not needed with FDT-based
            configuration.
*   *tegra2-common.h* - options common to all Tegra2 boards.
*   *chromeos.h* - options to enable verified boot. In particular
            `CONFIG_CHROMEOS` and `CHROMEOS_BOOTARGS` are defined here.

### Profiles

***TODO***: Add description of cros_choose_profile to a more generic page when
one is created.
In order to simplify the handling of different boards and board variants, a
`cros_choose_profile` tool is used:

```none
cros_choose_profile --help
USAGE: /usr/bin/cros_choose_profile [flags] args
flags:
  --board:  The name of the board to set up. (default: 'tegra2_seaboard')
  --build_root:  The root location for board sysroots. (default: '/build')
  --board_overlay:  Location of the board overlay. (default: '')
  --variant:  Board variant. (default: '')
  --profile:  The portage configuration profile to use. (default: '')
  -h,--[no]help:  show this help (default: false)
```

The `cros_choose_profile` utility selects which board and profile we are using.
It is normally run by the `setup_board` script.

### Ebuilds

The main U-Boot-related ebuilds are in
`src/third_party/chromiumos-overlay/sys-boot`:

*   `chromeos-u-boot`
    *   Builds all the variants of U-Boot by default. If you only want
                one you can build with a `USE` flag, such as `USE='developer
                emerge-tegra2_seaboard' chromeos-u-boot`.
    *   Each variant is built into its own subdirectory in the temporary
                build directory: developer, recovery, etc.
    *   The variants are then installed into the
                `chroot/build/${BOARD}/firmware` directory, named
                `u-boot-normal.bin`, `u-boot-developer.bin`, etc. For Seaboard
                this directory is `chroot/build/tegra2_seaboard/`firmware.
    *   Also installed is U-Boot's `include/autoconf.mk` and
                `System.map`, but only for the first variant built. These are
                used by `chromeos-bios` (see later).
    *   (note that `virtual/u-boot` pulls this ebuild in)
*   `tegra-bct-${BOARD}`
    *   *(This is for Tegra only. We would prefer to build these files
                in cros_bundle_firmware instead, as is now done for FDTs.)*
    *   There is one of these for each board and they provide the
                `vritual/tegra-bct` package. This ebuild selects configuration
                files for Flash (SPI or NAND) and SDRAM timings as required.
    *   Since this ebuild inherits `tegra-bct`, these functions will be
                used for building. See
                `src/overlays/overlay-tegra2/eclass/tegra-bct.eclass`
    *   The eclass functions set up a single configuration file
                consisting of the flash and SDRAM settings, then call
                `cbootimage` to turn these into a bct file.
    *   This bct file is installed into
                `chroot/build/${BOARD}/u-boot/bct` as `board.bct` and
                `board.cfg`.
*   `chromeos-bootimage`
    *   Run `cros_bundle_firmware` to create two firmware images for the
                board. This consists of U-Boot, FDT settings, recovery-mode
                screens and the required keys for performing a verified boot.

### Add New Boards to Ebuilds

The variables `U_BOOT_CONFIG_USE` and `U_BOOT_FDT_USE` are currently used to
specific the FDT file for U-Boot. These variables are set in the `make.defaults`
file for the board. For example, for tegra2 boards this is in
`src/overlays/overlay-tegra2/profiles/base/make.defaults`.

There is now no need to figure out the FDT or config in the ebuild - you can
just use these variables directly.

To check the settings for your board:

```none
emerge-tegra2_seaboard --info |tr ' ' '\n' |grep U_BOOT
```

## Building U-Boot

There are two main U-Boot repositories on the server: the stable `u-boot.git`
and the unstable `u-boot-next.git`. Normally you will use the stable repo.

### From ebuild

U-Boot is built from an ebuild like this:

```none
emerge-${BOARD} chromeos-u-boot
```

For example for the Seaboard:

```none
emerge-tegra2_seaboard  chromeos-u-boot
```

This builds it from scratch which takes about 40s on a fast machine.

### Incremental build

If you have run `cros_workon start chromeos-u-boot` (followed by `repo sync
u-boot`) then you can use the incremental build option which takes perhaps 12s.

```none
bin/cros_workon_make chromeos-u-boot
```

This is the recommended approach since it sets up the options for your board
automatically. If you look in the output of this command you should see the make
command it is using. If you really want to, you can use this command, which
takes the build time down to around 6s.

### Build script

NOTE: Advanced U-Boot coders only! This can reduce an incremental build to about
1s on a fast machine (16 core).
If you are working in `u-boot` and constantly flashing U-Boot for different
Tegra2 boards then you might find this script useful. Run it within your U-Boot
source tree. Beware this is not for beginners. Before you use this script, read
it through and compare it with the U-Boot ebuild. You can select an FDT to use
to change the board.
Note: this script could be enhanced to output the resulting u-boot binary to a
subdirectory with make ...O=&lt;target_dir&gt;.

```none
#! /bin/sh
# Script to build U-Boot without an ebuild and with upstream config files,
# then write it to SPI on your board.
#
# It builds for seaboard by default - use -b to change the board.
# For an incremental board, run with no arguments. To clean out and reconfig,
# pass the -c flag.
compiler=armv7a-cros-linux-gnueabi-
if grep -sq Gentoo /etc/lsb-release; then
    chroot=y
    # location of src within chroot
    SRC_ROOT=~/trunk/src
    gcc=gcc
else
    # Change this to point to your source dir
    SRC_ROOT=/c/cosarm/src
    BIN=${SRC_ROOT}/../chroot/usr/bin
    compiler=${BIN}/${compiler}
    export PATH=${BIN}:${PATH}
    gcc=$BIN/gcc
    echo $PATH
fi
. "${SRC_ROOT}/scripts/lib/shflags/shflags"
DEFINE_boolean config "${FLAGS_FALSE}" "Reconfigure and clean" c
DEFINE_boolean good "${FLAGS_FALSE}" "Use known-good U-Boot" g
DEFINE_string board "seaboard" \
    "Select config: chromeos_tegra2_<board>_<config>_config" b
DEFINE_boolean separate "${FLAGS_TRUE}" "Separate device tree" s
DEFINE_boolean verified "${FLAGS_FALSE}" "Enable verified boot" V
DEFINE_string dt "seaboard" \
    "Select device tree: seaboard, kaen, aebl" d
DEFINE_boolean build "${FLAGS_TRUE}" "Build U-Boot" B
# Parse command line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"
if [ "$FLAGS_build" -eq "${FLAGS_FALSE}" ]; then
    if [ "$FLAGS_config" -eq "${FLAGS_TRUE}" ]; then
        FLAGS_config=${FLAGS_FALSE}
        echo "Warning: disabled -c since -B was given"
    fi
fi
cd $SRC_ROOT/third_party/u-boot/files
target=all
if [ -n "${FLAGS_ARGV}" ]; then
  target=${FLAGS_ARGV}
fi
if [ "$FLAGS_config" -eq "${FLAGS_FALSE}" ]; then
    # Get the correct board for cros_write_firmware
    board=$(grep BOARD include/config.mk | awk '{print $3}')
    if [ -z "${FLAGS_board}" -a "$board" != "${FLAGS_board}" ]; then
        echo "Warning: U-Boot is configured for $board, " \
            "forcing reconfigure"
        FLAGS_config=${FLAGS_TRUE}
    fi
    FLAGS_board=${board}
fi
BASE+="-s "
BASE+="-j15 "
BASE+="ARCH=arm CROSS_COMPILE=${compiler} USE_PRIVATE_LIBGCC=yes "
BASE+="--no-print-directory "
BASE+="HOSTCC=$gcc HOSTSTRIP=true VBOOT=/build/tegra2_${FLAGS_board}/usr "
#BASE+="VBOOT_DEBUG=1 "
if  [ "$FLAGS_separate" -eq "${FLAGS_TRUE}" ]; then
    BASE+="DEV_TREE_SEPARATE=true "
fi
BASE+="DEV_TREE_SRC=tegra2-${FLAGS_dt} "
board=${FLAGS_board}
if [ "${FLAGS_verified}" -eq "${FLAGS_TRUE}" ]; then
        FLAGS_board=chromeos_tegra2_twostop
fi
# echo $BASE
if [ "$FLAGS_config" -eq "${FLAGS_TRUE}" ]; then
    make $BASE distclean
    make $BASE ${FLAGS_board}_config || { echo "Make failed"; exit 1; }
    make $BASE dep
fi
if [ "$FLAGS_good" -eq "${FLAGS_TRUE}" ]; then
    IMAGE=/build/tegra2_${FLAGS_board}/u-boot/u-boot-developer.bin
else
    if [ "$FLAGS_build" -eq "${FLAGS_TRUE}" ]; then
        make $BASE $target || { echo "Make failed"; exit 1; }
    fi
    IMAGE=u-boot.bin
fi
if  [ "$FLAGS_separate" -eq "${FLAGS_TRUE}" ]; then
    cat $IMAGE u-boot.dtb >u-boot.bin.dtb
    IMAGE=u-boot.bin.dtb
fi
bmp="/home/$USER/trunk/src/third_party/chromiumos-overlay/sys-boot/"
bmp+="chromeos-bootimage/files/default.bmpblk"
cros_bundle_firmware -w -u u-boot.bin -s -b tegra2_aebl --bootsecure \
    --bootcmd vboot_twostop --bmpblk ${bmp}
#--add-config-int silent_console 1
```

## Flashing U-Boot

The method here depends on the platform you are using. Please see the
documentation on [cros_bundle_firmware](/chromium-os/firmware-update) also.

### NVidia Tegra2x

There is a firmware writing utility available which can reflash U-Boot on your
board. For example, for Seaboard you can write the recovery U-Boot with:

```none
# Connect USB A-A cable to top USB port, reset with recovery button held down
If you have a T20-seaboard (no camera attachment on top), please build/flash your firmware with:
USE=tegra20-bct emerge-tegra2_seaboard tegra-bct chromeos-bootimage
cros_write_firmware -b tegra2_seaboard \
                    -i /build/tegra2_seaboard/u-boot/legacy_image.bin
If you have a T25-seaboard (black camera box above the screen), please build/flash your firmware with:
emerge-tegra2_seaboard tegra-bct chromeos-bootimage
cros_write_firmware -b tegra2_seaboard \
                    -i /build/tegra2_seaboard/u-boot/legacy_image.bin
```

The BCT controls early memory initialization on Tegra2 CPUs. It is something
that is customized for every variant of every board.

We need a different BCT for T20 and T25 Seaboards. Up until this point we have
just used an old version of the T20 BCT for both products. This is not such a
great thing for various reasons, one of which is problems with DVFS. Since we
are planning to enable DVFS realsoonnow(TM), you will need this change.

One thing to note: you probably shouldn't put "official" builds onto your T20
Seaboard, since they will (I think) clobber the firmware with the T25 version
(the default). You've been warned.

## Configuring U-Boot Environment

U-boot accepts user commands to set up its execution environment. The problem
with the current implementation is that at the default console baud rate of
115200 u-boot is not running fast enough and the UART gets easily overrun in
case a user tries pasting text through terminal connected to the console.

The attached `expect` script allows to work around the problem. The script is
just an example, which can be used it as follows. Start its execution on the
workstation which has a USB port connected to the target console

```none
./ttyusb <file with u-boot script>
```

and then hit reset on the target (if it is not already in u-boot console mode).
`<file with u-boot script>` is a text file including a set of u-boot commands,
one per line. Lines starting with # are ignored. `ttyusb` will send the commands
to the console, character by character, making sure that all characters are
echoed and the `CrOS> `prompt is returned after each command.
Once all commands are executed, the terminal session becomes the console
terminal for the target. To exit the console session type `^a`.
The serial port device is hardcoded in the script to `/dev/ttyUSB1,` edit your
copy of to get a different tty device used, if necessary.

## Memory Map

### Tegra 2x

Current (as of 2011-09-13):

We have 0x4000 0000 (1GB) of memory in our machines. RAM starts at physical
address 0x0.

<table>
<tr>
<td><b> Address 0x</b></td>
<td><b> Approx.</b></td>
<td><b> Size</b></td>
<td><b> Defined where</b></td>
<td><b> Notes</b></td>
</tr>
<tr>
<td> 0000 0100</td>
<td> 0.25KB</td>
<td> 1KB</td>
<td>gd-&gt;bd-&gt;bi_boot_params (TODO: used booting non-FIT images?)</td>
<td>Kernel boot parameters area (ATAGs)</td>
</tr>
<tr>
<td> 0000 8000</td>
<td> 32KB</td>
<td> 4MB</td>
<td><a href="http://git.chromium.org/gitweb/?p=chromiumos/third_party/kernel.git;a=blob;f=arch/arm/mach-tegra/Makefile.boot;hb=chromeos-2.6.38">zreladdr</a></td>
<td>Not used by u-boot, but good to keep in mind that this is the location the kernel will eventually decompress / relocate itself to (TODO: will it put any initramdisk here too?). It's important there's room so the decompressed kernel won't clobber the FDT.</td>
</tr>
<tr>
<td> 0010 0000</td>
<td> 1MB</td>
<td> 8MB</td>
<td><a href="http://git.chromium.org/gitweb/?p=chromiumos/third_party/u-boot.git;a=blob;f=include/configs/chromeos.h;hb=chromeos-v2011.06">CHROMEOS_KERNEL_LOADADDR</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=chromiumos/third_party/u-boot.git;a=blob;f=include/configs/chromeos.h;hb=chromeos-v2011.06">CHROMEOS_KERNEL_BUFSIZE</a></td>
<td>vboot loads the FIT image here; FDT and zImage will then be relocated elsewhere</td>
</tr>
<tr>
<td> 0040 8000</td>
<td> 4MB + 32KB</td>
<td> 4MB</td>
<td><a href="http://git.chromium.org/gitweb/?p=chromiumos/third_party/u-boot.git;a=blob;f=include/configs/tegra2-common.h;hb=chromeos-v2011.06">CONFIG_LOADADDR</a></td>
<td>Area to load scripts or kernel uImage (prior to decompression); used by legacy u-boot. TODO: Why not use 0x00100000?</td>
</tr>
<tr>
<td> 00e0 8000</td>
<td> 14MB + 32KB</td>
<td> 1MB</td>
<td><a href="http://git.chromium.org/gitweb/?p=chromiumos/third_party/u-boot.git;a=blob;f=include/configs/tegra2-common.h;hb=chromeos-v2011.06">TEXT_BASE</a></td>
<td>Start of U-Boot code region. Data appears immediately after</td>
</tr>
<tr>
<td> 01ff c000</td>
<td> 32MB - 16KB</td>
<td> 16KB</td>
<td><a href="http://git.chromium.org/gitweb/?p=chromiumos/third_party/u-boot.git;a=blob;f=include/configs/tegra2-common.h;hb=chromeos-v2011.06">CONFIG_SYS_BOOTMAPSZ</a> appears to control the 32MB (can be overridden by environment variables).</td>
<td>The 16KB comes from the padded size of the kernel's FDT (as the FDT grows, this should move).</td>
<td>U-boot ends up relocating the kernel's copy of the FDT to here.</td>
</tr>
<tr>
<td> 0200 0000</td>
<td> 32MB</td>
<td> 512KB</td>
<td><a href="http://git.chromium.org/gitweb/?p=chromiumos/third_party/kernel.git;a=blob;f=chromeos/config/armel/config.common.armel;hb=chromeos-2.6.38">CONFIG_CHROMEOS_RAMOOPS_RAM_START</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=chromiumos/third_party/kernel.git;a=blob;f=chromeos/config/config.common.chromeos;hb=chromeos-2.6.38">CONFIG_CHROMEOS_RAMOOPS_RAM_SIZE</a></td>
<td>Kernel preserved area (kcrashmem). This is preserved_start in the kernel, or kcrashmem= environment variable in U-Boot.</td>
</tr>
<tr>
<td> 0300 0000</td>
<td> 48MB</td>
<td> ?MB</td>
<td><a href="http://git.chromium.org/gitweb/?p=chromiumos/overlays/chromiumos-overlay.git;a=blob;f=sys-kernel/chromeos-kernel/files/kernel_fdt.its">kernel_fdt.its</a></td>
<td>Image is copied here by u-boot before jumping to.</td>
</tr>
<tr>
<td> 1c40 6000</td>
<td> 512MB - </td>
<td> 192MB + </td>
<td> 4MB + </td>
<td> 24KB</td>
<td> 8KB</td>
<td><a href="http://git.chromium.org/gitweb/?p=chromiumos/third_party/u-boot.git;a=blob;f=include/configs/tegra2-common.h;hb=chromeos-v2011.06">TEGRA_LP0_ADDR</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=chromiumos/third_party/u-boot.git;a=blob;f=include/configs/tegra2-common.h;hb=chromeos-v2011.06">TEGRA_LP0_SIZE</a></td>
<td>Used in suspend / resume.</td>
</tr>
<tr>
<td> 3768 0000</td>
<td> 1GB - ~144MB</td>
<td> varies; ~2MB</td>
<td><a href="http://git.chromium.org/gitweb/?p=chromiumos/third_party/u-boot.git;a=blob;f=board/nvidia/seaboard/tegra2-seaboard.dts;hb=chromeos-v2011.06">frame-buffer</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=chromiumos/third_party/u-boot.git;a=blob;f=common/lcd.c;hb=chromeos-v2011.06">lcd_get_size()</a></td>
</tr>
</table>
