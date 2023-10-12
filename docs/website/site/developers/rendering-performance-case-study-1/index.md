---
breadcrumbs:
- - /developers
  - For Developers
page_name: rendering-performance-case-study-1
title: 'Jank Case Study 1: theverge.com'
---

## wiltzius@

### Background: Reading the Tracing Tea Leaves

Most websites on mobile devices are painfully janky. The problems are obvious
and everywhere, but it can be challenging to isolate janky behavior and
understand its cause. This sort of performance investigation is not a very
widely known practice, and often involves advanced tools such as Chrome’s
[tracing](http://www.chromium.org/developers/how-tos/trace-event-profiling-tool).
This article seeks to provide an example of jank identification and triage.

There’s very little explaining the “why” of anything here, just examples of jank
(including tools used to identify it and any conclusions). Therefore it’s highly
recommended to start by reading the [how to bust jank with frame
viewer](/developers/how-tos/trace-event-profiling-tool/using-frameviewer),
[anatomy of
jank](/developers/how-tos/trace-event-profiling-tool/anatomy-of-jank) and [the
rendering critical path](/developers/the-rendering-critical-path) articles
before reading on; otherwise this is unlikely to prove helpful.

The Verge [recently
launched](http://www.theverge.com/2014/9/2/6096609/welcome-to-verge-2-0) a
responsive mobile website. It’s a nice success story for the mobile web.
Unfortunately in Chrome for Android the UX is a little lacking, so let’s dig
into it as an example.

### Investigation

Let’s start with [www.theverge.com](http://www.theverge.com), the home page.

Symptom: Frequent checkerboarding on scroll

Scroll halfway down the page. Then, with the finger held down, scroll up and
down in a small area. These small movements back and forth should be simple
composited scrolls that leaves the page content all in texture memory, since the
area being scrolled is small. But unfortunately we can see visible
checkerboarding during these movements (the patches of the screen being slowly
filled in).

There are a couple possible explanations for this: either (a) something’s being
invalidated during the scroll and requiring re-rasterization, or (b) Chrome is
out of VRAM (or thinks it’s out of VRAM) and is discarding the tiles in texture
memory as soon as they’re off-screen.

To dig into why this is happening, we can start with the Dev Tools timeline.
Here’s a timeline recording of that quick movement back and forth:

[<img alt="Movement"
src="/developers/rendering-performance-case-study-1/01-movement.png">](/developers/rendering-performance-case-study-1/01-movement.png)

You can see the (very, very long) painting operations as content is
re-rasterized, but we don’t have any insight into why this content needs to be
painted. There’s a lot of JavaScript, but aren’t any style recalculations or
layouts in the vicinity of the painting operations. We don’t have any leads on
what’s causing the paint.

Where the Dev Tools timeline fails to tell us anything, we can turn to tracing’s
Frame Viewer. Here’s a frame viewer trace of the same movement:

[<img alt="Frame Viewer"
src="/developers/rendering-performance-case-study-1/02-frameviewr.png">](/developers/rendering-performance-case-study-1/02-frameviewr.png)

This is zoomed in on a part of the timeline where the scroll is happening
(higher up in the tracing timeline you can see
“InputLatency:GestureScrollUpdate” events). Note, significantly, that there
appear to be no invalidations! This is easier to see if you disable “contents”
in the layer tree viewer pane and zoom way out:

[<img alt="Frame Viewer"
src="/developers/rendering-performance-case-study-1/03-zoom.png">](/developers/rendering-performance-case-study-1/03-zoom.png)

There aren’t any invalidation rects. The lack of invalidation rects indicates
that Chrome itself is throwing out content tiles for some reason. This will
happen on scroll, but it shouldn’t happen so soon after the tiles are off-screen
on small scroll in a localized area.

To get visibility into what the tile manager is doing, we can turn on coverage
rects in the frame viewer. In the top dropdowns enable “coverage rects” and for
the tile heatmap select “Is using GPU memory”

[<img alt="Rects"
src="/developers/rendering-performance-case-study-1/04-rects.png">](/developers/rendering-performance-case-study-1/04-rects.png)

Now we can see what content tiles are actually resident in GPU memory on any
given frame. Stepping through the frames of the scroll, you can see that there’s
a horizontal layer that goes from having it’s content resident in memory (all
content tiles are dark green) to losing it (all a content tiles become white
with red outlines).

#### YouTube Video

This may simply be the reality of memory constraints, but it also might be a
legitimate bug in tile prioritization. Particularly, it’s suspicious that there
are tiles much further away from the viewport that are still resident in memory
-- this indicates suboptimal resource management.

Conclusion: file a Chrome bug with these findings,
[crbug.com/412436](https://code.google.com/p/chromium/issues/detail?id=412436)

Symptom: Tap delay on menu button

[<img alt="Tap Delay"
src="/developers/rendering-performance-case-study-1/05-tapdelay.png">](/developers/rendering-performance-case-study-1/05-tapdelay.png)

The cause of the jank is immediately apparent: note the huge paint block after
the input events are processed.

(Also note the mouse input events, even on a phone, because mouse events are by
default emulated; the page could skip this work by not listening for them if
there isn’t a mouse.)

Turning on “compositor layer borders” under the Dev Tools “rendering” menu might
make it clear why this invalidation happens.

[<img alt="Invalidation"
src="/developers/rendering-performance-case-study-1/06-invalidation.png">](/developers/rendering-performance-case-study-1/06-invalidation.png)

Unfortunately it’s hard to see what’s happening, but because there are layer
borders around the menu we can guess that the page is trying to smartly use 3D
CSS to position the menu. A common problem here is that the final layers only
get created by JavaScript in response to the menu tap, causing a big
invalidation when new layers are created. The new layer can’t move for the
animation until it’s done rasterizing, which takes ages.

We can confirm this hypothesis with Frame Viewer. Here’s the layer tree just
before the mouse tap:

[<img alt="Hypothetsis"
src="/developers/rendering-performance-case-study-1/07-hypothetsis.png">](/developers/rendering-performance-case-study-1/07-hypothetsis.png)

And here’s just after:

[<img alt="image"
src="/developers/rendering-performance-case-study-1/08-after.png">](/developers/rendering-performance-case-study-1/08-after.png)

It’s very subtle, but you can see that the layer tree is actually different --
layers 73 and 74 have been replaced by layer 79, at the bottom of the list.

Enabling the invalidation rects, we can see the large invalidation corresponding
to that long paint from we saw in Dev Tools also appearing in the frame where
the layer tree changes:

[<img alt="Layer Tree"
src="/developers/rendering-performance-case-study-1/09-layertree.png">](/developers/rendering-performance-case-study-1/09-layertree.png)

Conclusions:

There are essentially two problems here. One is that the invalidation is
happening at all, which is under the control of the site (avoiding invalidation
is tricky but possible, in this case, by having the right layers already set up
before the animation begins).

The other problem is that the rasterization takes such a comically long time.
This is also somewhat under page control, since they could simplify the CSS, but
Chrome should really render faster. This isn’t even worth a Chrome bug, though,
because we know the answer here is to use Ganesh for faster rasterization. This
page is still blocklisted for now, but if we force it on in about:flags we can
see the paint time drop dramatically:

[<img alt="image"
src="/developers/rendering-performance-case-study-1/10-dramatically.png">](/developers/rendering-performance-case-study-1/10-dramatically.png)

Symptom: Grabbing and sliding menu frequently becomes unresponsive

When the menu is out, grabbing the side of it allows the user to slide it in and
out. Frustratingly the menu frequently becomes stuck, though, and unresponsive.

First investigation is just a Dev Tools timeline recording of the interaction:

[<img alt="image"
src="/developers/rendering-performance-case-study-1/11-investigation.png">](/developers/rendering-performance-case-study-1/11-investigation.png)

The timeline recording shows that the system is actually running fine (i.e. is
responsive and not bottlenecked). Touch moves are getting handled consistently
by the browser.

Conclusion: This is actually an application bug where touches outside of the
drawer area mid-touch-stream don’t update the drawer position.

Symptom: Delay before releasing menu to let it animate back in

After sliding the menu around, releasing it causes a hitch before the drawer
animates back to a hidden state.

As usual, start with the Dev Tools timeline:

[<img alt="image"
src="/developers/rendering-performance-case-study-1/12-asusual.png">](/developers/rendering-performance-case-study-1/12-asusual.png)

Note that whole-document layout right after the touchend event. It isn’t
possible to say anything conclusive from just the timeline, but it’s a good
guess that this is the root cause of the problem since the layout will probably
invalidate the whole document.

One step before that layout is the recalc style causing it:

[<img alt="image"
src="/developers/rendering-performance-case-study-1/13-recalc.png">](/developers/rendering-performance-case-study-1/13-recalc.png)

A class being removed (via jQuery). The JS is minified so we can’t tell much
more meaningfully from the stack trace or the JS itself, but if we had
unminified sources it would become obvious which line of JS was changing CSS and
causing the invalidation.

Note that we can tell that the whole document was invalidated, rather than just
part of it, because the subsequent paint events cover every part of the screen.
Expanding the paint, we see events for tiles on the main document as well as all
other layers:

[<img alt="image"
src="/developers/rendering-performance-case-study-1/14-otherlayers.png">](/developers/rendering-performance-case-study-1/14-otherlayers.png)

If we look at this with frame viewer, we can see different layer trees before:

[<img alt="image"
src="/developers/rendering-performance-case-study-1/15-difflayertree.png">](/developers/rendering-performance-case-study-1/15-difflayertree.png)

And after:

[<img alt="image"
src="/developers/rendering-performance-case-study-1/16-after.png">](/developers/rendering-performance-case-study-1/16-after.png)

Note the additional layers 82 and 83 at the bottom, plus a bunch of the layers
in the middle were reparented.

Conclusion: This is a double-sided issue just like the jank on menu tap: on the
one hand it’s an application bug, in the sense that it could technically be
worked around, but on the other hand Chrome could be made smarter not to
invalidate these layers (especially those that are just reparented). Enabling
Ganesh on this page will also make those invalidations much less painful.
Speculatively file [crbug.com/412551](http://crbug.com/412551) for the
reparenting-of-existing-layers question.

Symptom: Slow, unresponsive comment display on articles

Moving past the home page, let’s look at one of the articles. Randomly picking
this one:

<http://www.theverge.com/2014/9/3/6101831/a-closer-look-at-sonys-sleek-new-xperia-z3-and-z3-tablet-compact#comments>

That link is to an anchor at the comments section. After the page is finished
fully loading, tap “read them” to see comments.

Try scrolling or doing anything right after tapping “read them.” Note the page
is completely non-responsive during the comment load.

Using the Dev Tools timeline trace, you can see that this is all load-related
jank:

[<img alt="image"
src="/developers/rendering-performance-case-study-1/17-loadrelatedjank.png">](/developers/rendering-performance-case-study-1/17-loadrelatedjank.png)

The very long JavaScript block is dominated by HTML parsing, if we expand it:

[<img alt="image"
src="/developers/rendering-performance-case-study-1/18-verylong.png">](/developers/rendering-performance-case-study-1/18-verylong.png)

The app could theoretically improve this by changing the way it populated the
DOM. Looking at the call stack for these it’s largely performed by their
framework’s templating (backbone.js), though, so it probably won’t be easy to
change:

[<img alt="image"
src="/developers/rendering-performance-case-study-1/19-populated.png">](/developers/rendering-performance-case-study-1/19-populated.png)

Unfortunately because all of this loading is done by script there’s also no
obvious way for Blink to yield during these many calls, although this might make
for an interesting discussion; the ambitious might consider filing a bug here.

More obviously problematic is the huge style recalculation block that comes
afterward, which weighs in around half a second. This is worth a bug so at least
the pattern is understood: [crbug.com/412558](http://crbug.com/412558).

## Conclusion

When investigating a janky site there are almost a handful of ways Chrome could
be behaving better and a handful of things the site could be doing better.
Staring hard enough at nearly any page usually yields a plethora of both; jank
is all over the place on the web (particularly the mobile web).

Filing good bugs usually means making sure to very narrowly and reproducibly
specify the interaction you want to improve, and then attaching traces of that
interaction. Getting the bug looked at usually means getting in front of the
right people’s eyes and labelling it correctly, but there aren’t any good rules
for that.

Providing useful advice to website developers usually means trying to
specifically isolate the bit of JavaScript that’s behaving suboptimally (e.g.
causing invalidations or expensive style recalculation) and then pointing the
developer at it. Unfortunately it’s often hard to work around, but the generic
advice for web devs is to structure their code in a way that just does less work
-- if you’re careful about setting up the structure of your page and then
modifying as little as possible, updates can often be made surprisingly cheap.

That’s all for this example. Happy hunting!
