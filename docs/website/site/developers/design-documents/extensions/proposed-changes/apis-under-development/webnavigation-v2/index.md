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
page_name: webnavigation-v2
title: webNavigation v2
---

**Proposal Date**

*2012-06-14*

Who is the primary contact for this API?
*jochen@*

Who will be responsible for this API? (Team please, not an individual)

*jochen@, mpcomplete@*

Overview
*Now that the webNavigation API is released for a while, I learned about certain
short-comings I'd like to fix. This proposal captures the required additions /
changes to the current webNavigation API.*

Use cases

*   Currently, if a page is pre-rendered, and you navigate to that page,
            the navigation is aborted, and the page magically is there (without
            further visible webNavigation events). The next version of the
            webNavigation API should indicate when a prerendered page is swapped
            in.
*   *If a navigation crosses a process boundary, the tuple (tab ID,
            frame ID) is no longer unique, but there might be two frames with
            the same ID navigating for the same tab, but in two different
            processes. Also, the frame ID of the main frame might change during
            a provisional load. It seems that we can't hide these details of our
            process model (without considerable effort). The next version should
            allow for updating frame IDs during provisional loads, and have a
            truly unique identifier for frames.*

Do you know anyone else, internal or external, that is also interested in this
API?
*There were bug reports filed about this: <http://crbug.com/116643>
<http://crbug.com/117043>*

Could this API be part of the web platform?
*No, it's just an extension of an existing API*

Do you expect this API to be fairly stable? How might it be extended or changed
in the future?
When we add new fancy navigation features (e.g. similar to prerendering), the
API might need to be extended again.

**If multiple extensions used this API at the same time, could they conflict with each others? If so, how do you propose to mitigate this problem?**
No*
List every UI surface belonging to or potentially affected by your API:
None
**Actions taken with extension APIs should be obviously attributable to an
extension. Will users be able to tell when this new API is being used? How?**

*The API is only observing events, there should be no observable impact.*

**How could this API be abused?**
**The API can be used to monitor each and every navigation, but that's already possible with v1.**
**Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you could commit with your API (if you’ve got good ones, feel free to add more):**
**1) observe each and every navigation, send it to some 3rd-party server**
**2) ...**
**3) profit**
What security UI or other mitigations do you propose to limit evilness made possible by this new API?**
*The webNavigation API already triggers a warning at install time.*
Alright Doctor, one last challenge:**
Could a consumer of your API cause any permanent change to the user’s system using your API that would not be reversed when that consumer is removed from the system?**
No
How would you implement your desired features if this API didn't exist?
*You could guess that a page is being prerendered, or try to work around
duplicate frame IDs.*
Draft API spec
All existing events and methods that take or return a frame ID will in addition
take a process ID. A frame is then uniquely identified by the tripple (tab id,
process id, frame id).

The onCommitted signal's documentation is updated to state that the frame ID and
process ID might have changed since the onBeforeNavigate signal.

Furthermore, this new event is introduced. :

**onReplacedTab**

chrome.webNavigation.onReplacedTab.addListener(function (object details) { ...
});

Fired when a navigation is aborted, and instead a prerendered tab is swapped in.

Listener parameters

details (object)

sourceTabId (integer)

The ID of the tab being replaced

tabId (integer)

The ID of the tab replacing the sourceTabId

timeStamp (number)

The time when the browser was about to replace the tab in milliseconds since the
epoch.

Open questions
The proposed changes are "breaking changes" as an extension using v1 of the API
will still experience the above referenced bugs. Is that ok?
