---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/firmware-porting-guide
  - Firmware Overview and Porting Guide
page_name: u-boot-drivers
title: 3.  U-Boot Drivers
---

[TOC]

4/11/2013

This section lists the functional requirements for Chrome OS drivers and
describes how to implement the drivers using U-Boot APIs and configuration
files. It provides links to useful code samples from the [U-Boot source
tree](http://git.denx.de/?p=u-boot.git;a=tree).

## Board Configuration

Each board has a file that contains config options for each board component. For
example, the Samsung SMDK5250 (Exynos5250) board is specified in the file
[include/configs/smdk5250.h](http://git.denx.de/?p=u-boot.git;a=blob;f=include/configs/smdk5250.h;h=c0f86228b08a0fb4df48d70180144af2990b76c2;hb=HEAD).
This file controls which components are enabled and specifies certain parameters
for each board component.

## Driver Configuration

To add a driver for a particular class of peripheral, you need to do the
following:

*   Implement the APIs specified in the driver class header file.
*   Add config option(s) for the peripheral to the board configuration
            file in include/configs.
*   Add a node for the driver to the [device tree
            file](/chromium-os/firmware-porting-guide/2-concepts) (*.dts*),
            specifying its properties and values as well as any additional
            devices that are connected to it.

The following sections provide details for each class of driver, including
performance requirements and implementation tips. For Chrome OS implementations,
also see the [Main Processor Firmware
Specification](https://sites.google.com/a/google.com/chromeos-partners/pages/tech-docs/firmware/main-processor-firmware-specification-v10).

## Audio Codec and Inter-Integrated Circuit Sound (I2S)

The audio codec is commonly connected to I2S to provide the audio data and to
I2C to set up the codec (for example, to control volume, output speed, headphone
and speaker output).

#### Implementation Notes

The sound.c file defines the sound_init() function, which sets up the audio
codec and I2S support. Currently, this file supports the Samsung WM8994 audio
codec and the Samsung I2S driver. This system would need to be expanded for
other architectures to add support for new codec and I2S drivers. The
sound_play() file, also defined in sound.c, plays a sound at a particular
frequency for a specified period.

The samsung-i2s.c file defines the i2s_tx_init() function, which sets up the I2S
driver for sending audio data to the codec. It also defines
i2s_transfer_tx_data(), which transfers the data to the codec.

The wm8994.c file defines the wm8994_init() function, which initializes the
codec hardware.

#### Command Line Interface

The console commands sound_init and sound_play can be used to control the audio
codec.

<table>
<tr>
<td><b> Header File</b></td>
<td>include/sound.h</td>
</tr>
<tr>
<td> <b>Implementation File</b></td>
<td>drivers/sound/sound.c, drivers/sound/wm8994.c,</td>
<td>drivers/sound/samsung-i2s.c</td>
</tr>
<tr>
<td> <b>Makefile</b></td>
<td>drivers/sound/Makefile</td>
</tr>
<tr>
<td> <b>Example</b></td>
<td>drivers/sound/wm8994.c</td>
</tr>
</table>

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## Clock

The AP has a number clocks that drive devices such as the eMMC, SPI flash, and
display. Although the clock structure can be very complex, with a tree of 50 or
more interdependent clocks, U-Boot has a simple clock implementation. With
U-Boot, you need to turn on a clock, set its frequency, and then just let it
run.

#### Implementation Notes

The preferred technique is to create clock.c, which implements the clock
functionality. Important clock functions to implement include the following:

*   clock_set_rate() - sets the rate for a particular clock
*   clock_start_periph_pll() - starts a peripheral clock at a particular
            rate
*   clock_set_enable() - enable and disable a clock
*   reset_periph() - reset a peripheral
*   clock_get_rate() - queries the clock rate

*<table>*
*<tr>*
*<td><b> Header File</b></td>*
*<td>arch/arm/include/asm/arch-xxxx/clock.h</td>*
*</tr>*
*<tr>*
*<td> <b>Implementation File</b></td>*
*<td> typically in AP directory, e.g., arch/arm/cpu/armv7/arch-xxxx/clock.c</td>*
*</tr>*
*<tr>*
*<td><b> Makefile</b></td>*
*<td> typically, also in AP directory</td>*
*</tr>*
*<tr>*
*<td> <b>Example</b></td>*
*<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=arch/arm/cpu/tegra20-common/clock.c;h=12987a6893691f8646016ac1e242b71519a811da;hb=HEAD">arch/arm/cpu/tegra20-common/clock.c</a></td>*
*</tr>*
*</table>*

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## Ethernet

An Ethernet connection is often used during development to download a kernel
from the network. This connection is also used in the factory to download the
kernel and ramdisk.

U-Boot supports the following network protocols:

*   **TFTP** - for downloading a kernel and also for uploading trace
            data
*   **NFS** - also used for downloading a kernel
*   **BOOTP/DHCP** - for obtaining an IP address
*   **ping** - for checking network connectivity

Many x86 devices have a built-in Ethernet port. Another way to provide Ethernet
to a system is to connect a USB-to-Ethernet adapter to the USB port. If the
device has a built-in port, Ethernet is detected when the board starts up and is
available for use. To enable the USB-to-Ethernet connection, use the U-Boot
command `usb start`.

Another useful feature for development is that when you want to use an NFS root
from the network, U-Boot can provide suitable boot arguments to the kernel on
the Linux command line.

#### Implementation Notes

The structure` eth_device` in the file `net.h` describes the Ethernet driver.
The board file calls the `probe()` function, which probes for Ethernet hardware,
sets up the `eth_device` structure, and then calls `eth_register()`.

You need to implement the following functions for the Ethernet driver:

*   init() - brings up the Ethernet device
*   halt() - shuts down the Ethernet device
*   send() - sends packets over the network
*   recv() - receives packets over the network
*   write_hwaddr() - writes the MAC address to the hardware from the
            ethaddr environment variable.

For USB Ethernet, the structure `ueth_data` in the file `usb_ether.h` describes
the USB Ethernet driver. The `usb_ether.h` file also defines a set of three
functions that must be implemented for each supported adapter. For example, here
are the functions for the Asix adapter:

```none
#ifdef CONFIG_USB_ETHER_ASIX
    void asix_eth_before_probe(void);
    int asix_eth_probe(struct usb_device *dev, unsigned int ifnum,
                       struct ueth_data *ss);
    int asix_eth_get_info(struct usb_device *dev, struct ueth_data *ss,
                          struct eth_device *eth);
    #endif
```

The` *xxx*_eth_probe()` function probes for the device and must return nonzero
if it finds a device. The `*xxx*_eth_get_info()` function obtains information
about the device and fills in the `ueth_data` structure.

<table>
<tr>
<td><b> Header File</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=include/net.h;h=970d4d1fab13df2c093062e1cf015625bb5db558;hb=HEAD">include/net.h</a></td>
<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=include/usb_ether.h;h=7c7aecb30574d4536915f20f262a858470160421;hb=HEAD">include/usb_ether.h</a></td>
</tr>
<tr>
<td> <b>Implementation File</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/net/lan91c96.c;h=11d350eb8daeeefdf87724b62c4f0e4039a9e713;hb=HEAD">drivers/net/lan91c96.c</a> *(private to driver)*</td>
</tr>
<tr>
<td> <b>Makefile</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/net/Makefile;h=786a6567a5fc9ba5dbfc2c68262a789c5338a2ea;hb=HEAD">drivers/net/Makefile</a> </td>
</tr>
<tr>
<td> <b>Example</b></td>
<td><a href="http://drivers/usb/eth/asix.c">drivers/usb/eth/asix.c</a> *(specific adapter)*</td>
<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/usb/eth/usb_ether.c;h=f361e8b16857eeb7815f7ae594e83567246bce09;hb=HEAD">drivers/usb/eth/usb_ether.c</a> *(generic interface)*</td>
</tr>
</table>

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## GPIO

Modern APs can contain hundreds of GPIOs (General Purpose Input/Output pins).
GPIOs can be used to *control* a given line or to *sense* its current state. A
given GPIO can serve multiple purposes. Also, peripheral pins can often also be
used as GPIOs. For example, an AP MMC interface requires 11 pins that can be
used as GPIOs if the MMC function is not needed.

A GPIO can be either an *input* or an *output*. If an input, its value can be
read as 0 or 1. If an output, then its value can be set to 0 or 1.

#### Generic GPIO Interface

U-Boot provides a generic GPIO interface in
`[include/asm-generic/gpio.h](http://git.denx.de/?p=u-boot.git;a=blob;f=include/asm-generic/gpio.h;h=bfedbe44596aa31fb2266f7e052a63401f06d181;hb=HEAD)`.
This interface provides the following functions:

<table>
<tr>
<td><b> Function</b></td>
<td><b> Description</b></td>
</tr>
<tr>
<td> int gpio_<b>request</b>(unsigned gpio, const char \*label);</td>
<td> Request use of a specific GPIO</td>
</tr>
<tr>
<td> int gpio_<b>free</b>(unsigned gpio);</td>
<td> Frees the GPIO that was in use</td>
</tr>
<tr>
<td> int gpio_<b>direction_input</b>(unsigned gpio);</td>
<td> Makes the GPIO an input</td>
</tr>
<tr>
<td> int gpio_<b>direction_output</b>(unsigned gpio, int value);</td>
<td> Makes the specified GPIO an output and sets its value</td>
</tr>
<tr>
<td> int gpio_<b>get_value</b>(unsigned gpio);</td>
<td> Gets the value of the GPIO (either input or output)</td>
</tr>
<tr>
<td> int gpio_<b>set_value</b>(unsigned gpio, int value);</td>
<td> Sets the value of an output GPIO (0 or 1)</td>
</tr>
</table>

In U-Boot, GPIOs are numbered from 0, with enums specified in the AP header file
`gpio.h`. For example:

> enum gpio_pin {

> GPIO_PA0 = 0, /\* pin 0 \*/

> GPIO_PA1,

> GPIO_PA2,

> GPIO_PA3,

> GPIO_PA4,

> GPIO_PA5,

> GPIO_PA6,

> GPIO_PA7,

> GPIO_PB0, /\* pin 8 \*/

> GPIO_PB1,

> GPIO_PB2,

> .

> .

> .

> };

The generic GPIO functions specify the GPIO pin by its number, as described in
gpio.h.

#### Additional Functions

The generic GPIO interface does not cover all features of a typical AP. For
example, custom AP functions are required to specify the following:

*   **Drive strength** is defined in a chip-specific function.
*   **Pinmux** selects which function a pin has (for example, MMC, LCD,
            GPIO) and what is controlling the pin. The [pinmux
            module](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Pinmux)
            typically handles this function.
*   **Pullup and pulldown** functionality is defined in a chip-specific
            function so that there are no floating lines.

#### Command Line Interface

The GPIO driver has a corresponding `gpio` command line interface that can be
used to set and get GPIO values. See common/cmd_gpio.c for a list of commands
(input, set, clear, toggle). The gpio status command, which you must implement,
displays the status of all GPIOs in the system. This useful command should be
able to accept both numbers and names for GPIO pins, as defined in gpio.h.

*<table>*
*<tr>*
*<td><b> Header File</b></td>*
*<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=arch/arm/include/asm/arch-tegra20/gpio.h;h=e2848fec67ba58d038b20f8666ccf693f4beafd1;hb=HEAD">arch/arm/include/asm/arch-tegra20/gpio.h</a></td>*
*</tr>*
*<tr>*
*<td> <b>Implementation File</b></td>*
*<td> typically in AP directory, e.g., drivers/gpio/gpio-xxx.c</td>*
*</tr>*
*<tr>*
*<td><b> Makefile</b></td>*
*<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/gpio/Makefile;h=9df1e2632f774805b635edd317292cb3196fa6cf;hb=HEAD">drivers/gpio/Makefile</a></td>*
*</tr>*
*<tr>*
*<td> <b>Example</b></td>*
*<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/gpio/tegra_gpio.c;h=2417968214d7f075db3cec79af623c21cc16f012;hb=HEAD">drivers/gpio/tegra_gpio.c</a> (?)</td>*
*</tr>*
*</table>*

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## Inter-Integrated Circuit Communication (I2C)

The inter-integrated circuit communication (I2C) driver is the most-used driver
in U-Boot for Chrome OS. For example, the I2C driver is used to send and receive
data from the following devices:

*   PMIC (Power Module)
*   Trusted Platform Module (TPM)
*   Embedded Controller
*   Battery gas gauge
*   Battery charger
*   LCD/EDID
*   Audio codec

Because I2C drivers form a critical part of U-Boot, they should be tested to
ensure that a given I2C bus works correctly with multiple slaves and at all
supported speeds. Be sure the driver correctly handles NAK messages from slaves
and provides robust error handling.

#### Setup

Ordering of multiple I2C buses (there are usually half a dozen or more) is
specified in the device tree file aliases section (for example, see
board/samsung/dts/exynos5250-smdk5250.dts). Bus numbering is zero-based.

The following function is called by the board file to set up I2C ports:

> void board_i2c_init(const void \*blob);

In this function, `blob` is the device tree.

Given a node in the device tree, the following function returns the bus number
of that node:

> `int i2c_get_bus_num_fdt(int node);`

An I2C bus typically runs at either 100 kHz or 400 kHz. Ideally the driver
should support exactly these speeds. In no case should the driver exceed the
specified speed. The bus speed is specified in the device tree for a given bus.
Although the U-Boot driver header files include functions for setting I2C bus
speeds, these functions should not be used directly. Instead, set one speed for
each I2C bus in the device tree, choosing the speed that matches the slowest
device on a given bus.

#### Communication

Several of the I2C functions use the concept of a "current bus":

*   i2c_set_bus_num() sets the current bus number
*   i2c_get_bus_num() returns the current bus number

Typically, you follow this pattern:

1.  Call i2c_get_bus_num() to obtain the current bus.
2.  Store this bus number so that you can restore this state when you
            are finished with your transaction.
3.  Call i2c_set_bus_num() to set the bus for your current transaction.
4.  Perform processing on this bus (sending and receiving data).
5.  Finally, call i2c_set_bus_num() to reset the bus to its original
            number.

The functions` i2c_read()` and `i2c_write()` are used to receive and send data
from the I2C bus. Because multiple devices share the same bus, the functions
require information about both the chip address and the memory address within
the chip. For example, the syntax for i2c_read is as follows:

`int i2c_read(uchar chip, uint addr, int alen, uchar *buffer, int len);`

where

`chip` is the I2C chip address, in the range 0 to 127 `addr` is the memory
address within the chip (the register) `alen` is the length of the address (1
for 7-bit addressing, 2 for 10-bit addressing) `buffer` is where to read the
data `len` is how many bytes to read

#### Command Line Interface

The I2C bus has a corresponding `i2c` command line interface that can be used to
read and write data.

<table>
<tr>
<td><b> Header File</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=include/i2c.h;h=c60d07583bf68cbcc3b1dc6bf67f41a44585c920;hb=HEAD">include/i2c.h</a></td>
</tr>
<tr>
<td> <b>Implementation File</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=tree;f=drivers/i2c;hb=HEAD">drivers/i2c/*driverName.c*</a></td>
</tr>
<tr>
<td> <b>Makefile</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/i2c/Makefile;h=5dbdbe3672259c0d9a72bd79502fe99990f1cbde;hb=HEAD">drivers/i2c/Makefile</a></td>
</tr>
<tr>
<td> <b>Example</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/i2c/tegra_i2c.c;h=efc77fa910fcc7dcb969afc99d5c5822b99096f1;hb=HEAD">drivers/tegra_i2c.c</a></td>
</tr>
</table>

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## Keyboard

In Chrome OS, the keyboard is managed by the embedded controller (EC), which
reports key presses to the AP using messages. Implementing support in U-Boot for
the keyboard driver is different for x86 and ARM systems. On x86 systems, 8042
keyboard emulation is used over ACPI to report key presses. On ARM systems, the
Chrome OS EC protocol is used to report key scans.

#### Implementation Notes

***On x86 Systems***

On x86 systems, the 8042 protocol is handled by a keyboard driver that
communicates with the AP using an x86 I/O port. On these systems, you are
responsible for implementing the keyboard driver that reports key presses to the
AP.

<table>
<tr>
<td><b> Header File</b></td>
<td>include/</td>
</tr>
<tr>
<td> <b>Implementation File</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/input/keyboard.c;h=614592ef3c18febcf27f23a7ac600208d9db03aa;hb=HEAD">drivers/input/keyboard.c</a></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/input/i8042.c;h=26958aaa3eaca4e9a3dced2bd3d6fc81735a08bb;hb=HEAD">drivers/input/i8042.c</a></td>
</tr>
<tr>
<td> <b>Makefile</b></td>
<td>drivers/</td>
</tr>
<tr>
<td> <b>Example</b></td>
<td>drivers/</td>
</tr>
</table>

***On ARM Systems***

On ARM systems, the `cros_ec` driver communicates with the EC and requests key
scans. Each message contains the state of every key on the keyboard. This design
is chosen to keep the EC as simple as possible. On ARM systems, the U-Boot and
Linux code provides built-in support for converting key scans into key presses
through the input layer (input.c and key_matrix.c).

The device tree contains a keyboard node that has a linux, keymap property that
defines the keycode at each position in the keyboard matrix. You may need to
edit this file to reflect your keyboard.

**Setting up the keyboard driver.** Three functions are used to initialize and
register a new keyboard driver (see the function tegra_kbc_check() in
tegra-kbc.c for an example of waiting for input and then checking for key
presses):

*   input_init() - initializes a new keyboard driver.
*   read_keys() - called when the input layer is ready for more keyboard
            input.
*   input_stdio_register() - registers a new input device (see
            "Functions Used by Input Devices," below).

**Functions provided by the input layer.** The following functions are defined
by the input layer (input.c) and must be implemented for your driver: *(TRUE? or
do they just use these functions?)*

*   key_matrix_decode() - converts a list of key scans into a list of
            key codes.
*   input_send_keycodes() - sends key codes to the input system for
            processing by U-Boot.

Keyboard auto-repeat is handled by the input layer automatically.

**Functions used by input devices*.*** U-Boot supports multiple console devices
for input and output. Input devices are controlled by the environment variable
stdin which contains a list of devices that can supply input. It is common for
this variable to contain both serial input and keyboard input, so you can use
either type of input during development.

An input device has three main functions to implement for use by
input_stdio_register(). Each of these functions communications with the input
layer.

*   getc() - obtains a character and then passes it to input_getc() in
            the input layer.
*   tstc() - checks whether a character is present (but does not read
            it) and then passes the result to input_tst().
*   start() - starts the input device; is called by the input system
            when the device is selected for input.

**Configuration options.** The following configuration options describe how the
keyboard is connected to the EC. Include the appropriate option in the board
configuration file.

<table>
<tr>
<td> CONFIG_CROS_EC </td>
<td> Enable EC protocol</td>
</tr>
<tr>
<td> CONFIG_CROS_EC_I2C</td>
<td> Select the I2C bus for communication with the EC</td>
</tr>
<tr>
<td> CONFIG_CROS_EC_SPI</td>
<td> Select the SPI bus for communication with the EC</td>
</tr>
<tr>
<td> CONFIG_CROS_EC_LPC</td>
<td> Select the LPC bus for communication with the EC</td>
</tr>
<tr>
<td> CONFIG_CROS_EC_KEYB</td>
<td> Enable the keyboard driver</td>
</tr>
</table>
<table>
<tr>
<td><b> Header File</b></td>
<td>include/configs/*boardname*</td>
</tr>
<tr>
<td> <b>Implementation File</b></td>
<td>drivers/input/cros_ec_keyb.c</td>
<td>*(uses standard input layer of U-Boot:*</td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/input/input.c;h=04fa5f0bd200d2ce45f2a488561290bb21772862;hb=HEAD">drivers/input/input.c</a> *and*</td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/input/key_matrix.c;h=946a186a1ffaf2ad511cba5e0749efc53ddf8e16;hb=HEAD">drivers/input/key_matrix.c</a>)</td>
</tr>
<tr>
<td> <b>Makefile</b></td>
<td>drivers/</td>
</tr>
<tr>
<td> <b>Example</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/input/tegra-kbc.c;h=88471d3edf253e432458a44fb0c8d5ed29317d47;hb=HEAD">drivers/input/tegra-kbc.c</a></td>
</tr>
</table>

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## LCD/Video

The display is used to present several screens to the user. If the firmware is
unable to boot, the screen displays recovery boot instructions for the user.
Similarly, when the system enters developer mode, a special screen warns the
user before entering this unprotected mode.

*(sample screen here)*

#### Requirements

Total wait time to enable the LCD should be as close to zero as possible.
Initialization of the LCD can be interspersed with other startup operations, but
blocking time during the initialization process should be less than 10 ms.

#### Implementation Notes

**Board File.** Add a function to the board file, board_early_init_f(), to set
up three LCD parameters in the panel_info struct.

*   vl_col
*   vl_row
*   vl_bpix

U-Boot allocates memory for your display based on these parameters.
(Alternatively, set up the LCD driver in the driver .c file and then add a
function to the board file that calls the setup function in your driver .c file.
For an example of this technique, see drivers/video/tegra.c.)

**`lcd_ctrl_init(`**`)` **function.** Sets up the display hardware. You may want
to set up cache flushing in this function to speed up your display. See
lcd_set_flush_dcache(), which is provided for you in common/lcd.c.

**Efficient Initialization.** Because LCD initialization takes a long time
(sometimes as much as .5 to 1 second), you may want to use a function that
manages the initialization efficiently and keeps things moving. For example, see
tegra_lcd_check_next_stage() in tegra.c.

**`lcd_enable()` function.** As its name suggests, this function is used to
enable the LCD. However, it is normally a null operation in Chrome OS because
the lcd_check_next_stage() function will enable the LCD.

U-Boot controls drawing characters and images and scrolling. The driver
specifies a basic data structure that describes the screen parameters. The
generic driver is defined in the lcd.h file:

```none
    typedef struct vidinfo {
                 ushort  vl_col;         /* Number of columns (i.e. 640) */
                 ushort  vl_row;         /* Number of rows (i.e. 480) */
                 ushort  vl_width;       /* Width of display area in millimeters */
                 ushort  vl_height;      /* Height of display area in millimeters */
         
                 /* LCD configuration register */
                 u_char  vl_clkp;        /* Clock polarity */
                 u_char  vl_oep;         /* Output Enable polarity */
                 u_char  vl_hsp;         /* Horizontal Sync polarity */
                 u_char  vl_vsp;         /* Vertical Sync polarity */
                 u_char  vl_dp;          /* Data polarity */
                 u_char  vl_bpix;        /* Bits per pixel, 0 = 1, 1 = 2, 2 = 4, 3 = 8 */
                 u_char  vl_lbw;         /* LCD Bus width, 0 = 4, 1 = 8 */
                 u_char  vl_splt;        /* Split display, 0 = single-scan, 1 = dual-scan */
                 u_char  vl_clor;        /* Color, 0 = mono, 1 = color */
                 u_char  vl_tft;         /* 0 = passive, 1 = TFT */
         
                 /* Horizontal control register. Timing from data sheet */
                 ushort  vl_wbl;         /* Wait between lines */
         
                 /* Vertical control register */
                 u_char  vl_vpw;         /* Vertical sync pulse width */
                 u_char  vl_lcdac;       /* LCD AC timing */
                 u_char  vl_wbf;         /* Wait between frames */
         } vidinfo_t;
```

The LCD driver specifies where screen starts in memory; pixel depth, width, and
height of screen. It also declares functions to enable the screen and turn it
on, including the backlight.

The ARM and x86 platforms use slightly different APIs. ARM uses lcd.h and x86
uses video.h. The implementation files for both ARM and X86 are located in the
drivers/video directory.

#### ARM Files

*<table>*
*<tr>*
*<td><b> Header File</b></td>*
*<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=include/lcd.h;h=42070d76366e8465a84baf3e90a7e95bff429a62;hb=HEAD">include/lcd.h</a></td>*
*</tr>*
*<tr>*
*<td> <b>Implementation File</b></td>*
*<td> <a href="http://git.denx.de/?p=u-boot.git;a=tree;f=drivers/video;h=e7574c212fc1aef7e44d7a541a77c648f5519781;hb=HEAD">drivers/video</a></td>*
*</tr>*
*<tr>*
*<td> <b>Makefile</b></td>*
*<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/video/Makefile;h=ebb6da823cee9e945da43e4f76b4549c59cbe7df;hb=HEAD">drivers/video/Makefile</a></td>*
*</tr>*
*<tr>*
*<td> <b>Example </b></td>*
*<td> CONFIG_LCD___</td>*
*<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/video/tegra.c;h=750a2834383f035109ed2f6bd012f63bbfc64243;hb=HEAD">drivers/video/tegra.c</a></td>*
*</tr>*
*</table>*

#### x86 Files

On the x86 platform, video has its own U-Boot interface, but the existing
coreboot driver will initialize the video without any further modification to
U-Boot.

*<table>*
*<tr>*
*<td><b> Header File</b></td>*
*<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=include/lcd.h;h=42070d76366e8465a84baf3e90a7e95bff429a62;hb=HEAD">include/video.h</a></td>*
*</tr>*
*<tr>*
*<td> <b>Implementation File</b></td>*
*<td> drivers/video</td>*
*</tr>*
*<tr>*
*<td> <b>Makefile</b></td>*
*<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/video/Makefile;h=ebb6da823cee9e945da43e4f76b4549c59cbe7df;hb=HEAD">drivers/video/Makefile</a></td>*
*</tr>*
*<tr>*
*<td> <b>Example</b></td>*
*<td> CONFIG_VIDEO___</td>*
*<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/video/coreboot_fb.c;h=d93bd894a225763791d35f9d00ea562cc32a0c2e;hb=HEAD">drivers/video/coreboot_fb.c </a></td>*
*</tr>*
*</table>*

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## NAND

U-Boot provides support for the following types of raw NAND drivers:

*   MTD - drivers for communicating with a NAND device as a block
            device. A wide variety of NAND chips are supported.
*   ECC - support for error correction and detection
*   YAFFS2 - a type of file system used by NAND flash
*   UBIFS - another type of file system used by NAND flash

Chrome OS currently does not use raw NAND flash. Instead, it uses EMMC or SATA
drivers, which provide a high-level interface to the underlying NAND flash.

<table>
<tr>
<td><b> Header File</b></td>
<td>include/nand.h</td>
</tr>
<tr>
<td> <b>Implementation File</b></td>
<td>drivers/mtd/nand</td>
</tr>
<tr>
<td> <b>Makefile</b></td>
<td>drivers/mtd/Makefile</td>
</tr>
</table>

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## Pin Multiplexing

Each AP vendor is responsible for writing the code in `pinmux.c`, which uses the
device tree to set up the pinmux for a particular driver. U-Boot uses the same
pinmux bindings as the kernel. These settings are normally static (that is, the
settings are selected once and remain unchanged).

#### Implementation Notes

For maximum speed, U-Boot should initialize only the pins it uses during its
boot. All other pins are left in their default configuration and can be
initialized later by the kernel. For example, the WiFi pinmux is not required at
boot time and can be initialized later by the kernel.

<table>
<tr>
<td><b> Header File</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=arch/arm/include/asm/arch-tegra20/pinmux.h;h=797e158e68ae49b14cf646a76add1d5098eff8da;hb=HEAD">arch/arm/include/asm/arch-tegra20/pinmux.h</a></td>
</tr>
<tr>
<td> <b>Implementation File</b></td>
<td> *<table>*</td>
<td> *<tr>*</td>
<td> *<td>typically in AP directory, e.g., arch/arm/cpu/armv7/arch-xxxxx/pinmux.c</td>*</td>
<td> *</tr>*</td>
<td> *</table>*</td>
</tr>
<tr>
<td> <b>Makefile</b></td>
<td> *<table>*</td>
<td> *<tr>*</td>
<td> *<td> typically, also in AP directory</td>*</td>
<td> *</tr>*</td>
<td> *</table>*</td>
</tr>
<tr>
<td> <b>Example</b></td>
<td> *<table>*</td>
<td> *<tr>*</td>
<td> *<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=arch/arm/cpu/tegra20-common/pinmux.c;h=a2a09169e54bc46acbdc0b2bc5f89478a775b2be;hb=HEAD">arch/arm/cpu/tegra20-common/pinmux.c</a></td>*</td>
<td> *</tr>*</td>
<td> *</table>*</td>
</tr>
</table>

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## Power

U-Boot code initializes both the AP power as well as the power to the system
peripherals. Power to the AP is initialized using the PMIC (Power Management IC)
driver. In addition to PMIC, the drivers/power directory includes subdirectories
for power-related drivers for controlling the battery and for the fuel gauge
that measures the current amount of power in the battery. (Chrome OS may or may
not use these additional drivers.)

#### Implementation Notes

The board-specific code is located in
board/*manufacturer*/*boardname*/*boardname.c* (for example,
board/samsung/trats/trats.c). In this file, you implement the following
initialization function, which is called by the file arch/arm/lib/board.c:

> `int power_init_board(void);`

This function initializes the AP power through the PMIC and turns on peripherals
such as the display and eMMC. It also checks the battery if needed.

*<table>*
*<tr>*
*<td><b> Header File</b></td>*
*<td>include/power/pmic.h</td>*
*<td> include/power/battery.h</td>*
*</tr>*
*<tr>*
*<td> <b>Implementation File</b></td>*
*<td>drivers/power/pmic/</td>*
*<td> drivers/power/battery/</td>*
*<td> drivers/power/fuel_gauge</td>*
*</tr>*
*<tr>*
*<td> <b>Makefile</b></td>*
*<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/power/Makefile;h=8c7190156c302b60b9f5b8601bc7e4e7a0594b73;hb=HEAD">drivers/power/Makefile</a></td>*
*</tr>*
*<tr>*
*<td> <b>Example</b></td>*
*<td>CONFIG_POWER_MAX8998 (for PMIC) CONFIG_POWER_BATTERY_ <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=board/samsung/trats/trats.c;h=e540190984740da822678724a71ffed0764b1cb2;hb=HEAD">board/samsung/trats/trats.c</a></td>*
*</tr>*
*</table>*

<table>
</table>

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## Pulse Width Modulation (PWM)

The pulse width modulation (PWM) driver is often used to control display
contrast and LCD backlight brightness.

#### Implementation Notes

This driver requires you to implement generic functions defined in include/pwm.h
as well as chip-specific functions defined in the AP directory. Basic functions
to implement include the following:

*   pwm_init() - sets up the clock speed and whether or not it is
            inverted
*   pwm_config() - sets up the duty and period, in nanoseconds
*   pwm_enable() - enables the PWM driver
*   pwm_disable() - disables the PWM driver

*<table>*
*<tr>*
*<td><b> Header File</b></td>*
*<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=include/pwm.h;h=13acf85933b3afab39f20e04a5520077864f598b;hb=HEAD">include/pwm.h</a> (basic interface)</td>*
*<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=arch/arm/include/asm/arch-tegra20/pwm.h;h=9e03837ccbbc0418c6b7c4d58c187fb4cabc4bce;hb=HEAD">arch/arm/include/asm/arch-tegra20/pwm.h </a>(AP functions)</td>*
*</tr>*
*<tr>*
*<td> <b>Implementation File</b></td>*
*<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=arch/arm/cpu/armv7/tegra20/pwm.c;h=b655c5cd06f28d748a56b9c91367529d0a61cca5;hb=HEAD">arch/arm/cpu/armv7/tegra20/pwm.c</a></td>*
*</tr>*
*<tr>*
*<td> <b>Makefile</b></td>*
*<td>typically, also in AP directory</td>*
*</tr>*
*<tr>*
*<td> <b>Example</b></td>*
*<td> *<table>*</td>*
*<td> *<tr>*</td>*
*<td> *<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=arch/arm/cpu/armv7/tegra20/pwm.c;h=b655c5cd06f28d748a56b9c91367529d0a61cca5;hb=HEAD">arch/arm/cpu/armv7/tegra20/pwm.c</a></td>*</td>*
*<td> *</tr>*</td>*
*<td> *</table>*</td>*
*</tr>*
*</table>*

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## SDMMC and eMMC

The same driver is typically used by both the SDMMC and the eMMC. Secure Digital
Multimedia Memory Card (SDMMC) refers to an external SD card. eMMC is an
internal mass storage device.
We do not currently use eMMC boot blocks. Chrome OS can boot from an external SD
card or from internal eMMC. It is convenient to be able to boot from eMMC for
development purposes.

Chrome OS divides the disk using the EFI partitions table. The kernel is read
directly from the partition without using the file system. Be sure to enable the
EFI partition table in the board file using the following option:

*   #define CONFIG_EFI_PARTITION

#### Requirements

*SDMMC*

*   4-bit.
*   Speed not critical as this device is used only in developer/recovery
            modes.

*eMMC*

*   *8-bit data width, ideally DDR.*
*   *Speed: For reading, 40Mbytes/sec or better is required. Writing
            speed is less critical because the eMMC rarely performs write
            operations in U-Boot.

#### Implementation Notes

Set up the struct mmc and call mmc_register(mmc) for each MMC device (for
example, once for the SDMMC and once for the eMMC). Important functions to
implement include the following:

*   send_cmd() - send a command to the MMC device
*   set_ios() - to set the I/O speed
*   init() - to initialize the driver

Some of the functions perform differently depending on which type of device is
being initialized. For example, mmc_getcd() indicates whether an SC card is
currently in the slot. This function always returns TRUE for an eMMC card.

Key values to set in struct mmc include the following:

*   voltages - supported voltages
*   host_caps - capabilities of your chip
*   f_min - minimum frequency
*   f_max - maximum frequency

*<table>*
*<tr>*
*<td><b> Header File</b></td>*
*<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=include/mmc.h;h=a13e2bdcf16886a755a8cf8b74974b3425ca373b;hb=HEAD">include/mmc.h</a></td>*
*</tr>*
*<tr>*
*<td> <b>Implementation File</b></td>*
*<td><a href="http://git.denx.de/?p=u-boot.git;a=tree;f=drivers/mmc;hb=HEAD">drivers/mmc</a></td>*
*</tr>*
*<tr>*
*<td> <b>Makefile</b></td>*
*<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/mmc/Makefile;h=a1dd7302bfc732122fe6620ded02b46dd8c817fd;hb=HEAD">drivers/mmc/Makefile</a> </td>*
*</tr>*
*<tr>*
*<td> <b>Example</b></td>*
*<td>CONFIG_TEGRA_MMC drivers/mmc/tegra_mmc.c</td>*
*</tr>*
*</table>*

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## SPI

The SPI driver specifies the list of known SPI buses in a structure stored in
the drivers/spi/ directory (for an example, see the exynos_spi_slave structure
in drivers/spi/exynos_spi.c). The first field in this structure is another
spi_slave structure, slave, which describes the bus (slave.bus) and chip select
(slave.cs) for each SPI slave device.

Ordering of multiple SPI buses is specified in the device tree file (for
example, board/samsung/dts/exynos5250-smdk5250.dts).

Each device connected to the SPI bus (for example, the EC, touchpad, and SPI
flash could all be connected to this bus) contains a reference to the `spi_slave
structure` in its implementation file(for example, see
`[drivers/mtd/spi/winbond.c](http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/mtd/spi/winbond.c;h=f6aab3d32f459d82d0a9e606f4382fe67981160f;hb=HEAD)`
for the SPI flash chip and `drivers/misc/cros-ec.c for the EC`.

#### Implementation Notes

Use this function to set up a device on the SPI bus:

```none
    struct spi_slave *spi_setup_slave(unsigned int busnum,           // bus number 
                                      unsigned int cs,               // chip select
                                      unsigned int max_hz,           // maximum SCK rate, in Hz
                                      unsigned int mode)             // mode
```

To drop the device from the bus, use this function:

```none
    void spi_free_slave(struct spi_slave *slave)
```

Other key functions are used to claim control over the bus (so communication can
start) and to release it:

```none
    int spi_claim_bus(struct spi_slave *slave);
```

```none
    void spi_release_bus(struct spi_slave *slave);
```

When a slave claims the bus, it maintains control over the bus until you call
the `spi_release_bus()` function.

<table>
<tr>
<td><b> Header File</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=include/spi.h;h=60e85db9a46e052c97b638c58bb93114a378e3b7;hb=HEAD">include/spi.h</a></td>
</tr>
<tr>
<td> <b>Implementation File</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/spi/exynos_spi.c;h=be60ada2ba43a17ecda4a8b968b08b791f0af73f;hb=HEAD">drivers/spi/exynos_spi.c</a></td>
</tr>
<tr>
<td> <b>Makefile</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/spi/Makefile;h=824d357d94818ea9c05fa5b0d437aff181b50195;hb=HEAD">drivers/spi/Makefile</a></td>
</tr>
<tr>
<td> <b>Example</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/spi/exynos_spi.c;h=be60ada2ba43a17ecda4a8b968b08b791f0af73f;hb=HEAD">drivers/spi/exynos_spi.c</a></td>
</tr>
</table>

*[ back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## SPI Flash

The SPI flash stores the firmware and consists of a Read Only section that
cannot be changed after manufacturing and a Read/Write section that can be
updated in the field.

#### Requirements

#### *Size:* The SPI flash is typically 4 or 8 Mbytes, with half allocated to the Read Only portion and half to the Read/Write portion.

#### *Speed:* Because this component directly affects boot time, it must be fast: 5 Mbytes/second performance is required.

#### Implementation Notes

There are several steps to implementing the SPI flash. First, define a prototype
for your SPI flash, with a name in the form spi_flash_probe_*yourDeviceName*().
Add this function to spi_flash_internal.h.

Also, modify spi_flash.c, adding the config option that links your prototype to
the #define. For example, for the Winbond SPI flash, these lines link the
spi_flash_probe_winbond() prototype to its #define:

> `#ifdef CONFIG_SPI_FLASH_WINBOND`

> ` { 0, 0xef, spi_flash_probe_winbond, },`

> `#endif`

This code passes in the first byte of the chip's ID (here, 0xef). If the ID code
matches that of the attached SPI flash, the struct spi_flash is created.

You also need to define the spi_flash structure that describes the SPI flash:

```none
struct spi_flash {  struct spi_slave *spi;  const char       *name;  /* Total flash size */  u32              size;  /* Write (page)  size */  u32              page_size;  /* Erase (sector) size */  u32              sector_size;  int              (*read)(struct spi_flash *flash, u32 offset, size_t len, void *buf);  int              (*write)(struct spi_flash *flash, u32 offset, size_t len, const void *buf);  int              (*erase)(struct spi_flash *flash, u32 offset, size_t len);};
```

In this structure, you set the appropriate fields and either implement the
functions for reading, writing, and erasing the SPI flash or use existing
functions defined in spi_flash_internal.h.

<table>
<tr>
<td> <b>Header File</b> </td>
<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=include/spi_flash.h;h=9da90624f23d665fc58c95e37da02096b1adde07;hb=HEAD">include/spi_flash.h</a></td>
<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/mtd/spi/spi_flash_internal.h;h=141cfa8b26d75e6e816c7315530854a93035b803;hb=HEAD">drivers/mtd/spi/spi_flash_</a><a href="http://internal.h">internal.h</a></td>
</tr>
<tr>
<td> <b>Implementation File</b></td>
<td> <a href="http://git.denx.de/?p=u-boot.git;a=tree;f=drivers/mtd/spi;hb=HEAD">drivers/mtd/spi</a>/*yourDriver.c*</td>
</tr>
<tr>
<td> <b>Makefile</b></td>
<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/mtd/spi/Makefile;h=90f83924e2bf23b9ab4c621495ca236c14a19acd;hb=HEAD">drivers/mtd/spi/Makefile</a>/</td>
</tr>
<tr>
<td> <b>Example</b> </td>
<td>CONFIG_SPI_FLASH and</td>
<td> CONFIG_SPI_FLASH_winbond</td>
<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/mtd/spi/winbond.c;h=f6aab3d32f459d82d0a9e606f4382fe67981160f;hb=HEAD">drivers/mtd/spi/winbond.c</a></td>
</tr>
</table>

[*back to
top*](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)

## Thermal Management Unit (TMU)

The thermal management unit (TMU) monitors the temperature of the AP. If the
temperature reaches the upper limit, the TMU can do one of the following:

*   Slow the system down until it cools to a certain point
*   Power off the system before it melts

The `dtt` command can be used on the console to perform these functions.

#### Implementation Notes

To implement this driver, you have two tasks:

*   Create a TMU driver and connect it to the `dtt` command by modifying
            the file `common/cmd_dtt.c`.
*   Set up a thermal trip in your board file. The temperature limits
            should be defined in the device tree.

<table>
<tr>
<td><b> Header File</b></td>
</tr>
<tr>
<td> <b>Implementation File</b></td>
<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=common/cmd_dtt.c;h=cd94423d21910d61055bade1bc285dbed099b9f9;hb=HEAD">common/cmd_dtt.c</a></td>
</tr>
<tr>
<td> <b>Makefile</b></td>
</tr>
<tr>
<td> <b>Example</b></td>
<td>board/samsung/smdk5250/smdk5250.c</td>
</tr>
</table>

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## Timer

The U-Boot timer is measured in milliseconds and increases monotonically from
the time it is started. There is no concept of time of day in U-Boot.

#### Implementation

The timer requires two basic functions:

*   `timer_init() - `called early in U-Boot, before relocation
*   `get_timer() - can be called any time after initialization

#### Typical Use

The timer is commonly used in U-Boot while the system is waiting for a specific
event or response. The following example shows a typical use of the timer that
can be used to ensure that there are no infinite loops or hangs during boot:

```none
start = get_timer(0)
while ( ...) {
   if (get_timer (start) > 100){
```

```none
      debug "%s:Timeout while waiting for response\^");
      return -1;
   }
   .
   .
   .
}
```

A base_value is passed into the get_timer() function, and the function returns
the elapsed time since that base_value (that is, it subtracts the base_value
from the current value and returns the difference).

#### Other Uses

The `timer_get_us()` function returns the current monotonic time in
microseconds. This function is used in Chrome OS verified boot to obtain the
current time. It is also used by bootstage to track boot time (see
common/bootstage.c). It should be as fast and as accurate as possible.

### Delays

The __udelay() function, also implemented in `timer.c`, is called by U-Boot
internally from its `udelay (delay for a period in microseconds)` and `mdelay
(delay for a period in milliseconds)` functions (declared in `common.h`):

> void __udelay (unsigned long);

This function introduces a delay for a given number of microseconds. The delay
should be as accurate as possible. In no case should the delay occur for less
than the specified time.

#### Command Line Interface

The time command can be used to time other commands. This feature is useful for
benchmarking.

*<table>*
*<tr>*
*<td><b> Header File</b></td>*
*<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=include/common.h;h=4ad17eafb9b89a15975b0b8945a1b70c198d8d88;hb=HEAD">include/common.h</a></td>*
*</tr>*
*<tr>*
*<td> <b>Implementation File</b></td>*
*<td>timer.c, in the AP directory</td>*
*</tr>*
*<tr>*
*<td> <b>Makefile</b></td>*
*</tr>*
*<tr>*
*<td> <b>Example</b></td>*
*<td> *<table>*</td>*
*<td> *<tr>*</td>*
*<td> *<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=arch/arm/cpu/armv7/s5p-common/timer.c;h=bb0e795e66823d4963acd309ebbdd3e67bf7d4a4;hb=HEAD">arch/arm/cpu/armv7/s5p_common/timer.c</a></td>*</td>*
*<td> *</tr>*</td>*
*<td> *</table>*</td>*
*</tr>*
*</table>*

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## Trusted Platform Module (TPM)

The Trusted Platform Module (TPM) is the security chip that maintains rollback
counters for firmware and kernel versions and stores keys for the system. The
TPM is normally connected on an LPC, SPI, or I2C bus.

#### Requirements

The TPM is a critical contributor to Chromium OS boot time, so it must be as
fast as possible.

#### Implementation

All message formatting is handled in vboot_reference, so the functions you must
implement for the TPM in U-Boot are for low-level initialization, open/close,
and send/receive functionality:

```none
int tis_init();int tis_open();
int tis_close();
int tis_sendrecv(const uint8_t *sendbuf, size_t send_size, uint8_t *recvbuf,                    size_t *recv_len);
```

<table>
<tr>
<td> <b>Header File</b> </td>
<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=include/tpm.h;h=6b21e9c7349c298c6fc9253324112db58935f805;hb=HEAD">include/tpm.h</a></td>
</tr>
<tr>
<td> <b>Implementation File</b></td>
<td> drivers/tpm/*yourDriver.c*</td>
</tr>
<tr>
<td> <b>Makefile</b></td>
<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/tpm/Makefile;h=be11c8b595f1367daffc623aff8aa7c694a36749;hb=HEAD">drivers/tpm/Makefile</a></td>
</tr>
<tr>
<td> <b>Example</b></td>
<td> CONFIG_GENERIC_LPC_TPM</td>
<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/tpm/generic_lpc_tpm.c;h=6c494eb98a339c5b77859bdbc1ccbd8e6a6a6ff5;hb=HEAD">drivers/tpm/generic_lpc_tpm.c</a></td>
</tr>
</table>

[*back to
top*](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)

## UART

The UART is used for serial communication to drive the serial console, which
provides output during the boot process. This console allows the user to enter
commands for testing and debugging.

#### Requirements

*   115K2 baud
*   3-wire port (no handshaking)

#### Implementation Notes

You register this driver by setting up a structure (serial_device{}) and then
call serial_register(). The AP typically has built-in serial functions that this
driver can use.

```none
struct serial_device {	/* enough bytes to match alignment of following func pointer */	char	name[16];	int	(*start)(void);	int	(*stop)(void);	void	(*setbrg)(void);	int	(*getc)(void);	int	(*tstc)(void);	void	(*putc)(const char c);	void	(*puts)(const char *s);#if CONFIG_POST & CONFIG_SYS_POST_UART	void	(*loop)(int);#endif	struct serial_device	*next;};
```

In the final shipping product, the console needs to run in silent mode. For
example, this code in the driver tells the console to run in silent mode:

if (fdtdec_get_config_int(gd-&gt;fdt_blob, "silent_console", 0))

gd-&gt;flags |= GD_FLG_SILENT;

*<table>*
*<tr>*
*<td><b> Header File</b></td>*
*<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=include/serial.h;h=14f863ed20ec73f67ed2df791ad2ad4b904b689c;hb=HEAD">include/serial.h</a></td>*
*</tr>*
*<tr>*
*<td> <b>Implementation File</b></td>*
*<td> <a href="http://git.denx.de/?p=u-boot.git;a=tree;f=drivers/serial;h=82deabb40ad2e94538a483ad8abfe9600b48d330;hb=HEAD">drivers/serial/</a></td>*
*</tr>*
*<tr>*
*<td><b> Makefile</b></td>*
*<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/serial/Makefile;h=5e8b64873d9bb3beff8d9b7c41437908645601d8;hb=HEAD">drivers/serial/Makefile</a></td>*
*</tr>*
*<tr>*
*<td> <b>Example</b></td>*
*<td>CONFIG_SYS_NS16550 <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/serial/serial_ns16550.c;h=c1c0134bcb3fdd8ec7e6c8a93e9f463291b5b9ca;hb=HEAD">drivers/serial/serial_ns16550.c</a></td>*
*</tr>*
*</table>*

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## USB Host

USB host is used by Chrome OS in two ways:

*   *In recovery mode:* U-Boot software must be able to detect the
            presence of a USB storage device and load a recovery image from the
            device.
*   *For Ethernet connectivity:* On Chrome OS platforms without a
            built-in Ethernet connector, a USB-to-Ethernet adapter can be used
            to provide an Ethernet connection.

On x86 systems, USB is often used to connect other peripherals, such as cameras
and SD card readers, but Chrome OS does not require U-Boot drivers for these USB
peripherals.

U-Boot supports both the EHCI and OHCI standards for USB. Be sure to test a
variety of common USB storage devices to ensure that they work with your U-Boot
driver.

#### USB Hub

In some cases, the USB or AP is connected to a USB hub to expand the number of
USB ports. The board file may need to power up this hub and configure it. Be
careful to power up the hub only if USB is used by the system.

#### EFI Partition Table

Chrome OS uses an EFI partition table. To enable this table, add the following
entry to the board file:

*   `#define CONFIG_EFI_PARTITION`

#### Implementation Notes

USB should not be initialized in the normal boot path. Loading a kernel from USB
is a slow process (it could take a second or more) as well as a potential
security risk.

The two main functions to implement are the following:

```none
int ehci_hcd_init(int index, struct ehci_hccr **hccr, struct ehci_hcor **hcor)
int ehci_hcd_stop(int index)
```

These functions create (and destroy) the appropriate control structures to
manage a new EHCI host controller. Much of this interface is standardized and
implemented by U-Boot. You just point U-Boot to the address of the peripheral.

#### Configuration Options

The configuration options for USB are specified in the board configuration file
(for example, include/configs/seaboard.h). There are a number of #defines for
different aspects of USB, including the following:

<table>
<tr>
<td> CONFIG_USB_HOST_ETHER</td>
<td> Enables U-Boot support for USB Ethernet</td>
</tr>
<tr>
<td> CONFIG_USB_ETHER_ASIX</td>
<td> Enables the driver for the ASIX USB-to-Ethernet adapter </td>
</tr>
<tr>
<td> CONFIG_USB_ETHER_SMSC95XX</td>
<td> Enables the driver for the SMSC95XX USB-to-Ethernet adapter</td>
</tr>
<tr>
<td> CONFIG_USB_EHCI</td>
<td> Enables EHCI support in U-Boot</td>
</tr>
<tr>
<td> CONFIG_USB_EHCI_TEGRA</td>
<td> Enables EHCI for a specific chip</td>
</tr>
<tr>
<td> CONFIG_USB_STORAGE</td>
<td> Enables a USB storage device</td>
</tr>
<tr>
<td> CONFIG_CMD_USB </td>
<td> Enables the USB command</td>
</tr>
</table>

*<table>*
*<tr>*
*<td><b> Header File</b></td>*
*<td><a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/usb/host/ehci.h;h=1e3cd793b6091650ed1e7ed61b6bfca3f7046a69;hb=HEAD">drivers/usb/host/ehci.h</a></td>*
*<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/usb/host/ohci.h;h=d977e8ff3ce31ee93990a68990590bafdc3800ea;hb=HEAD">drivers/usb/host/ohci.h</a></td>*
*</tr>*
*<tr>*
*<td> <b>Implementation File</b></td>*
*<td>drivers/usb/host/ehci-controllerName.c</td>*
*<td> *<table>* </td>*
*<td> *<tr>*</td>*
*<td> *<td>drivers/usb/host/ohci-controllerName.c</td>*</td>*
*<td> *</tr>*</td>*
*<td> *</table>*</td>*
*</tr>*
*<tr>*
*<td> <b>Makefile</b></td>*
*</tr>*
*<tr>*
*<td> <b>Example</b></td>*
*<td> <a href="http://git.denx.de/?p=u-boot.git;a=blob;f=drivers/usb/host/ehci-tegra.c;h=a1c43f8331787c7cf84e0eabcf0d373ae1ebabe4;hb=HEAD">drivers/usb/host/ehci-tegra.c</a></td>*
*</tr>*
*</table>*

*[back to
top](/chromium-os/firmware-porting-guide/u-boot-drivers#TOC-Board-Configuration)*

## Other sections in *U-Boot Porting Guide*

1. [Overview of the Porting
Process](/chromium-os/firmware-porting-guide/1-overview)

2. [Concepts](/chromium-os/firmware-porting-guide/2-concepts)

3. U-Boot Drivers (this page)
