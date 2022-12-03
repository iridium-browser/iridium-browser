---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: developer-shell-access
title: Developer Shell Access
---

[TOC]

## Introduction

Developers often need shell access to their Chromium OS device in order to
sanely debug things in the system. Think opening crosh and typing "shell", and
then logging in as root with "sudo". Or logging into a VT console when the UI is
broken. Or logging in remotely via ssh.

However, this system is at odds with providing a simple system that is as secure
as possible, so we need to analyze the trade offs.

## Goals

*   Security should not be impacted when developer mode is turned off,
    *   both directly -- e.g. user sitting in front of the machine,
    *   or indirectly -- e.g. an exploit that manages to execute
                programs outside of the browser as the same user.
    *   This means no shell, ssh, VT, etc... access.
*   The system should be dynamic.
    *   Take a normal verified/secure image, put it into developer mode,
                and shell access will start working.
*   Passwords/ssh keys cannot be static at build time in release images.
    *   Users may opt to set their own passwords in their own personal
                builds.
    *   Test images may include preset authentication (discussion of
                this is ongoing, but out of scope of this doc).
*   Users can enable password protection on the fly.
    *   Keeps people from sitting down, getting a shell, and then
                running sudo on your system.
    *   Passwords protect all avenues of access equally.

## Summary

TODO(vapier)

## Buildtime

TODO(vapier)

## Runtime

### Developer Mode Disabled

#### VT Switching

When X is launched by the session manager, it is passed the -maxvt flag set to
0. This way X itself ignores the key combos.

#### Sysrq Magic Key

The hotkey-access.conf script will turn off all sysrq requests except for the
"x" key by updating `/proc/sys/kernel/sysrq`.

#### Crosh Shell

The crosh script is still available, but it does not allow access to the "shell"
command (among others).

#### SSH

The ssh sever is not included in the base image, so it will never autostart. If
it was started somehow, then the sections below apply (which is to say, it still
wouldn't allow logins).

#### sudo/su

These cannot be run directly (as no shell is available), but even then, access
is denied via pam.

#### pam

A custom pam stack ("chromeos-auth") is installed that handles authentication
for the "login" and "sudo" services. When developer mode is disabled, this stack
will skip itself and continue to the normal system stacks.

For more details on pam, see [The Linux-PAM System Administrators'
Guide](http://www.linux-pam.org/Linux-PAM-html/Linux-PAM_SAG.html).

#### groups

The chronos account is not part of the admin groups that would implicitly grant
access (e.g. `wheel`).

#### passwords

The system password database (/etc/shaddow) is in the read-only rootfs and
cannot be modified. The default images will list accounts with passwords set to
"\*" (so that password authentication will fail).

The user custom dev mode password is not checked at all (see the pam section
above).

### Developer Mode Enabled

#### VT Switching

When X is launched by the session manager, it is passed the -maxvt flag set to
2. This allows access to the VT2 console. Access is controlled by pam.

#### Sysrq Magic Key

The hotkey-access.conf script will enable all sysrq requests.

#### Crosh Shell

The crosh script allows access to the "shell" command (among others).

#### SSH

If it is launched by hand, or using a test image that autolaunches it, access is
controlled by the sections below.

#### sudo/su

Access is controlled by pam.

#### pam

A custom pam stack ("chromeos-auth") is installed that handles authentication
for the "login" and "sudo" services. When developer mode is enabled, this stack
will:

*   If a custom devmode password is set
    *   That password is allowed for authentication
*   If a custom devmode password is not set
    *   If the account is in /etc/shadow
        *   Access is not allowed
    *   If the account is in /etc/shadow but no password set yet
        *   Passwordless access is granted
    *   If the account is in /etc/shadow with a password set
        *   Access is not allowed

Note that this only applies to this particular stack. Other pam stacks may
allow/deny independently.

For more details on pam, see [The Linux-PAM System Administrators'
Guide](http://www.linux-pam.org/Linux-PAM-html/Linux-PAM_SAG.html).

#### groups

The chronos account is not part of the admin groups that would implicitly grant
access (e.g. `wheel`).

#### passwords

The system password database (/etc/shaddow) is in the read-only rootfs and
cannot be modified. The default images will list accounts with passwords set to
"\*" (so that password authentication will fail).

The user may set a custom password at runtime with the `chromeos-setdevpasswd`
script which is checked by pam.

## References

*   http://crbug.com/182572
*   http://crbug.com/217710
