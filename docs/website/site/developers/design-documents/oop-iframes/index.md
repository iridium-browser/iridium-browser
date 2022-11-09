---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: oop-iframes
title: Out-of-Process iframes (OOPIFs)
---

This page provides an overview of Chromium's support for out-of-process iframes
(OOPIFs), which allow a child frame of a page to be rendered by a different
process than its parent frame. OOPIFs were motivated by security goals like the
[Site Isolation](/developers/design-documents/site-isolation) project, since
they allow a renderer process to be dedicated to a single web site, even when
cross-site iframes are present. OOPIFs are a general mechanism, though, and can
be used for other features than security (e.g., the &lt;webview&gt; tag in
Chrome Apps).

Supporting OOPIFs required a large architecture change to Chromium. At a high
level, the browser process now tracks subframes directly, and core parts of the
browser (e.g., painting, input events, navigation, etc) have been updated to
support OOPIFs. Many other features in Chromium now combine information from
frames in multiple processes when operating on a page, such as Accessibility or
Find-in-Page.

[TOC]

## Status

### Current Uses

The first use of OOPIFs was --isolate-extensions mode, which launched to Chrome
Stable in M56. This mode used OOPIFs to keep web iframes out of privileged
extension processes, which offered a second line of defense against malicious
web iframes that might try to compromise an extension process to gain access to
extension APIs. This mode also moved extension iframes on web pages into
extension processes.

We enabled OOPIFs in general web pages when launching Site Isolation (for all
sites) on desktop in M67. The first uses on Android launched in M77, to enable
Site Isolation for sites that users log into.

Beyond Site Isolation, OOPIFs have been used for a number of Chrome
architectural features. They have replaced the plugin infrastructure for
implementing GuestViews, such as the &lt;webview&gt; tag in Chrome Apps. They
have also replaced plugin code for MimeHandlerView, which is how PDFs and some
other data types are rendered.

For any issues observed in these uses of OOPIFs, please file a bug in the
component
[Internals&gt;Sandbox&gt;SiteIsolation](https://bugs.chromium.org/p/chromium/issues/list?q=component:Internals%3ESandbox%3ESiteIsolation).

### Project Resources

- [Task Dependency Diagram](http://csreis.github.io/oop-iframe-dependencies/)
  - A visualization of the bugs and tasks that need to be completed for each
    upcoming milestone. (Out of date.)

- [Internals&gt;Sandbox&gt;SiteIsolation](https://bugs.chromium.org/p/chromium/issues/list?q=component:Internals%3ESandbox%3ESiteIsolation)
  - All known bugs with OOPIFs and Site Isolation in general. The overall
    tracking bug for OOPIFs is <https://crbug.com/99379>, and the launch is
    tracked at <https://crbug.com/545200>.

- [Feature Update FAQ](https://docs.google.com/document/d/1Iqe_CzFVA6hyxe7h2bUKusxsjB6frXfdAYLerM3JjPo/edit?usp=sharing)
  - General information for how to update Chromium features to be compatible
    with OOPIFs.

- [Affected Feature List](https://docs.google.com/document/d/1dCR2aEoBJj_Yqcs6GuM7lUPr0gag77L5OSgDa8bebsI/edit?usp=sharing)
  - The set of Chromium features we know are affected by OOPIFs.

- [Site Isolation Summit
    talks](http://www.chromium.org/developers/design-documents/site-isolation#TOC-2015-Site-Isolation-Summit-Talks)
  - A set of slides and videos covering the changes to Chromium's architecture
    and how features can be updated.

- Mailing list:
[site-isolation-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/site-isolation-dev)

## Architecture Overview

### Frame Representation

Much of the logic in the [content
module](http://www.chromium.org/developers/content-module) has moved from being
tab-specific to frame-specific, since each frame may be rendered in different
processes over its lifetime.

#### Proxies

[<img alt="image"
src="/developers/design-documents/oop-iframes/Cross%20Process%20Tabs%20%28for%20posting%29.png"
height=329
width=400>](/developers/design-documents/oop-iframes/Cross%20Process%20Tabs%20%28for%20posting%29.png)

Documents that have references to each other (e.g., from the frame tree,
window.opener, or named windows) can interact via JavaScript. When the documents
are same-site, they must live in the same process to allow synchronous access.
When they are cross-site, they can live in different processes but need a way to
route messages to each other, for calls like postMessage. Same-site documents
with references to each other are grouped together using the SiteInstance class,
as described on the [Process
Models](/developers/design-documents/process-models) page.

To support cross-process interactions like postMessage on a document's
DOMWindow, Chromium must keep a proxy for the DOMWindow in each of the other
processes that can reach it. As shown in the diagram at right, this allows a
document from site A to find a proxy in its own process for a DOMWindow that is
currently active on site B. The proxy can then forward the postMessage call to
the browser and then to the correct document in the process for site B.

OOPIFs require each renderer process to keep track of proxy DOMWindows for all
reachable frames, both main frames and subframes.

#### Browser Process

Chromium keeps track of the full frame tree for each tab in the browser process.
WebContents hosts a tree of FrameTreeNode objects, mirroring the frame tree of
the current page. Each FrameTreeNode contains frame-specific information (e.g.,
the frame's name, origin, etc). Its RenderFrameHostManager is responsible for
cross-process navigations in the frame, and it supports replicating state and
routing messages from proxies in other processes to the active frame.

#### Renderer Process

In each renderer process, Chromium tracks of proxy DOMWindows for each reachable
frame, allowing JavaScript code to find frames and send messages to them. We try
to minimize the overhead for each of the proxy DOMWindows by not having a
document, widget, or full V8 context for them.

Frame-specific logic in the content module's RenderView and RenderViewHost
classes has moved into routable RenderFrame and RenderFrameHost classes. We have
one full RenderFrame (in some process) for every frame in a page, and we have a
corresponding but slimmed down blink::RemoteFrame as a placeholder in the other
processes that can reference it. These proxies are shown with dashed lines in
the diagram below, which depicts one BrowsingInstance (i.e., group of related
windows) with two tabs, containing two subframes each.

[<img alt="image"
src="/developers/design-documents/oop-iframes/Frame%20Trees%202015%20%28for%20posting%29.png">](/developers/design-documents/oop-iframes/Frame%20Trees%202015%20%28for%20posting%29.png)

#### Inside Blink

[<img alt="image"
src="/developers/design-documents/oop-iframes/Frame%20DOMWindow%20Document%202015%20%28for%20posting%29.png">](/developers/design-documents/oop-iframes/Frame%20DOMWindow%20Document%202015%20%28for%20posting%29.png)

Blink has LocalFrame and LocalDOMWindow classes for frames that are in-process,
and it has RemoteFrame and RemoteDOMWindow classes for the proxies that live in
other renderer processes. The remote classes have very little state: generally
only what is needed to service synchronous operations. A RemoteDOMWindow does
not have a Document object or a widget.
LocalFrame and RemoteFrame inherit from the Frame interface. While downcasts
from Frame to LocalFrame are possible, this will likely cause bugs with OOPIFs
unless extra care is taken. LocalFrame corresponds to WebLocalFrame (in the
public API) and content::RenderFrame, while RemoteFrame corresponds to
WebRemoteFrame.

Blink has the ability to swap any frame between the local and remote versions.
(This replaces the old "swapped out RenderViewHost" implementation that Chromium
used for cross-process navigations.)

It is worth noting that the
[&lt;webview&gt;](https://developer.chrome.com/apps/tags/webview) implementation
is being migrated to work on top of the new OOPIF infrastructure. For the most
part, Blink code will be able to treat a &lt;webview&gt; similar to an
&lt;iframe&gt;. However, there is one important difference: the parent frame of
an &lt;iframe&gt; is the document that contains the &lt;iframe&gt; element,
while the root frame of a &lt;webview&gt; has no parent and is itself a main
frame. It will likely live in a separate frame tree.

This support for OOPIFs and &lt;webview&gt; has several major implications for
Blink:

*   Page's main frame may be local **or** remote. There are a number of
            places that assume that main frame will always be local (e.g., the
            current drag and drop implementation always uses the main frame's
            event handler to perform hit-testing). These places will need to
            change.
*   More generally, any given frame in the frame tree may be remote.
            Thus, we are rewriting code like the web page serializer, which
            depended on iterating through all the frames in one process to
            generate the saved web page.
*   Layout and rendering were formerly coordinated by the main frame.
            This (and other code like this) needs to change to use a new
            concept: the local frame root. The local frame root for a given
            LocalFrame A is the highest LocalFrame that is a part of a
            contiguous LocalFrame subtree that includes frame A. In code form:
    LocalFrame\* localRootFor(LocalFrame\* frame) {
    LocalFrame\* local_root = frame;
    while (local_root && local_root-&gt;parent()-&gt;isLocalFrame())
    local_root = toLocalFrame(local_root-&gt;parent());
    return local_root;
    }
    These contiguous LocalFrame subtrees are important in Blink because they
    synchronously handle layout, painting, event routing, etc.

Some earlier information on the refactoring goals can be found in the
[FrameHandle design doc](/developers/design-documents/oop-iframes/framehandle),
however that is largely obsolete.

Note: We are attempting to minimize the memory requirements of RemoteFrames and
RemoteDOMWindows, because there will be many more than in Chromium before
OOPIFs. Before, the space required for swapped out RenderViewHosts was O(tabs \*
processes) within a BrowsingInstance, and most BrowsingInstances only contain 1
or 2 tabs. OOPIFs will require O(frames \* processes) space for proxies. This
could be much higher, because the number of frames can be much larger than the
number of tabs, and because the number of processes will increase based on
cross-site frames. Fortunately, RemoteFrames require far less memory than
LocalFrames, and not all cross-site iframes will require separate processes.

### Navigation

Chromium now has support for cross-process navigations within subframes. Rather
than letting the renderer process intercept the navigation and decide if the
browser process should handle it, all navigations are intercepted in the browser
process's network stack. If the navigation crosses a site boundary that requires
isolation (according to our Site Isolation policy), the browser process will
swap the frame's renderer process. This can be done because the browser process
knows the full frame tree, as described above. Until
[PlzNavigate](https://docs.google.com/document/d/1cSW8fpJIUnibQKU8TMwLE5VxYZPh4u4LNu_wtkok8UE/edit?usp=sharing)
launched, this was implemented using CrossSiteResourceHandler to transfer
navigations to a different process when needed.

A tab's session history also becomes more complicated when subframes may be
rendered by different processes. Originally, Blink took care of tracking the
frame tree in each HistoryItem in the renderer process, and the browser process
just tracked each back/forward entry using NavigationEntry. We removed the frame
tracking logic from Blink's HistoryController to keep track of each frame's
navigations in the browser process directly.

We also changed the representation of a tab's session history to more closely
match the HTML5 spec. Rather than cloning the frame tree for each HistoryItem,
we now keep track of each frame's session history separately in the browser
process, and we use a separate "joint session history" list for back and forward
navigations. Each entry in this list will have a tree of pointers to each
frame's corresponding session history item.

All details of navigation refactoring were described in this [design
document](https://docs.google.com/document/d/1Z-24Xmb5A00eMSfaBB5MQLTrp9LQ5hm3GKMfpFi44fQ/edit).

### Rendering

To render an iframe in a different process than its parent frame, the browser
process passes information back and forth between the renderer processes and
helps the GPU process composite the images together in the correct sizes and
locations. We use the
[Surfaces](http://www.chromium.org/developers/design-documents/chromium-graphics/surfaces)
implementation to maintain a set of textures from multiple renderer processes,
compositing them into a single output image. More details are available in this
[design
document](http://www.chromium.org/developers/design-documents/oop-iframes/oop-iframes-rendering).

### Input Events

Similar to rendering, we use the
[Surfaces](http://www.chromium.org/developers/design-documents/chromium-graphics/surfaces)
implementation to do hit testing in the browser process to deliver input events
directly to the intended frame's renderer process. We also manage focus in the
browser process to send keyboard events directly to the renderer process of the
focused frame. More details are available in this [design
document](https://docs.google.com/document/d/1RQUbxiK60Z7CZDcWeryqKeX-bewZzgsirjF2Dprfx3U/edit?usp=sharing).
