---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/origin-trials
  - Origin Trials
page_name: portals
title: Portals
---

**This origin trial is [now
available](https://developers.chrome.com/origintrials/#/view_trial/-7680889164480380927).**

---

Google Chrome is running an [origin
trial](https://github.com/GoogleChrome/OriginTrials/blob/gh-pages/developer-guide.md)
to give developers an opportunity to experiment with the
[Portals](https://web.dev/hands-on-portals/) feature that we are working on, and
gather feedback about how we can make it better.

## This experiment will begin in Chrome 85 and end after Chrome 86. During this experiment, you can use an origin trial token to experiment with using portals to load same-origin content (i.e. content served from the same origin as the main page).

## Frequently Asked Questions

### How do I sign up?

### Go to the [Origin Trials developer console](https://developers.chrome.com/origintrials/#/view_trial/-7680889164480380927).

Your tokens will periodically expire and need to be renewed using this console.

### How can I provide feedback?

Thanks for asking! When you renew your origin trial token, we’ll ask for your
feedback. This is the best place to explain how you’re using portals, what’s
working well, and what isn’t.
If you find bugs in Chrome’s implementation (for example, Chrome crashes when
you use portals), please [file a Chromium bug](https://crbug.com/new). Make sure
to mention Portals in your report so that the bug is triaged appropriately.

If you identify specific issues with the [CG draft
specification](https://wicg.github.io/portals/) or the design of the feature
(for example, if it’s difficult to achieve the effect you’re looking for using
the available API), please [file a spec
issue](https://github.com/WICG/portals/issues/new).

I've noticed some differences between what Chrome does and what the explainer
and/or spec say. What gives?

We apologize for the inconvenience: both are a work in progress.

In some places, the Chrome implementation contains features which haven't been
incorporated into the spec. For example:

    the &lt;portal&gt; element fires a [load
    event](https://github.com/WICG/portals/issues/26) similar to &lt;iframe&gt;

    portal activation [can fail in some cases that aren't yet
    explained](https://github.com/WICG/portals/issues/185), for example because
    the page has not yet loaded or failed to load

    a portal is a single object as seen by screen readers and other
    accessibility tools, and in this regard is more similar to a link or button
    than a frame

    The Chrome implementation integrates with session history and the back
    button, but this is not yet explained or specified.

In other places, the explainer discusses changes we have not yet implemented in
Chrome. For example:

    the explainer discusses cross-origin portals, which are not permitted during
    this origin trial

    integration with document.requestStorageAccess is alluded to, but not
    currently implemented in Chrome

    some changes to how portals are sized and how they interact with rendering
    and visibility APIs are explained in the API, but currently in Chrome
    &lt;portal&gt; elements behave in the same way as &lt;iframe&gt;

    The explainer alludes to integration with feature policy via the allow=""
    attribute, but this is not yet implemented in Chrome

### Why was the page blocked from loading in a portal?

For the duration of this experiment, pages are only permitted to load URLs that
match their origin. This means that the scheme (http or https), full hostname
(including subdomains) and port (if specified) must match. This restriction
applies to any URLs encountered by following redirects, and any navigations that
occur after loading within the portal.

If your content attempts to load a cross-origin URL in a portal, it will be
blocked and the previously loaded page, if any, will be displayed instead.

When this happens, a warning will be logged to the developer console, including
the origin of the URL that was blocked.

[<img alt="Navigating a portal to cross-origin content (from
https://www.example.com) is not currently permitted and was blocked."
src="/blink/origin-trials/portals/consolewarning.png">](/blink/origin-trials/portals/consolewarning.png)

Your load may also be blocked for other reasons. If either document has a
[Content Security Policy](https://developer.mozilla.org/en-US/docs/Web/HTTP/CSP)
that does not permit embedding, Chrome will respect it. For example, if the URL
to be loaded in the portal has a
[frame-ancestors](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Security-Policy/frame-ancestors)
directive that does not include self (or your origin), the load will be blocked.

### I was already using Portals, and I started seeing that warning message. What gives?

First, thanks for trying out Portals! If Portals were previously working for
you, you are a developer who has previously turned on the Portals feature flag.
(Note that this is not recommended in your main browser, just in your
development environment where you're experimenting with Portals.)

This origin trial allows developers like you to enable Portals for Chrome users.
Since this experiment does not yet cover loading cross-origin content, we've
changed the default behavior to be limited to same-origin content. To return to
allowing cross-origin content, enable the cross-origin portals flag via
chrome://flags/#enable-portals-cross-origin or run Chrome with the command-line
switch --enable-features=PortalsCrossOrigin.

### I'm seeing a warning that a &lt;portal&gt; was moved to a document where it was not enabled. What does that mean?

The origin trial enables the portals feature only for documents that supply an
origin trial token. If you attempt to make a portal element and then insert it
into another document, it will not be able to function correctly in that
document.

### Why can’t I use this from a local file:// URL?

Portals is a powerful feature, and we need to be able to establish the origin of
the content, and of the content it is loading in a portal, in order to ensure
that it is used safely. Unfortunately this means that you will need to run an
HTTP server in order to experiment with portals.

## Known Issues

### Breakpoints set in portalactivate event handlers don't pause execution

When inspecting a page using Chrome DevTools (*Ctrl + Shift + I* or *Right Click
+ Inspect*), execution doesn't pause for breakpoints set inside a portalactivate
event handler after a portal activates. As a workaround, you can either use
console debugging, or inspect the portal page through chrome://inspect/#pages
before activation and set a breakpoint there. Breakpoints set when inspecting
pages through chrome://inspect work correctly, so they should also work when
using remote debugging (chrome://inspect/#devices) to debug a page running on a
remote device. This issue is tracked [here](https://crbug.com/1025761).

[<img alt="Image showing chrome://inspect#pages"
src="/blink/origin-trials/portals/DevTools_Breakpoint_Workaround.png" height=203
width=400>](/blink/origin-trials/portals/DevTools_Breakpoint_Workaround.png)
