---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/trace-event-profiling-tool
  - The Trace Event Profiling Tool (about:tracing)
page_name: anatomy-of-jank
title: Anatomy of Jank
---

author: wiltzius@

## Intro

Rendering performance is a huge topic of great interest to the Chrome project
and web developers. As the web platform is essentially an application
development framework, ensuring it has the capability to handle input and put
new pixels on the screen at a speed that keeps up with the display’s refresh
rate is critical.

“Performance” is an oft-cited problem area for web developers. We spend a lot of
time investigating and (hopefully) improving performance of the platform, often
guided by specific bad examples. This is the pursuit and elimination of jank.

But what does jank mean? What are people complaining about when they say a web
page “feels slow”? This document attempts to categorize the various symptoms
that all get put under the “jank” category, for the purposes of accurate
identification and precise discussion.

To understand the descriptions of potential causes of jank, it's important to
understand roughly how rendering works. See [The Rendering Critical
Path](/developers/the-rendering-critical-path) for a primer; these two documents
should be read together.

Finally, a note on the videos of examples -- video playback performance can
tragically mask subtle performance problems as it frequently isn't rendered at
60Hz. The examples here should be obvious enough that the problem is still
visible, but the originals of these videos are attached to the page, which you
can download for slightly more accurate recordings.

## Symptom Taxonomy

[TOC]

### Incomplete content

##### Checkerboarding

#### YouTube Video

<pre><code><i>Brief:</i> Parts of the page not showing up, particularly during a fast scroll or animation, and instead patches of either a gray checkerboard pattern or the background of the page showing through.
</code></pre>

Some background, for the curious. The checkerboarding problem is a very
fundamental one for a document viewer: do you preserve responsiveness to user
input or do you only show fully correct content? Chrome has developed a lot of
(expensive) machinery to combat this problem, preferencing responsiveness.
Checkerboarding occurs mainly during fast scrolling, when the page cannot be
rasterized quickly enough to put up a new viewport’s worth of content every
frame. When this happens Chrome will continue to scroll in an attempt to
preserve the physical metaphor of page under the user’s finger... but in place
of the missing content there will just be a blank space. On desktop platforms
Chrome will draw a greyish checkerboard pattern; on Android (where this is more
common) it draws the background color of the page.

To try to avoid having to checkerboard Chrome will pre-rasterize content around
the viewport and keep it as bitmaps in memory. This policy trades CPU and memory
resource consumption for a greater likelihood that the page content will be
ready if/when the user starts scrolling. It’s essentially a giant buffer to
guard against how slow software rasterization can be.

This approach has several limitations. For one, it’s a bit of a resource hog. It
also isn’t under the control of the app developer at all, so if for instance
they would prefer to jank (not produce a frame) instead of checkerboard when the
scroll hits an unrasterized region of the page, they’re out of luck. It’s also
ineffective if parts of the page that are pre-rasterized get invalidated by
JavaScript -- now that pre-rasterized content is stale. It’s still useful to
have around, since at least Chrome can display the pre-rasterized content until
the new content is ready (this is the whole pending tree vs. active tree
architecture; see the compositing design doc for an explanation), but it isn’t a
replacement for fast rasterization. This is one of the (many) reasons motivating
the move to GL-based rasterization (Ganesh), and when using Ganesh Chrome
pre-paints only a very small buffer around the screen.

> ###### Checkerboarding special case: low resolution tiles

> #### YouTube Video

> `*Brief:* Parts of the page displaying, but displaying at low resolution
> instead of high resolution.`

> This is a “feature” of impl-side painting, designed mainly to gracefully
> handle pinch/zoom scenarios. Chrome will quickly rasterize large sections of
> the pending tree (i.e. the content that’s queued up to go on screen) at low
> resolution first, with the aim of at least showing something. The cause here
> is identical to the cause of checkerboarding; this is essentially an
> intermediate state between having the content fully rasterized and not
> rasterized at all (checkerboard).

> ###### Checkerboarding common cause: URL bar jank

> `*Brief:* Full-screen invalidations caused by the URL bar’s showing / hiding
> on scroll.`

> There are times when Chrome doesn’t have a chance to rasterize enough content
> before a scroll gets to a region of the page (e.g. on very long pages), but
> for the most part impl-side painting handles static content well. As mentioned
> in the checkerboarding section, however, it copes poorly when the page is
> frequently invalidated by JavaScript. The resize event fired when the URL bar
> appears and disappears (per the current Chrome for Android omnibox UX) is a
> tragically common, totally unnecessary cause of invalidation on many mobile
> websites. Chrome itself has to do a re-layout to account for the additional
> pixels that just got added or subtracted from the total document height, but
> this is an optimized layout pass that shouldn’t (theoretically) cause
> full-document invalidations. Unfortunately many pages listen for the resize
> event, assume getting it means the page has actually changed dimensions
> dramatically as in the case of e.g. a rotation, and then modify their entire
> document’s style in response. This typically ends up looking exactly the same,
> but invalides everything as a result.

> ##### *Partial content during network load*

> `*Brief:* When loading a web page from the network Chrome will attempt to
> render the page as quickly as possible, even if it doesn’t have all the
> required resources yet. This is a long-standing feature of the platform to let
> you read content as quickly as possible, and not technically jank, but is
> included for completeness.`

> Unfortunately, it also looks significantly less polished than the style of
> many native applications, which show nothing (or a splash screen) until a
> basic UI shell is ready, and fill that shell in with data atomically when the
> data is ready. It’s possible to emulate this behavior on the web platform, and
> it looks much more polished to do so, but that’s unfortunately not a very
> common practice at this point.

### Framerate

##### Low sustained framerate during any kind of animation (“janky animations”)

#### YouTube Video

<pre><code><i>Brief:</i> animation with a mostly-steady framerate that’s below 60Hz.
</code></pre>

There’s typically a consistent bottleneck in the system when this is the case,
and about:tracing is designed for diagnosing exactly this case. If encountered,
a standard trace (no need for Frame Viewer) of the low-framerate animation
should show what operations are the long pole for the animation.

It’s worth noting that there are two types of animation: those handled entirely
by the compositor thread (so-called “accelerated” animations) and those that
need to synchronously call into Blink or v8. The only type of accelerated
animations are CSS animations (and CSS transitions and Web Animation) on
opacity, transform, and certain CSS filters; plus scrolling and pinch/zoom.
Everything else goes through Blink. Accelerated animations will never be
bottlenecked on anything in the RendererMain thread, but non-accelerated
animations can be.

##### Delayed beginning to any animation

#### YouTube Video

<pre><code><i>Brief:</i> a discernible pause between when an animation is meant to begin (e.g. in response to a finger tap) and when it actually begins. This is distinct from low framerate since sometimes the animation itself is fine, there’s just a pause before it starts.
</code></pre>

This is commonly caused by accelerated animations that are set up but require
rasterization before beginning. E.g. creating a new layer and setting a CSS
animation on its transform -- the animation will not start until the new layer
is done being painted. The timeline delay here is designed to prevent animations
beginning immediately only to drop a number of frames. The result is typically a
much smoother-looking interaction overall, but it can be surprising.

##### Low framerate during to swipe input (throughput)

<pre><code><i>Brief:</i> special case of low-framerate animation, specifically when the animation is moving something in response to a touch movement. All touch-event-driven animations are non-accelerated animations.
</code></pre>

Unlike the delay at the beginning of an animation, which can be stretched out
tens of milliseconds without the user really noticing, the only acceptable frame
time for a running animation is the vsync interval (e.g. 16ms). Touch input can
only be programmatically handled by JavaScript on the RendererMain thread, which
means that all visual touch-input-based effects are technically non-accelerated
animations. Note that they can still avoid painting by using an accelerated
animation property, but they’ll subject to jank from all other activity on the
main thread because the JavaScript input events can be blocked.

##### Isolated long pauses (“jank”) during JS-driven animations (incl touch)

<pre><code><i>Brief:</i> Single long pauses, rather than a consistent low framerate, during animations
</code></pre>

Sometimes an animation is mostly fine, but stutters at one point. This is often
harder to isolate, but about:tracing and the Dev Tools timeline are invaluable
for figuring out what went wrong. Special cases of this include the delay at the
beginning of animations. All non-accelerated animations are subject to jank of
this type from any other activity on the Blink main thread (e.g. JavaScript
timers executing).

### Latency

Long input latency (rather than low framerate)

<pre><code><i>Brief:</i> Most jank is related to interruptions in frame production, but latency represents another class of problem manifesting as longer delays between input events entering the system and corresponding new frames being output. It’s possible to have a good / high framerate but bad / long latency
</code></pre>

Some examples of high latency are covered in the framerate examples, for
instance delays at the beginning of an animation that’s kicked off in response
to input. Generally if the entire pipeline takes long enough that it doesn’t fit
into frame budget, we categorize it as a framerate rather than a latency issue.
However, there are some edge cases where latency can be increased by seemingly
unrelated issues.

##### High scrolling latency or delays beginning scrolling when the document has touch event handlers

#### YouTube Video

<pre><code><i>Brief:</i> Calling preventDefault on a touch move event is spec’d to prevent scrolling from happening. Chrome therefore tries to give JavaScript a chance to run any registered touch move event handlers before scrolling the page. If JavaScript takes a while to respond, however, it can increase input latency during the scroll.
</code></pre>

There’s a delicate balance between honoring the contract with touch event
handlers the page has registered and staying responsive. In particular, becuase
JavaScript touch event handlers are stuck on the main thread with everything
else, completely unrelated activity such as XHR parsing or style recalculation
or JavaScript timers that run on the main thread will all block touch event
handlers from running (none of these tasks currently yield, and as of this
writing Blink’s entire event loop is a FIFO queue with no prioritization,
although that’s changing with the advent of the Blink scheduler). The result is
that if style recalculation runs for 100ms right when the user is dragging the
page around, the scroll won’t move for 100ms until style recalc is finished, the
touch event gets run and preventDefault doesn’t get called during it.

Note that browser behavior here is wildly under-specified. Chrome’s behavior has
evolved over time, and changed significantly recently with the advent of
[asynchronous touch move
processing](http://updates.html5rocks.com/2014/05/A-More-Compatible-Smoother-Touch).
One of the more egregious behavior hacks here is that Chrome typically gave
touch events a ~200ms deadline to run, and if the event wasn’t processed by then
it would get dropped and the page would scroll anyway. This was designed to
preserve basic responsiveness even in the face of adversarial content.

##### Scroll-linked effects out of sync

<pre><code><i>Brief:</i> visual effects linked to the scroll position can become 1-2 frames out of sync from the actual scroll
</code></pre>

This is somewhat complicated, but boils down to two different modes of operation
in Chrome’s multi-stage rendering pipeline. In low-latency mode each stage of
the pipeline will complete fast enough that all of the stages complete within
16ms. If any of the stages run long, Chrome will fall out of low latency mode
into high latency mode. At that point there will be an extra frame of latency in
the entire rendering pipeline, which effectively increases frame budget by
allowing thread parallelism but at the cost of latency.

Note that technically there is currently a low/high latency mode for both the
main thread &gt; compositor thread step and the compositor thread &gt; browser
UI thread step. The problem described here is specific to the main thread &gt;
compositor thread step.

The synchronization issue is that scrolling is a compositor-thread operation
whereas the scroll-linked effects are necessarily main thread operations. This
means that in high latency mode the main thread may be 1 frame behind the
compositor thread, which means that the scroll position that the compositor is
using to position the page and the scroll position Blink/JavaScript is currently
aware of will be 1 step out of sync. Other than keeping the page in low latency
mode by never blowing frame budget, there’s currently nothing a page can do
about this.

It’s also worth noting, though, that input latency is rarely a visible problem
outside of these scroll-linked effects (like pull to refresh, or poorly
implemented parallax, etc). The platform’s biggest problem remains that it’s
incredibly difficult to maintain a high framerate.
