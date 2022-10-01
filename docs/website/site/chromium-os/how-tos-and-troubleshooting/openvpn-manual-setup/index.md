---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: openvpn-manual-setup
title: OpenVPN Manual Setup
---

TODO: Merge this content into the Chrome OS format.

This document, [OpenVPN on
ChromeOS](https://docs.google.com/document/d/18TU22gueH5OKYHZVJ5nXuqHnk2GN6nDvfu2Hbrb4YLE/pub)
(published Google Doc format) contains step by step instructions on getting
customized OpenVPN client settings on ChromeOS.

The first part discusses setting up an OpenVPN server (which you can skip if you
already have one) and the second shows how to import the necessary certificates
into your ChromeOS machine, and finally the last part sets up the network config
settings as an ONC file which is then imported into ChromeOS.

Note:

*   This does not require a machine to be in developer mode, and can run
            on a standard ChromeOS image.
*   Google enterprise customers may have access to easier methods of
            doing this via the ChromeOS device administration features of your
            account, where an ONC policy can be installed at sign on. These
            instructions here should work as well, but were intended for use on
            consumer (e.g. @gmail.com) google profiles.
