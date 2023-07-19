---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: packages
title: packages
---

We have many packages in the Chromium OS tree. While many come from upstream
Gentoo (which can be browsed <http://packages.gentoo.org/>), there are a good
number which are specific to our project. Here you can dive down into them.

## Core system

*   chromeos: meta package to pull in all dependencies for a Chromium OS
            install
*   chromeos-base: Chromium OS specific config files (usually /etc) and
            user/group id management
*   [libchrome](/chromium-os/packages/libchrome): the base / utility
            library from the Chromium browser (used by many Chromium OS
            projects)
*   [libchromeos](/chromium-os/packages/libchromeos): custom Chromium OS
            utility library
*   metrics: simplify metrics gathering by other packages
*   chromeos-chrome: the Chromium browser

## Debug/Development

*   [debugd](https://chromium.googlesource.com/chromiumos/platform2/+/HEAD/debugd/):
            daemon for debugging issues
*   chromeos-dev: set of packages used in developer images and available
            by running
            [dev_install](/chromium-os/how-tos-and-troubleshooting/install-software-on-base-images)
            in release images
*   [crash-reporter](/chromium-os/packages/crash-reporting): crash
            processing program/uploader
*   [crosh](https://chromium.googlesource.com/chromiumos/platform2/+/HEAD/crosh/):
            interactive shell for running low level commands, running ssh,
            etc...
*   [dev_install](/chromium-os/how-tos-and-troubleshooting/install-software-on-base-images):
            install dev packages into a base (release) image
*   [gmerge](/chromium-os/how-tos-and-troubleshooting/using-the-dev-server):
            installing packages live onto your device
*   [google-breakpad](http://code.google.com/p/google-breakpad): crash
            reporting and symbol/stack analysis library
*   ssh-known-hosts: known ssh keys of Chromium OS servers

## Factory/Updates

*   chromeos-factoryinstall
*   memento_softwareupdate
*   update_engine

## Firmware

*   chromeos-coreboot: 1st stage boot loader on x86 platforms
*   chromeos-ec: embedded controller firmware for many ChromeOS devices
*   chromeos-u-boot: 2nd stage boot loader
*   coreboot-utils: utilities for working with coreboot firmeware images
*   [dtc](http://www.t2-project.org/packages/dtc.html): device tree
            (text files which describe hardware layouts) compiler
*   [flashmap](http://flashmap.googlecode.com): utility for manipulating
            firmware images
*   vboot_reference: tools for working with verified boot
*   vboot_reference-firmware: firmware for supporting verified boot

## Graphics/Input

*   xf86-input-cmt: **C**hromium OS **m**ulti**t**ouch driver (similar
            to xf86-input-synaptics, but better!)

## Misc

*   entd: enterprise daemon -- deprecated in favor of libpolicy (in
            libchromeos package)
*   [power_manager](https://chromium.googlesource.com/chromiumos/platform2/+/HEAD/power_manager/README.md):
            userspace daemon for performing power-management-related tasks

## Media (Audio/Video)

*   adhd: Chromium OS audio server

## Networking

*   cashew: network statistics package -- integrated into shill now
*   cromo: modemmanager compatible dbus interface to support closed
            source modem drivers (which are discouraged)
*   flimflam: network manager -- deprecated in favor of shill
*   gobi3k-sdk: SDK for working with Qaulcomm's gobi3k devices
*   gobi-cromo-plugin: plugin for supporting gobi3k modems with cromo
            (to be replaced & merged with modemmanager)
*   minifakedns:
*   mobile-providers:
*   modem-diagnostics:
*   modem-utilities:
*   [modemmanager](http://cgit.freedesktop.org/ModemManager/ModemManager/):
            daemon and libraries for managing modem/mobile devices (such as 3G
            cellular)
*   shill: network manager (similar to the NetworkManager project, but
            better)
*   vpn-manager:
*   [wpa_supplicant](http://hostap.epitest.fi/wpa_supplicant/):
            utilities for managing WiFi 802.11 connections

## Security (Crypto/etc...)

*   chaps:
*   chromeos-ca-certificates: certificates from [certificate
            authorities](http://en.wikipedia.org/wiki/Certificate_authority)
            that the browser in ChromeOS will trust (i.e. https://)
*   chromeos-cryptohome: manager for per-user encrypted home storage
*   chromeos-minijail:
*   libchrome_crypto: the crypto/ utility library from the Chromium
            browser (used by a few crypto-related Chromium OS projects)
*   root-certificates:
*   tpm:
*   trousers:
*   verity: full disk encryption and verification (see dm_verity kernel
            driver)
*   [biod](/chromium-os/packages/biod): biometrics daemon that is
            responsible for interfacing with kernel interfaces of biometric
            devices, securely storing biometric data, and
            capturing/authenticating biometric input.

## Testing

*   autotest: Chromium OS tests
