---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/firmware-porting-guide
  - Firmware Overview and Porting Guide
page_name: firmware-ec-write-protection
title: Firmware / EC Write Protection
---

On customer-ship devices, read-only [AP
firmware](/chromium-os/developer-information-for-chrome-os-devices/custom-firmware)
and [embedded controller firmware](/chromium-os/ec-development) is
write-protected. By design, this write protection cannot be removed without
physical access to the device.

**Application Processor (AP) Firmware**

AP firmware (also known as "SOC firmware", "host firmware", "main firmware" or
even "BIOS") typically resides on a SPI ROM. Protection registers on the SPI ROM
are programmed to protect the read-only region, and these registers normally
cannot be modified while the SPI ROM WP (write protect) pin is asserted. This
pin is asserted through various physical means (see below), but with effort,
users can unprotect devices they own.

**Embedded Controller (EC) Firmware**

The Chrome OS Embedded Controller (EC) typically has a WP input pin driven by
the same hardware that generates SOC firmware write protect. While this pin is
asserted, certain debug features (eg. arbitrary I2C access through host
commands) are locked out. Some ECs [load code from external
storage](/chromium-os/ec-development/ec-image-geometry-spec), and for these ECs,
RO protection works similar to SOC firmware RO protection (WP pin is asserted to
EC SPI ROM). Other ECs use internal flash, and these ECs emulate SPI ROM
protection registers, disabling write access to certain regions while the WP pin
is asserted.

**Methods of Asserting Write Protect**

Throughout the history of Chrome OS devices, three main methods have been
implemented for asserting (and removing) write protect:

*   Switch - a toggle switch asserts WP to the SOC firmware SPI ROM and
            EC. WP can be deasserted by disassembling the device and flipping
            the switch.
*   Screw - a screw shorts a pad on the PCB. While this screw is
            inserted, WP is asserted. WP can be deasserted by disassembling the
            device and removing this special screw.
*   [cr50](https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/board/cr50/)
            - a special security chip asserts the WP signal. While a battery is
            present on the device, the WP signal will be asserted. Disassembling
            the device and physically disconnecting the battery causes WP to be
            deasserted.

More information about which protection method is used for a particular device,
and where to locate the switch / screw, is available on the [developer info
page](/chromium-os/developer-information-for-chrome-os-devices).

**Disabling Write Protect for Screw / Switch Protection**

1.  Disassemble the device, locate the WP screw / switch, and remove it.
2.  Reassemble the device, then boot to [developer
            mode](/chromium-os/chromiumos-design-docs/developer-mode).
3.  To disable SOC firmware protection, run
            "[flashrom](/chromium-os/packages/cros-flashrom) -p host
            --wp-disable". To disable EC firmware protection (external storage),
            run "flashrom -p ec --wp-disable". It's not possible to disable EC
            firmware WP once enabled for ECs with internal storage. Instead, RO
            firmware must be reflashed while WP is deasserted. RO firmware will
            then be writable, regardless of the state of WP.
4.  Now that SPI / EC protection registers are cleared, the screw /
            switch can be re-inserted and the read-only region of SOC / EC
            firmware will remain writable.

**Disabling Write Protect for cr50**

If you have a
[suzyQ](https://chromium.googlesource.com/chromiumos/third_party/hdctools/+/HEAD/docs/ccd.md#suzyq-suzyqable),
you can disable write protect without opening the case. You can [enable
Case-Closed Debugging
(CCD)](https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/docs/case_closed_debugging_cr50.md#ccd-setup)
and then [use a cr50 console command to disable write
protect](https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/docs/case_closed_debugging_cr50.md#wp-control).
The write protect command is only accessible through suzyQ.

If you don't want to go through the ccd open process or don't have a suzyQ, you
can open the case and remove the battery to disable write protect.

1.  Disassemble the device, locate the battery connector, remove the
            battery connector from the PCB to disconnect the battery.
2.  Reassemble the device, insert the original OEM charger (necessary
            since the battery is no longer providing power to the system), then
            boot to developer mode.
3.  See (3) above, commands to disable protection are identical.
4.  Disassemble, re-attach the battery, then reassemble.
