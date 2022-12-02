---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: cras-chromeos-audio-server
title: 'CRAS: Chromium OS Audio Server'
---

## Why

Chromium OS needs a way to route audio to different devices. Currently Chromium
OS only plays audio to the first ALSA device in the system. Devices attached
after boot are ignored, and no priority is given to the built in sound device.
The goal of the new server is to allow for sound to be routed dynamically to
newly attached audio-capable monitors (DisplayPort and HDMI), USB webcam, USB
speakers, bluetooth headsets, etc. And to do this in a way that requires as
little CPU as possible and that adds little or no latency.

## How

Put an audio server in between Chromium and ALSA that can change how audio flows
on the fly. Chromium will connect to this audio server, send audio data to it,
and the audio server will render this to the selected ALSA device through
alsa-lib. For input the process will be reversed, the server will get audio
samples from alsa and forward them to Chromium.

## Details

The design decisions were driven by three main requirements: Low Latency, Low
CPU usage, and dynamic audio routing. This will handle the streaming of PCM
only; audio decode will all be done in Chromium.

The basic design combines the timer-driven wakeup scheme of pulseaudio with the
synchronous audio callbacks of JACK. This synchronous callback eliminates local
buffering of data in the server, allowing the server to wake up less for a given
latency. The server wakes up when a timer fires, sometime before there is an
ALSA over/underrun and calls back to Chromium to get/put audio data. This
minimizes latency as the only additional latency needed is that to exchange IPC
messages between the server and client. Because ALSA wakeups aren’t used, on
some devices the minimal latency will be lower than without the server; there is
no need to wake up at an ALSA period boundary. The timer can be armed to wake up
when there are an arbitrary number of samples left in the hardware buffer. ALSA
will always be configured with the largest possible buffer size, yet only some
fraction of that will be used at any given time.

Audio data will be exchanged through shared memory so that no copying is needed
(except into the ALSA buffer). Communication will happen through UNIX domain
sockets. There will be separate communication connections for control (high
latency, lower priority) and audio data (low latency and higher priority).

At each wakeup a timestamp will be updated that, for playback streams, will
indicate the exact time at which the next written sample will be rendered to the
DAC. For capture, the timestamp will reflect the actual time the sample was
captured at the ADC.

The key to keeping system resource usage low is to do as little as possible in
the server when it wakes up and similar for the callback in the client. Less
than two percent of the CPU of a CR48 is used while playing back audio with 20ms
latency and waking up every 10ms. The minimum the server can do is read in the
streams, mix them, and write the result to ALSA. Other processing can take place
in the server; DSP blocks can be configured to process the buffers as they pass
through, this will add to the processing time and increase the minimum latency.

The lowest latency stream will be used to drive the timer wakeup interval (and
will have the most accurate latency). Only clients that need to send more data
will be woken.

### Device Enumeration

The server keeps a separate list of inputs and outputs. The server will listen
to udev events that notify it of newly added or removed devices. Whenever a
device appears or disappears, the priority list of devices will be re-evaluated
and the highest priority device will be used.

Devices will be given a priority based on the device type and the time they are
inserted. The priority is as follows (for both input and output):

1.  Headset jack and USB audio devices.
2.  DisplayPort and HDMI outputs (when EDID indicates the sink is audio
            capable).
3.  Built-in speakers.

If two devices of the same priority are attached, the device that was plugged
most recently wins.

On device removal, all streams are removed from the device and the device is
closed. The streams will be moved to the next device in the list when this
happens.

### Mixing

The server is required to mix for the case where multiple streams are being
played at once. The mixing will be done immediately before the samples are sent
to the playback hardware. If a DSP block is enabled, it will be applied
post-mix.

### Per-Board Configuration

Without a config, the server will automatically discover devices and mute/volume
controls. This provides all that is needed for x86 most of the time. On x86 the
main use of config files is to set volume level mappings between the UI and
hardware, and to configure codec-specific features.

The volume curve file is per-codec, per-device and the format is described in
the main README file.

ALSA UCM is used to do low level codec config. On ARM systems, this setup is
necessary to get audio paths routed correctly when a headset is attached and to
initialize codec setup after start up. In addition to this, the UCM file is used
to map jacks to audio devices.

### More documents

*   [Chrome OS Audio Software
            Overview](https://docs.google.com/document/d/1pDdzlJlNacvOB8CS4Qxg-0aQvEuCK8lFKaWaqckSl3w/edit?usp=sharing)
*   [CRAS ELC
            2014.pdf](https://drive.google.com/file/d/1WBYe-M_xFaHIajn-hfQRBrKhPkbgHNF9/view?usp=sharing)
*   [Gain in
            CRAS](https://docs.google.com/document/d/1FfTGylzC-uGPhzxnotfxXjDeBPRPIMxkzqdQbQWJ7aE/edit?usp=sharing)
*   [UCM Usage
            Guide](https://docs.google.com/document/d/1AcXZI9dvJBW0Vy6h9vraD7VP9X_MpwHx0eJ6hNc4G_s/edit?usp=sharing)
