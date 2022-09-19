---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: chamelium-audio-streaming
title: Chamelium Audio streaming
---

[TOC]

## Introduction

Chamelium provides capturing and streaming feature so user can monitor audio
playing from ChromeOS for a long time.

This document provides the steps to demo this feature.

## Prepare a Chromebook with tick noise

Install an image on ChromeOS such that there will be tick noise caused by
underrun.

Kevin image R57-9178.0.0 is a good example.

## Play sine tone on Chromebook

Play a sine tone video on youtube, e.g. [1HR 528Hz sine
tone](https://www.youtube.com/watch?v=dZ5G13MEMgc)
Listen to it using headphone and find there is tick noise.

## Connect the audio path

Connect the Chromebook to Chamelium audio board using 4 ring audio cable.

Connect Chamelium blue port (LineIn) to audio board blue port using 3 ring audio
cable.

[<img alt="image"
src="/chromium-os/testing/chamelium-audio-streaming/IMG_0001.jpg" height=300
width=400>](/chromium-os/testing/chamelium-audio-streaming/IMG_0001.jpg) [<img
alt="image" src="/chromium-os/testing/chamelium-audio-streaming/IMG_0002.jpg"
height=300
width=400>](/chromium-os/testing/chamelium-audio-streaming/IMG_0002.jpg) [<img
alt="image" src="/chromium-os/testing/chamelium-audio-streaming/IMG_0003.jpg"
height=300
width=400>](/chromium-os/testing/chamelium-audio-streaming/IMG_0003.jpg)

## Find Chamelium's IP

Connect the USB serial port to workstation.

Refer to [network
setup](/chromium-os/testing/chamelium#TOC-Verify-your-network-setup) to get the
IP address of Chamelium.

## Connect to Chamelium server

Assume the IP address of Chamelium is 192.168.100.4

In Chromium OS chroot, use test_server script to connect to Chamelium server.

$ cd ~/trunk/src/platform/chameleon/

$ ./client/test_server.py --chameleon_host=192.168.100.4

2017-03-07 17:36:47,219:INFO:Connected to http://192.168.100.4:9992 with MAC
address 94:eb:2c:00:01:41

2017-03-07 17:36:47,219:INFO:In interactive shell, p is the proxy to chameleond
server

2017-03-07 17:36:47,287:INFO:

Port 1 is DP.

Port 2 is DP.

Port 3 is HDMI.

Port 4 is VGA.

Port 5 is Mic.

Port 6 is LineIn.

Port 7 is LineOut.

Port 8 is USBIn.

Port 9 is USBOut.

Port 10 is USBKeyboard.

Port 11 is USBTouch.

Port 14 is ClassicBluetoothMouse.

Port 17 is Unknown.

E.g.

p.StartCapturingAudio(6) to capture from LineIn.

p.StopCapturingAudio(6) to stop capturing from LineIn.

p.Plug(3) to plug HDMI.

p.Unplug(3) to unplug HDMI.

Python 2.7.10 (default, Oct 23 2016, 07:38:19)

\[GCC 4.9.x 20150123 (prerelease)\] on linux2

Type "help", "copyright", "credits" or "license" for more information.

(InteractiveConsole)

&gt;&gt;&gt;

&gt;&gt;&gt; **ConnectCrosToLineIn()**

&gt;&gt;&gt; **p.StartCapturingAudio(6, False)**

&gt;&gt;&gt; **DetectAudioValue0**()

2017-03-07 17:46:27,219:INFO:connecting to 192.168.100.4:9994

2017-03-07 17:46:27,222:DEBUG:Major 1, minor 0

2017-03-07 17:46:27,222:INFO:dump realtime audio page mode Best effort

2017-03-07 17:46:27,224:INFO:Start to detect continuous 0s.

2017-03-07 17:46:27,224:INFO:Channels=\[0, 1\], margin=0.010000,
continuous_samples=5, duration=3600 seconds, dump_frames=48000.

2017-03-07 17:46:27,224:WARNING:Receive error code 7, 'Drop realtime audio page
52183'

2017-03-07 17:46:27,225:INFO:Value of channel 0 is 0.125715

2017-03-07 17:46:27,225:INFO:Value of channel 1 is 0.123948

2017-03-07 17:46:27,225:INFO:Value of channel 0 is 0.134183

2017-03-07 17:46:27,225:INFO:Value of channel 1 is 0.132190

2017-03-07 17:46:27,225:INFO:Value of channel 0 is 0.142132

2017-03-07 17:46:27,226:INFO:Value of channel 1 is 0.140410

2017-03-07 17:46:27,226:INFO:Value of channel 0 is 0.149582

2017-03-07 17:46:27,226:INFO:Value of channel 1 is 0.147740

2017-03-07 17:46:27,226:INFO:Value of channel 0 is 0.156259

2017-03-07 17:46:27,226:INFO:Value of channel 1 is 0.154545

2017-03-07 17:46:27,226:INFO:Value of channel 0 is 0.162134

2017-03-07 17:46:27,226:INFO:Value of channel 1 is 0.160298

2017-03-07 17:46:27,226:INFO:Value of channel 0 is 0.167443

2017-03-07 17:46:27,226:INFO:Value of channel 1 is 0.165158

2017-03-07 17:46:27,226:INFO:Value of channel 0 is 0.171803

2017-03-07 17:46:27,227:INFO:Value of channel 1 is 0.169648

2017-03-07 17:46:27,227:INFO:Value of channel 0 is 0.175156

2017-03-07 17:46:27,227:INFO:Value of channel 1 is 0.173238

2017-03-07 17:46:27,227:INFO:Value of channel 0 is 0.178005

2017-03-07 17:46:27,227:INFO:Value of channel 1 is 0.175745

2017-03-07 17:46:28,102:WARNING:Detected continuous 5 0s of channel 0

2017-03-07 17:46:28,102:WARNING:Detected continuous 5 0s of channel 1

2017-03-07 17:46:28,103:WARNING:Save to detect0_20170307174627/audio_52511.wav

2017-03-07 17:46:28,105:WARNING:Continuous 139 0s of channel 0

2017-03-07 17:46:28,106:WARNING:Continuous 140 0s of channel 1

2017-03-07 17:46:28,106:WARNING:Save to detect0_20170307174627/audio_52512.wav

...

The monitoring will keep running.

## Adjust volume on Chromebook, or adjust the parameter of detector

If there is false alarm, adjust the volume on Chromebook to make it louder, or
use a smaller value for **margin** passed to DetectAudioValue0.

Use help function to get the docstrings of DetectAudioValue0:

&gt;&gt;&gt;help(DetectAudioValue0)

Help on function DetectAudioValue0 in module __main__:

DetectAudioValue0(channels=None, margin=0.01, continuous_samples=5,
duration=3600, dump_samples=48000)

Detects if Chameleon captures continuous audio data close to 0.

This function will get the audio streaming data from stream server and will

check if the audio data is close to 0 by the margin parameter.

-margin &lt; value &lt; margin will be considered to be close to 0.

If there are continuous audio samples close to 0 in the streamed data,

test_server will log it and save the audio data to a wav file.

E.g.

&gt;&gt;&gt; ConnectCrosToLineIn()

&gt;&gt;&gt; p.StartCapturingAudio(6, False)

&gt;&gt;&gt; DetectAudioValue0(duration=24\*3600, margin=0.001)

Args:

channels: Array of audio channels we want to check.

E.g. \[0, 1\] means we only care about channel 0 and channel 1.

margin: Used to decide if the value is closed to 0. Maximum value is 1.

continuous_samples: When continuous_samples samples are closed to 0, trigger

event.

duration: The duration of monitoring in seconds.

dump_samples: When event happens, how many audio samples we want to

save to file.

## Check the recorded defect waveform

Use audacity to view the wav file.

The first wav file is where underrun is detected.

Press the black arrow on the left of a channel and select **Spectrogram** so the
part where waveform dropped to zero is obvious.

![](/chromium-os/testing/chamelium-audio-streaming/Screenshot%20from%202017-03-07%2017_51_43.png)

The second wav file is where normal playback is resumed.
