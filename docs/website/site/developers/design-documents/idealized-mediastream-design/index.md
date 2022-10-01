---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: idealized-mediastream-design
title: Idealized MediaStream Design
---

Idealized MediaStream Design in Chrome

Designed November 2013 by the [Piranha
Plant](https://code.google.com/p/chromium/wiki/PiranhaPlant)
[team](mailto:piranha-plant@chromium.org). Document author
[joi@chromium.org](mailto:joi@chromium.org).

# Introduction

This document describes the idealized design for MediaStreams (local and remote)
that we are aiming for in Chrome. By idealized we mean that this is not the
current design at the time of writing, but rather the design we want to
incrementally move towards.

# Background

The [MediaStream](http://dev.w3.org/2011/webrtc/editor/getusermedia.html)
specification has evolved from being focused around
[PeerConnections](http://dev.w3.org/2011/webrtc/editor/webrtc.html) for sending
media to remote endpoints and &lt;audio&gt; and &lt;video&gt; tags for local
playback, to a spec where streaming media can have multiple different endpoints,
including the above as well as
[recording](http://www.w3.org/TR/mediastream-recording/) and
[WebAudio](https://dvcs.w3.org/hg/audio/raw-file/tip/webaudio/specification.html)
and possibly more in the future.

Because of this heritage, Chrome’s current implementation of MediaStreams is
heavily dependent on [libjingle](https://code.google.com/p/libjingle/), even for
streams that are neither sent nor received via a PeerConnection.

At the same time, the specification now seems relatively stable, and we are
aware of various things we will want to be able to achieve in the near future
that the current design is not well-suited for (examples include relaying,
hardware capture devices that can encode directly to a transport encoding,
hardware decoders that can directly render media, embedder-specific features
that wish to act as endpoints for streaming media, and more).

This document assumes familiarity with the
[MediaStream](http://dev.w3.org/2011/webrtc/editor/getusermedia.html) and
[PeerConnection](http://dev.w3.org/2011/webrtc/editor/webrtc.html) HTML5
specifications, with Chrome's [multi-process
architecture](http://www.chromium.org/developers/design-documents/multi-process-architecture),
its [Content](http://www.chromium.org/developers/content-module),
[Blink](http://www.chromium.org/blink) and
[Media](http://www.chromium.org/audio-video) layers, as well as common
[abstractions](http://www.chromium.org/developers/coding-style/important-abstractions-and-data-structures)
and approaches to
[threading](http://www.chromium.org/developers/design-documents/threading).

# Objectives

Objectives include:

    Create the simplest design that will support features we expect to need in
    the future (see background section).

    Make life easier for Chrome developers using MediaStream.

    Improve quality, maintainability and readability/understandability of the
    MediaStream code.

    Limit use of libjingle to Chrome’s implementation of PeerConnection.

    Balance concerns and priorities of the different teams that are or will be
    using MediaStreams in Chrome.

    Do the above without hurting our ability to produce the WebRTC.org
    deliverables, and without hurting interoperability between Chrome and
    software built on the WebRTC.org deliverables.

# Design

## Sources, Tracks and Sinks<img alt="image" src="https://lh4.googleusercontent.com/HN2JTb9zkbg98uWT5ed8imaPP-Ee7JSuUeAi22aoN6TV8VWViBa0gYQlpgCGiekw-DicKIvwcTMeHhFzcu2UO9z0Nly-4dnlQ08AjyJIJK1MuK0zddVXsKkl-Q" height=515px; width=624px;>

Figure 1 - Overview of Sources, Tracks and Sinks. Local media capture assumed.

The main Blink-side concepts related to MediaStreams will remain as-is. These
are WebMediaStream, WebMediaStreamTrack, and WebMediaStreamSource.

In Content, there will be a content::MediaStreamTrack and
content::MediaStreamSource, each with Audio and Video subclasses and each
available to higher levels through the Content API. These will be directly owned
by their Blink counterparts via their ExtraData field.

A MediaStream is mostly just a collection of tracks and sources. The Blink-side
representation of this collection should be sufficient, with no need for this
collection concept in Content.

A content::MediaStreamSource can be thought of as receiving raw data and
processing it

(e.g. for echo cancellation and noise suppression) on behalf of one or more
content::MediaStreamTrack objects.

A media::MediaSink interface (with Audio and Video subclasses) can be
implemented by Content and layers above it in order to register as a sink (and
unregister prior to destruction) with a content::MediaStreamTrack, thereby
receiving the audio or video bitstream from the track, and necessary metadata.

Sinks are not owned by the track they register with.

A content::MediaStreamTrack will register with a content::MediaStreamSource (and
unregister prior to destruction). The interface it registers will be a
media::MediaSink. Additionally, on registration the track will provide the
[constraints](http://dev.w3.org/2011/webrtc/editor/getusermedia.html#mediastreamconstraints)
for the track, which among other things indicate which type of audio or video
processing is desired.

<img alt="image"
src="https://lh5.googleusercontent.com/vVe5rDS6SzeAAygbYo3Ynrjq5UCH65dVTJidG708hpSuW6XvCVWPLWQiDVbvAP-01sJFcBtLpqNmPGY6LhsLen2z3psmsOzJOkaczSGG3zw98sycvrX_INRx3Q"
height=333px; width=624px;>

Figure 2 - Showing processing module ownership in a source.

A source will own the processing modules for the tracks registered with it, and
will create just one such module for each equivalent set of constraints. Shown
here are audio processing modules or APMs, but in the future we expect to have
something similar for video e.g. for scaling and/or cropping.

## Sinks for Media

### Local Sinks

In the Content and Blink layers, there will be local sink implementations, i.e.
recipients of the audio and video media from a MediaStreamTrack, for
&lt;audio&gt; and &lt;video&gt; tags, [WebAudio
](https://dvcs.w3.org/hg/audio/raw-file/tip/webaudio/specification.html)and
[recording](http://www.w3.org/TR/mediastream-recording/).

### Remote-Destined Sinks

The Content and Blink layers will also collaborate to enable piping the media
from the various tracks in a MediaStream over a PeerConnection.

### Application-Specific

Layers embedding Content (e.g. the Chrome layer) may also implement
application-specific sinks for MediaStreamTracks. These might be local or
remote-destined.

## Push Model and Synchronization

Above the level of sources, we will have a push model. Sources will push data to
tracks, which in turn will push data to sinks.

Sources may either have raw data pushed to them (e.g. in the case of local media
capture), or will need to pull data (e.g. in the case of media being received
over a PeerConnection). This distinction will be invisible to everything above a
source.

A source will have methods to allow pushing media data into it. It will also
allow registering a media::MediaProvider (which will have Audio and Video
subclasses) from which the source can pull. The source takes ownership of the
MediaProvider.

<img alt="image"
src="https://lh3.googleusercontent.com/XHk0O8xJDiezATLchY1z6pI-aeq9zXlMvsN4BUfstJZs6mk0FzZTAxpfa3xbxUNUfkSNihpobofg9EQGGMfgBs5apJJXq4xW29QQMWJKU_WGsnHSpOTdjwk5hQ"
height=448px; width=624px;>

Figure 3 - Pushed to source vs. pulled by source

When audio is being played locally that needs to be pulled by a source, the
“heartbeat” that will cause the source to pull will be the heartbeat of the
local audio output device, which will be sent over IPC and end up on the capture
thread. When there is no such heartbeat available (e.g. audio is not being
played, data is just being relayed to another peer over a PeerConnection or only
video is playing) an artificial heartbeat for pulling from MediaProviders and
pushing out from the source will be generated on the capture thread.

The reason to use the audio output device as the heartbeat is to ensure that
data is pulled at the right rate and does not fall behind or start buffering up.

Both audio and video data, from all sources, will have timestamps at all levels
above the source (the source will add the timestamp if it receives or pulls data
that is not already timestamped). Media data will also have a GUID identifying
the clock that provided the timestamps. Timestamps will allow synchronization
between audio and video, e.g. dropping video frames to catch up with audio when
video falls behind. A GUID will allow e.g. synchronization of two tracks that
were sent over separate PeerConnections but originated from the same machine.
TODO: Specify this in more detail before implementing.

## MediaStreams with Local Capture

To create a MediaStream capturing local audio and/or video, the
[getUserMedia](http://dev.w3.org/2011/webrtc/editor/getusermedia.html) API is
used. The result is a MediaStream with a collection of MediaStreamTracks each of
which has a locally-originated MediaStreamSource.

In this section, we add some detail on the local audio and video pipelines for
such MediaStreams.

<img alt="image"
src="https://lh5.googleusercontent.com/TP8PmdhcA59qxU_1azStCn61FPUraoeEG2kw4Bz_-oyvZpZMoUcDYTtUurXwFnVoPAaBIPxhr5EITQMLbHrzehzq8y6wgM9ibdR90F11FiowFAyhEdjQspA-nQ"
height=510px; width=414px;>

Figure 4 - Detail on getUserMedia request.

Figure 4 shows the main players in a getUserMedia request. A UserMediaRequest is
created on the Blink side, the request to open devices ends up in the
CaptureDeviceManager on the browser side, and the UserMediaHandler is
responsible for creating the hierarchy of a WebMediaStream object on the blink
side, and the blink- and Content-side source objects for each opened device, and
the initial track for each source.

### Local Audio Pipeline<img alt="image" src="https://lh4.googleusercontent.com/O0MUgRkOUg0sdLy7Q4JlvJ1axESTtUPBe5mut532JBX4dzgoyh1v9cvNUQypGLqdLQ9be0T7Trv0qfihEpNoEsnFeM7grcDWBqVyFxSYzAmct1eh76gaez8DtA" height=476px; width=426px;>

Figure 5 - Local Audio Pipeline Overview

When a local audio capture device is opened, the end result is that there is an
AudioStream instance on the browser side that receives the bitstream from the
physical device, and an AudioInputDevice instance on the renderer side that, in
the abstract, is receiving the same bitstream (over layers of IPC and such, as
is typical in Chrome). The AudioInputDevice pushes the audio into the
MediaStreamAudioSource.

Setting up an AudioInputDevice/AudioStream pair involves several players. At a
somewhat abstract level:

    The MediaStreamAudioSource has a session ID, which receives it on creation,
    in step 8 of figure 4;

    This session ID is passed through the various layers. The AudioInputHost
    looks up session ID-keyed information in the AudioInputDeviceManager, then
    using that information causes the AudioInputController to create an
    AudioStream for the device via the AudioManager.

### Local Video Pipeline

Similar to the audio capture pipeline. Key players are the browser/renderer pair
VideoCaptureController and VideoCaptureClient (analogous to AudioStream and
AudioInputDevice), and the browser-side VideoCaptureHost and
VideoCaptureManager.

TODO: Finish this section and provide a chart matching the class hierarchy and
names between the two.

Figure 6 (placeholder) - Local Video Pipeline Overview

## MediaStreams With PeerConnection

### Sent Over PeerConnection<img alt="image" src="https://lh4.googleusercontent.com/0sq2VE9aHu2tNBW-sL4brXuYwqhbazksb4OnToj8lxPtNmNTUaIGOsBNTjGnZ7FnqmXJQzn3HfYp-fWjgigtV2nZtMPJa2eNOaJPNJvXLCL18rSjSJ8_yyCn9Q" height=359px; width=624px;>

Figure 7 - MediaStream Sent Over PeerConnection

When a web page calls PeerConnection::AddStream to add a MediaStream to a
connection, the sequence of events will be as follows:

    content::RTCPeerConnectionHandler will add a SentPeerConnectionMediaStream
    to its map of such objects, keyed by the WebMediaStream that is being added.

    SentPeerConnectionMediaStream creates the libjingle-side
    webrtc::MediaStream.

    For each content::MediaStreamTrack, it creates a webrtc::MediaStreamTrack,
    and a content::LibjingleSentTrackAdapter which will push media bits to the
    webrtc::MediaStreamTrack.

    SentPeerConnectionMediaStream registers the LibjingleSentTrackAdapter as a
    sink on the content::MediaStreamTrack.

    Once all adapters and libjingle-side tracks are set up,
    SentPeerConnectionMediaStream adds the webrtc::MediaStream to the
    webrtc::PeerConnection.

Ownership is such that the WebMediaStream and related objects on the Blink side
own the content::MediaStreamSource and its tracks, and the
SentPeerConnectionMediaStream owns the libjingle object hierarchy and the
LIbjingleSentTrackAdapter (likely one implementation for audio, and one for
video).

In this structure, the only usage of libjingle in Content is by
LibjingleSentTrackAdapter and by SentPeerConnectionMediaStream. The rest of the
MediaStream implementation in Content and Blink does not know about libjingle at
all. Note also that no libjingle-side objects except for the PeerConnection are
created until a MediaStream is added to it.

### Received Via PeerConnection<img alt="image" src="https://lh4.googleusercontent.com/TULqa8D9Rb6grUNNNZ8srWtwt0hv9ZPV7aoi2vLoX-YVa4gNNSUK6MAchJdkVCB43kenkyxW1Bb3hK0b_ibwHj51BYuxeANLidyq7kAAZ9mda4-COU_qLuN6_g" height=332px; width=624px;>

Figure 8 - MediaStream Received via PeerConnection

When a webrtc::PeerConnection receives a new remote MediaStream, the sequence of
events is as follows:

    On the libjingle side, the webrtc::MediaStream and its tracks are created.
    Note that these tracks do not have a source.

    content::RTCPeerConnectionHandler receives an onAddStream notification. It
    creates a ReceivedPeerConnectionMediaStream and adds it to a map of such
    objects keyed by MediaStream labels for MediaStreams received over a
    PeerConnection.

    The ReceivedPeerConnectionMediaStream creates the Blink-side object
    representing the MediaStream.

    It then creates one local Source per received track. This is important;
    since received tracks don’t have sources, locally they act like sources.

    It then creates one local Track per local Source.

    Finally, in the audio case it creates a
    content::LibjingleReceivedTrackAdapter that retrieves data from the
    webrtc::MediaStreamTrack, and registers it as a media::MediaProvider for the
    local content::MediaStreamSource. The source will pull data via the
    MediaProvider interface. In the video case, the adapter will push video to
    the source.

Ownership is such that the WebMediaStream and related objects on the Blink side
are referenced by the ReceivedPeerConnectionMediaStream, and they own the
content::MediaStreamSource and its tracks. The LibjingleReceivedTrackAdapter is
owned by the content::MediaStreamSource object.
content::RTCPeerConnectionHandler owns the webrtc::PeerConnection, which owns
the libjingle-side object hierarchy.

In this structure, the only usage of libjingle in Content is by
LibjingleReceivedTrackAdapter, ReceivedPeerConnectionMediaStream and
RTCPeerConnectionHandler. The rest of the MediaStream implementation in Content
and Blink does not know about libjingle at all. Note also that no libjingle-side
objects except for the PeerConnection are created until the PeerConnection
receives a new MediaStream from a peer.

### MediaStream Mutability

This section explains how addition and removal of MediaStreamTracks to a
MediaStream being sent/received over a PeerConnection will be done.

In the case where a track is added to a local MediaStream,
RTCPeerConnectionHandler will receive a notification that this has happened
(TODO: specify how it receives this notification). It will check if it has a
SentPeerConnectionMediaStream for the given MediaStream. If so, it will create
the libjingle-side track and add it to the libjingle-side MediaStream, as well
as creating the requisite LibjingleSentTrackAdapter and registering it as a sink
for the local track that was added to the MediaStream.

When a track is added to a MediaStream received over a PeerConnection, the
libjingle-side code will add the webrtc::MediaStreamTrack to the
webrtc::MediaStream. The content::ReceivedPeerConnectionMediaStream object
(which observes webrtc::MediaStream) will receive a notification that a track
was added, and will add a Blink- and Content-side source and track to the
Blink-side MediaStream it already owns. It will create and own a
LibjingleReceivedTrackAdapter, configure it to retrieve media from the
libjingle-side track, and set it as the media::MediaProvider for the local
content::MediaStreamSource (see previous section, Received via PeerConnection).

It is important to note that when a track is added to or removed from a
MediaStream that is not being sent over or received via a PeerConnection, the
libjingle code will not be involved in any way and no libjingle-side objects
will be created or destroyed.

### NetEQ

NetEQ is a subsystem that is fed an input stream of RTP packets carrying encoded
audio data, and from which client code pulls decoded audio. When audio is pulled
out of it, NetEQ attempts to create the best audio possible given the data
available from the network at that point (which may be incomplete, e.g. in the
face of packet loss or packets arriving out of order).

Currently, there is a single NetEQ for audio on a MediaStream received over a
PeerConnection, and the NetEQ sits at the libjingle level.

Several medium-term usage scenarios would be better served if we had more
control over where to place the NetEQ. Two examples are:

    Relaying audio data from one peer to another peer. In this case we might
    want to have no decoding done on the data whatsoever, and therefore skip the
    NetEQ.

    [Recording](http://www.w3.org/TR/mediastream-recording/) audio. Since
    recording does not have the same near-real-time demands as does a
    &lt;video&gt; tag showing a live video/audio chat with a remote peer, it can
    afford to let more audio data buffer up in the NetEQ before it starts
    pulling audio from it, and possibly even to delay the next pull up to some
    maximum time if the NetEQ reports that it does not have full audio data yet.
    If you are recording audio at the same time as playing it, this implies you
    would want one NetEQ that is pulled out of at near real-time for playing the
    audio, and a separate one that is pulled out of more slowly for recording,
    since this will improve the quality of the recorded audio (fewer missing
    packets of audio data).

To fulfill the above needs, we plan to look into moving the NetEQ to the same
layer as the intended location of the APM (see Figure 2). This would allow
individual tracks on a source to set up the NetEQ using their desired
constraints, and in the scenarios above would allow you to have a track that
receives the raw audio data without it being decoded, or to clone your
near-real-time track and set the constraints on the clone to get a track with
NetEQ usage more suited to recording.

## MediaStream Threading Model

TODO: This needs more work to flesh it out on the video side, and to figure out
the idealized design. The following mostly documents the current situation for
audio. For video, a separate capture thread may not be needed and for some
clients it is undesirable to have a separate thread for capture in both the
audio and video cases.

We expect the current threading model around MediaStreams in Content to remain
mostly unchanged. On the renderer side, the main threads are:

    The main renderer thread (RenderThread). This is where most signaling takes
    place.

    A single media render thread. This is where media is prepared for output.
    This thread takes care of things such as resampling to APM format to provide
    the original image for echo cancellation (may happen 2 times or more;
    possibly some of this can be avoided).

    A single media capture thread. This thread does the following:

        processing

        resampling

        AEC, noise suppression, AGC

On the browser side, messages to/from the renderer are sent/received on the IPC
thread. There are also media render and media capture threads similar to the
renderer side, whose job is to open physical devices and write to or read from
them. The browser-side media render thread may also do resampling of audio when
the renderer is providing audio in a different format than the browser side
needs, but this should be minimized by reconfiguring the renderer side when
needed.

### PeerConnection Threading

For PeerConnection, there are two main threads:

    The “signaling thread” which in Chrome is actually the same as the
    RenderThread. The signaling thread is where the “API” for libjingle lives.

        This is a bit confusing because calls from the RenderThread are made to
        a proxy class, that in the general case would post the tasks to a
        different thread. In the special case where the calling and destination
        threads are the same, this ends up as a simple same-thread dispatch,
        i.e. equivalent to a method call.

            This is a libjingle thing, other clients desire thread-independent
            calling.

            It would be nice to get rid of the need for the proxy but we may
            need it for Java code in Android. TODO: Check with Ami.

    The worker thread (created and owned within libjingle), of which there is
    initially one, but may be more.

        This is separate from the signaling thread.

        It prepares network packets.

        It calls libjingle for media work.

## Directory Structure

We plan to make a couple of minor changes to the directory structure under
//content to help better manage dependencies and more easily put in place an
appropriate ownership structure:

    Code that #includes anything from libjingle will be isolated to
    //content/renderer/media/webrtc.

        Outside of this directory, //content will not directly depend on
        libjingle.

    Code related to the HTML5 MediaStream spec will be isolated to
    //content/{common|browser|browser/renderer_host|renderer}/media/media_stream.
