---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/quarantined-pages
  - Quarantined pages
page_name: meeting-notes
title: Meeting Notes
---

[Meeting Notes from February 9, 2009](#02092009)

---

February 9, 2009

### Browser

> Mostly stability work this week, with several crashers and layout tests fixed.
> For a status report, see:
> <http://code.google.com/p/chromium/wiki/StabilizeTrunk?ts=1229629370&updated=StabilizeTrunk>

### Multiplatform

*   Mac Update
    *   Creating and destroying tabs (renderer process is getting
                created/destroyed)
    *   shutting down is working properly
    *   plugins are loading things (callbacks are working correctly) but
                not displaying
    *   close to having visible bits on the screen, jrg is working on
                telling the renderer to draw
    *   bits are traveling across the wire but it hasn't been tested so
                we'll know once the renderer is drawing
*   Linux Update
    *   Displays a window and a toolbar
    *   Pieces are in place for bits on the wire
    *   Displays flash without glitches (woo dean!) in window'ed mode

### Infrastructure

> Not much work on the new network stack, one crasher fixed and one security bug
> under investigation. Wan-teh is also starting work on supporting NTLM auth. On
> the stability front, the build is more stable this week in terms of renderer
> crashes (200 different stack traces at the beginning of last week, but only 45
> stack traces on Friday). In the build area, maruel disabled incremental
> linking for 32-bit OSes (Vista 64 users not affected) because things grew too
> large. WebCore is moving into its own DLL, should help. Rahul and Pam are
> Sheriff and Deputy this week, be nice to them by keeping the tree green.

### Webkit

*   Darin, Pam and Brett are doing the merge this week.

    *   Daily merge calendar:
                <http://www.google.com/calendar/hosted/google.com/embed?src=google.com_h1kjbmjo6p29ta0rluu8eh5qjo%40group.calendar.google.com&ctz=America/Los_Angeles>

*   Continuing to unfork. About 64 files forked, and starting to
            upstream V8 bindings, which account for a large number.
*   Stability push: doing very well on layout tests, but accumulation of
            crashers is bringing stability of dev channel down. If you can fix a
            crasher, please help!

### Extensions

*   Aaron finished URLPattern integration for user scripts, now working
            on giving them a separate JS context.
*   Erik is making progress on loading extensions once they've been
            installed.
*   Matt is working on the new extension process model:
            <http://www.chromium.org/developers/design-documents/extensions/process-model>
