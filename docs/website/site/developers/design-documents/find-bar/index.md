---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: find-bar
title: Find Bar
---

#### Background

Google Chrome implements find for web pages using a "Find Bar" which appears
from the top right (or top left, depending on language) corner of the browser
window. The bar contains a text field for typing in search terms and some
controls for advancing find selection and closing the bar.

This is a proposal for lifetime management of the Find Bar. At present, the Find
Bar is created and owned by the WebContentsViewWin, and a unique one is
maintained per Tab. The FindBar is implemented as a WidgetWin subclass called
FindBarWin, which is a WS_CHILD of the frame. Whenever the selected tab changes,
if the newly selected Tab has a FindBar, it is made visible. Whenever a tab is
dragged out of a window and into another, the FindBarWin associated with that
Tab is re-parented to the new window.

To increase the flexibility of the TabContents/WebContents API and to enhance
the consistency in how bits of Chrome browser UI are maintained, we have been
moving the creation and ownership of bits of the Chrome browser UI out of the
TabContents and other objects and into the BrowserView. (This has already been
completed for the TabStrip, the browser toolbar, InfoBars, and some other
objects). The BrowserView class has become the class that represents the
contents of the window independent of frame type. For more information on this
read the [Browser Window document](/developers/design-documents/browser-window).
The general intent of this design approach is that the "View" in the MVC sense
(the hierarchy of objects implementing the BrowserWindow interface) is able to
initialize its state from the set of Model objects that back it, (Browser,
TabStripModel, TabContents, etc).

#### New FindBar Creation and Ownership

In light of this transition, it made sense that the Find Bar, as a part of the
Chrome Browser UI, be created and managed with the rest of the Browser UI. The
logical place for this is the BrowserView class, which creates and lays out the
toolbars, info bars, and other elements of the Browser Window. In this new
world, an instance of the FindBarWin class is created for every BrowserView at
the time it is added to the frame's Widget, and is maintained for the lifetime
of that BrowserView.

The WebContents object is extended to provide a basic interface for finding
(that drives the underlying RenderViewHost and WebKit) and also carries some
state about the last search string the user entered, whether a find session is
currently active for the tab, and the last set of Find results that were
returned from WebKit (in the form of a FindNotificationDetails object held as a
member variable).

<img alt="image"
src="/developers/design-documents/find-bar/FindBarTabSwitching.png">

The BrowserView implements the TabStripModelObserver interface, and so is
notified of tab switches, additions and removals. Whenever the selected tab
changes, the BrowserView tells its FindBarWin to change the WebContents it's
active for (if the newly selected Tab is a WebContents). FindBarWin code
initializes its UI from the state of the newly set WebContents so that the View
is brought up to date with the Model.

When it comes time to perform a find, the FindBar code (in FindBarWin and
FindBarView) drives the WebContents it is attached to directly by starting a
find session. The WebContents implements the RenderViewHostDelegate interface
which includes an OnFindReply method which is called when the renderer process
returns with find results. WebContents updates its last saved results and then
notifies interested observers via the NOTIFY_FIND_RESULT_AVAILABLE notification:

<img alt="image" src="/developers/design-documents/find-bar/FindBarGeneral.png">

The FindBarWin then updates its contained view (FindBarView) with the current
result values from the WebContents. This change means that both the UI and the
automation provider (used for UI tests for Find in Page) obtain their state from
the same notification.

#### Layout

The Find Bar is always positioned attached to the lowest row in the "visible
toolbar" of the Browser window. This means the Bookmark Bar if the Bookmark Bar
is persistently visible across tabs, or the main toolbar if the Bookmark Bar is
not persistently visible across tabs. This requires some work in Find Bar's
positioning logic to identify the state of the parent BrowserView so it can
position itself appropriately. With the old design, this was harder because the
FindBarWin object didn't have direct access to the BrowserView so had to do a
lot of work to figure out the bounds of various elements of the Browser UI and
position relative to them. With the new design, there is a direct 1:1 mapping
between the FindBarWin and its parent BrowserView. The BrowserView does Layout
for all of its child views (including the toolbars the FindBarWin must layout
relative to) and so under the new design the FindBarWin can just ask the
BrowserView what its bounding box should be.

#### Tab Drag & Drop

When a Tab is detached from one window and dropped into a different one, a few
things happen. In both the source and the target windows, a different tab is
selected. In the source window, another tab becomes active and the Find Bar for
that window is updated with the state of that tab, which might not have a find
session active in which case it'll be hidden. In the target window, the dragged
tab will cause that window's find bar to be initialized with the state of the
tab that was inserted and selected. Native child windows are never reparented
(and thus there are no focus manager transitions) under this design.

#### Prepopulate State

When the Find Bar appears, it is pre-filled with the string that was searched
for last in that Tab. If there is no previous text the Find Bar shows no string.
The ownership changes described here do not affect this behavior - every
WebContents maintains a new find_text_ member that tracks the last text the user
searched for. In the future, we want to change this such that if there is no
last find string for a specific tab but there is for another tab, then we
populate with that string instead. This proposal does not affect this behavior
at this time however.

#### Long Running Find Queries

Find queries in long documents can take some time to execute. Handling the
results from a find operation is contained entirely at the WebContents level.
When a query is initiated from a particular WebContents that WebContents
receives its results. When the WebContents receives the results it notifies
interested observers, which may include the Find Bar UI, if that Tab is active.
When the selected tab changes, the Find Bar code un-registers itself as an
observer of find result notifications sourced from the last selected WebContents
and registers itself as an observer of find result notification sourced from the
new one. This ensures that the Find Bar only receives update notifications from
the selected WebContents, even if a find operation is pending in multiple tabs.
When the user switches back to another tab, the Find Bar will be initialized
with the current result state of that tab and observe any future results in it
for as long as that tab remains selected.
