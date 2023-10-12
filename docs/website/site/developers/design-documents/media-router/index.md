---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: media-router
title: Media Router & Web Presentation API
---

**This document is obsolete and will be removed soon. [The new version is
here](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/media/media_router.md).**

The media router is a component in Chrome responsible for matching clients that
wish to render content outside the browser (*media sources*) with devices and
endpoints capable of rendering that content (*media sinks*). When a media source
is linked with a m*edia sink* (in general, requiring user permission), a *media
route* is created that allows two-way messaging between the client and the sink.
The media route allows the client to negotiate a peer-to-peer media streaming
session with the media sink via messaging (e.g., via
[WebRTC](http://www.webrtc.org/) or [Cast
Streaming](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/renderer/extensions/cast_streaming_native_handler.h&q=cast_streaming&sq=package:chromium&type=cs)),
aka "mirroring." The media route can also be used to control remotely rendered
media without an associated peer-to-peer media streaming session, aka
"flinging". The media route can be terminated at user or client request, which
denies access to the media sink from the application.

The Web [Presentation API](http://www.w3.org/2014/secondscreen/) allows a Web
application to request display of Web content on a secondary (wired, or
wireless) screen. The content may be rendered locally and streamed to the
display or rendered remotely. The Web application controls the content by
two-way messaging.

Note that the non-Blink parts of the media router will be implemented only in
desktop Chrome and ChromeOS. Presentation API functionality will be implemented
in Chrome for Android using analogous platform components such as the [Android
Media Route Provider
framework](https://developer.android.com/guide/topics/media/mediarouteprovider.html).

Also note that a separate design is in progress for offscreen rendering,
capture, and streaming of WebContents (required for full Presentation API
support).

## Objectives

The objectives of this project:

*   Allow use of media sinks from a multitude of clients, including: Web
            applications via the [Presentation
            API](http://w3c.github.io/presentation-api/); Chrome apps; the
            browser itself; and the Chrome OS system shell.
*   Support mirroring locally rendered content to external screens,
            including on-screen and off-screen tabs, Chrome apps windows, and
            the system desktop.
*   Support "flinging" HTML5 documents to remote devices capable of
            rendering them.
*   Support the [Cast Chrome Sender
            SDK](https://developers.google.com/cast/docs/reference/chrome/) on
            desktop and Android without any user installed extensions.
*   Allow new types of media sinks to be added to Chrome by implementing
            additional Media Route Providers.

The following are non-goals but may be objectives for future work:

*   Multicast of local content to multiple sinks at once.
*   Support for third party media route providers (in Javascript or
            NaCl) or run-time installation of media route providers.
*   Support for sinks that are not primarily intended to render media.

## Overview

The media router consists of four distinct components:

1.  The Chrome Media Router is a browser service exposed in-process via
            C++ API and is exposed to other processes via a set of two Mojo
            interfaces: the Presentation interface and the Media Router API
            interface. Its job is to field requests from clients for media sink
            availability, media route construction/destriction, and media route
            control via messaging. It also controls the Media Router Dialog and
            delegates many functions to the Media Router component extension.
2.  The Media Router extension is an external component extension
            responsible for direct interaction with media sinks. The component
            extension will initially support use of
            [Cast](http://www.google.com/cast/) and
            [DIAL](http://www.dial-multiscreen.org/) devices with more types of
            sinks to be added over time. The component extension interacts with
            the Chrome Media Router via the Media Router API Mojo service, and
            uses some chrome.\* platform APIs, such as chrome.dial,
            chrome.cast.channel, and chrome.mdns to implement network level
            interaction with Cast and DIAL devices.
3.  Users interact with the Chrome Media Router through the Media Router
            Dialog. This WebUI component allows users to choose the destination
            media sink for new media routes and view and stop active media
            routes. It may be pulled up by the user clicking the Cast icon in
            the browser toolbar, or at the request of a Web application via the
            Presentation API.
4.  The Presentation Mojo interface acts as a bridge between the Chrome
            Media Router and the Blink implementation of the Presentation API
            ([launch
            bug](https://code.google.com/p/chromium/issues/detail?id=412331)).

## Architecture

The following diagram illustrates the architecture of the components described
above.

[<img alt="image"
src="/developers/design-documents/media-router/Chrome%20Media%20Router%20Architecture%20%281%29.png">](/developers/design-documents/media-router/Chrome%20Media%20Router%20Architecture%20%281%29.png)

## Chrome Media Router

The Chrome Media Router is a browser-resident service that serves as a
media-protocol-agnostic platform for parties interested in media routing. It
provides its clients with a set of APIs for media routing related queries and
operations, including:

*   Register for notifications when a sink is available that can render
            a media source. (Media sources are represented as URIs and can
            represent local media or remotely hosted content.)
*   Request routing of media for that source, which will show the user
            the media router dialog to select a compatible sink. If the user
            selects a sink, the media route is returned to the application to
            allow it to control media playback.
*   Accept media control actions from the Media Router Dialog for an
            active media route and forwarding them to the associated route
            provider.
*   Send and receive arbitrary (string) messages between the application
            the media sink.
*   Terminate media routes, and notify the client and media route
            provider when that happens.

The Chrome Media Router, itself, does not directly interact with media sinks.
Instead it delegates these requests and responses to a media route provider in
the component extension. The Chrome Media Router will contain bookkeeping of
established routes, pending route requests, and other related resources, so it
does not have to request this information from the route provider each time.

The following pseudocode describes how a client of the Chrome Media Router
(through its C++ API) would use it to initiate and control a media sharing
session.

#### Media Router API Example

```none
MediaRouter* media_router = MediaRouterImpl::GetInstance();// Find out what screens are capable of rendering, e.g.// www.youtube.comMediaSource youtube_src = MediaSource::ForPresentationUrl("http://www.youtube.com");// MyMediaSinksObserver should override MediaSinksObserver::OnSinksReceived to handle updates to// the list of screens compatible with youtube_srcMediaSinksObserver* my_observer = new MyMediaSinksObserver(youtube_src);media_router->RegisterObserver(my_observer);// Ask the user to pick a screen from the list passed to// my_observer and capture the sink_id (code not shown)// Request routing of media for that source.// |callback| is passed a MediaRouteResponse& that contains a MediaRoute result if successful.media_router->StartRouteRequest(youtube_src, sink_id, callback);// The MediaRoute can be used to post messages to the// sink. media_router->PostMessage(media_route.media_route_id,  "some data", "optional_extra_data_json");// The MediaRoute can be closed which signals the sink// to terminate any remote app or media streaming// session.media_router->CloseRoute(media_route.media_route_id);
```

The Media Router interacts with the component extension via a Mojo service, the
Media Router API, that exposes functionality whose implementation is delegated
to the extension.

#### Media Router API Mojo Interface

```none
// Interface for sending messages from the MediaRouter (MR)                                                                                                                               // to the Media Router Provider Manager (MRPM).                                                                                                                                           interface MediaRouterApiClient {                                                                                                                                                            // Signals the media route manager to route the media located                                                                                                                             // at |source_urn| to |sink_id|.                                                                                                                                                          RequestRoute(int64 request_id, string source, string sink_id);                                                                                                                                                                                                                                                                                                                      // Signals the media route manager to close the route specified by |route_id|.                                                                                                            CloseRoute(string route_id);                                                                                                                                                                                                                                                                                                                                                        // Signals the media route manager to start querying for sinks                                                                                                                            // capable of displaying |source|.                                                                                                                                                        AddMediaSinksQuery(string source);                                                                                                                                                                                                                                                                                                                                                  // Signals the media route manager to stop querying for sinks                                                                                                                             // capable of displaying |source|.                                                                                                                                                        RemoveMediaSinksQuery(string source);                                                                                                                                                                                                                                                                                                                                               // Sends |message| with optional |extra_info_json| via the media route                                                                                                                    // |media_route_id|.                                                                                                                                                                      // |extra_info_json| is an empty string if no extra info is provided.                                                                                                                     PostMessage(string media_route_id, string message, string extra_info_json);                                                                                                             };                                                                                                                                                                                                                                                                                                                                                                                  // Interface for sending messages from the MRPM to the MR.                                                                                                                                [Client=MediaRouterApiClient]                                                                                                                                                             interface MediaRouterApi {                                                                                                                                                                  // Called when the provider manager is ready.                                                                                                                                             OnProviderManagerReady(string extension_id);                                                                                                                                                                                                                                                                                                                                        // Called when the Media Route Manager receives a new list of sinks.                                                                                                                      OnSinksReceived(string source,                                                                                                                                                                            array<MediaSink> sinks,                                                                                                                                                                   array<MediaRoute> routes);                                                                                                                                                                                                                                                                                                                                           // Called after a MediaRoute is established.                                                                                                                                             OnRouteResponseReceived(int64 request_id, MediaRoute route);                                                                                                                                                                                                                                                                                                                        // Called when route establishment fails.                                                                                                                                                 OnRouteResponseError(int64 request_id, string error_text);                                                                                                                              };                     
```

## Media Router Component Extension

The component extension manages discovery of and network interaction with
individual media sinks. For the purposes of this discussion a sink is a
LAN-connected device that speaks the Cast or DIAL protocol, but in theory it
could be any other type of endpoint that supports media rendering and two-way
messaging. The extension consists of three types of components:

*   Media Route Providers: Each provider is a Javascript bundle that
            knows how to find and communicate with a specific type of media
            sink. It communicates with the media sink using HTTP/XHR or via
            device-specific network protocols (e.g., Cast Channel and Cast
            Streaming).
*   Media Route Provider Manager: This is responsible for dispatching
            requests from the Chrome Media Router to individual providers. It
            also registers providers on startup.
*   Mirroring Service: If a media source is requested that represents
            the tab or desktop contents, this service acts on the behalf of the
            application to initiate the mirroring session. This is handled
            internally to the component extension and is not exposed to the rest
            of the browser, it appears to be just another media route.

A component extension is used rather than implementing functionality directly
into the browser since remote display functionality is implemented by first and
third parties using a mix of open source and proprietary code, and must be
released on a schedule independently of Chrome (i.e. tied to specific hardware
release dates). We only plan to open source the DIAL media route provider.

The component extension is written in JavaScript using the Closure library and
will be available via two public channels; the default, “stable” extension will
be installed as an external component without the user needing to visit the Web
Store. Users may choose to install a pre-release “beta” extension via the Web
Store, which disables the stable version.

Initially Media Route Providers will be implemented for Cast and DIAL devices
with others to follow. Over time media route providers that do not rely on
proprietary protocols will be unbundled and included in the Chromium repository,
once script packaging and deployment issues are resolved. As an external
component, the extension is installed on the initial run of the browser. It is
built around an event page so it registers itself with the Media Router,
registers itself with discovery APIs to be notified of display availability, and
then suspends. The component extension will only be active when there are
applications with pending sink availability requests or media routes, or when
there is active network traffic between the extension and a media sink.

There are several modules to the extension that are loaded on-demand. The main
event page bundles are 238kb. Updates are independent of Chrome.

## Tab/Desktop Mirroring

Tab and desktop mirroring will request routing of a media source with URN like
urn:google:tab:3 representing tab contents. When the component extension
receives a request to route this source, the media route provider manager will
query route providers to enumerate sinks that can render streamed tab contents.
Once a sink is selected by the user, the mirroring service will create the
appropriate MediaStream using the chrome.tabCapture extension API. The
MediaStream will then be passed to a Cast Streaming or WebRTC session depending
on the preferred protocol of the selected sink. When the media route is
terminated, the associated streaming session and media capture are also
terminated. A similar approach will be used for desktop mirroring but using
chrome.desktopCapture instead.

## Presentation API

Media routing of Web content will primarily be done through the Presentation
API. Some media sinks (e.g. Cast) can render a subset of Web content natively,
or render an equivalent app experience (e.g., via DIAL). For generic Web
documents, we plan on rendering it in an offscreen WebContents and then using
the Tab Mirroring approach outlined above. The design of the offscreen rendering
capability will be added later to this document.

The Presentation API implementation in Blink will live in content/ and will
operate on the frame level. It will delegate the calls to the embedder's Media
Router implementation (Android Media Router / Chrome Media Router for Android /
Chrome, respectively) via a common PresentationServiceDelegate interface. A
draft Mojo interface follows (not yet complete):

#### PresentationService Mojo interface

```none
interface PresentationService {                                                                                                                                                               // Returns the last screen availability state if it’s changed since the last                                                                                                              // time the method was called. The client has to call this method again when                                                                                                              // handling the result (provided via Mojo callback) to get the next update                                                                                                                // about the availability status.                                                                                                                                                         // May start discovery of the presentation screens. The implementation might                                                                                                              // stop discovery once there are no active calls to GetScreenAvailability.                                                                                                                // |presentation_url| can be specified to help the implementation to filter                                                                                                               // out incompatible screens.                                                                                                                                                              GetScreenAvailability(string? presentation_url) => (bool available);                                                                                                                                                                                                                                                                                                                // Called when the frame no longer listens to the                                                                                                                                         // |availablechange| event.                                                                                                                                                               OnScreenAvailabilityListenerRemoved();                                                                                                                                                }; 
```

Here is how the presentation API will roughly map to Chrome Media Router API:

<table>
<tr>
<td> <b>Presentation API</b></td>
<td><b>Chrome Media Router </b></td>
</tr>
<tr>
<td> Adding onavailablechange listener</td>
<td> RegisterObserver(), with result propagated back to the RenderFrame / Presentation API.</td>
</tr>
<tr>
<td> startSession</td>
<td> Opens Media Router Dialog (via MediaRouterDialogController) -&gt; User action -&gt; StartRouteRequest()</td>
</tr>
<tr>
<td> joinSession</td>
<td> StartRouteRequest()</td>
</tr>
<tr>
<td> postMessage</td>
<td> PostMessage()</td>
</tr>
<tr>
<td> close</td>
<td> CloseRoute()</td>
</tr>
<tr>
<td> Adding onmessage listener</td>
<td> RegisterMessageObserver() (tentative)</td>
</tr>
<tr>
<td> Adding onstatechange listener</td>
<td> RegisterRouteStateChangeObserver() (tentative)</td>
</tr>
</table>

## Media Router Dialog

End user control of media routing is done through the Media Router Dialog. The
media router dialog is a constrained, tab modal dialog implemented using WebUI.
It auto-resizes to fit the currently rendered contents and appears in the top
center of the browser. The dialog supports a number of views, including a screen
selector, screen status, error/warning messages, and informational messages. To
avoid excess whitespace, the dialog appropriately resizes to the current view.

## The media router dialog is activated by clicking on the Cast icon, which is always available to the user. The Cast icon implements the action icon interface and appears on either the toolbar action menu (normally) or in the omnibox if there is an available Casting experience (a detected media sink).

## [<img alt="image"
src="/developers/design-documents/media-router/media_router_overflow.jpg">](/developers/design-documents/media-router/media_router_overflow.jpg)

## Clicking on the Cast icon brings up a menu of available media sinks that are compatible with the current content. For Web documents not using the Presentation API, these will include sinks that can render tab or desktop capture. For Web documents, it will include media sinks compatible with the URL requested to be presented through the Presentation API (once we are able to declare this URL through the API and proactively filter to compatible displays).

## [<img alt="image"
src="/developers/design-documents/media-router/media_router_screen_selector.jpg">](/developers/design-documents/media-router/media_router_screen_selector.jpg)

## Media Route providers may customize the appearance of the active media
activity and inject custom controls into the WebUI (subject to UX guidelines).
We are prototyping this approach using &lt;extensionview&gt;.

## [<img alt="image"
src="/developers/design-documents/media-router/media_router_universal_remote.jpg">](/developers/design-documents/media-router/media_router_universal_remote.jpg)

## The Media Router Dialog Controller keeps track of whether or not the initiator tab is currently open. When the tab is closed or navigated away to another URL, the media router dialog is closed.
## The HTML resources are built using [custom Polymer elements](https://www.polymer-project.org/). Data is passed between the WebUI and backend by calling the following:
## WebUI: chrome.send(functionName);
## Backend: web_ui()-&gt;CallJavascriptFunction(function_name, arg, ...);

## [<img alt="image"
src="/developers/design-documents/media-router/media_router_ui_diagram.jpg">](/developers/design-documents/media-router/media_router_ui_diagram.jpg)

## The WebUI and controller classes are located in the proposed location:
## chrome/browser/ui/webui/media_router
## The dialog resources, which include HTML, CSS, JS, and image files, are located in the proposed location:
## chrome/browser/resources/media_router

## We will use the &lt;extensionview&gt; HTML tag to embed the custom media controller UX. This will allow the component extension to flexibly customize and control the UX instead of having the functionality implemented directly into the browser. ExtensionView allows us to embed a page from the component extension into the Media Router WebUI. We will use the load API to take in the full media controller URL.

The extension will utilize
[chrome.runtime.\*](https://developer.chrome.com/extensions/runtime)
functionality to message between the controller embedded in the ExtensionView
and the extension itself.

## Offscreen Rendering

TODO(miu)

## Security

The entire project should be security reviewed from a holistic and architectural
perspective. Specific security-related aspects:

*   The Chrome Media Router will be designed to have a minimal
            processing of the URIs and messages passed through it (perhaps only
            checking for syntactic validity).
*   The Media Router Dialog will allow the MRPs to inject custom content
            into it, so for example, the inline controls for a game can differ
            from those for a movie. This content will be rendered out-of-process
            in an &lt;extensionview&gt; to prevent any escalation of privileges
            from compromised content.
*   The individual platform APIs used by the component extension MRPs
            (chrome.dial, chrome.mdns, chrome.cast.channel,
            chrome.cast.streaming) have been security reviewed previously.

## Team members

*   Mark Foltz
            &lt;[mfoltz@chromium.org](mailto:mfoltz@chromium.org)&gt;, TL
*   Jennifer Apacible
            &lt;[apacible@chromium.org](mailto:apacible@chromium.org)&gt;, WebUI
*   Derek Cheng
            &lt;[imcheng@chromium.org](mailto:imcheng@chromium.org)&gt;, Media
            Router
*   Haibin Lu
            &lt;[haibinlu@chromium.org](mailto:haibinlu@chromium.org)&gt;,
            Component extension
*   Kevin Marshall
            &lt;[kmarshall@chromium.org](mailto:kmarshall@chromium.org)&gt;,
            Mojo integration
*   Anton Vayvod
            &lt;[avayvod@chromium.org](mailto:avayvod@chromium.org)&gt;,
            Presentation API, Chrome for Android implementation
*   Yuri Wittala &lt;[miu@chromium.org](mailto:miu@chromium.org)&gt;,
            Offscreen rendering

## Code location

The patches to implement the Media Router have been developed in an internal
repository. They will be upstreamed into mainline Chromium with the primary code
location of

chrome/browser/media/router

for the media router, and other components living in appropriate locations
according to their type.
