---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/instant
  - Instant
page_name: instant-support
title: Instant Support
---

**ALL CONTENT BELOW IS OUTDATED, INSTANT IS UNLAUNCHED**

This document assumes some familiarity with the Chrome Instant feature,
including the capabilities in Instant Extended. It's written primarily for
developers working on the feature.

## Instant URL

An *Instant URL* is a URL that matches the instant_url template of the default
search engine. So, given the default Chrome installation,
"http://www.google.com/webhp" and "http://www.google.com/webhp?foo=bar#q=quux"
are considered Instant URLs, whereas "http://www.google.com/accounts" is not.
Instant Extended allows Instant URLs to match more template fields of the
default search engine, with restrictions. So, in the extended mode,
"https://www.google.com/?espv=1" and
"https://www.google.com/search?espv=1&q=foo" are also Instant URLs, whereas the
corresponding URLs without the "espv=1" parameter are not.

**Why does this matter?**
The Chrome Instant functionality only works with Instant URLs. So, in extended
mode, we extract query terms from a URL into the omnibox only if the URL is an
Instant URL. Similarly, we create an InstantTab (described below) only if the
URL is an Instant URL.
In addition, we try to bucket all Instant URLs (and only Instant URLs) into a
dedicated renderer process (the *Instant renderer*). I say *try to* because we
won't always succeed, as you'll see below. We also install the SearchBox
extension only in the Instant renderer (and not other renderers). The SearchBox
extension is responsible for implementing Chrome's side of the [SearchBox
API](/searchbox), so this means that only webpages loaded in the Instant
renderer can access the API.

**Terminology note:** URLs can be classified into Instant URLs or non-Instant
URLs. Webpages support Instant or they don't. In other words, to talk about
Instant support, you need an actual page (WebContents). Throughout this
document, we use the terms URL and page consistently in this manner. Of course
pages have URLs, so we also say that "a page has an Instant URL" or "a
non-Instant URL page".

## Determining Instant Support

**What is Instant support?**

We want to know whether the page that we've loaded in the hidden Instant overlay
actually supports Instant, i.e., the SearchBox API. We start by loading the
default search engine's Instant URL into the overlay, but after it goes through
redirects, it may end up in a page that may or may not support Instant.

**Why do we want to determine Instant support?**
Say the page isn't known to support Instant yet. When the user types in the
omnibox, we should immediately fallback to the local omnibox popup. Doing
otherwise is bad. If the user types, and we send `onchange()` to the page,
hoping that it will eventually respond, we would be making the user wait.
Conversely, if we know that the page supports Instant, we can and should send
onchange() to it, instead of falling back to the local omnibox popup, since the
local popup is an inferior experience.

**How do we determine Instant support?**
We start by assuming the page doesn't support Instant. At some point, the
browser sends an IPC to the renderer. The SearchBox receives it, checks if
`window.chrome.searchBox.onsubmit()` is a JS function defined in the page, and
responds. When the response IPC is received by the browser, it thus determines
Instant support.
Also, if the browser receives an IPC at any point that's part of the SearchBox
API (such as *SetSuggestions* or *ShowInstantPreview*), it considers the page to
support Instant.
Only pages that are Instant URLs can support Instant. A random non-Instant URL
webpage can define the onsubmit() method, but there'll be no SearchBox extension
available to receive or send the appropriate IPCs. In other words, a random
webpage can't fool the browser into thinking it supports Instant (however, see
the caveat in footnote \[1\]).

**When do we determine Instant support?**
The browser waits for the page to fully load and then sends the IPC mentioned
above. This happens in the Instant implementation of
`WebContentsObserver::DidFinishLoad()`.

**Why don't we just check to see if the final page loaded in the Instant renderer?**
First, even if the page is an Instant URL, it might not actually support
Instant. For example, "http://www.google.com/webhp" might be a recognized
Instant URL (so it gets assigned to the Instant renderer), but when it loads,
the page may disable Instant due to server side experiments or other failures.
Second, the page may go through one or more redirects that cause a
*cross-process navigation* (see the OpenURLFromTab section below). If one of
these redirects is renderer-initiated (e.g.: using `location.href = "..."` or a
`<meta http-equiv=refresh>` tag), we'll still get a DidFinishLoad(). Say
"http://www.google.com/webhp" (the Instant URL we initially load) uses a JS
redirect to "http://www.google.com/accounts" (a non-Instant URL), which, after
verifying your login cookies, redirects you back to the "/webhp" URL, again
through a JS redirect. At some point, we'll get a DidFinishLoad() for the
"/accounts" URL. If we check the renderer at that time, we will wrongly conclude
that the page doesn't support Instant.
In fact, we do actually send the request IPC after "/accounts" loads (because of
DidFinishLoad(), as mentioned above). But by the time the response IPC comes
back, the browser already knows about the JS redirect and is in the midst of
handling it, so it knows that the response is for an older page, and ignores it.
This is achieved by checking `WebContents::IsActiveEntry()` using the page_id
contained in the response IPC.
Of course, it's possible for the JS redirect to happen much later through a
delayed timer, in which case, the "/accounts" URL is still the active entry when
the browser receives the response IPC. In this case, we'll conclude that the
page doesn't support Instant. This is okay since we can't possibly handle
arbitrarily delayed redirects. Our method works for the common case of an
immediate JS redirect.

**What happens if the final page isn't an Instant URL? We will never get a response IPC, right?**
True, since there's no SearchBox extension in that page's renderer. However, in
the absence of a response IPC, the browser will continue to treat the page as
not supporting Instant, so it all works out.

## OpenURLFromTab

`WebContentsDelegate::OpenURLFromTab()` is called whenever there's a
cross-process navigation. This can happen in a few different ways:
**Case 1:** The initial page load of the Instant URL in the hidden overlay may
go through a series of HTTP redirects, any of which may trip the cross-process
bit. Say we start by loading URL A. It redirects to URL B, which is "claimed" by
an installed app (i.e., the app has listed B in its list of URL patterns in its
manifest.json). Apps are loaded in separate renderer processes, so the redirect
from A to B causes a cross-process navigation. HTTP redirects that are not
claimed by any app do **not** trip the cross-process bit, even if they are not
Instant URLs. I.e., if URL A is an Instant URL, but URL B is not, B will still
be loaded in the same Instant renderer process that we started out with. \[1\]
Alternatively, we'll hit the cross-process case if any of the redirects are
renderer-initiated (as mentioned in the previous section, using location.href or
a &lt;meta&gt; tag). Normally, renderer-initiated navigations (including
redirects) are considered cross-process only if they actually cross an app or
extension boundary (see `RenderViewImpl::decidePolicyForNavigation()`). For
Instant however, we have added some code that makes **all** renderer-initiated
navigations be initially treated as cross-process (see
`ChromeContentRendererClient::ShouldFork()`).
**Case 2:** Say we are showing the Instant overlay (URL A), and the user does
something (such as click on a link) that causes the page to navigate to URL B,
tripping a renderer-initiated cross-process navigation (as explained above).
Assume that the click by itself doesn't cause the overlay to be committed
(because the overlay is showing at less than full height).
Let's look at what happens if B is not considered to be an Instant URL (in both
cases above). In Case 2, we want to commit the overlay if possible, since it
would be a mistake to try to continue using the overlay (B cannot support the
Instant API, so the overlay would just stop working). However, in Case 1, we
don't want to commit or discard the overlay just yet, since it's part of the
initial series of redirects, and the final Instant page hasn't even been loaded
yet.
The way we distinguish these cases is by looking at whether the overlay is
already known to support Instant. In Case 1, the Instant support determination
hasn't yet been performed. In Case 2, it has, since we can't possibly be showing
the overlay if it didn't support Instant. So, our algorithm for OpenURLFromTab()
is this:

```none
OpenURLFromTab(url) {
  if (!supports_instant_) {
    // Case 1: Allow (perform) the navigation.
    contents_->GetController().LoadURL(url);
  } else {
    // Case 2: Commit the overlay if possible. Allow or deny the navigation based on whether the commit succeeds or fails.
    if (CommitIfPossible(...)) {
      // The Browser is now the WebContentsDelegate, not us.
      contents_->GetDelegate()->OpenURLFromTab(url);
    } else {
      // Deny (don't perform) the navigation.
    }
  }
}
```

Note that when we get an OpenURLFromTab() call, it's incumbent upon us to
actually perform the navigation. If we don't do anything, the navigation (or
redirect) won't happen.

**Wait, the above algorithm doesn't check whether `url` is a non-Instant URL, which is the reasoning given for distinguishing Case 2 above. Why not?**
Actually, we want to commit the overlay on any user action, even if the
navigation is to an Instant URL. This is because the user could've clicked on a
link to say "http://www.google.com/webhp". The user expects the click to result
in a committed tab, with the full webpage in it. It would be weird if it was
still an overlay. The overlay isn't expected to randomly navigate on its own, so
we'll assume that any call to OpenURLFromTab() is due to a user action (thus,
committing the overlay is an appropriate action for us to take).

**What happens if the overlay tries to update its hash state, i.e., the "#q=..."
fragment of its URL, to keep state (i.e., without any user action)?**

Thankfully, updating the fragment does not result in a call to OpenURLFromTab(),
and thus we don't attempt to erroneously commit the overlay. TODO(sreeram): How
about pushState?

**If we can't commit, why do we not allow the navigation?**
In the `else { // Deny }` part above, we could've chosen to still perform the
navigation, and then do a new round of Instant support determination. Then, if
the page ends up supporting Instant, we could keep using it as the overlay.
Otherwise, we could discard it. This is needlessly complex and probably would
introduce subtle bugs due to the second round of Instant support determination.
Practically, this should never be needed. We *ought* to be able to commit all
the time.
The only time we wouldn't be able to commit the overlay is if the current
omnibox text is a URL (and not a search). But in that case, the overlay
shouldn't be showing any links other than suggestions (in particular, there
should be no search results, search tools, Google+ widgets or such). If the user
clicks on a suggestion, the page will request for it through the SearchBox API
(*navigateContentWindow* for URLs and *show(reason=query)* for queries), so the
page shouldn't cause an OpenURLFromTab() call. If something slips through the
cracks (say because the overlay showed a "Learn more" link that the user
clicked), we'll just disallow the navigation, which means the overlay remains
showing the Instant page, and continues to work.

## InstantTab

In Instant Extended mode, an *InstantTab* represents a committed page (i.e., an
actual tab on the tabstrip, and not an overlay) that's an Instant URL.
Typically, this is either the server-provided NTP (New Tab Page) or a search
results page. Such a page can also be used to show Instant suggestions and
results, so it's appropriate to ask whether an InstantTab supports the Instant
API.
An InstantTab is a lightweight wrapper that's deleted and recreated as the user
switches tabs (i.e., Instant only has a single InstantTab object, not one per
tab). When the user switches to a tab with an Instant URL, we create an
InstantTab wrapper around it, starting as usual by assuming that it doesn't
support Instant. We then immediately send the request IPC. The rest of the
Instant support determination works similar to the overlay. Since we (Instant)
are not the WebContentsDelegate for a committed tab, none of the
OpenURLFromTab() issues arise here. Note that we reset the InstantTab when the
user switches tabs. We don't store the result of the Instant support
determination anywhere permanently in the tab's WebContents.
This generally works well, except for the following case: If the tab is a
server-provided NTP ("http://www.google.com/webhp"), it's possible that we just
created the WebContents and had to immediately commit it, so the page hasn't
fully loaded yet. Since the common case is to open a browser with the NTP, we
don't want to fall back to the local NTP just because we haven't finished
loading the server-provided NTP. But, if the user starts typing into the omnibox
before the NTP finishes loading, we don't want to wait for the server page
(since that could take an arbitrary amount of time). In such a case (user typing
before the NTP finishes loading), we'll fallback to the overlay (local omnibox
popup).

**Does that mean that we fallback to the overlay whenever the user types, but we haven't yet determined Instant support for an InstantTab?**
No, that would mean practically every time the user switches to a tab and
immediately starts typing, we'll end up with the overlay. This is obviously bad
(because the user is bounced out of whatever search modes/tools they had in the
tab).

**Perhaps the solution is to store the Instant support bit with the WebContents? So that, when we switch to a tab, we won't have to perform the determination all over again?**
This might help somewhat, but it doesn't solve all problems. For example, an
Instant URL tab might have been created without Instant (i.e., through a link
click or a bookmark navigation). When the user switches to it the first time, we
still have to perform Instant support determination. Also, storing this bit with
the WebContents means storing it somewhere within the SearchTabHelper, and this
introduces unnecessary complexity (what should happen to that bit if the tab
navigates in the background?) and leaks Instant concepts to an unrelated part of
the codebase.

**What's the solution then?**
If we are on an InstantTab, and we haven't yet determined its Instant support,
switch to the overlay only if the page hasn't finished loading. The reason this
works is that, if the page has finished loading , we can go ahead and blindly
send the onchange() to it as the user types. Note that we are not waiting for
the page to load to determine Instant support. We send the IPC as soon as the
user switches to the tab. Waiting for a page to load is the long pole that takes
an indeterminate amount of time. Waiting for an IPC is a much smaller, mostly
determinate amount of time (a few milliseconds, usually). So, if it turns out
that the page doesn't support Instant, we'll quickly discover that and fallback
to the overlay anyway.

**What happens if the tab is a "sad tab", i.e., the page has crashed?**

Right. We need to not wait for Instant support in that case.

**What happens if the page isn't an Instant URL? Won't we end up waiting indefinitely for a response IPC that never arrives? So, won't we send onchange() into the void?**
No. An InstantTab is only created for pages that are Instant URLs, so they
should all have been bucketed into the Instant renderer process, and thus should
have the SearchBox extension. So, a response IPC is guaranteed.

Well, not really. Recall that a renderer-initiated navigation normally isn't
cross-process unless it crosses an app boundary. So, it's possible that a tab
started out with a non-Instant URL (thus, it was **not** assigned to the Instant
renderer), then the user clicked on a link to an Instant URL. Now, this tab is
eligible to be treated as an InstantTab. But the page is still in the
non-Instant renderer, so we'll never get the response IPC back. This can only
happen with InstantTab and not with the overlay, because the overlay always
starts out with an Instant URL, and thus always starts out in the Instant
renderer.

So, putting all this together, here's our algorithm regarding InstantTab:

```none
ResetInstantTab() {
  WebContents* contents = GetActiveWebContents();
  if (contents->GetURL() is an Instant URL
      AND contents hasn't crashed
      AND contents->GetRenderViewHost()->GetProcess()->GetID() is an Instant renderer) {
    overlay_.reset();  // Discard the overlay, if any.
    instant_tab_.reset(contents);
    instant_tab_->DetermineInstantSupport();  // Send the request IPC.
  } else {
    instant_tab_.reset();  // Don't use InstantTab.
  }
}
Update() {
  if (instant_tab_) {
    if (overlay_) {
      // We previously switched to the overlay because the InstantTab wasn't done loading.
      // It probably has finished loading by now, but no matter. We'll continue using the overlay
      // until it's discarded, to avoid a jarring switch as the user types.
      overlay_->Update(...);
    } else {
      if (instant_tab_->contents()->IsLoading()) {
        // The InstantTab hasn't finished loading. Use the overlay instead.
        overlay_.reset(new InstantOverlay(...));
        overlay_->Update(...);
      } else {
        // Use the InstantTab.
        instant_tab_->Update(...);
      }
    }
  } else {
    // Use the overlay_, creating one if necessary.
  }
}
ActiveTabChanged() {
  overlay_.reset();  // Discard the overlay.
  ResetInstantTab();
}
```

Questions? Comments? Send them to sreeram@chromium.org.

---

\[1\] The astute reader would have observed that thus, it's technically possible
for us to end up with a non-Instant URL page in the Instant renderer process,
one which may even define the onsubmit() method and thus pass the Instant
support determination test. This is fine. It can only happen if the Instant URL
that we start with willfully redirects (using HTTP redirects) to such a
non-Instant URL.
