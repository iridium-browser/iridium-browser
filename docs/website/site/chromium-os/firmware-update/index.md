---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: firmware-update
title: Firmware Update
---

[TOC]

## WARNING: Remember to backup your original firmware! The flash memory contains
some device information that cannot be recovered if overwritten.

## Glossary

*   **BCT (Binary Chip Timings)**
    *   **BCT** contains timing parameters for theSOC and it used to set
                up its memory and basic peripherals, to allow it to read the
                boot loader. The BCT is packaged together with U-Boot for
                reading by the SOC on boot.
*   **Boot ROM**
    *   An SOC's **Boot ROM** is responsible for setting up the SOC to a
                very basic level (using theBCT), setting up SDRAM and figuring
                out where the boot loader is. This is typically capable of
                loading the boot loader from USB, SPI flash, UART, eMMC / SD
                card and NAND flash. The Boot ROM is part of the SOC and cannot
                be changed other than by making a new chip. Some Boot ROMs
                include security features which require that boot code be signed
                by a special tool. Typically, Boot ROM consists of 16-32 KB of
                code.
*   **Device tree**
    *   see **FDT**.
*   **DTB file (Device Tree Binary)**
    *   A file containing a binary representation of a device tree
                created by the *device tree compiler* from the source *DTS
                file*.
*   **DTS file (Device Tree Source)**
    *   A file containing a source representation of the device tee.
                This file is compiled into binary form for use by U-Boot and
                kernel.
*   **FDT (Flattened Device Tree)**
    *   A *flattened device tree* describes the hardware that is seen by
                U-Boot and the kernel. The device tree consists of a number of
                nodes containing properties like memory addresses, interrupts,
                size, width / height (for LCD), etc. Each node also contains a
                `compatible` property which indicates which device driver should
                be used to implement the features provided by that node. We also
                use the FDT to pass provide configuration and verified boot
                information from U-Boot to the kernel. There is a different FDT
                for each board type, and even for each SKU (since a particular
                manufacturing run may change the chips that are used in some
                cases).
*   **GBB (Google Binary Block)**
    *   This read-only area contains screen images for recovery mode and
                keys for both normal and recovery mode.
*   **SOC (System on Chip)**
    *   A name for the integrated chips common in the ARM world. They
                include most peripherals on chip including SDRAM,control, SPI
                buses, NAND, USB and LCD. Roughly speaking a basic system can be
                created with an SOC, some SDRAM, some Flash and a power
                controller
*   **U-Boot**
    *   The boot loader is responsible for setting up all required
                peripherals (such as LCD, SPI flash, UARTs, eMMC) and then
                loading and starting the kernel. On Chrome OS this includes
                security requirements, so we split U-Boot into two parts: a read
                only image that we ship with, and a read write image which can
                override this when we need to update the boot loader in the
                field. U-Boot is linked with the VBoot library which provides
                its security features. U-Boot is also responsible for taking the
                kernel's FDT, updating it with security and boot parameters, and
                presenting it to the kernel. There is a single U-Boot image for
                all boards for a single SOC type. For example, for Tegra2 we
                have one U-Boot image. All run-time configuration of U-Boot is
                done through the FDT.
*   **VBoot**
    *   The verified boot library includes routines for accessing and
                checking keys and signatures. A detailed description of how this
                works [is provided
                here](/chromium-os/chromiumos-design-docs/verified-boot-data-structures).

## `cros_bundle_firmware`

### Summary

This is a tool for creating a firmware image and writing it to your board. It
can handle the following tasks:

*   Package together U-Boot (the boot loader) and a FDT to produce a
            boot image
*   Package together the boot image, a BCT and appropriate signature to
            product a signed image
*   Create a GBB with appropriate keys and images
*   Create a full firmware image containing all the sections required
            for boot
*   Write this image to a board through the USB A-A cable

### Usage

To create an image for a particular board, use:

```none
cros_bundle_firmware -b <board> -o <filename>
```

This will generate an image `<filename>` for the board. The tool finds files it
needs in `/build/<board>/firmware`, things like `u-boot.bin`, the device tree
source files (in dts subdir) and so on. There are options to specify each of
these manually if you want to, and this is what the ebuild does. However, it
does make for a very long command line. If you are unsure what files it is
picking up, use -v3.

By default `cros_bundle_firmware` is quiet. Unless there are warnings/errors,
all you will see is progress, and a command prompt when done. Use -v2, -v3 or
even -v4 to change that.

Things you can do with this tool, which involve adding options:

*   Specify your own U-Boot image:
    *   `-u u-boot.bin`
*   Specify the U-Boot used for flashing (if your U-Boot doesn't support
            it)
    *   `-U /build/<board>/firmware/u-boot.bin`
*   Specify your own FDT source file:
    *   `-d my-board.dts`
    *   Note you will need to add a -I &lt;dir&gt; option for each
                include directory needed by your fdt. For example, `-I
                arch/arm/dts -I board/samsung/dts`
    *   The FDT file is automatically compiled
*   Write the firmware to an SD card:
    *   `-w sd:.`
    *   The `.` means to write to the only SD card connected to your
                machine. You can explicitly name the card if you like. Try `-w
                sd` to get a list of options.
*   Write the firmware over USB to the board
    *   `-w usb`
    *   Note this works for Tegra, but currently (Apr-12) gives checksum
                errors for Exynos
*   Generate a minimal image just for testing with none of the GBB and
            other Chrome OS stuff
    *   `-s`
*   Save all the temporary files so you can see what happened:
    *   `-O <dir>`

A full list of options is below, or use the --help flag.

### Options

```none
Usage: cros_bundle_firmware [options]
Options:
  -h, --help            show this help message and exit
  --add-config-str=ADD_CONFIG_STR
                        Add a /config string to the U-Boot fdt
  --add-config-int=ADD_CONFIG_INT
                        Add a /config integer to the U-Boot fdt
  -b BOARD, --board=BOARD
                        Board name to use (e.g. tegra2_kaen)
  --bootcmd=BOOTCMD     Set U-Boot boot command
  --bootsecure          Boot command is simple (no arguments) and not
                        interruptible
  -c BCT, --bct=BCT     Path to BCT source file: only one can be given
  -d FDT, --dt=FDT      Path to fdt binary blob .dtb file to use
  --bl1=EXYNOS_BL1      Exynos preboot (BL1) file
  --bl2=EXYNOS_BL2      Exynos Secondary Program Loader (SPL / BL2) file
  --hwid=HARDWARE_ID    Hardware ID string to use
  -B BMPBLK, --bmpblk=BMPBLK
                        Bitmap block to use
  -F FLASH_DEST, --flash=FLASH_DEST
                        Create a flasher to flash the device (spi, mmc)
  -k KEY, --key=KEY     Path to signing key directory (default to dev key)
  -I INCLUDEDIRS, --includedir=INCLUDEDIRS
                        Include directory to search for files
  -m, --map             Output a flash map summary
  -o OUTPUT, --output=OUTPUT
                        Filename of final output image
  -O OUTDIR, --outdir=OUTDIR
                        Path to directory to use for intermediate and output
                        files
  -p, --preserve        Preserve temporary output directory
  -P POSTLOAD, --postload=POSTLOAD
                        Path to post-load portion of U-Boot (u-boot-post.bin)
  -s, --small           Create/write only the signed U-Boot binary (not the
                        full image)
  -S SEABIOS, --seabios=SEABIOS
                        Legacy BIOS (SeaBIOS)
  -u UBOOT, --uboot=UBOOT
                        Executable bootloader file (U-Boot)
  -U UBOOT_FLASHER, --uboot-flasher=UBOOT_FLASHER
                        Executable bootloader file (U-Boot) to use for
                        flashing (defaults to the same as --uboot)
  -C COREBOOT, --coreboot=COREBOOT
                        Executable lowlevel init file (coreboot)
  -v VERBOSITY, --verbosity=VERBOSITY
                        Control verbosity: 0=silent, 1=progress, 3=full,
                        4=debug
  -w WRITE, --write=WRITE
                        Write firmware to device (usb, sd)
```

## `cros_write_firmware`

If you already have an image, you can use this tool to write it to an SD card or
over USB. However, you might find '`cros_bundle_firmware -w`' more flexible.

```none
cros_write_firmware -b daisy -i <image> -w sd:.
```

## Updating your U-Boot to 2011.12 on Daisy

You can do this without USB download using an SD card if you like. Please follow
these steps:

1.  Get an SD card and put it into your card reader. The contents of
            this will be overwritten.
2.  Get the latest source:

    ```none
    $ repo sync
    ```

3.  Build everything

    ```bash
    $ cros build-packages --board daisy
    ```

    ```none
    $ emerge-daisy chromeos-u-boot
    ```

    ```none
    $ emerge-daisy sys-boot/chromeos-bootimage
    ```

4.  Write the firmware to your SD card (-F means to create an automatic
            flasher)

    ```none
    $ cros_bundle_firmware --add-config-int load_env 1 -b daisy -w sd:. -F spi
    # Add '-d exynos5250-snow' for snow
    ```

5.  Insert the SD card into your daisy board
6.  Hold down the button which is labelled T20_REC / FW_DEBUG / DFU_REC
            depending on whether you are using [Servo 2](/chromium-os/servo),
            Daisy or Min-Servo.
7.  Press COLD_RST_L / KBC_RST / C_RST, then PWR_BUTTON to switch on, to
            boot from the SD card.
8.  When it starts up, you should see a console on your servo UART, and
            it will start flashing your board.
9.  When finished, type 'reset' into the terminal, or press COLD_RST_L /
            KBC_RST / C_RST again, then PWR_BUTTON to switch on.

Steps 8 can also be done via servod:

```none
dut-control fw_up:on fw_up cold_reset:on cold_reset cold_reset:off cold_reset \
            pwr_button:press pwr_button sleep:2 pwr_button:release pwr_button \
            cold_reset:off cold_reset fw_up:off fw_up cpu_uart_en:on 
```

## x86 Development Flow

**Building from the firmware branch**

The ToT is not guaranteed to build usable firmware for older boards, so it is
often necessary to retrieve an old branch to do so. These steps were used to
build the Link firmware from source:

<pre><code>
repo init -u <i>&lt;internal-manifest-url&gt;</i> \
    --repo-url='https://chromium.googlesource.com/external/repo.git'
    -b firmware-link-2695.B 
repo sync
cros_workon-link start chromeos-base/chromeos-ec \
    sys-boot/chromeos-u-boot \
    chromeos-base/vboot_reference \
    chromeos-base/vboot_reference-firmware \
    sys-boot/chromeos-coreboot-link \
    sys-boot/chromeos-seabios \
    chromeos-base/chromeos-firmware-link
emerge-link chromeos-ec \
    vboot_reference-firmware \
    chromeos-u-boot \
    chromeos-coreboot-link \
    chromeos-bootimage
</code></pre>

This should create `image-link.bin` in `/build/link/firmware` under the chroot.

**Using flashrom**

The `flashrom` command can be run from a root shell on the device, or it can be
used from a development PC connected via servo. See
[here](/chromium-os/packages/cros-flashrom) for more detailed instructions on
usage.

**Using the SPI flash emulator**

The instructions in this section assume that you have a [Dediprog
EM100](http://www.dediprog.com/pd/spi-flash-solution/em100pro) hooked up to the
system. This hardware allows for downloading a new firmware image to the board
in mere seconds, rather than waiting a few minutes to reflash a physical SPI
memory device.

To build a new firmware image, write it to your em100 and reset your link (using
servo2):

```none
$ USE=dev emerge-link chromeos-u-boot
$ cros_bundle_firmware -b link --bootcmd vboot_twostop -w em100
```

(the 'dev' flag reduces the 870KB of spew from the U-Boot ebuild, printing only
warnings and errors)

If you want a U-Boot serial console, but don't want to coreboot to print out all
its serial info, you can build a coreboot WITHOUT the `USE=pcserial` flag, and
then:

```none
$ cros_bundle_firmware -b link --add-node-enable console 1 --bootcmd vboot_twostop -w em100
```

This enables the console node in the device tree, thus turning on the serial
console in U-Boot.
