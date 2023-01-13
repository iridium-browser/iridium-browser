---
breadcrumbs:
- - /teams
  - Teams
page_name: input-dev
title: Input Team
---

*The Chromium Input team (aka input-dev)* is a web platform team focused on
making touch (P1) and other forms of input (P2) awesome on the web. High-level
goals:

*   Measure & improve input-to-paint latency. Web pages should stick to
            your finger!
*   Reduce web-developer pain points around input and scrolling,
            including improving interoperability between browser engines,
            predictability, simplicity and consistency between chromium
            platforms.
*   Enable high-performance rich touch interactions (such as
            pull-to-refresh) on the web.
*   Ensure Chrome UI features driven by input over the contents area
            provide a great experience to both end-users and web developers.
*   Enable the productivity of any chromium developer working with
            input-related code through effective code ownership and code health
            initiatives.

See also our concrete [quarterly objectives](/teams/input-dev/input-objectives).

**Contacts**

[input-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/input-dev)
is our mailing list, and we also use the #input-dev channel on [Chromium
Slack](https://chromium.slack.com).

**Useful bug queries**

*   [Blink&gt;input,
            Blink&gt;scroll](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3ABlink%3EInput%2CBlink%3EScroll&sort=-id+-modified+-opened&colspec=ID+Pri+Summary+Modified+Opened&x=m&y=releaseblock&cells=ids)
            - all (or most) bugs we're tracking
*   We triage the bugs almost daily with [this
            process](https://docs.google.com/document/d/1IDjoUbe8_1lbhM10EVHwvMi0g9EUXImy0ufxtfvJqe0/edit).
*   [Available
            bugs](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3ABlink%3EInput%2CBlink%3EScroll+status%3AAvailable+&sort=-id+-modified+-opened&colspec=ID+Pri+Summary+Modified+Opened&x=m&y=releaseblock&cells=ids)
*   [Code cleanup
            ideas](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3ABlink%3EInput%2CBlink%3EScroll+Hotlist%3DCodeHealth+&sort=-id+-modified+-opened&colspec=ID+Pri+Summary+Modified+Opened&x=m&y=releaseblock&cells=ids)

**Related specifications & Working Groups**

If you want to get involved with any of the specifications or just get all the
updates make sure to join their public mailing lists as well.

*   [W3C UI Events](https://w3c.github.io/uievents/)
*   [W3C Pointer Events](https://w3c.github.io/pointerevents/)
*   [W3C Pointer Lock](https://w3c.github.io/pointerlock/)
*   [W3C Touch Events](https://w3c.github.io/touch-events/)
*   [W3C CSS](https://drafts.csswg.org/)
*   [WHATWG DOM](https://dom.spec.whatwg.org/)
*   [WHATWG HTML](https://html.spec.whatwg.org/)

**Presentations & Talks**

*   [Overscroll
            Customization](https://www.youtube.com/watch?v=sp1R-0dd7qg&t=18m54s)
            lightning talk
*   [Scroll into
            text](https://www.youtube.com/watch?v=sp1R-0dd7qg&t=15m50s)
            lightning talk
*   [Event Delegation to
            Workers](https://docs.google.com/presentation/d/1BCEbLCg-o_Ko65byel5QGnO7Cwf5aPZPjqnnMNbbA5E/edit#slide=id.g5636de1525_0_0)
*   [Fling On
            Browser](https://docs.google.com/presentation/d/1miFAvKuz7tRT4IX_nCwZshFkUDX8bDYqfPlTOm2LS8U/)
*   [Scroll & Input
            Prediction](https://docs.google.com/presentation/d/18Dv2KBJxHnNezrTCzz29IyczhJwSIs8kTzwl2FuvGq0/edit#slide=id.p)
*   [WPT
            Automation](https://drive.google.com/file/d/1eBa7OvO6O9UefK-AYkclFz46ArGZ5iyH/view)
*   [Browserside User
            Activation](https://docs.google.com/presentation/d/1sEBwQZJ8w47OC7m2LexiJor8nUWPhqDaXqP5aWhYK60/)
*   [Root
            Scroller](https://docs.google.com/presentation/d/1nfM4jjQt9pijIThK8KSwavdZtWgifDjo6odHNg-ThvM/)
*   Extensible Scrolling - BlinkOn 3 (Nov 2014):
            [slides](https://docs.google.com/a/chromium.org/presentation/d/1P5LYe-jqC0mSFJoBDz8gfJZMDwj6aGeFYLx_AD6LHVU/edit#slide=id.p),
            [video](https://www.youtube.com/watch?v=L8aTuoQWI8A)
*   [Smooth to the touch: chromium's challenges in input on
            mobile](https://docs.google.com/a/chromium.org/presentation/d/1VYfCKye4TM-QiR_hiLvwYxhci_xc5YcA4oZxtrp2qes/edit#slide=id.p)
            - BlinkOn 2 (May 2014)
*   [Input - BlinkOn
            1](https://docs.google.com/a/chromium.org/presentation/d/1J1jG0XF6k42PA4s-otHFXZxrou7aKwYKYF90xPOe9bQ/edit#slide=id.p)
            (Sept 2013)
*   [Point, click, tap, touch - Building multi-device web
            interfaces](https://developers.google.com/events/io/sessions/361772634)
            - Google I/O 2013

**Design Docs**

*   [Scroll
            Unification](https://docs.google.com/document/d/1op5USoxDnN6yxB8EiFHYcGHacrZZRVKVqu4mSXFd6Ns/edit)
*   [Chromium input latency
            tracking](https://docs.google.com/a/chromium.org/document/d/1NUYMVyUJSU2NYrpGhKNfjSmmVZyPYoqbrPs7tbdY9PY/edit)
*   [Scroll-blocks-on: scrolling performance vs. richness
            tradeoff](https://docs.google.com/a/chromium.org/document/d/1aOQRw76C0enLBd0mCG_-IM6bso7DxXwvqTiRWgNdTn8/edit)
*   [Chromium throttled async touchmove
            scrolling](https://docs.google.com/a/chromium.org/document/d/1sfUup3nsJG3zJTf0YR0s2C5vgFTYEmfEqZs01VVj8tE/edit)
*   [Gesture
            Recognition](http://www.chromium.org/developers/design-documents/aura/gesture-recognizer)
*   [Vsync-aligned buffered
            input](https://docs.google.com/document/d/1L2JTgYMksmXgujKxxhyV45xL8jNhbCh60NQHoueKyS4/edit?usp=sharing)
*   [Touchpad and Wheel
            Latching](https://docs.google.com/document/d/1BizkQyW_FDU98sGxbHOZsAQWGYIA3xxXoFQZ3_XNo1o/edit)
*   [Touchpad pinch
            zoom](https://docs.google.com/document/d/1cYdt9r9stHLA2lbJ2I-Ucl_djhdSyngYFbmufZaLRHE/edit?usp=sharing)
*   User Activation v2
    *   [How Chrome uses user
                gestures](https://docs.google.com/document/d/1mcxB5J_u370juJhSsmK0XQONG2CIE3mvu827O-Knw_Y/edit?usp=sharing)
                (May 2017)
    *   [Case study: Popup with user-activation across
                browsers](https://docs.google.com/document/d/1hYRTEkfWDl-KO4Y6cG469FBC3nyBy9_SYItZ1EEsXUA/edit?usp=sharing)
                (May 2017)
    *   [User Activation v2: Main
                design](https://docs.google.com/document/d/1erpl1yqJlc1pH0QvVVmi1s3WzqQLsEXTLLh6VuYp228/edit?usp=sharing)
                (May 2017)
    *   [User Activation v2 with site
                isolation](https://docs.google.com/document/d/1XL3vCedkqL65ueaGVD-kfB5RnnrnTaxLc7kmU91oerg/edit?usp=sharing)
                (May 2018)
*   Input for Worker:
    *   Initial design discussion: [Low-latency Event Handling in
                Worker](https://docs.google.com/document/d/165f85uAKlknlQHwPkmpqLVq0O50XVkxzgTkO4utAsds/edit?usp=sharing)
                (July 2017)
    *   JS API proposals for [Routing Worker Events through the
                Compositor
                Thread](https://docs.google.com/a/chromium.org/document/d/1Ah3-O7Emp7cURyh-TINME0fId9laU0ctMCwjmlArgqU/edit?usp=sharing)
                (July 2017)
*   [Plumbing mouse as mouse on
            Android](https://docs.google.com/document/d/1mpBR7J7kgTXvp0QACVjhxtwNJ7bgGoTMmxfxN2dupGg/edit?usp=sharing)
            (October 2016)
*   [Coalesced
            Events](https://docs.google.com/document/d/1x-e8fH3I0DBrmrNufPQwRJHMYnW5q9CYZVEHe7BsfpI/edit?usp=sharing)
            (October 2016)

# **Learning Resources**

*   [Chromium touch pipeline
            overview](https://docs.google.com/a/chromium.org/presentation/d/10oIOTWFKIHArnfk8ZZx-9evvDpWC9EwRjDrZIz83Dz0/edit)
*   [Scrolling in blink](http://bit.ly/blink-scrolling)
*   [Android tracing cheat
            sheet](https://docs.google.com/presentation/u/1/d/1poMF7AEu5vd21BzUTIYfm2SXurEMlv2OVDlzs6JNRfg/edit?usp=sharing)
*   [Blink coordinate
            spaces](/developers/design-documents/blink-coordinate-spaces)
*   [Touch event behavior details across
            browsers](https://docs.google.com/a/chromium.org/document/d/12k_LL_Ot9GjF8zGWP9eI_3IMbSizD72susba0frg44Y/edit#heading=h.nxfgrfmqhzn7)
*   [Issues with touch
            events](https://docs.google.com/a/chromium.org/document/d/12-HPlSIF7-ISY8TQHtuQ3IqDi-isZVI0Yzv5zwl90VU/edit#heading=h.spopy4jje82p)
*   [Debugging common website touch
            issues](https://docs.google.com/a/chromium.org/document/d/1iQtI4f47_gBTCDRALcA4l9HL_h-yZLOdvWLUi2xqJXQ/edit)
