---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: chamelium-usb-audio
title: Chamelium USB Audio
---

[TOC]

## Background

USB audio supports multi-channel and higher precision playback / capture
functionality. This document tells you about how to use chamelium with external
USB audio devices.

## Needed equipment

1.  An USB audio device
2.  Type-A USB to mini-USB converter

## Setup Steps

1.  Run command $ touch /etc/default/.usb_host_mod and reboot chamelium.
2.  Run command $ modprobe snd-usb-audio to enable USB audio module.
3.  Plug in a USB audio device in the ***middle*** mini-USB port (with
            the converter) and use command $ aplay -l ($ arecord -l for
            microphone) to check if the device is plugged in.
4.  Test your USB audio device by the following command
    1.  $ arecord -f dat /tmp/test.wav (Test capture function)
    2.  $ aplay -f dat /tmp/test.was (Test playback function)

## **Recover Steps**

1.  Run command $ rm /etc/default/.usb_host_mod and reboot chamelium
