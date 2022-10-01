---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/ec-development
  - Chromium Embedded Controller (EC) Development
page_name: usb-low-power-boot
title: USB-C / PD Low Power Boot
---

(Additional discussion on [crbug.com/537269](http://crbug.com/537269))

**==Problem==**:

Chrome OS devices may require more than 15W of power, even in firmware. For
security reasons, our EC does not attempt to negotiate a PD power contract until
it has jumped to RW, so we are limited to pulling 15W (3A @ 5V) from a USB-C /
PD charger in RO. In addition, we have no knowledge of how much power a PD
charger may be able to deliver while in EC RO. Therefore, in order to guarantee
that a device is able to boot successfully into Chrome OS, the system must have
a minimum battery level before booting, even when a PD charger is attached.

**==Samus==**:

The solution implemented on Samus is to wait for 1% battery charge level before
allowing power-on. If a high-power PD charger is attached, we’ll negotiate a
sufficient power contract once jumping to EC RW, and the system will be usable
indefinitely. If a low-power charger is attached, the 1% battery level should be
enough to allow us to boot Chrome OS, see a low-power shutdown message, and
automatically power down cleanly.

[<img alt="image"
src="/chromium-os/ec-development/usb-low-power-boot/samus_low_bat.png">](/chromium-os/ec-development/usb-low-power-boot/samus_low_bat.png)

Samus uses a flashing red lightbar to notify the user that the device is waiting
for sufficient battery charge to allow boot. Future PD-powered devices may not
have such a lightbar.

**==Future Improvement==**:

Ideally we’d like to handle the most common user case (high-powered charger
attached) without needless delay. Here is a proposed method to do so:

*   If the system has enough battery charge (defined as &gt;= 1% on
            Samus, but can be defined arbitrarily on other devices) then it will
            boot as normal, since any charger (or even no charger) will be
            sufficient to cleanly boot into Chrome OS and then cleanly shutdown.
*   If the system does not have enough battery charge and no charger is
            attached, the EC will disallow power on through inhibiting the power
            button. From the user POV, this is a “dead battery” case.
*   If a charger is attached, and the charger advertises &lt; 15W
            through its CC pullup, the EC will disallow power on through
            inhibiting the power button. The charger isn’t likely to speak PD,
            and isn’t likely to be able to provide &gt; 15W in any case.
*   If a charger is attached, and the charger advertises 15W through its
            CC pullup, the EC will set a limit power flag (“LP”) indicating that
            the system is in a low-battery + low-power charger state, and boot
            the AP.

*   If LP is set and the system is booting into recovery mode, the AP
            will power itself down. Recovery mode is never allowed to jump to EC
            RW.
*   If LP is set and the system is not booting into recovery mode, after
            SW sync and jump to EC RW, the AP will wait for some timeout period
            (3 sec) while polling LP. If the EC negotiates a sufficient PD power
            contract (perhaps &gt;= 30W, but should be adjusted by platform)
            while in EC RW, it will clear the LP flag.
*   If LP is cleared within the timeout period, the AP will boot as
            normal.
*   If LP is not cleared within the timeout period, the AP will shut
            itself down.

This proposal can only be implemented if the device is able to power-on the AP
through software sync completion while consuming &lt; 15W. In order to meet this
requirement, devices may have to limit power through various means, such as
disabling USB ports, asserting PROCHOT, etc.

[<img alt="image"
src="/chromium-os/ec-development/usb-low-power-boot/new_low_bat.png">](/chromium-os/ec-development/usb-low-power-boot/new_low_bat.png)

**==Future Chromeboxes:==**

Chromeboxes are a special case because they lack a battery. Such devices may
never be able to boot a low-power charger. If we were to follow the method
above, Chromeboxes would ==never== be able to boot into recovery mode. Thus, the
following additional changes need to be made for batteryless devices:

*   If a charger is attached and the system is booting into
            user-triggered (not “broken” recovery mode) recovery mode, the EC
            will attempt to negotiate a PD contract from RO EC FW. If a
            sufficient contract is negotiated, the system will boot to recovery
            as normal. If not, the LP flag will be set.
*   If LP is set and the system is booting into recovery mode, the AP
            will notify the user of the broken condition by some method, such as
            blinking LEDs, or displaying the “broken” screen (power-allowing).

We will assume that the charger attached for user-triggered recovery is
trustable, and will provide instructions to users that they should use the OEM
charger (power-cycled beforehand for extra paranoia) while booting in recovery
mode for maximum security.

**==Other Idea Considered (but rejected):==**

*   EC-RO does RSA-3072 verification of EC-RW and jumps to it without
            involvement of the AP, in case the charger is not sufficient to boot
            the AP at all (&lt; 15W). This idea was rejected, because a charger
            that advertises 2.5W - 7.5W isn’t likely to speak PD or be able to
            negotiate to provide &gt; 15W.
*   Display “low battery” firmware screen to notify the user that the
            battery isn’t sufficiently charged. This requires initializing the
            display, which may push system power usage above 15W. We can revisit
            this idea if we have power data that shows it is feasible.
*   Allow Chromeboxes (and other battery-less devices) to jump to do SW
            sync and then jump to EC RW while in recovery mode. If we’re
            flashing an EC RW image via SW sync in recovery mode, it must come
            from RO FW, so it’s presumably no different than the EC RO image.
            Therefore, this idea is no different than allowing PD communication
            from EC RO in recovery mode.
*   Have the AP determine LP status by checking the battery / charger
            state through various host commands, rather than relying upon a
            single memmap flag set by the EC. Though this would simplify
            implementation, it means that board-specific charge / power related
            constants (such as minimum power level sufficient to not set LP)
            would have to live somewhere in system FW.

**==CLs:==**

**<https://chromium-review.googlesource.com/#/c/306774/>**

**<https://chromium-review.googlesource.com/#/c/307885/>**

**<https://chromium-review.googlesource.com/#/c/307952/>**
