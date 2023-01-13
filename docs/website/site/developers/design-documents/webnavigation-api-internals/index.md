---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: webnavigation-api-internals
title: WebNavigation API internals
---

**# **This document aims at explaining how the [webNavigation API](http://developer.chrome.com/extensions/webNavigation.html) is tracking the navigation state of a WebContents (aka RenderViewHostDelegate). If you’re interested in the source, the WebContentsObserver is defined in [src/chrome/browser/extensions/api/web_navigation/web_navigation_api.cc](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/browser/extensions/api/web_navigation/web_navigation_api.cc)****

**## What is the current RenderViewHost?**

**Contrary to popular belief, a WebContents has not a 1:1 relation to a
RenderViewHost, but in fact, it's a 1:n relation. A WebContents has one visible
RenderViewHost at a time, but the RenderViewHost can change during navigation,
and the WebContents keeps around previous RenderViewHosts in case the user comes
back to them. Therefore, a WebContentsObserver has to pay close attention to
which RenderViewHost triggered a given signal.**

**The WebNavigation API tracks two RenderViewHosts per WebContents, the current
RenderViewHost, and the pending RenderViewHost. The former is the visible one:
the latest RenderViewHost in which a top-level navigation was committed, or, if
no such RenderViewHost exists, the first RenderViewHost connected to the
WebContents.**

**The latter is the latest RenderViewHost in which a provisional top-level navigation was started other than the current RenderViewHost.**

**We ignore all navigations from RenderViewHosts but those two RenderViewHosts. We also do not observe navigation events from interstitial pages such as SSL certificate errors - these are rendered with an InterstitialPage as RenderViewHostDelegate.**

**Those two RenderViewHosts are determined as follows.**

*   **WebContentsObserver::AboutToNavigateRenderView**

**Every top-level navigation starts with this call. If this is the first call, the passed in RenderViewHost becomes the current RenderViewHost, otherwise, if it isn't the current RenderViewHost, it becomes the pending RenderViewHost.**

*   **WebContentsObserver::DidCommitProvisionalLoadForFrame**

**As soon as a top-level navigation is committed, the corresponding RenderViewHost becomes the current RenderViewHost. We discard the pending RenderViewHost.**

*   **WebContentsObserver::DidFailProvisionalLoad**

**If the pending RenderViewHost fails the provisional load, we discard it.**

*   **WebContentsObserver::RenderViewGone:**

**If the current RenderViewHost crashes, we discard both RenderViewHosts.**

*   **WebContentsObserver::RenderViewDeleted:**

**If the pending RenderViewHost is deleted for whatever reason, we discard it. If the current RenderViewHost is deleted, we discard it and if there is a pending RenderViewHosts it becomes the current RenderViewHost.**

**Note that the current RenderViewHost is not only deleted when the tab is closed. The instant search and prerender features can swap in a different WebContents of a tab.**

**## Navigation of a top-level Frame**

**For a successful navigation, a frame has to go through several states (see
also Adam’s recent presentation “[How WebKit
works](https://docs.google.com/a/google.com/presentation/pub?id=1ZRIQbUKw9Tf077odCh66OrrwRIVNLvI_nhLm2Gi__F0#slide=id.p)”).
Keep in mind that you can receive these signals from any number of
RenderViewHosts. You should ignore all signals but from the current
RenderViewHost or the pending RenderViewHost.**

**Since a frame is only uniquely identified within a renderer process, and a
WebContents can be a delegate for several RenderViewHosts in different renderer
processes, we need the tuple (RenderViewHost, frame_id) to identify a frame
uniquely within a given WebContents.**

*   **WebContentsObserver::DidStartProvisionalLoadForFrame**

**At this point, the URL load is about to start, but might never commit (invalid URL, download, etc..). Only when the subsequently triggered resource load actually succeeds and results in a navigation, we will know what URL is going to be displayed.**

*   **content::NOTIFICATION_RESOURCE_RECEIVED_REDIRECT**

**While not strictly necessary to track the navigation state, we observe this notification to determine whether a server-side redirect happened.**

*   **WebContentsObserver::DidCommitProvisionalLoadForFrame**

**At this point, the navigation was committed, i.e. we received the first headers, and an history entry was created.**

**If the navigation only changed the reference fragment, or was triggered using the history API (e.g. [window.history.replaceState](https://developer.mozilla.org/en-US/docs/DOM/window.history)), we will receive this signal without a prior DidStartProvisionalLoadForFrame signal.**

*   **WebContentsObserver::DocumentLoadedInFrame**

**WebKit finished parsing the document. At this point scripts marked as defer were executed, and content scripts marked “document_end” get injected into the frame.**

*   **WebContentsObserver::DidFinishLoad**

**The navigation is done, i.e. the spinner of the tab will stop spinning, and the onload event was dispatched.**

**If we’re displaying replacement content, e.g. network error pages, we might see a DidFinishLoad event for a frame which we didn’t see before. It is safe to ignore these events.**

**## Navigation of a sub-frame**

**The navigation events for sub-frames do not differ from the events for the
top-level frame. Sub-frame navigations can start at any time after the
provisional load of their parent frame was committed. If a sub-frame is part of
the parent document (as opposed to created by JavaScript), the parent frame’s
DidFinishLoad signal will be sent after all of its sub-frames DidFinishLoad
signals.**

**It’s also possible for sub-frames of a frame to send navigation signals while their parent frame already started a new provisional load.**

**## Navigation failures**

**A navigation can fail for a number of reasons. Since we’re tracking the state
of all frames (and extensions using the webNavigation API might do the same), it
is important to not have frames hanging around forever. They should either
finish navigation, or fail.**

**The following is a list of events we observe to determine whether a navigation
has failed.**

*   **content::NOTIFICATION_RENDER_VIEW_HOST_DELETED**

**When the frame’s RenderViewHost was deleted, the navigation failed.**

*   **WebContentsObserver::AboutToNavigateRenderView**

**When the pending RenderViewHost is replaced by a new pending RenderViewHost, all frames navigating in the old pending RenderViewHost fail.**

*   **WebContentsObserver::DidCommitProvisionalLoadForFrame**

**If the main frame of the current RenderViewHost commits, all navigations in the pending RenderViewHost fail. If the main frame committed a real load (as opposed to a reference fragment navigation or an history API navigation), all sub frames in the current RenderViewHost fail.**

**If a frame in the pending RenderViewHost commits, all navigations in the current RenderViewHost fail.**

*   **WebContentsObserver::DidFailProvisionalLoad**

**This error would occur if e.g. the host could not be found.**

*   **WebContentsObserver::DidFailLoad**

**This error would occur if e.g. window.stop() is invoked, or the user hits ESC.**

*   **WebContentsObserver::WebContentsDestroyed**

**All frames we know about fail.**
