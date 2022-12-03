---
breadcrumbs:
- - /developers
  - For Developers
page_name: the-rendering-critical-path
title: The Rendering Critical Path
---

author: wiltzius@

[TOC]

### Rendering Architecture in Brief

To understand the symptoms and potential causes of jank, it's important to have
a basic understanding of the browser's rendering pipeline. This article will
briefly describe that pipeline assuming knowledge of the web platform but no
knowledge of how the rendering engine works, with links to relevant design docs
for more info.

This article is adapted from a similar section in [Using Frame Viewer to Bust
Jank](/developers/how-tos/trace-event-profiling-tool/using-frameviewer). It goes
into greater detail, but doesn't cover any examples. See that document for a
more illustrated version of this process, which annotates a trace of an example
style modification.

We'll focus on how the page is rendered *after* page load -- that is, how the
browser handles screen updates in response to user input and JavaScript changes
to the DOM. Rendering when loading a page is slightly different, as compromises
to visual completeness are often made in order to get some content to appear
faster; this is out of scope for this document which will focus on *interactive*
rendering performance.

Much more detail about the specifics of how rendering inside of Blink works
(i.e. the style, layout, and Blink paint systems), as well as a (somewhat dated)
discussion of how it might evolve, can be found in the [Rendering Pipeline
doc](https://docs.google.com/a/chromium.org/document/d/1wYNK2q_8vQuhVSWyUHZMVPGELzI0CYJ07gTPWP1V1us/edit#heading=h.6cdy1o585rsa).
This document is intended to be more descriptive, higher-level, and include the
related input and compositing systems.

### The Critical Rendering Path

After loading, the page changes in response to two inputs: user interaction
mediated by the user agent (default pressed button styles, scrolling,
pinch/zooming, etc) and updates to the DOM made by JavaScript. From there, a
cascade of effects through several rendering sub-systems eventually produces new
pixels on the user’s screen.

The main subsystems are:

    Input handling \[technically not related to graphics per se, but
    nevertheless critical for many interactions\]

    Parsing, which turns a chunk of HTML into DOM nodes

    Style, which resolves CSS onto the DOM

    Layout, which figures out where DOM elements end up relative to one another

    Paint setup, sometimes referred to as recording, which converts styled DOM
    elements into a display list (SkPicture) of drawing commands to paint later,
    on a per-layer basis

    Painting, which converts the SkPicture of drawing commands into pixels in a
    bitmap somewhere (either in system memory or on the GPU if using Ganesh)

    Compositing, which assembles all layers into a final screen image

    and, less obviously, the presentation infrastructure responsible for
    actually pushing a new frame to the OS (i.e. the browser UI and its
    compositor, the GPU process which actually communicates with the GL driver,
    etc).

As an example, updating the innerHTML of a DOM element that’s visible on-screen
will trigger work in every one of the systems above. In bad cases any one of
them can cost tens of milliseconds (or more), meaning every one of them can be
considered a liability for staying within frame budget -- that is, any one of
these stages can be responsible for dropping frames and causing jank.

### Browser Thread Architecture

The browser has multiple threads, and because of the dependencies implied in the
above pipeline which thread an operation runs on matters a lot when it comes to
identifying performance bottlenecks. Style, layout, and some paint setup
operations run on the renderer’s main thread, which is the same place that
JavaScript runs. Parsing of new HTML from the network gets in own thread (in
most cases), as does compositing and painting; however parsing of e.g. innerHTML
is currently performed synchronously on the main thread.

Keep in mind that JavaScript is single threaded and does not yield by default,
which means that JavaScript events, style, and layout operations will block one
another. That means if e.g. the style system is running, Javascript can’t
receive new events. More obviously, it also means that other JavaScript events
(such as a timer firing, or an XHR success callback, etc) will block JS events
that may be very timely -- like touch events. NB this essentially means
applications responding to touch input must consider the JavaScript main thread
the application’s UI thread, and not block it for more than a few milliseconds.

The ordering of these events is often unfortunate, since currently Blink has a
simple FIFO queue for all event types. The [Blink Scheduler
project](https://docs.google.com/a/chromium.org/document/d/11N2WTV3M0IkZ-kQlKWlBcwkOkKTCuLXGVNylK5E2zvc/edit)
is seeking to add a better notion of priorities to this event queue. The
scheduler will never be able to actually shorten non-yielding events, though, so
is only part of the solution.

It's worth noting here that scrolling is “special” in that it happens outside of
the main JavaScript context, and scroll positions are asynchronously reported
back to the main thread and updated in the DOM. This prevents janky scrolling
even if the page executes a lot of JavaScript, but it can get in the way if the
page is well-behaved and wants to respond to scrolling.

It's also worth noting that the pipeline above isn’t strictly hierarchically
ordered -- that is, sometimes you can update style without needing to re-lay-out
anything and without needing to repaint anything. A key example is CSS
transforms, which don’t alter layout and hence can skip directly from style
resolution (e.g. changing the transform offset) to compositing if the necessary
content is already painted. The below sections note which stages are optional.
Different types of screen updates require exercising different stages in the
pipeline. One way to keep animations inexpensive is to avoid stages of this
pipeline altogether. The following sections cover a few common examples.

Lastly, note that user interactions that the browser handles, rather than the
application, are largely unexceptional: clicking a button changes its style
(adding pseudo-classes) and those style changes need to be resolved and later
stages of the pipeline run (e.g. the depressed button state needs to be painted,
and then the page needs to be re-composited with that new painted content).

### Scrolling

Updating the viewport's scroll position has been designed to be as cheap as
possible, relying heavily on the compositing step and its associated GPU
machinery.

In the simple case, when the user tries to scroll a page events flow through the
browser as follows:

1.  Input events are delivered to the browser process's UI thread
2.  The browser's gesture detector processing the input stream decides
            that the user is attempting to scroll the page content
3.  Input events are sent directly from the browser UI thread to the
            compositor thread in the Renderer process (notably bypassing the
            Renderer's main thread, where JavaScript and Blink live)
    1.  Optionally, if there are touch event or mousewheel handlers
                registered on the part of the page being scrolled these events
                are added to Blink's event queue and the compositor will block
                further work on this frame until it receives an acknowledgment
                that those events have run and have not been preventDefault'd
4.  The compositor updates layer positions relative to the viewport
    1.  Optionally, if newly exposed parts of the page have not been
                painted in previous frames, the compositor paints them.
        1.  Optionally, if newly exposed parts of the page have not been
                    *recorded* in previous frames, the compositor asks Blink to
                    record new sections of DOM
5.  The compositor issues GL commands to the GPU process to recomposite
            painted textures on the GPU with the updated layer positions in step
            4
6.  The compositor asynchronously sends a message to the Renderer's main
            thread with updated scroll position information.
    1.  Optionally at this point updated JavaScript onScroll events can
                be fired by Blink

This flow skips a lot of detail, which is covered in the [Accelerated
Compositing design
doc](/developers/design-documents/gpu-accelerated-compositing-in-chrome) if
you're curious.

Important notes:

*   In the common case, only compositing and the GPU infrastructure are
            exercised in producing a new frame during scrolling; all other steps
            are skipped.
*   Registering touch event handlers (see step 3.1) add those JavaScript
            touch event handlers as a synchronous step on the critical scrolling
            path.
*   The compositor will almost always still produce frames even if its
            waiting on Blink for new content (e.g. for touch event handler
            acknowledgments in step 3.1, or for new recordings of DOM content in
            4.1.1). Instead of the content it's trying to produce, it'll put up
            either old content that's already been painted in previous frames
            *or* if there's no such content available it'll put up a
            checkerboard pattern.

### Modifying Style of Existing DOM

Modifying the style of existing DOM elements from JavaScript (whether in
response to input or not) will exercise most of the rendering pipeline, although
it'll skip parsing. Depending on the styles modified, it may also skip layout
and painting. Paul Irish and Paul Lewis have written a [good article on
html5rocks](http://www.html5rocks.com/en/tutorials/speed/high-performance-animations/)
covering which styles affect which pipeline stages.

Here's an outline of how modifying style propagates through the browser:

1.  JavaScript runs, modifying some style (setting inline style,
            modifying a class list, etc). This may happen in a
            requestAnimationFrame callback, in an input event, in a timer
            callback, or anywhere else.
2.  When the compositor next asks Blink to paint part of the document,
            Blink will optionally recalculate style on any part of the DOM that
            it may need to paint.
    *   This style recalculation is theoretically scoped to only the
                part of the DOM tree whose style is dirty, but in practice often
                includes the entire document (the Dev Tools timeline will show
                how many DOM nodes were operated on in each recalc style event).
3.  Optionally, if the style recalculation has dirtied the layout of any
            elements that may need to be painted, layout is run on the relevant
            DOM subtree(s).
    *   Notice the "may" in that sentence, since before layout is run
                and final element positions are known it isn't possible to tell
                what will be where. In practice, layouts often occur on the
                entire document. The Dev Tools timeline will show the root of
                the tree under operation for each layout event.
4.  Optionally, if any composited layer properties have changed (new
            composited layers, or new transforms or opacity of existing
            composited layers) the main-thread copy of the compositor layer tree
            is updated.
5.  Now whatever part of the document the compositor requested be
            painted can actually be painted, but note that this doesn't actually
            fill in pixels -- Blink here paints into a display list (currently
            an SkPicture).
    *   This is step 6 in the overview of main subsystems above, often
                called SkPicture recording.
6.  New SkPictures and the updated compositor layer tree are transferred
            from the main thread to the compositor thread (the so-called
            "commit")
7.  The compositor prioritizes all content tiles and decides if any
            tiles need to be re-rastered (i.e. actually painted) to put up this
            new content
8.  The compositor then paints these tiles
    1.  With software painting, this is done on a software raster worker
                thread
    2.  With Ganesh aka GPU rasterization, this is done on the
                compositor thread itself, converting the display list into GL
                commands
9.  The compositor then generates the GL commands to produce a new frame
            and hands this frame's information to its parent compositor in the
            browser
10. The browser compositor issues the GL commands for a new frame
            including the browser UI and the child compositor's content

Important notes:

*   It's possible to trigger style recalculation or layout outside of
            the rendering pipeline by reading back CSS properties that are
            lazily computed. In the worst case script can enter a write / read
            loop where it sets a style, reads a property that forces synchronous
            layout or style recalculation, and then repeats this process. We
            call this layout thrashing; Wilson Page has written a [good article
            on the
            subject](http://wilsonpage.co.uk/preventing-layout-thrashing/).

This flow summarizes a lot of detail, again covered in the [Accelerated
Compositing design
doc](/developers/design-documents/gpu-accelerated-compositing-in-chrome).

### Adding new DOM

Adding new DOM or replacing a DOM subtree requires parsing the new DOM. Whether
this parsing happens synchronously on the main thread or asynchronously
off-thread depends on how the DOM is inserted. An incomplete listing:

*   XHR responses are currently processed on the main thread;
            crbug.com/402261 tracks work to make it asynchronous
*   Setting the srcdoc of an iframe is asynchronous, see
            crbug.com/308321 for the work that made this possible
*   data: URLs are always asynchronously parsed;
            https://codereview.chromium.org/210253003 for the work that made
            this possible
*   During page load, most parsing is done asynchronously using the
            threaded parser

After parsing, rendering proceeds roughly as above in "modifying style of
existing DOM"
