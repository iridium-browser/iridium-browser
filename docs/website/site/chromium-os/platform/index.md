---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: platform
title: The Chromium OS Platform
---

[TOC]

## Introduction

Chromium OS has many userland programs and sub-projects that make up the
"platform" of the system. You can find them listed (as well as more details on
each one) in the main [Packages page](/chromium-os/packages). This guide is
about the platform those projects build off. It is meant as the authoritative
guide when writing a new project (or updating an existing one). You should not
deviate from the choices outlined here without a very very good reason (and
chances are that what you think is a very good reason is really not).

## Goals

Here are some explicit goals for the Chromium OS platform:

*   We should align & share code with Chromium whenever possible.
*   C++ is the language you should be using.
    *   Only use C when C++ is not a viable option (e.g.
                kernel/bootloader/etc...).
    *   Shell code is rarely acceptable beyond short snippets.
*   Each platform project should align & share code whenever possible.
    *   Use the same build system.
    *   Use the same set of utility libraries.

## Decisions

### Build System

Rather than have every package implement its own build system (scons, cmake,
autotools, hand written makefiles, common.mk, etc...), The One True Build System
we use is [GYP](https://code.google.com/p/gyp/). More details on how it
integrates can be found in the [Platform2
page](/chromium-os/getting-started-with-platform2).

### Style

We follow the various Google style guides for Chromium OS projects. For third
party projects, you should follow the style that project has decided on.

*   [C++](http://www.chromium.org/developers/coding-style) - Chromium
            C++ Style Guide
*   [Python](/chromium-os/python-style-guidelines) - Chromium OS Python
            Style Guide (a superset of the Google Python Style Guide)
*   [Shell](/chromium-os/shell-style-guidelines) - Chromium OS
            shell-script Style Guide

### Inter-Process Communication (IPC)

Most communication between processes is done via D-Bus. See the ["D-Bus best
practices"
doc](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/dbus_best_practices.md)
for more information.

#### Data Serialization

If you need to serialize data, either for reading back later (from
disk/cache/etc...) or for passing to another process (via ipc/network/etc...),
then you should use:

*   IPC (local machine or network)
    *   [dbus](http://dbus.freedesktop.org/) - for simple stuff, dbus
                itself includes marshaling of basic types
    *   [protocol buffers](https://code.google.com/p/protobuf/)
                (protobufs) - for larger/binary content, or when communicating
                via the network
*   Reading/Writing to disk
    *   base::JSON\* - for general saved runtime state and/or config
                files (reading & writing)
    *   base::IniParser - for simple config files (read only); but json
                is still preferred

### Utility Libraries

First off, whenever you want to do some new that you suspect someone else has
done already, you should look through Chromium's base library and Chromium OS
library (libchromeos). We build & ship these in Chromium OS; see the [libchrome
page](/chromium-os/packages/libchrome) and [libchromeos
page](/chromium-os/packages/libchromeos) for building against them.

Here is a common run down of functionality provided by libchrome that platform
projects use:

<table>
<tr>
<td> logging</td>
<td> base/logging.h</td>
</tr>
<tr>
<td> command line flags</td>
<td> base/command_line.h</td>
</tr>
<tr>
<td> string parsing</td>
<td> base/strings/\*</td>
</tr>
<tr>
<td> file path manipulation</td>
<td> base/file_path.h</td>
</tr>
<tr>
<td> file read/write utilities</td>
<td> base/file_util.h</td>
</tr>
<tr>
<td> JSON manipulation</td>
<td> base/json/\*</td>
</tr>
<tr>
<td> INI file reading</td>
<td> base/ini_parser.h</td>
</tr>
<tr>
<td> DBus bindings</td>
<td> dbus/\*</td>
</tr>
<tr>
<td> Message Loop</td>
<td> base/message_loop.h</td>
</tr>
</table>

For libchromeos, see the [libchromeos page](/chromium-os/packages/libchromeos)
for and overview of the functionality implemented there.

In particular, you should avoid using:

*   [gflags](https://code.google.com/p/gflags/) - command line parsing
*   [glog](https://code.google.com/p/google-glog/) - logging

### Metrics

If you want your program to gather & report statistics at runtime (like UMA
stats), then you should use:

*   metrics - the Chromium OS metrics library

### Sandboxing (User/Process/etc... Isolation)

All daemons should always be run with as restricted permissions as possible.
This often means using a dedicated UID/GID, chroots, and seccomp filters. This
is all handled by:

*   [minijail](/chromium-os/developer-guide/chromium-os-sandboxing) -
            the Chromium OS jail manager

### Install Locations

<table>
<tr>
<td>Type</td>
<td>Where</td>
</tr>
<tr>
<td> public executables</td>
<td> `/usr/bin/`</td>
</tr>
<tr>
<td> public libraries</td>
<td> `/usr/$(get_libdir)/`</td>
</tr>
<tr>
<td> internal executables/libraries/plugins</td>
<td> `/usr/$(get_libdir)/${PN}/`</td>
</tr>
<tr>
<td> data files</td>
<td>`/usr/share/${PN}/` </td>
</tr>
<tr>
<td> include files</td>
<td> `/usr/include/`</td>
</tr>
</table>

You should avoid /sbin/ and /bin/ and /usr/sbin/ paths. They are unnecessary and
you should just stick to /usr/bin/.

Similarly, you should avoid /$(get_libdir)/ (e.g. /lib/).

You should avoid /opt/. It is for exceptional cases only (mostly
non-public/binary-only projects).

### Crypto

You should use [OpenSSL](http://www.openssl.org/) for crypto services. A few
projects still use
[NSS](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/NSS), but the
plan is to migrate away from that, so no new code should be using it.

### Testing

Since you will of course be writing unittests for all of your projects, you
should use (which is to say unittests are a hard requirement):

*   [gtest](https://code.google.com/p/googletest/) - unittest framework
*   [gmock](https://code.google.com/p/googlemock/) - mocking framework
