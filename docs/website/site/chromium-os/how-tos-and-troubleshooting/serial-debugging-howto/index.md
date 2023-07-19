---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: serial-debugging-howto
title: Chromium OS [serial] Console Debugging HOWTO
---

[TOC]

## Introduction

The kernel team has been working to remove cruft from the Chromium OS kernel
configs. Since there are no Chromebooks with a serial port, serial port drivers
were removed from the stock configuration, which is a minor inconvenience for
those who still do a lot of debugging using the serial port.

A quick howto was e-mailed some time ago
[here](http://groups.google.com/a/chromium.org/group/chromium-os-dev/browse_thread/thread/5c8a44b76cecb2ce)
for using mini-PCIe serial cards, but the information is somewhat limited in
scope and is worth correcting and modernizing. There are several ways to
accomplish what is described here, please feel free to chip in.

This HOWTO assumes you have built a bootable Chromium OS image and are not
afraid to edit some config files. We'll assume the target platform is 32-bit
"x86-generic".

## Enable Kernel Serial Drivers

When the team goes to release a board to the public, right before release, the
default serial console enable flags (USE and TTY_CONSOLE \[see **Enabling Serial
Console Login** below\] in `make.conf` are reverted.

For x86 based systems, use the pcserial USE flag when building your kernel:

```none
chromium-os$ USE=pcserial cros_workon_make --board="${BOARD}" chromeos-kernel
```

For Exynos-based systems, add the samsung_serial USE flag:

```none
chromium-os$ USE=samsung_serial cros_workon_make --board="${BOARD}" chromeos-kernel
```

## Note that on some systems these serial drivers are always loaded due to them being needed by some hardware, ex: Bluetooth modules sometimes communicate with the AP via serial.

Note that you will likely need to replace "chromeos-kernel" in your
cros_workon_make call with an ebuild relevant to your board, like
chromeos-kernel-3_18 for asuka.

## Enable Console Logging

By default, the kernel command line is set up to enable no consoles and thus log
no output. This leads to a cleaner/speedier boot, but is not helpful when you
need to see failure messages. Thus you'll need change some kernel command-line
parameters to get meaningful output.

When you create the image with `cros build-image` simply use the
`--enable-serial` flag to select the console for logging output.  The value it
takes is exactly the same as what the kernel expects for `console=` (see the
next section for more details).

```bash
$ cros build-image --enable-serial ttyS0
```

Note that every board will have a a different bus layout resulting in
potentially different ttyS# port for serial console. See **Platform Specific
Settings**, below.

### The console= option

The console= option is described in detail in
[kernel-parameters.txt](https://chromium.googlesource.com/chromiumos/third_party/kernel/+/HEAD/Documentation/kernel-parameters.txt)
in the Linux kernel source tree.

### Platform Specific Settings

#### Virtual Machines (QEMU/KVM)

If you boot the image in a VM, and you use the
[cros_start_vm](/chromium-os/how-tos-and-troubleshooting/running-chromeos-image-under-virtual-machines)
script to launch it, then serial output will be logged to a
`/tmp/kvm.xxx.serial` file (exact name will be shown when starting the VM).

If you want to interact with the serial console directly, you'll need to launch
qemu yourself and pass the -nographics flag directly. The serial device should
be connected to stdin/stdout by default.

#### X86

On most x86 platforms, ttyS0 will refer to the southbridge's UART accessed via
ports specified in Super IO configuration space. However, for add-in PCIe cards
you may need to specify a MMIO address to access the UART. For example,
"console=uart8250,mmio,0x50401000,115200n8". An alternative is to specify the
ttyS# port configured by the kernel for the specific hardware and connection
that you're testing on. (E.g. "ttyS2".)

See Appendix A below for more details.

#### ARM Exynos

You probably want to use ttySAC3 for your console.

## Enabling Serial Console Login

See the [Controlling Enabled Consoles
document](/chromium-os/developer-guide/using-serial-tty) for all the details.

## Build Chromium OS image using your custom kernel config

Set the `pcserial` `USE` flag when calling `cros build-packages`:

```bash
USE=pcserial cros build-packages
```

## Appendix

### A: How to find alternate IO and MMIO addresses for your UART

If you already have UART support but simply do not know what options to pass to
the kernel, you may examine a few sources to figure it out.

Method 1: Look at /proc/iomem

localhost ~ # cat /proc/iomem | grep serial

**50401000-50401007 : serial**

**note: for two port pci-serial cards you will see two entries like this:**

**e0801000-e0801008 : serial**

**e0801200-e0801207 : serial**

and the each range will correspond to the two different serial ports

Method 2: Look at PCI device config

localhost chronos # lspci | grep -i serial

02:00.0 Serial controller: NetMos Technology PCIe 9901 Multi-I/O Controller

localhost chronos # lspci -v -s 02:00.0

02:00.0 Serial controller: NetMos Technology PCIe 9901 Multi-I/O Controller
(prog-if 02 \[16550\])

Subsystem: Device a000:1000

Flags: bus master, fast devsel, latency 0, IRQ 16

**I/O ports at 2000 \[size=8\]**

**Memory at c0401000 (32-bit, non-prefetchable) \[size=4K\]**

**Memory at c0400000 (32-bit, non-prefetchable) \[size=4K\]**

Capabilities: \[80\] Power Management version 3

Capabilities: \[88\] MSI: Enable- Count=1/32 Maskable- 64bit+

Capabilities: \[c0\] Express Legacy Endpoint, MSI 00

Kernel driver in use: serial

Method 3: Look at dmesg for lines such as this:

\[ 0.676698\] Serial: 8250/16550 driver, 4 ports, IRQ sharing enabled

\[ 0.678173\] serial 0000:04:00.3: PCI INT D -&gt; GSI 18 (level, low) -&gt; IRQ
18

\[ 0.678716\] 1 ports detected on Oxford PCI Express device

\[ 0.678910\] ttyS0: detected caps 00000700 should be 00000100

\[ 0.679365\] 0000:04:00.3: ttyS0 at **MMIO** **0x50401000** (irq = 18) is a
16C950/954

\[ 0.679899\] console \[ttyS0\] enabled, bootconsole disabled
