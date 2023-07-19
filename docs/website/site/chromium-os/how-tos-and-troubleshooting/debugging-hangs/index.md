---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: debugging-hangs
title: Debugging system hangs (go/cros-debug-hang)
---

If your Chromebook appears hung (i.e. the screen is frozen and the keyboard
and/or trackpad are unresponsive), try the following steps:

*   Pre-Recovery Steps
    *   Note any visible system indicators like LEDs. Are they blinking
                or solid. What color?
    *   Note charging/discharging behavior if possible. Does plugging a
                charger in change LED.
    *   Note USB response. Does touching an gnubby blink. Does charging
                a phone via a USB port work?
    *   If possible, please attach an external monitor and check if the
                external monitor works.
*   Recovery Steps
    *   Press **Alt+VolumeUp+x**. If you're using a Chromebox, use F10
                instead of VolumeUp. These three keys need to be pressed in that
                order. The VolumeUp key here is the key in the top row on the
                keyboard and not the side button.
        *   This should cause the Chrome browser to restart, and it may
                    take a few seconds. If the system becomes responsive, go to
                    last step.
    *   Press **Alt+VolumeUp+x** a second time within 20 seconds from first time.
        *   This should cause the kernel to panic-reboot. If the system
                    becomes responsive, go to last step.
    *   Press **Alt+VolumeUp+r** which should cause a warm reset &
                reboot. If so, go to last step. (This may not work on older
                hardware.)
    *   Press and hold the **power button** for 8+ seconds and the
                system will power off (if shutdown is clean) or it will reboot
                via EC reset
    *   Press **Refresh+Power button(F3).** This causes an EC reset on
                most systems and should be used only as a last resort as it
                minimizes the amount of available debug info.
*   Last Step
    *   Power on (if necessary) Log in, and immediately file feedback
                using Alt+Shift+i.
        *   Make sure that "Send system and app information" is checked.
        *   Make sure to identify what recovery steps you used including
                    the ones you believed failed.

You may also be able to see crash report IDs at chrome://crashes.

For a complete and up-to-date list of debug key sequences including those for
ChromiumOS devices without a keyboard check
[here](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/debug_buttons.md).
