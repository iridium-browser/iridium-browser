---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: debugging-features
title: Debugging Features
---

[TOC]

This page describes Chromium OS features that are supported beginning in release
M41.

## Introduction

You can enable debugging features on your Chrome OS device to support installing
and testing custom code on your device. These features allow you to:

*   Remove rootfs verification so you can modify OS files
*   Enable SSH access to the device using the standard test keys so you
            can use tools such as `cros flash` to access the device
*   Enable booting from USB so you can install an OS image from a USB
            drive
*   Set both the dev and the system root login password to a custom
            value so you can manually SSH into the device

Debugging features lower system security, primarily by allowing remote access
via SSH using publicly known keys. For this reason, you can enable them only
during the Out of the Box Experience (OOBE) when the device is booted in
Developer Mode. Once you've finished OOBE (e.g. you've logged in to an account),
you can't access this section again. You'd have to powerwash it to reset back.

## Enabling debugging features

To enable debugging features, do the following:

1.  Use the [powerwash
            process](https://support.google.com/chromebook/answer/183084) or the
            [recovery
            process](https://support.google.com/chromebook/answer/1080595) to
            wipe your hard drive. After the device reboots and you sign in
            again, you’ll see the first screen.
    **Note:** For managed Chrome OS devices, do NOT follow the powerwash or
    recovery instructions because you won’t be able re-enroll the device
    afterwards. Instead, [wipe the data on your Chrome OS
    device](https://support.google.com/chrome/a/answer/1360642) to reset the
    device.
2.  Set the device to Developer Mode (see [Developer Information for
            Chrome OS
            Devices](http://www.chromium.org/chromium-os/developer-information-for-chrome-os-devices)).
            The system reboots, then displays the “OS Verification is OFF"
            screen.
3.  Press Ctrl+D to dismiss this screen. The device reboots and shows
            the first screen.
4.  Click the **Enable debugging features** link. A confirmation dialog
            displays.
5.  Click **Proceed**. The system reboots and displays a dialog with
            password prompts.
6.  \[OPTIONAL\] Set the new root password.
    **Note:** If you leave the root password blank, the password defaults to .
7.  Click **Enable**. The screen displays messages indicating success or
            failure.
8.  Click **OK**. You'll see the first screen again.
9.  Follow the remaining prompts to configure your Chrome device.

## Disabling debugging features

On a managed Chrome OS device, most debugging features remain enabled, even
after powerwashing or wiping the data. Use one of the following processes to
disable features, depending on your requirements:

*   [Chrome OS recovery
            process](https://support.google.com/chromebook/answer/1080595)
*   The [cros
            flash](http://www.chromium.org/chromium-os/build/cros-flash) script
            for updating the Chrome OS image directly
*   The [cros
            flash](http://www.chromium.org/chromium-os/build/cros-flash) script
            for updating the Chrome OS image via USB (or directly using the
            `--clobber-stateful` flag)
*   [Powerwash](https://support.google.com/chromebook/answer/183084)
            process

This table summarizes the features reset by each procedure (✓ means the feature
is reset):

<table>
<tr>
<td>Chrome OS Recovery</td>
<td>`cros flash`</td>
<td>`cros flash usb://`</td>
<td> or</td>
<td> `cros flash --clobber-stateful`</td>
<td>Powerwash</td>
</tr>
<tr>
<td> Rootfs verification</td>
<td>✓</td>
<td>✓</td>
<td>✓</td>
</tr>
<tr>
<td> SSH w/ test keys</td>
<td>✓</td>
<td>✓</td>
<td>✓</td>
</tr>
<tr>
<td> USB boot</td>
<td>✓</td>
</tr>
<tr>
<td> System password</td>
<td> (for SSH login)</td>
<td>✓</td>
<td>✓</td>
<td>✓</td>
</tr>
<tr>
<td> Dev password</td>
<td> (if present, overrides</td>
<td> system password for VT2 login)</td>
<td>✓</td>
<td>✓</td>
<td>✓</td>
</tr>
</table>

To manually disable booting from USB:

1.  Make sure the device is in developer mode.
2.  Open the terminal window by pressing Ctrl + Alt + F2.
3.  Log in as `root`.
4.  Type the default password `test0000,` or the custom password you set
            previously.
5.  At the command prompt, type `crossystem dev_boot_usb=0`
