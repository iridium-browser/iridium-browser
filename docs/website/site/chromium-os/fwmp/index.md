---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: fwmp
title: Firmware Management Parameters
---

[TOC]

Firmware Management Parameters (aka FWMP) are optional settings that can be
stored in the TPM to control some aspects of developer mode for developers and
system administrators.

## What's in the FWMP?

The FWMP contains a set of flags and an optional developer key hash.

The flags are as follows:

<table>
<tr>
<td> Flag</td>
<td>Name </td>
<td>Meaning </td>
</tr>
<tr>
<td> 0x01</td>
<td> FWMP_DEV_DISABLE_BOOT</td>
<td>Disable developer mode. If this flag is set, booting the device in developer mode will take you straight to the TONORM screen, which asks you to confirm turning developer mode off.</td>
</tr>
<tr>
<td> 0x02</td>
<td> FWMP_DEV_DISABLE_RECOVERY</td>
<td>Disable developer features of recovery images.</td>
</tr>
<tr>
<td> 0x04</td>
<td> FWMP_DEV_ENABLE_USB</td>
<td>Enable Ctrl+U to boot from USB.</td>
<td>Same effect as 'crossystem dev_boot_usb=1'</td>
</tr>
<tr>
<td> 0x08</td>
<td> FWMP_DEV_ENABLE_LEGACY</td>
<td>Enable Ctrl+L to boot from legacy OS</td>
<td>Same effect as 'crossystem dev_boot_legacy=1'</td>
</tr>
<tr>
<td> 0x10</td>
<td> FWMP_DEV_ENABLE_OFFICIAL_ONLY</td>
<td>Only accept developer images signed with the official Chrome OS key.</td>
<td>Same effect as 'crossystem dev_boot_signed_only=1'</td>
</tr>
<tr>
<td> 0x20</td>
<td> FWMP_DEV_USE_KEY_HASH</td>
<td>Only accept developer images signed a specific key. If this is set, the SHA-256 digest of the kernel key data is compared with the digest stored in the FWMP. This enables you to decide what developer images will boot on your device, instead of blindly trusting them all. Particularly handy when combined with FWMP_DEV_ENABLE_USB.</td>
</tr>
</table>

The key hash is the SHA-256 of the key data for the kernel key. There isn't a
tidy way to extract this from a keyblock yet; coming soon.

## Setting the FWMP

Use cryptohome to set the FWMP. To do this, the TPM must just have been owned,
or you must know the owner password:

> cryptohome --action=set_firmware_management_parameters
> --flags={flags_as_decimal_or_0xhex}
> \[--developer_key_hash={hash_as_hex_string}\]

To remove the FWMP:

> cryptohome --action=remove_firmware_management_parameters

And, of course, you can see what it contains; this works even if you don't know
the owner password:

> cryptohome --action=get_firmware_management_parameters

System administrators can automatically set the FWMP on enterprise-enrolled
devices during the initial device enrollment.

## Removing the FWMP

If you have somehow locked yourself out of your system - say, by setting
FWMP_DEV_DISABLE_BOOT, or by setting FWMP_DEV_USE_KEY_HASH but specifying the
wrong hash, all is not lost.

If your Chrome OS device is NOT enterprise-enrolled, disable developer mode,
recovery your system to a fresh state, then log in. That will automatically
remove the FWMP. And whatever else was on your system.

If your Chrome OS device is enterprise-enrolled, see your system administrator.

## I Can't Get Into Developer Mode

If you've enabled developer mode, and you're getting this warning at boot time:

> Developer mode is disabled on this device by system policy.

> For more information, see http://www.chromium.org/chromium-os/fwmp

that's because FWMP_DEV_DISABLE_BOOT is set. See the previous section on
removing the FWMP.
