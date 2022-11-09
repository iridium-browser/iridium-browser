---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: chamelium-audio-board
title: Chamelium Audio Board
---

[TOC]

## Introduction

Chamelium audio board routes audio between four endpoints with two stereo audio
buses:

*   DUT device headphone jack
*   Chamelium FPGA board line-out/line-in
*   Peripheral speaker/microphone
*   Bluetooth audio module

It helps Chamelium automate audio testing including:

*   3.5mm headphone
*   3.5mm external microphone
*   Onboard internal speaker
*   Onboard internal microphone
*   Bluetooth A2DP playback
*   Bluetooth HSP/HFP playback
*   Bluetooth HSP/HFP record

## Components

*   Chamelium

> > <img alt="AB_2.jpg"
> > src="https://lh6.googleusercontent.com/J7i4-sT2ydPrJmJQo1IZ-e_0PUmoN0R_rXN-1QLIMLDaUIlUfeLj9ZZVe2GgvjhoatKCLu9S-w4iLn8nyxZVVtBvEcKNogcUMgS-9YCHhThkHhem-sSRYnPfeD-F4tNziOghxMaU"
> > height=274 width=400>

*   Audio Board Block Diagram

> > [<img alt="image"
> > src="/chromium-os/testing/chamelium-audio-board/Chameleon%20Audio%20-%20New%20Page%20%284%29.png"
> > height=283
> > width=400>](/chromium-os/testing/chamelium-audio-board/Chameleon%20Audio%20-%20New%20Page%20%284%29.png)

*   Audio Board

> > <img alt="AB_1.jpg"
> > src="https://lh5.googleusercontent.com/OyD9hcw981gB6wUOTVHdweNJAV28gvT9hBp5wiFxnv80SXrfFr0l43tmMqFicK-XM6Mbqn8MOsCme7IOAVkMxr1fwlolWN7IKNP_wpfeDa9g_S0qub2SQMe4EZjgtfRNvX3jp54P"
> > height=308 width=400>

*   Two 3-ring (TRS) audio jack cables
*   One 4-ring (TRRS) audio jack cable
*   One serial ribbon cable (connecting CN4 on audio board with
            J13_LTC_CON on FPGA)
*   USB-A to mini-USB cable
*   3-ring (TRS) 3.5mm microphone

## Bootstrap

    Connect audio board and FPGA board with the serial ribbon grey cable (see
    the assembled picture).

    Connect audio board and Chamelium line-in and line-out (blue jack to blue
    jack, green jack to green jack) using two 3.5mm 3-ring(TRS) cables.

    For audio jack external audio - connect tested device with audio board using
    3.5mm 4-ring (TRRS) cable.

    For USB audio - connect tested device with the middle USB port on FPGA (top)
    board.

    For internal speaker test - connect a mono microphone (two conductor, TS
    connector) to the pink port on the FPGA (top) board.

    For internal microphone test - connect a speaker to black SPK port on audio
    board.

    For Bluetooth test - Chamelium Eth MAC and audio board BT MAC should be
    related in chameleon_info.

## Setup Chamelium Info

    Use the interactive script ~/trunk/src/platform/chameleon/client/test_server
    to connect to chameleond server.
    ~/trunk/src/platform/chameleon/client/test_server
    --chameleon_host=&lt;chameleon ip&gt;

    Get the MAC address of chamelium: &gt;&gt;&gt; p.GetMacAddress()

    '94:eb:2c:00:01:27'

    Disable the bluetooth module on audio board:
    &gt;&gt;&gt; p.AudioBoardDisableBluetooth()

    ssh into Cros device:
    ssh chromeos1-row5-rack1-host2.cros

    Use bluetoothctl to turn on adapter and start scanning

localhost ~ # bluetoothctl

\[NEW\] Controller 7C:7A:91:9A:EE:89 Chromebook \[default\]

\[bluetooth\]# power on

Changing power on succeeded

\[bluetooth\]# scan on

Discovery started

\[CHG\] Controller 7C:7A:91:9A:EE:89 Discovering: yes

    Enable the bluetooth module on audio board using test_server: &gt;&gt;&gt;
    p.AudioBoardResetBluetooth()

    In bluetoothctl console, check there is a new **SX3868-3Y** device being
    scanned:

\[NEW\] Device 00:1F:84:01:03:5B SX3868-3Y

\[CHG\] Device 00:1F:84:01:03:5B RSSI: -67

\[CHG\] Device 00:1F:84:01:03:5B RSSI: -56

\[CHG\] Device 00:1F:84:01:03:5B RSSI: -68

    Now we know Chamelium with MAC address '94:eb:2c:00:01:27' has a Bluetooth
    module with MAC address ‘00:1F:84:01:03:5B’ on its audio board.

    Add the mapping info to client/cros/chameleon/chameleon_info.py

_CHAMELEON_BOARD_INFO = {

'94:eb:2c:00:00:fb': ChameleonInfo('00:1F:84:01:03:68'),

'94:eb:2c:00:01:2b': ChameleonInfo('00:1F:84:01:03:5E'),

'94:eb:2c:00:01:27': ChameleonInfo('00:1F:84:01:03:5B'), }

## Run the test

For example, run basic headphone test on a squawks board 5 times:

test_that --board=squawks --fast --debug --args="chameleon_host=$CHAMELIUM_IP"
$DUT_IP audio_AudioBasicHeadphone --iterations 5

## Test output/input devices basic function

### Output audio devices

*   Internal speaker - audio_AudioBasicInternalSpeaker
*   Headphone - audio_AudioBasicHeadphone
*   HDMI/DisplayPort - audio_AudioBasicHDMI
*   USB headphone - audio_AudioBasicUSBPlayback
*   Bluetooth headphone - audio_AudioBasicBluetoothPlayback

### Input audio devices

*   Internal microphone - audio_AudioBasicInternalMicrophone
*   External microphone - audio_AudioBasicExternalMicrophone
*   USB microphone - audio_AudioBasicUSBRecord
*   Bluetooth microphone - audio_AudioBasicBluetoothRecord

## Advanced tests

*   Volume control: audio_AudioVolume
*   Suspend/Resume, Reboot: audio_AudioAfterSuspend,
            audio_AudioAfterReboot, audio_AudioQualityAfterSuspend
*   Quality and artifacts: audio_AudioArtifacts
*   BT connection: audio_AudioBluetoothConnectionStability
*   BT playback + record: audio_AudioBasicBluetoothPlaybackRecord
*   USB playback + record: audio_AudioBasicUSBPlaybackRecord
*   Chrome media player: audio_MediaBasicVerification
*   WebRTC: audio_AudioWebRTCLoopback
*   ARC: audio_AudioARCPlayback, audio_AudioARCRecord
