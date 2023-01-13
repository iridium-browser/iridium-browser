---
breadcrumbs:
- - /developers
  - For Developers
page_name: npapi-deprecation
title: 'NPAPI deprecation: developer guide'
---

We [recently
updated](http://blog.chromium.org/2014/11/the-final-countdown-for-npapi.html)
our plans to phase out support for NPAPI in early 2015. This guide provides more
details about what to expect and alternatives to NPAPI.

# Timeline

**January 2014**

Starting in Chrome 32*—*expected to reach the Stable channel in mid-January
2014*—*when a user visits a page with a blocked NPAPI plug-in, they will see:

[<img alt="image"
src="/developers/npapi-deprecation/Screen%20Shot%202013-12-03%20at%2010.10.16%20AM.png"
height=65
width=400>](/developers/npapi-deprecation/Screen%20Shot%202013-12-03%20at%2010.10.16%20AM.png)

Note that users who have already installed the plug-in in previous versions of
Chrome will still need to go through this "click-to-accept" experience. You can
test the behavior on [this
site](http://www.medicalrounds.com/quicktimecheck/troubleshooting.html).

**Mid-2014**

In mid-2014, the blocking UI will become more difficult to navigate, as a means
of discouraging NPAPI use by developers. With the harder-to-bypass blocking UI,
users will see a puzzle piece in place of the plug-in and a "Blocked plug-in"
[page action icon](http://developer.chrome.com/extensions/pageAction.html) in
the Omnibox:

<table>
<tr>

<td>Puzzle piece with right-click context menu</td>

<td>Blocked plug-in page action</td>

</tr>
<tr>
<td><img alt="image" src="https://lh4.googleusercontent.com/ug4oXz-OqmestmLBdUpyW4GRhLcWuyNt5CrzPTSX2zwEbgXYtLFwPRgotMFpeDqiDX91l7biB4ysP78f9ArErYfaz5a-VQMGQ3CdBadxR-MObR6BZk2LURQy" height=281px; width=297px;></td>

<td>(The yellow box will animate in.)</td>

<td><a
href="/developers/npapi-deprecation/Plugin-blocked%20yellow%20slide%20and%20bubble.png"><img
alt="image" src="/developers/npapi-deprecation/Plugin-blocked-yellow-slide.png"
height=93 width=200></a></td>

<td><a
href="/developers/npapi-deprecation/Plugin-blocked%20yellow%20slide%20and%20bubble.png"><img
alt="image"
src="/developers/npapi-deprecation/Plugin-blocked%20yellow%20slide%20and%20bubble.png"
height=192 width=320></a></td>

</tr>
</table>

    Right clicking the puzzle piece will bring up a context menu allowing the
    user to run or hide the plug-in (just once).

    Left clicking the page action icon will bring up a bubble giving the user
    the choice to "Always allow plug-ins on this site" or "Run all plug-ins this
    time."

Note that there will not be a yellow info bar (i.e. "drape") at the top of the
page. Also, the page action icon will appear even if the plug-in itself is
invisible. Visit this site in Canary to see the new UI in action:
http://www.medicalrounds.com/quicktimecheck/troubleshooting.html

This behavior is similar to existing behavior when all plug-ins are blocked by
default (“Settings” =&gt; “Advanced Settings” =&gt; “Privacy - Content Settings”
=&gt; “Plug-ins,” select “Block all,” and then load, for example,
<http://techcrunch.com>).

**January 2015**

Currently Chrome supports NPAPI plugins, but they are blocked by default unless
the user chooses to allow them for specific sites (via the [page action
UI](/developers/npapi-deprecation)). A small number of the most popular plugins
are allowed by default. In January 2015 all plugins will be blocked by default.
Even though users will be able to let NPAPI plug-ins run by default in January,
we encourage developers to migrate of off NPAPI as soon as possible. Support for
NPAPI will be completely removed from Chrome by September 2015.

**April 2015**

In April 2015 (Chrome 42) NPAPI support will be disabled by default in Chrome
and we will unpublish extensions requiring NPAPI plugins from the Chrome Web
Store. All NPAPI plugins will appear as if they are not installed, as they will
not appear in the navigator.plugins list nor will they be instantiated (even as
a placeholder). Although plugin vendors are working hard to move to alternate
technologies, a small number of users still rely on plugins that haven’t
completed the transition yet. We will provide an override for advanced users
(via chrome://flags/#enable-npapi) and enterprises (via Enterprise Policy) to
temporarily re-enable NPAPI (via the [page action
UI](/developers/npapi-deprecation)) while they wait for mission-critical plugins
to make the transition. In addition, setting any of the plugin Enterprise
[policies](/administrators/policy-list-3) (e.g. EnabledPlugins,
PluginsAllowedForUrls) will temporarily re-enable NPAPI.

**September 2015**

# In September 2015 (Chrome 45) we will remove the override and NPAPI support will be permanently removed from Chrome. Installed extensions that require NPAPI plugins will no longer be able to load those plugins.

# Exceptions

The only allowed plug-ins are the ones mentioned in the [blog
post](http://blog.chromium.org/2013/09/saying-goodbye-to-our-old-friend-npapi.html).
That list is based entirely on usage. There is no "process" for other plug-ins
to be universally allowed and the list will be removed in January 2015.

**Enterprise**

Enterprise administrators will be able to allow specific NPAPI plug-ins by
adding them to the
[EnabledPlugins](http://www.chromium.org/administrators/policy-list-3#EnabledPlugins)
policy list, to avoid their users seeing the UI mentioned above. Setting this
policy also re-enables NPAPI plugins. This, however, will not be relevant once
support for NPAPI is completely removed from Chrome in September 2015. Hence we
recommend enterprises and enterprise app developers as well to move entirely off
NPAPI as soon as possible.

On Mac, version 39 onward, Chrome will be 64 bit only and this will imply 32 bit
NPAPI plugins will stop to work on Chrome on Mac and there will be no way to
allow it by policy. [64 bit
plugins](https://support.google.com/chrome/answer/6083313) however can still be
allowed and will continue to work until overall NPAPI removal in September 2015.

# Alternatives to NPAPI

With the deprecation of NPAPI, some developers have asked which modern
technologies can be used to implement features which in the past would have
relied on a platform-specific NPAPI plug-in. In answer to these questions we
have composed the following list of common NPAPI use cases and web platform
alternatives.

In general, the core standards-based web technologies (HTML/CSS/JS) are suitable
for most client software development. If your application requires access to
features outside the web sandbox, myriad Chrome [Extension and App
APIs](http://developer.chrome.com/extensions/api_index.html) offer access to OS
features.

## Video and audio

A common use case for NPAPI plug-ins on the modern web is embedded video and/or
audio. A range of modern web technologies exist to facilitate media streaming.
The basic building blocks are WebRTC and media elements:

    HTML5 Media Elements. The [HTML5 Specification](http://www.w3.org/TR/html5/)
    provides a rich media platform through the
    [&lt;audio&gt;](http://www.w3.org/TR/html5/embedded-content-0.html#the-audio-element)
    and
    [&lt;video&gt;](http://www.w3.org/TR/html5/embedded-content-0.html#the-video-element)
    elements. More complicated use cases can be achieved using the
    [&lt;canvas&gt;](http://www.w3.org/TR/html5/embedded-content-0.html#the-canvas-element)
    element (for example check out the [Video FX Chrome
    Experiment](http://www.chromeexperiments.com/detail/videofx/)).

    WebRTC. [WebRTC](http://dev.w3.org/2011/webrtc/editor/webrtc.html) was
    designed for real time communication between peers and the technology can
    also be used for applications like live streaming media and data. Google’s
    [Chromecast
    device](http://www.google.com/intl/en/chrome/devices/chromecast/) uses
    WebRTC to stream HD video between a browser and TV.

Several features on top of these building blocks support more advanced use
cases:

### Adaptive Streaming

The ability to adapt media streaming to an individual consumer is critical in
delivering high-quality content to a large audience. In the past this capability
has been provided by technologies such as Silverlight’s smooth streaming and
Quicktime’s HTTP live streaming. The [Media Source
Extensions](https://dvcs.w3.org/hg/html-media/raw-file/tip/media-source/media-source.html)
to the HTML media element provide the capability to adapt a stream to an
individual consumer on the modern web. Html5rocks has put together a great
[example](http://updates.html5rocks.com/2011/11/Stream-video-using-the-MediaSource-API)
of how to use the Media Source Extensions to implement some of these common use
cases.

### Video Conferencing

Several of the most popular NPAPI extensions including Facebook Video Chat and
Google Talk provide video conferencing functionality within the browser. With
the introduction of [WebRTC](http://dev.w3.org/2011/webrtc/editor/webrtc.html)
video conferencing is facilitated directly through JavaScript APIs. The [Cube
Slam Chrome Experiment](http://www.chromeexperiments.com/detail/cube-slam/)
provides an example of peer to peer video conferencing via WebRTC.

### Digital Rights Management

[Encrypted Media
Extensions](https://dvcs.w3.org/hg/html-media/raw-file/tip/encrypted-media/encrypted-media.html)
give HTML5 video the DRM capabilities that previously would have required the
use of a platform specific plug-in. The [WebM
project](http://www.webmproject.org/) has provided a
[demo](http://downloads.webmproject.org/adaptive-encrypted-demo/adaptive/index.html)
which performs video playback using the Encrypted Media Extensions of the video
element. For more information, check out [the EME HTML5 Rocks
article](http://www.html5rocks.com/en/tutorials/eme/basics/).

### Closed Captioning

### [WebVTT](http://dev.w3.org/html5/webvtt/) and the [&lt;track&gt;](http://www.html5rocks.com/en/tutorials/track/basics/) element (a child element of &lt;video&gt;) enable web developers to add timed-text captioning capabilities to their HTML apps.

## Communicating with native applications

Try the [Native Messaging
API](http://developer.chrome.com/extensions/messaging.html#native-messaging) for
Chrome Apps and Extensions.

## Games & 3D

[Native Client (NaCL)](https://developers.google.com/native-client/) provides a
rich environment for cross-platform game development. Many games have already
been ported to or designed for NaCL. A number of
[examples](https://developers.google.com/native-client/community/application-gallery)
and [detailed
tutorials](https://developers.google.com/native-client/devguide/tutorial) to get
started with NaCL are available on the NaCL development site. The [WebGL
specification](http://www.khronos.org/registry/webgl/specs/latest/1.0/) provides
a high-performance platform for hardware-accelerated 3D graphics in the browser.
[Chrome experiments has an entire
category](http://www.chromeexperiments.com/webgl/) dedicated to examples and
demos of various WebGL use cases.

## Security

Some services have relied on NPAPI-based security techniques. We recommend
switching to [TLS](http://en.wikipedia.org/wiki/Transport_Layer_Security) or,
soon, [Web Crypto](http://www.w3.org/TR/WebCryptoAPI/#use-cases).

## Hardware access

In the past it has often been necessary to write platform specific plug-ins to
access system hardware such as webcams, microphones, USB devices, and bluetooth.
Direct access to local media streams such as webcams and microphones can now be
requested directly from the web via the WebRTC [Media
Capture](http://dev.w3.org/2011/webrtc/editor/getusermedia.html) specification.
Chromium also provides an [App API for access to USB
hardware](http://developer.chrome.com/apps/usb.html) and another [API for
accessing Bluetooth devices](http://developer.chrome.com/apps/bluetooth.html).

## Screen capture

Chrome extensions can perform screen capture or streaming using either [Desktop
Capture](http://developer.chrome.com/extensions/desktopCapture.html) for full
screen capture or the [Tabs
API](http://developer.chrome.com/extensions/tabs.html) captureVisibleTab for
individual tab content capture.
