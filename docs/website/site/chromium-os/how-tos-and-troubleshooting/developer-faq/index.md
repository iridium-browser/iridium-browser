---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: developer-faq
title: High-Level Developer FAQ
---

[TOC]

This FAQ is for developers; check out the [General Chromium OS
FAQ](/chromium-os/chromium-os-faq) for non-development related questions. If
that doesn't work out, try the [discussion groups](/system/errors/NodeNotFound).

---

## Should I call my build Chromium OS or Google Chrome OS?

You should call it Chromium OS.

### Will the autoupdate feature work on both Google Chrome OS and Chromium OS?

We don't plan to support autoupdate on Chromium OS, as we did not build the
binaries, and we don't know what modifications you've made to the system, so we
don't want to blow away any changes you may have made to the code. Therefore
Google will not autoupdate Chromium OS systems, but you're welcome to set up
your own autoupdate server.

Google Chrome OS will autoupdate to keep consumer machines running the latest
and greatest at all times.

### Will the verified boot feature work on both Google Chrome OS and Chromium OS?

The verified boot procedure relies on custom firmware and hardware modifications
and hence will work only for Google Chrome OS.

### ---

### ## What are the minimum hardware requirements?

The open-source nature of Chromium OS allows it to be ported to an expanding
range of hardware; however, some base requirements are likely to remain fixed.

Hardware-accelerated OpenGL or OpenGL ES support is mandatory, and the Chromium
browser's memory footprint is an obvious lower bound for RAM.

See the [Developer Hardware
list](/chromium-os/getting-dev-hardware/dev-hardware-list) for examples of
netbooks on which developers are successfully running Chromium OS.

### What about other hardware: Does Chromium OS support my wifi card?

In order to ensure the best user experience with Google Chrome OS, we're going
through a careful hardware selection and testing process for hardware
components.

For Chromium OS, the open source community and Google are working to add support
for a very broad range of hardware. If the device you're interested in has an
open source driver already in the upstream Linux kernel, please send a request
to
[chromium-os-dev](https://groups.google.com/a/chromium.org/group/chromium-os-dev/topics)
-- if you can include a proposed patch, even better.

Please see our [supported developer hardware
wiki](/chromium-os/getting-dev-hardware/dev-hardware-list) for more details.

### Is it true that you don't support hard disk drives (HDDs)?

Firstly, we should point out that the information in the open source release has
been misinterpreted as saying that we don't support local storage. Most Google
Chrome OS devices use SSDs although we have a few that also use HDDs. The
reasons to prefer SSD is performance and reliability.

Chromium OS will indeed work with conventional HDDs, though the disk accesses
are optimized for flash-based storage, like reduced read-ahead.

---

## Chromium OS issues

### I can't log in

Login may fail under various circumstances. For example, if you do not have
network connectivity and you have never logged in before, then you will not be
able to log in.

The login screen should display a message beneath the username/password input
field. For example, if you have network connectivity and provide the wrong
credentials, you will be told that either your username or password is
incorrect.

To troubleshoot networking at this point, you have to jump to a virtual terminal
(only enabled on dev machines) by using Ctrl+Alt+F2 (you might need to use the
[shared user password](/system/errors/NodeNotFound) that to login if you set
it).

If you are able to log in to the virtual terminal, reconfigure the networking
service (`sudo restart shill`). **Note:** If you are having trouble with
wireless, just plug in an Ethernet cable. It is much easier to troubleshoot a
networking issue once you have logged in.

### How is timezone managed?

On Chromium OS, local timezone is managed by the Chromium browser. The default
timezone is Pacific Time (PST). To change it, please refer to
<https://support.google.com/chromebook/answer/177871?hl=en>.

In the case of a Chrome-less build, the timezone is left unset and defaulted to
UTC. To set to a different timezone, the symlink /var/lib/timezone/localtime
should be linked to a specific zone file, such as /usr/share/zoneinfo/UTC.

---

## Miscellaneous

### What are the shortcut keys?

To get a visual overlay, hit Ctrl-Alt-/ and then hold modifier keys like Ctrl,
Alt, and Shift to see the associated hotkeys.

Most browser shortcuts also apply:
<http://www.google.com/support/chrome/bin/answer.py?hl=en&answer=95743>

### Are native applications supported?

Google Chrome OS is a web-centric system, so all applications are web
applications; this provides powerful and simple manageability and security. To
write applications that will benefit from native code execution we recommend
using NativeClient, an open source project that allows web apps to run native
code securely within a browser. See
<http://code.google.com/chrome/nativeclient/> for more details.

Of course Chromium OS is open source, and it's Linux. This means that as a
developer you can do pretty much anything you want, including installing any
Linux application.
