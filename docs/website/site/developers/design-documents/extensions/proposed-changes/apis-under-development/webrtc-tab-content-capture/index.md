---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/proposed-changes
  - Proposed & Proposing New Changes
- - /developers/design-documents/extensions/proposed-changes/apis-under-development
  - API Proposals (New APIs Start Here)
page_name: webrtc-tab-content-capture
title: Tab Content Capture into MediaStreams
---

**\*\* This launched as of Chrome 31. Documentation:
https://developer.chrome.com/extensions/tabCapture \*\***

**Proposal Date**
**August 14th, 2012**

**Who is the primary contact for this API?**
**mfoltz@chromium.org**

**Who will be responsible for this API? (Team please, not an individual)**

**[mfoltz@](mailto:mfoltz@google.com)chromium.org, [keiger@google.com](mailto:keiger@google.com), [markdavidscott@google.com](mailto:markdavidscott@google.com)**

****Overview****
****The proposed APIs enable tab output to be captured as a media stream, and transmitted using WebRTC. Supporting APIs are also defined to notify and query the capture status for tabs.****
****Use cases****
****This API enables a special form of screencasting, but in which users are able to share the contents of a tab rather than sharing their entire desktop. This avoids a number of issues with desktop sharing, such as the presence of icons, taskbars, window chrome, pop-ups, and other elements that it is often not desirable (or in some cases, even embarassing - e.g. when a new mail notification or IM shows up for all to see) to share.****
****This API also enables the creation of WebRTC-based screencasting approaches, allowing the receiver of such content to run in a standard browser, entirely plug-in (and extension) free.****
****Versus desktop screencasting, this API enables multitasking use cases, since switching active tabs doesn’t impact the content that’s being transmitted.****
****Finally, this can later enable multi-screen applications that function by creating multiple tabs with one or more of those tabs being screencast for display on a different surface.****

****Do you know anyone else, internal or external, that is also interested in this API?****
****We are aware of 3 projects at Google that have an interest in this functionality, and believe that it will be externally interesting to many others.****

****Could this API be part of the web platform?****
****We believe that this functionality is useful both for extensions and the open web - but they are independently useful for each.****
****The extension approach to these APIs, proposed here, allows for the capture of the contents of any tab, without any updates needed to the content displayed in the tab. Thus, any existing content or application can be accommodated.****
****By contrast, a web platform approach (which we hope to pursue later after proving the approach here) is also useful, but needs significantly more restrictions around security and privacy (e.g. limiting access to elements that are owned by the site calling the API). This lends to a different set of use cases (e.g. the multiscreen cases).****
****We believe that extension API work and experience will help in making a better proposal for the open web in the relatively near future.****

****Do you expect this API to be fairly stable? How might it be extended or changed in the future?****
****Yes, we believe that this API will be fairly stable, since it is modelled after an existing API (getUserMedia) and has narrowly defined semantics.****
****Extensions or changes to incorporate other capture scopes - e.g.
window-level or desktop-level - is possible but should not result in significant
changes to the API.****

****If multiple extensions used this API at the same time, could they conflict with each others? If so, how do you propose to mitigate this problem?****
****Nothing inherently prevents multiple tabs from being captured independently,
but there may be implementation limits on how many tabs can be captured
simultaneously (and in our initial implementation, we believe that capturing 1
tab is sufficient). To address this, the proposed API includes a notification to
indicate when tab capture status changes so we can notify an application if a
tab is no longer being captured due to a newer conflicting action.****

****List every UI surface belonging to or potentially affected by your API:****
****The proposed APIs don’t expose any new UI surfaces to extensions. It is
proposed that we provide a visual indication to the user when a tab is being
captured, though the exact design for this is still under consideration. One
proposed approach is a color or animation on the tab itself or its favicon,
another is use of the existing mechanism used by WebRTC to indicate when camera
capture is active (this may be preferred for consistency).****

****Actions taken with extension APIs should be obviously attributable to an extension. Will users be able to tell when this new API is being used? How?****
****Per the above, we plan to have a visual indicator of the tab(s) being
captured. Hovering over this indicator will indicate which extension is
performing the capture.****

****How could this API be abused?****
****From a security/privacy perspective, since this API enables tab contents to be captured, it could be used to capture user browsing and to send that as a video stream to an external site. However, since we require the tabs permission, this is not substantively different from capturing the HTML/JS content of a tab (in fact, it’s less effective than the latter). It’s also no more sensitive than the tab snapshot API in terms of what it has access to.****
****From a security perspective, creating a video stream that’s targetted at an arbitrary destination coud be used in DDOS-type attacks, though this is not significantly more effective than sending a large number of HTTP requests or transmitting a large binary file.****
****From a performance perspective, too many simultaneous tabs being captured could impact performance. To mitigate this, we plan to limit the number of tabs that can be captured simultaneously (starting with a value of 1).****
****Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you could commit with your API (if you’ve got good ones, feel free to add more):****
****1) Per prior answer, scrape screen and send it to a malicious site.****
****2) Per prior answer, create “fake” video stream and use it for DDOS attack.****
****3) Per prior answer, create a bunch of bogus tab captures and send them somewhere meaningless, in an attempt to waste resources and trigger a crash.****
****What security UI or other mitigations do you propose to limit evilness made possible by this new API?****
****Per the above, we’re proposing a visual indicator for tabs that are being captured which identifies the extension performing the capture, and to ensure that the user is aware that capture is ongoing.****
****Alright Doctor, one last challenge:****
****Could a consumer of your API cause any permanent change to the user’s system using your API that would not be reversed when that consumer is removed from the system?****
****No; no permanent changes are possible as a result of the proposed API.****
****How would you implement your desired features if this API didn't exist?****
****Existing alternatives depending on building native applications or using dangerous NPAPI plug-ins with full system access.****
****Draft Manifest Changes****
****At this point in time, we believe that the tabs permission is sufficient.
We’re very open to input and advice on this, though!****

****Draft API spec****

****getTabMedia****
****chrome.experimental.capture.getTabMedia(integer tabId,****
****object options, function callback)****
****Captures the visible area of the tab with the given tabId. Extensions must have the host permission for the given tab as well as the “tabs” permission to use this method.****
****Parameters:****
****tabId (optional integer): The tabId of the tab to capture. Defaults to the active tab.****
****options (optional object): Configures the returned media stream.****
****video (boolean): Whether to return video (default true)****
****audio (boolean): Whether to return audio (default true)****

****callback (function): Called when the media stream is ready.****

****Callback function:****
****function (stream)****
****stream (optional LocalMediaStream): The LocalMediaStream that was created, or null if no stream was created. chrome.extension.lastError will be set if the stream could not be created (see below)****
****Errors:****

*   ****“Permission Denied” - The user did not grant permission for the
            tab to be captured****
*   ****“Busy” - Capture resources are occupied by an existing capture
            operation (i.e., we can’t capture two tabs at once)****
*   ****“Closed” - The tab was closed before the media stream could be
            created.****

****See below for details on the LocalMediaStream structure.****

****onTabCaptured****
****chrome.experimental.capture.onTabCaptured.addListener(****
****function(object captureInfo) { ... })****
****Event fired when the capture status of a tab changes. This allows extension authors to keep track of the capture status of tabs, to keep UI elements like page actions and infobars in sync.****
****Parameters:****
****captureInfo (object): Details of the change.****
****tabId (integer): The id of the tab whose status changed.****
****status (string): The new capture status of the tab, one of “requested”, “cancelled”, “pending”, “active”, “muted”, “stopped”****
****Status state diagram is attached.****

****getCapturedTabs****
****chrome.experimental.capture.getCapturedTabs(function callback)****
****Returns a list of tabs that have requested capture or are being captured, i.e. status != stopped and status != cancelled. This allows extensions to inform the user that there is an existing tab capture that would prevent a new tab capture from succeeding (or, to prevent redundant requests for the same tab).****
****Callback function:****
****function(&lt;array of CaptureInfo&gt; result) { ... }****
****CaptureInfo is as specified above.****
****Open questions****
****- Confirm permissions approach stated above****
****- Can a WebKit object be returned through an extension API?****
****- Control over aspect ratio, clipping, scaling of resulting media?****
