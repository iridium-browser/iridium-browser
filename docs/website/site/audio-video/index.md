---
breadcrumbs: []
page_name: audio-video
title: Audio/Video
---

Everything you need to know about audio/video inside Chromium and Chromium OS!

### Whom To Contact

It's best to have discussions on chromium-dev@chromium.org or
media-dev@chromium.org for media specific matters.

We are component
[Internals&gt;Media](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3AInternals%3EMedia)
on the Chromium bug tracker.

### Documentation

See
[media/README.md](https://chromium.googlesource.com/chromium/src/+/HEAD/media/README.md).
For historical reference, here's the original [design doc for HTML5
audio/video](/developers/design-documents/video).

### Codec and Container Support

Container formats

*   MP4 (QuickTime/ MOV / MPEG4)
*   Ogg
*   WebM
*   WAV

*   HLS \[Only on Android and only single-origin manifests\]

Codec formats (Decode Only)

==Audio==

*   FLAC
*   MP3
*   Opus
*   PCM 8-bit unsigned integer
*   PCM 16-bit signed integer little endian
*   PCM 32-bit float little endian
*   PCM μ-law
*   Vorbis

*   AAC \[Main, LC, HE profiles only, xHE-AAC on Android P+\] \[Google
            Chrome only\]

==Video==

*   VP8
*   VP9

*   AV1 \[Only Chrome OS, Linux, macOS, and Windows at present\]

*   Theora \[Except on Android variants\]

*   H.264 \[Google Chrome only\]
*   H.265 \[Google Chrome only and only where supported by the underlying OS\]

*   MPEG-4 \[Google Chrome OS only\]

### Code Location

**Chromium**

media/ - Home to all things media!
media/audio - OS audio input/output abstractions
media/video/capture - OS camera input abstraction
media/video - software/hardware video decoder interfaces + implementations
third_party/ffmpeg - Chromium's copy of FFmpeg
third_party/libvpx - Chromium's copy of libvpx

**Blink**

third_party/blink/renderer/core/html/media/html_media_element.{cpp,h,idl} -
media element base class

third_party/blink/renderer/core/html/media/html_audio_element.{cpp,h,idl} -
audio element implementation

third_party/blink/renderer/core/html/media/html_video_element.{cpp,h,idl} -
video element implementation

**Particularly Interesting Bits**

media/base/mime_util.cc - defines canPlayType() behaviour and file extension
mapping

media/blink/buffered_data_source.{cc,h} - Chromium's main implementation of
DataSource for the media pipeline

media/blink/buffered_resource_loader.{cc,h} - Implements the sliding window
buffering strategy (see below)

third_party/blink/public/platform/web_media_player.h - Blink's media player
interface for providing HTML5 audio/video functionality

media/blink/webmediaplayer_impl.{cc,h} - Chromium's main implementation of
WebMediaPlayer

### How does everything get instantiated?

WebFrameClient::createMediaPlayer() is the Blink embedder API for creating a
WebMediaPlayer and passing it back to Blink. Every HTML5 audio/video element
will ask the embedder to create a WebMediaPlayer.

For Chromium this is handled in RenderFrameImpl.

### GN Flags

There are a few GN flags which can alter the behaviour of Chromium's HTML5
audio/video implementation.

ffmpeg_branding

Overrides which version of FFmpeg to use

Default: $(branding)

Values:

Chrome - includes additional proprietary codecs (MP3, etc..) for use with Google
Chrome

Chromium - builds default set of codecs

proprietary_codecs

Alters the list of codecs Chromium claims to support, which affects
&lt;source&gt; and canPlayType() behaviour

Default: 0(gyp)/false(gn)

Values:

0/false - &lt;source&gt; and canPlayType() assume the default set of codecs

1/true - &lt;source&gt; and canPlayType() assume they support additional
proprietary codecs

### How the %#$& does buffering work?

Chromium uses a combination of range requests and an in-memory sliding window to
buffer media. We have a low and high watermark that is used to determine when to
purposely stall the HTTP request and when to resume the HTTP request.

It's complicated, so here's a picture:

[<img alt="image"
src="/audio-video/ChromiumMediaBuffering.png">](/audio-video/ChromiumMediaBuffering.png)
