---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: prerender
title: Chrome Prerendering
---

## Overview

> Prerendering is a feature in Chrome to improve user-visible page load times.
> Prerendering is triggered by &lt;link rel="prerender"&gt; elements in
> referring pages. A hidden page is created for the prerendered URL, which will
> do full loading of all dependent resources, as well as execution of
> Javascript. If the user navigates to the page, the hidden page will be swapped
> into the current tab and made visible.

> ![](/developers/design-documents/prerender/CroppedPrerenderingDiagram.png)

> Although the core of prerendering is a fairly simple change to Chrome, there
> are a number of issues which make the implementation more complex:

    *   Minimizing resource contention.
    *   Handling dynamic media \[video, audio, plugins, canvas\]
    *   Cancellation of pages on certain corner cases.
    *   Minimizing server side effects.
    *   Mutations to shared local storage \[cookies, sessionStorage,
                etc.\]

> Note that this document is not intended to be a comprehensive list of issues.

### Base Mechanism

> #### Creating a Prerendered Page

> Prerender is initiated when a page contains a &lt;link rel=”prerender”&gt;
> resource. The ResourceDispatcherHost receives a resource request with
> ResourceType::Prerender - but this request will never be sent out to the
> network. Instead, it is used as a signal to create a PrerenderContents, and
> the request itself is cancelled.

> The PrerenderContents is stored in the PrerenderManager, which maintains a
> directory of all PrerenderContents created by the same profile. A small number
> of recently created PrerenderContents are allowed. The current implementation
> only keeps one page around for a maximum of 30 seconds, but this may change in
> the future. Older pages are pruned, and a Least-Recently-Created eviction
> algorithm is used if capacity has been reached.

> #### Using a Prerendered Page

> There are two cases where a prerendered page may be used instead of a new page
> load:

    *   In a call to TabContents::NavigateToPending entry, which is
                triggered e.g. when a user types in a new URL to visit.
    *   In a call to TabContents::DidNavigate, which is, among other
                cases, exercised when a user clicked on a link on the current
                page.

> In both cases, the current profile’s PrerenderManager is checked to see if a
> valid PrerenderContents matches the destination URL. If a match is found,
> then:

    *   The PrerenderContents is removed from the PrerenderManager, so
                it can only be used once.
    *   The PrerenderContents’ TabContentsWrapper is swapped in for the
                existing TabContentsWrapper using the TabContents delegate
                ReplaceAt mechanism.
    *   The old TabContentsWrapper is kept alive until unload handlers
                complete running, and then is destroyed.

> #### Visibility API

> A page visibility API has been added to expose the current visibility state of
> a web page, such as whether it is prerendered, hidden, or potentially
> visibile.

> See <http://w3c-test.org/webperf/specs/PageVisibility/> for the proposed API.
> Note that the current implementation adds a webkit prefix since the API may
> still change.

> This can be used for a variety of purposes, such as lowering the volume of a
> game while it’s not visible, or pausing an intro sequence until the page
> transitions out of the prerender state.

### Minimizing Resource Contention

> Prerendering runs the risk of negatively impacting page load time of other
> pages due to resource contention. Although some of these issues are also
> tickled with the existing prefetch support, prerendering makes the potential
> for problems more severe: more resources will be fetched because the
> subresources are also retrieved, not just the top level page; and CPU and
> memory consumption will likely be higher.

> To minimize network contention, all resources in a prerendered page are
> retrieved at the lowest priority level. Currently the priority is only used to
> order pending requests for a particular domain: the scheduling may need to
> change for this priority to only allow idle requests if there are no active
> network requests at all. There is currently no way to cancel active
> connections when a higher priority request comes along--this may be needed so
> long-lived speculative requests for prerender do not block requests for a page
> that the user is actively visiting on the same domain. Finally, there is no
> way to change resource priorities after a request has started--this may be
> needed to bump up the priority of requests after a page has transitioned from
> prerender to visible.

> Memory utilization is being handled by restricting the number of prerendered
> pages to only 3 (At most 2 per page), and is restricted if there is not enough
> available RAM on the system at the time of the prerender. Additionally, if the
> memory for the page exceeds [150
> MiB](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/browser/prerender/prerender_config.cc&l=9)
> then the prerendering is cancelled and the memory returned to the system.

> CPU utilization is being handled by lowering the priority of the render
> process which contains the prerendered page. A prerendered page is only
> created if it can be assigned to a unique process, so this minimizes the
> chance that we will decrease the priority of a render process containing an
> active tab.

> Minimizing GPU utilization is currently not handled. One problem is that GPU
> usage is measured per render process rather than per RenderView.

> Minimizing disk cache utilization is currently not handled. The disk cache is
> not currently priority based. Also, the cache hit rate may decrease over time
> for users with prerender enabled due to more unused resources being inserted.
> The eviction algorithm for the disk cache may also need to change to more
> aggressively evict resources which were speculatively retrieved but never
> used.

> If a prerendered page tries to prerender another page, the requested prerender
> is deferred until and unless the first prerender is navigate to.

> Prerenders are destroyed if not used within 5 minutes. The source page can
> cancel the prerender earlier if desired.

### Handling Dynamic Media

> #### Plugin deferral

> While a page is in the prerendered state, plugin instantiation (and loading)
> will be deferred until the page has been activated. The main rationale is to
> prevent audio or video media from playing prior to the user actually viewing
> the page, as well as to minimize the possible exploit surface.

> A BlockedPlugin instance is created for each plugin element on the original
> page. This is the same instance used by the Click-to-Play feature. It places a
> simple shim plugin in the place of the originally intended plugin, and retains
> the parameters that are needed to correctly create and initialize the
> originally intended plugin. When the page transitions out of the prerendered
> state, all BlockedPlugin instances created for prerendering purposes will swap
> in the originally intended plugins.

> #### HTML5 media elements

> Playback is deferred the page completes, similar to plugins.

Cancellation on Corner Cases

> Pages are canceled if any of the conditions happen:

> *   The top-level page is not an HTTP/HTTPS scheme, either on the
              initial link or during any server-side or client-side redirects.
              For example, both ftp are canceled. Content scripts are allowed to
              run on prerendered pages.
> *   window.opener would be non-null when the page is navigated to.
> *   A download is triggered. The download is cancelled before it
              starts.
> *   A request is issued which is not a GET, HEAD, POST, OPTIONS, or
              TRACE.
> *   A authentication prompt would appear.
> *   An SSL Client Certificate is requested and requires the user to
              select a certificate.
> *   A script tries to open a new window.
> *   alert() is called.
> *   window.print() is called.
> *   Any of the resources on the page are flagged by Safe Browsing as
              malware or phishing.
> *   The fragment on the page does not match the navigated-to location.

> When a problem is detected, the cancellation is done synchronously and the
> offending behavior is typically stopped. For example, an XmlHttpRequest PUT
> will cancel the request before it goes over the network, and prevent the page
> from being swapped in. The prerendered page may live for a little while longer
> due to the asynchronous nature of RenderView cancellation, but it will not be
> swapped in.

> Additional behavior may cause cancellation in the future, and some of the
> existing cancellation causes may be relaxed in the future.

### Mutations to shared local storage

> Shared local storage such as DOM storage, IndexedDB, and HTTP cookies present
> challenges for prerendering. Ideally mutations made by the prerendered page
> should not be visible to other tabs until after the user has activated the
> page. Mutations made by other pages should be reflected in the prerendered
> page, which may have already read from local storage before the other pages
> made the changes.

> One option is to not worry about these issues. Since the prerendered pages are
> only retained for a short period of time after their creation, the window for
> race-like conditions is fairly short. However, this may result in confusion
> for users (for example, if a prerender starts for a page, the user logs out on
> the main page, and then the prerendered page becomes active with the user’s
> old credentials). Additionally, the Visibility API provides ways for pages to
> defer any mutating behavior until after the page becomes visibile. This is the
> current approach taken in Chrome.

> A second option is to cancel the prerender any time local storage is accessed.
> This may be feasible for more recent versions of local storage \[such as
> IndexedDB\] but is not an option for more commonplace schemes, particularly
> HTTP cookies. Long-term, the cancel prerendering approach will also lead to
> resistance of adoption of new forms of local storage.

> A third option is to have a local storage sandbox while the page is
> prerendered, and attempt to transactionally commit changes to the shared local
> storage when the page becomes activated. See
> <https://www.chromium.org/developers/design-documents/cookies-and-prerender>
> for some thoughts about how to do this for Cookies.

### Minimizing server side effects

> Prerendering is only triggered when the top level page is retrieved with a
> GET, and is assumed to be idempotent. Additionally, any non-GET, HEAD, TRACE,
> POST, or OPTIONS requests from XmlHttpRequests will not be sent over the
> network and the page will be cancelled.

### Following redirects

> ## If the server sends a redirect response for a subresource with a "Follow-Only-When-Prerender-Shown: 1" header, Chrome will hold off on following the redirect and on fetching the respective subresource until the prerender is shown to the user.

## Any Questions?

> Please send mail to prerender@chromium.org, or read the group archives at
> <http://groups.google.com/a/chromium.org/group/prerender/topics>
