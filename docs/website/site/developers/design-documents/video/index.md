---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: video
title: Audio / Video Playback
---

## Interested in helping out? Check out our [bugs](https://bugs.chromium.org/p/chromium/issues/list?q=component%3AInternals%3EMedia)! New to Chromium? [GoodFirstBug](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3AInternals%3EMedia+Hotlist%3DGoodFirstBug) is your friend!

Filing a new bug:
[Template](https://bugs.chromium.org/p/chromium/issues/entry?template=Audio/Video%20Issue)

**Documentation:**

- [HTMLAudioElement](https://html.spec.whatwg.org/#htmlaudioelement)

- [HTMLMediaElement](https://html.spec.whatwg.org/#htmlmediaelement)

- [HTMLVideoElement](https://html.spec.whatwg.org/#htmlvideoelement)

Much of the documentation below is out of date and has significant gaps. For the
most up to date documentation please see the
[README.md](https://chromium.googlesource.com/chromium/src/+/HEAD/media/README.md)
file in the media/ directory (which is where all the code for Chromium's media
pipeline lives).

### Overview

There are several major components to Chromium's media playback implementation,
here are three most folks are commonly interested in:

*   [Pipeline](https://cs.chromium.org/chromium/src/media/base/pipeline.h)
    *   Chromium's implementation of a media playback engine
    *   Handles audio/video synchronization and resource fetching
*   FFmpeg{[Demuxer](https://cs.chromium.org/chromium/src/media/filters/ffmpeg_demuxer.h),
            [AudioDecoder](https://cs.chromium.org/chromium/src/media/filters/ffmpeg_audio_decoder.h),
            [VideoDecoder](https://cs.chromium.org/chromium/src/media/filters/ffmpeg_video_decoder.h)}
    *   Open source library used for container parsing and software
                audio and video decoding.
*   Blink's
            [HTMLMediaElement](https://cs.chromium.org/chromium/src/third_party/WebKit/Source/core/html/media/HTMLMediaElement.h)
    *   Implements the HTML and Javascript bindings as specified by
                WHATWG
    *   Handles rendering the user agent controls

#### Pipeline

The pipeline is a pull-based media playback engine that abstracts each step of
media playback into (at least) 6 different filters: data source, demuxing, audio
decoding, video decoding, audio rendering, and video rendering. The pipeline
manages the lifetime of the renderer and exposes a thread safe interface to
clients. The filters are connected together to form a filter graph.

Design goals:

- Use Chromium threading constructs such as
[TaskRunner](https://cs.chromium.org/chromium/src/base/task_runner.h)

- Filters do not determine threading model

- All filter actions are asynchronous and use callbacks to signal completion

- Upstream filters are oblivious to downstream filters (i.e., DataSource is
unaware of Demuxer)

- Prefer explicit types and methods over general types and methods (i.e., prefer
foo-&gt;Bar() over foo-&gt;SendMessage(MSG_BAR))

- Can run inside security sandbox

- Runs on Windows, Mac and Linux on x86 and ARM

- Supports arbitrary audio/video codecs

Design non-goals:

- Dynamic loading of filters via shared libraries

- Buffer management negotiation

- Building arbitrary filter graphs

- Supporting filters beyond the scope of media playback

The original research into supporting video in Chromium started in September
2008. Before deciding to implement our own media playback engine we considered
the following alternative technologies:

- DirectShow (Windows specific, cannot run inside sandbox without major hacking)

- GStreamer (Windows support questionable at the time, extra ~2MB of DLLs due to
library dependencies, targets many of our non-goals)

- VLC (cannot use due to GPL)

- MPlayer (cannot use due to GPL)

- OpenMAX (complete overkill for our purposes)

- liboggplay (specific to Ogg Theora/Vorbis)

Our approach was to write our own media playback engine that was audio/video
codec agnostic and focused on playback. Using FFmpeg avoids both the use of
proprietary/commercial codecs and allows Chromium's media engine to support a
wide variety of formats depending on FFmpeg's build configuration.

[<img alt="image"
src="/developers/design-documents/video/video_stack_arch.png">](/developers/design-documents/video/video_stack_arch.png)

As previously mentioned, the pipeline is completely pull-based and relies on the
sound card to drive playback. As the sound card requests additional data, the
[audio
renderer](https://cs.chromium.org/chromium/src/media/base/audio_renderer.h)
requests decoded audio data from the [audio
decoder](https://cs.chromium.org/chromium/src/media/base/audio_decoder.h), which
requests encoded buffers from the
[demuxer](https://cs.chromium.org/chromium/src/media/base/demuxer.h), which
reads from the [data
source](https://cs.chromium.org/chromium/src/media/base/data_source.h), and so
on. As decoded audio data data is fed into the sound card the pipeline's global
clock is updated. The [video
renderer](https://cs.chromium.org/chromium/src/media/base/video_renderer.h)
polls the global clock upon each vsync to determine when to request decoded
frames from the [video
decoder](https://cs.chromium.org/chromium/src/media/base/video_decoder.h) and
when to render new frames to the video display. In the absence of a sound card
or an audio track, the system clock is used to drive video decoding and
rendering. Relevant source code is in the
[media](https://cs.chromium.org/chromium/src/media/) directory.

The pipeline uses a state machine to handle playback and events such as pausing,
seeking, and stopping. A state transition typically consists of notifying all
filters of the event and waiting for completion callbacks before completing the
transition (diagram from
[pipeline_impl.h](https://cs.chromium.org/chromium/src/media/base/pipeline_impl.h)):

// \[ \*Created \] \[ Any State \] // | Start() | Stop() // V V // \[ Starting
\] \[ Stopping \] // | | // V V // \[ Playing \] &lt;---------. \[ Stopped \] //
| | Seek() | // | V | // | \[ Seeking \] ----' // | ^ // | Suspend() | // V | //
\[ Suspending \] | // | | // V | // \[ Suspended \] | // | Resume() | // V | //
\[ Resuming \] ---------'

The pull-based design allows pause to be implemented by setting the playback
rate to zero, causing the audio and video renderers to stop requesting data from
upstream filters. Without any pending requests the entire pipeline enters an
implicit paused state.

### Integration

The following diagram shows the current integration of the media playback
pipeline into WebKit and Chromium browser; this is slightly out of date, but the
gist remains the same.

<table>
<tr>
<td> <img alt="image" src="/developers/design-documents/video/video_stack_chrome.png"> </td>
<td> (1) WebKit requests to create a media player, which in Chromium's case creates WebMediaPlayerImpl and Pipeline.</td>
<td> (2) BufferedDataSource requests to fetch the current video URL via ResourceLoader.</td>
<td> (3) ResourceDispatcher forwards the request to the browser process.</td>
<td> (4) A URLRequest is created for the request, which may already have cached data present in HttpCache. Data is sent back to BufferedDataSource as it becomes available.</td>
<td> (5) FFmpeg demuxes and decodes audio/video data.</td>
<td> (6) Due to sandboxing, AudioRendererImpl cannot open an audio device directly and requests the browser to open the device on its behalf.</td>
<td> (7) The browser opens a new audio device and forwards audio callbacks to the corresponding render process.</td>
<td> (8) Invalidates are sent to WebKit as new frames are available.</td>
</tr>
</table>
