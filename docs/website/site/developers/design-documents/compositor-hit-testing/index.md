---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: compositor-hit-testing
title: Compositor (Touch) Hit Testing
---

@leviw, @yusufo, @rbyers

# Implementation status:

# Implemented: [crbug.com/135818](http://crbug.com/135818), overhauled: [crbug.com/](http://crbug.com/248522)[248522](http://crbug.com/248522), [crbug.com/261307](http://crbug.com/261307)

**NOW Obsolete** - replaced by much more elegant [RenderingNG hit
testing](https://docs.google.com/document/d/1WZKlOSUK4XI0Le0fgCsyUTVw0dTwutZXGWwzlHXewiU/edit#heading=h.pzukwhsoocqx)

## Background & Problem Statement

User events such as a mouse click are received by the browser process, then
marshaled to blink where the click event is hit tested through a page's DOM,
checking for event handlers along the way. On touch-capable devices, a finger
drag can be used to scroll the page, but a Touch event handler on the page may
also optionally override this default behavior by calling
[preventDefault](https://developer.mozilla.org/en-US/docs/DOM/event.preventDefault).
Because there is no way to determine programmatically if an event handler will
prevent this default scrolling behavior, if a Touch event occurs where there's
an event handler, we have to first marshal the event to blink to run through its
event handler. The blink thread is often slow to respond, particularly during
page load, which can result in very long delays between a Touch intended to
scroll the page, and the scroll actually occurring.

The original solution for this problem was for blink to inform the embedding
application of when a page had at least one Touch event handler registered. If
there were no Touch event handlers registered, we wouldn't send the events to
blink, and would scroll immediately. This document describes a more flexible
solution where regions of the page where Touch event handlers are active are
used by the compositor to avoid waiting for blink hit testing for as much of a
page as is possible.

### Project goal

Remove latency for touch scrolling wherever possible without changing any
behavior for pages that use touch event handlers. In particular we want to allow
scroll to start quickly during page load on pages that have some touch handlers
but are not covered by them.

### Tracking hit test rects in blink

In blink, we hook into the creation of Touch event handlers and track
EventTargets with handlers in EventHandlerRegistry. During paint,
HitTestDisplayItems are emitted for all objects with blocking event handlers. As
an optimization, a cache of the HitTestDisplay item data is stored on PaintChunk
for all display items in the chunk. Then, after compositing, all hit test rects
for a cc::Layer are projected into the cc::Layer's coordinate space using
PaintArtifactCompositor::UpdateTouchActionRects. This approach of painting hit
test data is described in more detail in
[PaintTouchActionRects](https://docs.google.com/document/d/1ksiqEPkDeDuI_l5HvWlq1MfzFyDxSnsNB8YXIaXa3sE/view#).

### Hit testing in the compositor

The hit testing is currently done just for the touchStart events since the point
at which these event hit determines where the next train of events will be sent
until we receive another touchStart (due to a different gesture starting or due
to another finger being pressed on screen). On the compositor, (as of the fix
for [bug
](goog_353685820)[351723](https://code.google.com/p/chromium/issues/detail?id=351723))
we do a ray cast at the point of the touch and consult the
touchEventHandlerRegion for each layer until we hit a layer we know is opaque to
hit testing. If there is a hit, the compositor forwards this touch event to the
renderer and then it is sent to blink to be processed as usual. If there is no
touchEventHandlerRegion that was hit, the compositor sends an ACK with
NO_CONSUMER_EXISTS.

### Browser side processing

As far as the browser side is concerned, only the ACKs it receives for the
outgoing touch events matter in determining the current state. Currently there
are four states that the ACK can be at. INPUT_EVENT_STATE_ACK_UNKNOWN is the
initial default state that the touch_event_queue is at and might not be used on
different platforms(ex: Android). When a touchStart event comes the touch event
queue on the browser side always sends this touch event through IPC to the
compositor. Then the touch event queue waits for the ACK for that touchStart to
make a decision about the rest of the touch events in queue.

If it receives NO_CONSUMER_EXISTS, it stops sending touch events to the
compositor until the next touchStart arrives and sends them directly to the
platform specific gesture detector. This is mostly the case for regular browsing
helps the gesture detector take over after a single touch event gets ACKed back
from the compositor making it possible for the gesture to be generated fast
enough to not cause any visible lag.

If it receives either NOT_CONSUMED or CONSUMED, this means there was a hit in
the touchEventHandlerRegion and we should continue sending the touchMoves and
touchEnd following this event to the compositor (which will send them to the
renderer without doing any hit testing). If the ACK was CONSUMED, then the
touchEventHandler had called preventDefault and neither this particular touch
event nor the rest of the touch events until the next touchStart should be sent
to the gesture detector. If the ACK was NOT_CONSUMED, this might mean either the
touchEventHandlerRegion was too conservative and when the touchStart was hit
tested in blink it didn't hit any touchEventHandlers or the touchEventHandler
didn't preventDefault or process that particular touch event. In this case the
touch_event_queue still forwards this event to the gesture_detector.
