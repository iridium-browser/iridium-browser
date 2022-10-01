---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: video-capture
title: Video Capture
---

Last updated March 2015. Contributor: mcasas@

### Overview

There are several components allowing for Video Capture in Chrome

*   the Browser-side classes
*   the common Render infrastructure supporting all MediaStream use
            cases
*   the local &lt;video&gt; playback and/or Jingle/WebRtc PeerConnection
            implementation.

At the highest level, both Video and Audio capture is abstracted as a
[MediaStream](http://w3c.github.io/mediacapture-main/getusermedia.html); this JS
entity is created via calls to
[GetUserMedia()](http://www.html5rocks.com/en/tutorials/getusermedia/intro/#toc-gettingstarted)
with the appropriate bag of parameters. What happens under the hood and what
classes do what is the topic of this document. I'll speak about Video Capture
although some concepts are interchangeable with Audio. I'll speak of
WebCam/Camera but the same reasoning applies to Video Digitizers, Virtual
WebCams etc.

### Browser

In the Cr sandbox model, all of what pertains to using hardware is abstracted in
the Browser process, to which Render clients direct requests via IPC mechanisms,
and Video Capture is no different.

[<img alt="image"
src="/developers/design-documents/video-capture/VideoCaptureBrowserClasses.jpg"
height=253
width=400>](/developers/design-documents/video-capture/VideoCaptureBrowserClasses.jpg)

The goal of this set of classes is to enumerate, open and close devices for
capture, process captured frames, and keep accountancy of underlying resources.

At the Browser level there should be no device-specific code, that is, no
non-singletons, created before the user first tries to enumerate or use any kind
of capture (Video or Audio) device -- and that's the reason why "Video Capture
Device Capabilities" under chrome://media-internals is empty before actually
using any capture device.

Let's have a look at the diagram. In the early Chrome startup times there'd be
just one class,
[MediaStreamManager](https://code.google.com/p/chromium/codesearch#chromium/src/content/browser/renderer_host/media/media_stream_manager.h&sq=package:chromium&type=cs&q=mediastreammanager&l=64)
(MSM) sitting lonely on the upper left corner. MSM is a singleton class created
and owned by BrowserMainLoop on UI thread, using a bunch of
[MediaStream\*](https://code.google.com/p/chromium/codesearch#search/&sq=package:chromium&type=cs&q=content/browser/renderer_host/media/media_stream_*)
classes to have conversations over IPC and to request the user for permissions
(MediaStreamUI\*). MS\* classes deal with both Audio and Video.

MSM creates on construction two other important singletons, namely the
ref-counted
[VideoCaptureManager](https://code.google.com/p/chromium/codesearch#chromium/src/content/browser/renderer_host/media/video_capture_manager.h&sq=package:chromium&type=cs&q=videocapturemanager)
(VCM) and the associated
[VideoCaptureDeviceFactory](https://code.google.com/p/chromium/codesearch#chromium/src/media/video/capture/video_capture_device_factory.h&sq=package:chromium&type=cs&q=VideoCaptureDeviceFactory)
(VCDF). VCDFactory is used to enumerated Device names and their capabilities, if
they have any, and to instantiate VCDevices. There's one implementation
per-platform and there are a few available:

*   Platform specific: Linux/CrOS (based on
            [V4L2](http://linuxtv.org/downloads/v4l-dvb-apis/index.html) API),
            Mac (both
            [QTKit](https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/QTKitApplicationProgrammingGuide/Introduction/Introduction.html#//apple_ref/doc/uid/TP40008156-CH1-SW1)
            and
            [AVFoundation](https://developer.apple.com/library/mac/documentation/AudioVideo/Conceptual/AVFoundationPG/Articles/00_Introduction.html#//apple_ref/doc/uid/TP40010188)
            APIs), Windows (both
            [DirectShow](https://msdn.microsoft.com/en-us/library/windows/desktop/dd375454%28v=vs.85%29.aspx)
            and [Media
            Foundation](https://msdn.microsoft.com/en-us/library/windows/desktop/ms694197%28v=vs.85%29.aspx)
            APIs) and Android
            ([Camera](http://developer.android.com/reference/android/hardware/Camera.html)
            and
            [Camera2](http://developer.android.com/reference/android/hardware/camera2/package-summary.html)
            APIs).
*   Debug/Test devices:
            [FakeVCD](https://code.google.com/p/chromium/codesearch#chromium/src/media/video/capture/fake_video_capture_device.h&sq=package:chromium&type=cs&q=fakevideocapturedevice&l=21)
            and
            [FileVCD](https://code.google.com/p/chromium/codesearch#chromium/src/media/video/capture/file_video_capture_device.h&sq=package:chromium&type=cs&rcl=1426491802&l=26).
            FakeVCD produces a stream with a rolling PacMan over a green canvas,
            while FileVCD plays uncompressed Y4M files.
*   Other captures: Desktop and Tab (Web contents) Capture.

VCM manages the lifetime of pairs
&lt;[VideoCaptureController](https://code.google.com/p/chromium/codesearch#chromium/src/content/browser/renderer_host/media/video_capture_controller.h&sq=package:chromium&type=cs&q=videocapturecontroller),
[VideoCaptureDevice](https://code.google.com/p/chromium/codesearch#chromium/src/media/video/capture/video_capture_device.h&sq=package:chromium&type=cs&q=videocapturedevice&l=29)&gt;.
VideoCaptureDevices (VCD) are adaptation layers between whichever infrastructure
is provided by the OS for Video Capture and VideoCaptureController (VCC), whose
mission is to bridge captured frames towards Render clients through the IPC.

VCC interacts with the Render side code via a list of VideoCaptureHosts (VCH).
VCH is a dual citizenship bridge between commands coming from Render to VCM
([VideoCaptureHostMsg_Start](https://code.google.com/p/chromium/codesearch#chromium/src/content/common/media/video_capture_messages.h&cl=GROK&ct=xref_jump_to_def&l=91&gsn=VideoCaptureHostMsg_Start)
and relatives) and those flowing from VCC towards Renderer. There are as many
VCHosts as Render clients and they are created by RenderProcessHostImpl, another
high level entity.

Large data chunks cannot be passed through the IPC pipes, so a Shared Memory
mechanism is implemented in
[VideoCaptureBufferPool](https://code.google.com/p/chromium/codesearch#chromium/src/content/browser/renderer_host/media/video_capture_buffer_pool.h&sq=package:chromium&type=cs&q=videocapturebufferpool&l=46)
(VCBP -- not in the diagram, lives inside VCC). These ShMem-based "Buffers", as
they are confusingly called, are allocated on demand and reserved for Producer
or Consumers. VCC keeps accountancy of those and recycles them when all clients
have sent the [corresponding release
message](https://code.google.com/p/chromium/codesearch#chromium/src/content/browser/renderer_host/media/video_capture_host.cc&sq=package:chromium&type=cs&l=228).

VideoFrames do not understand Buffers from this Pool so VCC has to keep count of
them and recycle its uses via, again, IPC.

It's relatively common that the Capture Devices do not provide exactly what the
Render side requests or needs, hence an adaptation layer is inserted between VCC
and VCD:
[VideoCaptureDeviceClient](https://code.google.com/p/chromium/codesearch#chromium/src/content/browser/renderer_host/media/video_capture_device_client.h&sq=package:chromium&type=cs&q=videocapturedeviceclient).
VCDClient adapts sizes, applies rotations and otherwise converts the incoming
pixel format to YUV420, which is the global transport pixel format. The output
of such conversion rig is a ref-counted VideoFrame, which is the generic video
transport class. The totality of VCDs assume a synchronous capture callback,
hence VCDC copies/converts the captured buffer onto the ShMem allocated as a
"Buffer". This VideoFrame-Buffer couple is then replicated to each VCHost on
VCController.

(For the curious, IPC listener classes derive from BrowserMessageFilter).

Device Monitoring?

**Common Render Infra**

**Local &lt;video&gt; Playback**

**Remote WebRTC/Jingle encode and send**
