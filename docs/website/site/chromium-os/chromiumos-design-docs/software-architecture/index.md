---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: software-architecture
title: Software Architecture
---

[TOC]

## Abstract

Chromium OS consists of three major components:

*   The Chromium-based browser and the window manager
*   System-level software and user-land services: the kernel, drivers,
            connection manager, and so on
*   Firmware

![](/chromium-os/chromiumos-design-docs/software-architecture/overview.png)

## High-level design

We'll look at each component, starting with the firmware.

### Firmware

The firmware plays a key part to make booting the OS faster and more secure. To
achieve this goal we are removing unnecessary components and adding support for
verifying each step in the boot process. We are also adding support for system
recovery into the firmware itself. We can avoid the complexity that's in most PC
firmware because we don't have to be backwards compatible with a large amount of
legacy hardware. For example, we don't have to probe for floppy drives.

Our firmware will implement the following functionality:

*   [**System
            recovery**](/chromium-os/chromiumos-design-docs/firmware-boot-and-recovery)**:**
            The recovery firmware can re-install Chromium OS in the event that
            the system has become corrupt or compromised.
*   **[Verified
            boot](/chromium-os/chromiumos-design-docs/verified-boot):** Each
            time the system boots, Chromium OS verifies that the firmware,
            kernel, and system image have not been tampered with or become
            corrupt. This process starts in the firmware.
*   [**Fast
            boot**](/chromium-os/chromiumos-design-docs/firmware-boot-and-recovery)**:**
            We have improved boot performance by removing a lot of complexity
            that is normally found in PC firmware.

![](/chromium-os/chromiumos-design-docs/software-architecture/firmware.png)

### System-level and user-land software

From here we bring in the Linux kernel, drivers, and user-land daemons. Our
kernel is mostly stock except for a handful of patches that we pull in to
improve boot performance. On the user-land side of things we have streamlined
the init process so that we're only running services that are critical. All of
the user-land services are managed by Upstart. By using Upstart we are able to
start services in parallel, re-spawn jobs that crash, and defer services to make
boot faster.

Here's a quick list of things that we depend on:

*   **D-Bus:** The browser uses D-Bus to interact with the rest of the
            system. Examples of this include the battery meter and network
            picker.
*   **Connection Manager:** Provides a common API for interacting with
            the network devices, provides a DNS proxy, and manages network
            services for 3G, wireless, and ethernet.
*   **WPA Supplicant:** Used to connect to wireless networks.
*   **Autoupdate:** Our autoupdate daemon silently installs new system
            images.
*   **Power Management:** (ACPI on Intel) Handles power management
            events like closing the lid or pushing the power button.
*   **Standard Linux services:** NTP, syslog, and cron.

### Chromium and the window manager

The window manager is responsible for handling the user's interaction with
multiple client windows. It does this in a manner similar to that of other X
window managers, by controlling window placement, assigning the input focus, and
exposing hotkeys that exist outside the scope of a single browser window. Parts
of the ICCCM (Inter-Client Communication Conventions Manual) and EWHM (Extended
Window Manager Hints) specifications are used for communication between clients
and the window manager where possible.
The window manager also uses the XComposite extension to redirect client windows
to offscreen pixmaps so that it can draw a final, composited image incorporating
their contents itself. This lets windows be transformed and blended together.
The window manager contains a compositor that animates these windows and renders
them via OpenGL or OpenGL|ES.

![](/chromium-os/chromiumos-design-docs/software-architecture/chrome.png)
