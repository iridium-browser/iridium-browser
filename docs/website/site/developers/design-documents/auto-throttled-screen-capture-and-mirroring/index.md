---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: auto-throttled-screen-capture-and-mirroring
title: Auto-Throttled Screen Capture and Mirroring
---

Author: Yuri Wiitala (miu@chromium.org)
Last Updated: September 2015

# Background

Chromium currently has three built-in screen capture methods (tab content,
window, and full desktop) and two video remoting methods (WebRTC/PeerConnection,
and Cast Streaming). Regardless of the combination chosen, the traditional
approach to mirroring sessions has been to fix a number of key parameters for
the entire session, such as video resolution, maximum frame rate, encoder bit
rate, and end-to-end latency (a.k.a. target playout delay, a fixed window of
time between frame capture and remote display of the frame).

The traditional approach, however, fails to account for a constantly-changing
system environment during a mirroring session. For example, an encoder might
become starved for CPU when automated background processes start to run on the
sending device. Another example: Network throughput, latency, and packet loss
can wildly fluctuate, especially when using WiFi networks, and especially in
crowded environments.

The traditional approach fails even in ideal environments: Whether it is a
Google Engineer shipping default settings (a one size fits all approach) or an
end-user customizing settings (often under misconceptions about how the system
works), there is no optimal choice. For example, a video resolution of 1280x720
may be too much for an old laptop to handle, while at the same time a
high-performance gaming desktop could easily handle 1920x1080 or higher. Either
way, the user experience is not optimized.

# Overview

This design discusses the goals to be achieved and how the Sender will
accomplish those goals by altering parameters while a mirroring session is
running. This document will focus mainly on tab capture, to produce video, and
Cast Streaming, to encode and transmit the video to a Receiver (remote device
and display). However, this design should be applicable to other pairings of
screen capture type and remoting method.

The user experience depends on the content being mirrored: interactive content
(e.g., slide-show presentations, browsing news articles, browsing photo
galleries, and other mostly-static content), and animated content (e.g., movies,
sports broadcasts, and other video; and real-time rendered visualizations).

For interactive content, the user experience is maximized when end-to-end
latency is low, so that user input actions have an immediate result on the
remote display; and also when video frame quality is high, so text and/or
diagrams are crisp and readable. Therefore, this design focuses on dropping
frames in order to maintain the maximum possible video resolution and encoder
target bit rate, all within the hardware constraints of the system.

For animated content, roughly the opposite is true. Dropping frames is a very
perceivable event and should be avoided at all costs. Thus, the system should
increase end-to-end latency and decrease video frame quality to compensate. Note
that users tend not to perceive small adjustments in video resolution (e.g., a
switch between 1280x720 and 1440x810) so long as the smooth video is not
interrupted.

# Requirements

Zero configuration: Other than hard-coded "sanity bounds," there should be no
configuration options exposed to the end-user. Based on the type of content
being mirrored, the system must continuously self-configure to optimize for the
user experience. Performance problems must be detected and abated. Likewise,
where underutilized, the system should take opportunities to improve quality.

Detect animating content: The decisions made by the system depend on knowing
what type of content is being mirrored. In addition, knowing the animation frame
rate is critical when calculating new session parameters.

Avoid extra work: Model, predict, and react to upcoming "stalls" before they
force the mirroring pipeline (capturer, encoder, etc.) or Receiver to drop a
frame. In other words, a video frame should not be captured if it has no hope of
getting transmitted and displayed.

Support variable media: Every component of the mirroring pipeline in the Sender,
as well as the Receiver, must support varying the video resolution, frame rate,
and encoder bit rate.

# Approach

The overall approach is to control the data volume of the video being produced
by the screen capturer to a point where all components of the mirroring pipeline
are operating at a sustainable rate. As each frame progresses through the
pipeline, each component's processing performance is measured for the frame.
Heuristics look at how the performance is trending and trigger reductions (or
increases) in data volume by altering the resolution and/or rate of video frames
being produced.

A video frame is produced by sending the GPU a request to scale a texture and
read the result back into a pooled buffer. Once the read-back is complete, the
buffer is handed over to an encoder and then the resulting encoded frame bytes
are transmitted over the network to the Receiver.

### Measuring utilization

In order to determine the end-to-end system's capability, the potential
bottlenecks in the mirroring pipeline must be monitored, and feedback signals
are sent back to the control logic.

GPU: GPUs are optimized for throughput, and the requests made to scale,
YUV-convert, and read-back data are internally re-ordered and pipelined. In
addition, it's possible for multiple frame captures to be executing
simultaneously within the GPU. Here's an example timing diagram for frame
captures:

\[\[TBD: Research cause/solution to "Compositor self-throttling" in "Open
Issues" section below. That will likely change our design approach here.\]\]

One way to measure the GPU's utilization is to measure "lag." First, the time
difference between each successive capture request is noted (e.g., Frame2Start -
Frame1Start, and Frame3Start - Frame2Start). Then, the time difference between
each successive capture completion is noted (e.g., Frame2Finish - Frame1Finish,
and Frame3Finish - Frame2Finish). The ratio between the two differences is
labeled "lag," where a value greater than 100% indicates the GPU is not keeping
up with the capture requests.

Buffer Pool: Chromium currently uses a fixed-size video frame buffer pool \[and
this design decision needs to be revisited to account for differences in
available RAM, and greater needs for high frame rate content\]. The utilization
of the buffer pool is monitored, and when it grows beyond a safety threshold,
the system interprets this as an overload signal. Either frames are being
produced at too-fast a rate, frames are not being encoded fast enough, or both.

Encode time: Whether the encoder is a software or hardware-based implementation,
the amount of time it takes to encode each frame is measured, relative to the
frame's duration. For a software encoder, this time utilization measurement will
indicate whether the Sender's CPU is over-utilized. A value that exceeds 100% is
interpreted as follows: "This frame took longer to encode than the duration it
will be shown on the Receiver's display. If this occurs too often, the pipeline
will be forced to drop frames."

Note that: 1) The measurement must be based on the passage of time according to
a high-resolution clock. 2) The measurement accounts for all the effects of
thread starvation issues and CPU core availability without having to directly
measure these things.

Encode bit rate: Network conditions are always changing throughout a session.
Various measurements are taken to estimate network bandwidth and round-trip
latency. The estimates are then used to determine a target encode bit rate.

Encoders do their best to match the target bit rate, but have to play an
up-front "guessing game" with each frame to determine how much entropy will be
preserved, and this directly impacts the size of the output. They usually over-
or undershoot the target bit rate, and then remember this error and spread a
correction over future frame encodes.

A naive utilization measurement is to divide the actual bit rate by the target
bit rate for each frame. However, using this to determine a bottleneck in the
pipeline is flawed because the encoder is being tasked with the goal of
achieving as close to the target bit rate as possible, regardless of the
complexity of the input frames. Thus, a 100% value would rarely indicate encoder
output is at a maximum sustainable rate. Instead, the encoder is just trying to
use more bandwidth to maximize quality and could possibly do an acceptable job
with less.

Therefore, the encode bit rate utilization needs to be adjusted so that it can
be interpreted to mean "the fraction of the target bandwidth required to encode
this frame." The solution is to account for the complexity of the input frames.
Many encoder implementations, such as libvpx (VP8/VP9), will provide a
"quantizer index" value that represents how much of the entropy was discarded
when encoding the frame. Higher quantizer values imply lower quality and smaller
output. The quantizer value was the encoder's guess when it played the "guessing
game" described above. And, the results from using that guess are known (the bit
rate utilization). From these values, the desired utilization measurement can
computed as follows:

`BitRateUtilization = ActualBitRate / TargetBitRate`

`IdealQuantizer = BitRateUtilization * ActualQuantizer`

`UtilizationMeasurement = IdealQuantizer / MaximumValidQuantizer`

Some examples to show how this works:

Example 1: Let's assume an input frame is too complex to be encoded within the
limitation of the current target bit rate. After the encode is complete, the
`BitRateUtilization` turned out to be 140% and the `ActualQuantizer` used for
the encode was 58. Thus, the `IdealQuantizer` is computed to be 81.2, and with a
`MaximumValidQuantizer` of 63 (for VP8), the `UtilizationMeasurement` is 129%.

Example 2: Let's assume a very simple input frame is being encoded and the
encoder uses almost all of the allowed bit rate to maximize quality, but could
do well with less bandwidth. The `BitRateUtilization` turns out to be 90% and
the `ActualQuantizer` used was 5. The `IdealQuantizer` would then be 4.5, and
the `UtilizationMeasurement` is 7%.

There are some encoder APIs that do not provide the quantizer, or any similar
proxy for it. In this case, it may be necessary to parse frame headers from the
encoded bitstream to extract this information. If that is not feasible, an
implementation will need custom logic to model the encoder's response to the
input video and provide the `ActualQuantizer` value.

### Pre-processing of utilization feedback signals

All utilization signals are in terms of 0% (no load) to 100% (maximum
sustainable load), with values greater than 100% indicating a pipeline component
will stall or drop frames if the data volume/complexity is not reduced. These
raw signals must be translated into metrics that are actionable by control
logic.

Attenuation: Running at the "red line" is not a good strategy. Therefore, all
utilization signals should be attenuated so that they are measuring relative to
a "comfortable maximum." This allows the pipeline components some breathing room
to be able to process the occasional edge-case frame, one that suddenly takes
more processing resources to handle, without actually reaching the true maximum
load.

Time-weighted averaging: The per-frame utilization signals are typically very
volatile. To ensure outlier data points don't trigger premature decisions, all
signals are passed through a low-pass filter before being interpreted. The
weighting of the averaging will depend on the volatility and the risk in making
bad decisions.

### The actionable metric: Capable pixels per frame

Extensive research was performed to determine that the load experienced by all
mirroring pipeline components is typically linearly proportional to the number
of pixels per second being processed. In addition, Receiver video quality was
tested, both qualitatively and quantitatively, using PSNR and SSIM, and found to
also be linearly proportional to the pixel rate, all other parameters fixed.

One exception is with the encoded output size; and while the result was observed
with VP8, it is likely to be the same for other codecs. The complexity of the
content turned out to have a huge factor on the relationship between the output
size and the input pixel rate. For videos with very high entropy (e.g., "Ducks
take off from rippling water"), the encoded output size was linearly
proportional to the pixel rate. However, with many typical samples of video
(e.g., action movie clips, sports, 3D animation shorts, etc.) the relationship
was actually sublinear. In other words, increasing the input pixel rate by 2X
would result in less than a 2X increase in the output size. The conclusion we
draw here is to simply assume the worst case--that relationship is linear--and
allow the control logic to converge at an optimum over time.

Thus, only one control "knob," the pixel rate, is needed to ensure every
component in the end-to-end system can handle the load. Two things affect the
pixel rate: the frame resolution, and the frame rate. Of the two, the frame
resolution has a much larger impact on the pixel rate and, for animated content,
has a lesser impact on the end-user experience. Any adjustment in capture
resolution provides a "squared" increase or decrease in the pixel rate, compared
to only a linear change when adjusting the frame rate.

So, for now, the design only considers controlling load by changing the capture
resolution. See Open Issues below for considerations regarding high frame rate
content. The capture resolution is determined by first computing the
pipeline-capable number of pixels per frame, as follows:

`PipelineUtilizationi = GREATEST(GpuUtilizationi, EncodeTimeUtilizationi, …)`

`CapablePixelsi = CaptureResolutioni / PipelineUtilizationi`

`CapablePixelsPerFramet = TIME_WEIGHTED_AVERAGE(CapablePixels0, …)`

Note that the frame rate is not a term included in the math above. However, it
is accounted for in the utilization feedback signals: An increasing frame rate
would cause the per-frame utilization to increase, depending on how resources
such as CPU and network bandwidth are limited.

### Deciding the capture resolution

Just before requesting the capture of each frame, logic is needed to decide what
the resulting frame resolution should be. In other words, given the size of the
source content and the current capable pixels per frame (see above), should the
source content be down-scaled and, if so, to what size?

First of all, note that the capable pixels per frame can fluctuate
frame-to-frame in a rather noisy fashion. Suppose a naive calculation is used to
compute the frame resolution:

`NaiveFrameWidtht = SQRT(CapablePixelsPerFramet * SourceContentAspectRatio)`

`NaiveFrameHeightt = SQRT(CapablePixelsPerFramet / SourceContentAspectRatio)`

The above would result in a different frame resolution for each and every frame,
with several obvious drawbacks:

    Most down-scaling algorithms would have a hard time producing quality
    results with the peculiar/odd resolutions. For example, sharp lines and text
    in the source content image would appear fuzzy and/or distorted in the
    down-scaled result.

    There would be extra taxing of the CPU because most encoders require an
    expensive re-initialization whenever frame sizes change. Also, there would
    be extra taxing of the available network bandwidth, as most encoders would
    need to emit a full key frame whenever frame sizes change.

    Receivers of the video stream would need to scale the decoded video frames
    to the resolution of the receiver display. Again, scaling from odd
    resolutions to standard display resolutions will produce poor-quality
    results. In addition, many receivers have scalers that are
    performance-optimized only when going between standard resolutions.

Therefore, to mitigate these issues, this design calls for using a fixed set of
capture resolutions that have the same aspect ratio as the source content. Also,
capture resolution changes are limited to occur no more than once every three
seconds. The step between nearby capture resolutions must be small enough that
the end-user won't notice the difference when a change is made. However, the
step must be large enough to prevent constant flip-flopping between two or more
resolutions. For example, when considering 16:9 source content at 1920x1080, a
good step difference is 90 lines.

Now that the inputs and outputs to the decision logic have been well-defined, a
strategy for changing resolutions must be described. At a high-level, the
strategy depends on whether the source content is interactive or animating. When
interactive, the goal is to maximize the resolution at the expense of dropping
frames. Therefore, capture resolution changes are allowed every three seconds to
reach the highest-supported resolution.

When source content is animating, a more-complex strategy must be employed since
the goal is to provide a smooth capture that avoids dropping frames. In this
case, the strategy is to allow aggressive and immediate decreases in capture
resolution in order to preserve frame rate. Then, if the feedback signals
indicate the end-to-end system has been consistently under-utilized for a long
(e.g., 30-second) proving period, allow a single step upward in capture
resolution. This strategy avoids rapid flip-flopping which would both: 1)
increase the risk of a frame being dropped while the end-to-end system
repeatedly adjusts to the new resolutions; and 2) possibly impact the end-user
experience, since many repeated resolution changes could become noticeable and
bothersome.

### Detecting animating content

Change events are provided by the Chromium compositor as each frame is provided
to it by the renderer. These change events represent candidate frames for
capture. They include both the local presentation timestamp and the damage
region within the frame. For each change event, a decision is made as to whether
to request capture of the frame from the GPU.

In order to determine whether the source content is animating, the recent
history of change events is analyzed using on-line heuristics. Not only is the
goal to detect animating content, but to detect the largest region of animating
content. The largest region is important because the user experience is
optimized when timing the capture of video frames to those changes. For example,
if the source content is a rendered web page that contains a tiny spinner
graphic animating at 60 FPS plus a large video playing at 24 FPS, then the goal
of the system is to capture at a rate of 24 FPS:

In order to accomplish this, the recent history of change events participate in
a voting process. Each change event contributes a vote for its damage region,
and that vote is weighted in proportion to the number of pixels in its damage
region. If there is a damage region that gets a supermajority of the votes
(i.e., 2/3 or more), it is selected as the overall detected animation region.
Then, the timestamps from all the change events having the same damage region
are used to determine the average animation rate.

There are a number of cases where animation is decidedly not detected. First,
there may be an insufficient history of change events. Second, the supermajority
vote might not be won by any of the candidate damage regions. Third, when a
damage region wins the vote, the change event timestamps must indicate a
sequence of regular time periods between events without significant pauses.

# Open Issues

Receiver feedback signals

Everything discussed in the design above for the most part focuses on
performance bottlenecks sender-side. It is worth exploring whether and how to
include receiver feedback signals into this design. One issue is that the
latency in the feedback would be much higher, at minimum a full network
round-trip time period, and so potential see-sawing effects would have to be
mitigated.

High frame rate content

The current design ignores throttling of frame rate. For the vast majority of
typical usage scenarios, this is fine. However, the exception would be high
frame rate content (e.g., 60 FPS). In this case, a second trade-off needs to be
researched: When should the source content be sub-sampled (e.g., 30 FPS) instead
of the capture resolution dropped? In other words, to optimize the user
experience, when is it better to drop the frame rate instead of the resolution?

Adaptive Latency

In Cast Streaming, the end-to-end system declares a fixed window of latency
between when a video frame would be presented sender-side and the exact moment
it is displayed receiver-side. The default is currently 400 ms, which means
capture, encoding, transmission to the receiver, decoding, and presentation are,
in-total, allowed to take up to this much time. The point of this is to allow
for any part of the pipeline to occasionally take much longer in processing a
frame (e.g., network packets needing re-transmission) without
interrupting/glitching the user experience.

There is research in-progress to identify whether and when it makes sense to
dynamically adjust the end-to-end latency throughout a session. For example, for
interactive content, it makes sense to drop the latency so that user input
actions on the sender side result in spontaneous responses receiver side.
Likewise, for animating content, it makes sense to increase the latency,
effectively adding more "buffering" to the end-to-end system, to help prevent
unpredictable events from causing forced frame drops. In addition, it may or may
not become important to account for these latency adjustments with respect to
the auto-throttling of capture.

Remoting over the WAN (WebRTC use cases)

Transmitting the video stream over a wide-area network, and/or broadcasting to
multiple receivers introduces additional variance and additional considerations
for how feedback utilization signals are generated and used.

Research and work on this issue is being tracked
[here](http://crbug.com/516190).

Compositor self-throttling

There is currently a known issue on lower-end systems: It is possible to achieve
a state where animated content is captured from the Chromium compositor,
encoded, and remoted without dropping any frames in the capture pipeline; but
the frame rate of the source content is actually much higher. Frames are being
dropped because the compositor is under too much load and is reacting by
down-throttling the rendering frame rate. The auto-throttling control logic has
no signals making it aware of this fact. If it were aware, and the tab capture
resolution were decreased enough, the load on the compositor would decrease to
the point where the rendering frame rate would raise again and match that of the
source content.

Research and work on this issue is being tracked
[here](http://crbug.com/517714).

# Status

Experiment launched in Chrome 45. Stable launched in Chrome 46.
