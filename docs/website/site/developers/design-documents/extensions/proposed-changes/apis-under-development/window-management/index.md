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
page_name: window-management
title: Window Management
---

Proposal Date

August 3rd, 2012
Who is the primary contact for this API?
sadrul@google.com
Who will be responsible for this API? (Team please, not an individual)

sadrul@, reveman@

Overview
Using this API, an extension will be able to (1) manage the positioning of the
window (including size, transform etc.), (2) perform window operations (e.g.
close, minimize etc.), (3) handle some input events for window management
purposes (e.g. install custom keyboard/mouse/gesture bindings etc.), and (4)
apply custom window decorations. This is primarily targeted for ChromeOS (to be
more specific, the primary target is the Ash desktop environment used in
ChromeOS. Ash is expected to be used on Windows too at some point, and the
expectation/hope is that this API will work there too). [Design
doc](https://docs.google.com/a/chromium.org/document/d/1szxvr95ymXNbJfpHfVmtcglr-F2bAgokxbZR_quVwY8/edit)

Use cases

*   **Useful especially for developers**: A lot of the developers use a
            window manager that allows managing the finer details of window
            management using some scripting language. On ChromeOS, using an app
            for doing the same would be very useful. (Specifically if/when ash
            is used on linux platforms: see
            <https://chromiumcodereview.appspot.com/10789018/>)
*   Easier to make and test changes: With JS, it would be a lot easier
            to implement and try out new window-management behaviour.

Do you know anyone else, internal or external, that is also interested in this
API?
There is interest in the ChromeOS test team to use this (since pyauto tests will
be updated to use extension API). There is also interesting among developers and
UX leads to experiment with this API.

Could this API be part of the web platform?
Unlikely.

Do you expect this API to be fairly stable? How might it be extended or changed
in the future?
The API can potentially include a lot of features. However, the plan is to start
small, adding support for a limited set of functionality, and then add to the
API. However, it should be fairly easy to keep the API backwards-compatible when
new API is added.

**If multiple extensions used this API at the same time, could they conflict with each others? If so, how do you propose to mitigate this problem?**
Multiple extensions can use the API at the same time. The reason for allowing
this is because a particular extension might be used for managing window
positioning, while at the same time, another extension might be used for
managing input-handling. However, if multiple active extensions use this API to
manage window positioning, then they can potentially conflict with each other
(although in the end, only one will 'win'). There is no plan to mitigate this
problem at this point.

List every UI surface belonging to or potentially affected by your API:
Initially all toplevel windows (e.g. browser windows, panels, modal windows).
This can perhaps be extended to also support notification windows, desktop
windows etc.

**Actions taken with extension APIs should be obviously attributable to an
extension. Will users be able to tell when this new API is being used? How?**

When the extension is installed/enabled, the user will be required to grant the
extension "wm" permission, so the user will have the knowledge that the
extension manages windows. But there will be no more visual cues in the UI after
that.

**Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you could commit with your API (if you’ve got good ones, feel free to add more):**
**1) The API allows closing windows. So an extension could potentially close a window/modal window immediately after it is opened. However, it will not be able to trigger the default action on the modal windows.**
**2) The extension could close/hide all notification windows before they are displayed. (if notification windows are exposed through the API. see above)**
**3) Add emacs-like key-bindings for window management (e.g. close window, move/resize window, focus window etc.)**
What security UI or other mitigations do you propose to limit evilness made possible by this new API?**
I do not think it is necessary to have a UI for mitigating the evilness.
However, it might make sense to add the extension icon to the active window's
title-bar.**

**Alright Doctor, one last challenge:**
**Could a consumer of your API cause any permanent change to the user’s system using your API that would not be reversed when that consumer is removed from the system?**
**The API does not provide access to the user's preferences, or other such data.
The only preference-data that can get modified by the API is the browser
window-position/size. Once the extension is disabled, the effects of the
extension should be completely reversible, possibly trivially.**

**How would you implement your desired features if this API didn't exist?**
**There unfortunately is no alternative. If a user wants to have different
window-management behaviour, then the user needs to change source and recompile
chrome.**

Draft API spec

**<table>**
**<tr>**
**<td>API Function (chrome.experimental.wm.XXX)</td>**
**<td>Details (asynchronous unless explicitly stated otherwise)</td>**
**</tr>**
**<tr>**
**<td>close</td>**
**<td>Closes a window.</td>**
**</tr>**
**<tr>**
**<td>transform</td>**
**<td>Applies transforms (e.g. rotation, scaling, translation) to a window. These transforms can be applied with an animation, and the app can decide how long the animation lasts.</td>**
**</tr>**
**<tr>**
**<td>opacity</td>**
**<td>Changes the opacity of a window. The opacity can be applied with an animation, and the app can decide how long the animation lasts.</td>**
**</tr>**
**<tr>**
**<td>windows</td>**
**<td>Gets a list of all the windows. (synchronous)</td>**
**</tr>**
**<tr>**
**<td>setWindowBounds</td>**
**<td>Sets the size of a window. (can be animated) This may (or may not?) override the resizability of a window.</td>**
**</tr>**
**<tr>**
**<td>stackWindowAbove(window1, window2)</td>**
**<td>Changes the stack-ordering of windows.</td>**
**</tr>**
**<tr>**
**<td>screenBounds</td>**
**<td>Gets the bounds of the screen (synchronous). The details for dealing with multi-monitor configuration TBD (will likely mirror how ash/aura/views deals with this).</td>**
**</tr>**
**<tr>**
**<td>minimize</td>**
**<td>Minimizes a window.</td>**
**</tr>**
**<tr>**
**<td>maximize</td>**
**<td>Maximizes a window. This may (or may not?) override the resizability of a window.</td>**
**</tr>**
**<tr>**
**<td>fullscreen</td>**
**<td>Makes the window fullscreen. This may (or may not?) override the resizability of a window.</td>**
**</tr>**
**<tr>**
**<td>\[feel free to add more here\]</td>**
**</tr>**
**</table>**

**<table>**
**<tr>**
**<td>Events</td>**
**<td>Details (synchronous unless explicitly stated otherwise)</td>**
**</tr>**
**<tr>**
**<td>onWindowCreated(AshWindow)</td>**
**<td>Triggered when a new window is created (but before the window is made visible.)</td>**
**</tr>**
**<tr>**
**<td>onWindowClosed(AshWindow)</td>**
**<td>Triggered when a window gets closed. This will be triggered whenever the window is closed (i.e. by the user, by the web-page, or by the WM app from the close API call)</td>**
**</tr>**
**<tr>**
**<td>onWindowShown(AshWindow)</td>**
**<td>Triggered when a window becomes visible.</td>**
**</tr>**
**<tr>**
**<td>onWindowHidden(AshWindow)</td>**
**<td>Triggered when a window is hidden.</td>**
**</tr>**
**<tr>**
**<td>onWindowActivated(AshWindow)</td>**
**<td>Triggered when an inactive window becomes active (e.g. user clicked on the window to make it active, or the previously active window was closed, or by other means).</td>**
**</tr>**
**<tr>**
**<td>onWindowInputEvent(AshWindow, Event, EventModifiers)</td>**
**<td>Triggered when an ‘interesting’ input event happens on the window (e.g. user presses Alt+Ctrl+Key, or does a four-finger swipe gesture, or clicks with Alt down etc.)</td>**
**</tr>**
**<tr>**
**<td>onWindowMoved(AshWindow, OldPosition)</td>**
**<td>Triggered when a window is moved. This will be triggered whenever the window is moved (i.e. by the user, by the web-page, or by the WM app from the setWindowBounds API call)</td>**
**</tr>**
**<tr>**
**<td>onWindowResized(AshWindow, OldSize)</td>**
**<td>\[similar to above\]</td>**
**</tr>**
**<tr>**
**<td>\[feel free to add more here\]</td>**
**</tr>**
**</table>**

**Some more details: a Window object is represented in the app as a dictionary with the following fields:**

*   **id: A unique id for the window for the duration of the session.**
*   **type: Window type (one of \[“modal”, “panel”, “normal”\]).**
*   **state: Window state (one of \[“normal”, “minimized”, “maximized”,
            “fullscreen”\])**
*   **title: The title of the window (this is typically user-visible;
            could be empty).**
*   **name: The name of the window (this is typically not visible to
            user; could be empty).**
*   **bounds: The location/size of the window.**
*   **active: Whether the window is currently active or not.**
*   **visible: Whether the window is currently visible or not.**

**All the callbacks (event callbacks, and callback to windows function call)
receive these details about the window(s).**

Open questions
