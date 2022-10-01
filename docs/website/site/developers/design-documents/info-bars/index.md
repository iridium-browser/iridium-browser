---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: info-bars
title: Info Bars
---

Info Bars are informational messages associated with a Tab. They are displayed
when something happens in the Tab that the user should be aware of, e.g. a
plugin has crashed, or the user can save a password they just entered. They are
designed to be non-modal so as not to interrupt the workflow.

#### Proposed Design

The TabContents object provides a simple API that clients can use to add
InfoBars to the presentation of that Tab. A client must have an object
implementing an InfoBarDelegate interface which provides some basic information
and callbacks for an InfoBar UI. It calls TabContents::AddInfoBar to add the
InfoBar to that Tab. The TabContents uses the NOTIFY_TAB_CONTENTS_INFOBAR_ADDED
notification (source = TabContents, details = the delegate) to broadcast the
change to the UI. There is also a symmetrical
NOTIFY_TAB_CONTENTS_INFOBAR_REMOVED.

On the UI side, Info Bars are hosted and owned by the window UI, more
specifically by a views::View subclass called InfoBarContainer that is hosted
within the BrowserView class.

There are several different types of InfoBar, and corresponding InfoBarDelegate
interfaces that provide common InfoBar appearance. The simplest is AlertInfoBar
and AlertInfoBarDelegate, which allows for the display of a string message and
optional icon. If the client does not want to implement this message it can
create an instance of SimpleAlertInfoBarDelegate, which is constructed with a
string and an icon, and deletes itself automatically when the InfoBar is closed.

Another common type is ConfirmInfoBar and ConfirmInfoBarDelegate, which allows
the client to ask the user a question and control buttons to receive their
response. The delegate interface notifies the client of the button chosen.

It is possible for the client to create custom InfoBars by subclassing the
InfoBar view and the InfoBarDelegate interface. InfoBarDelegate should be
platform independent, as it is used by TabContents which is platform
independent. The implementation of an InfoBar may vary significantly from
platform to platform.

Here is a diagram showing the relationship between the objects in a couple of
use cases:

[<img alt="image"
src="/developers/design-documents/info-bars/2infobars.png">](/developers/design-documents/info-bars/2infobars.png)

#### Add Flows

An instance of object X "x1" implements AlertInfoBarDelegate, and its lifetime
is managed by some other object. Here is the flow for its addition to
TabContents t:

t-&gt;AddInfoBar(x1);

This adds x1 to t's list of InfoBarDelegates, and broadcasts a
NOTIFY_TAB_CONTENTS_INFOBAR_ADDED that the InfoBarContainer in the associated
hosting BrowserView observes. InfoBarContainer calls x1's CreateInfoBar method
to construct a platform-specific View for the InfoBar and adds it to its list of
children, beginning an animation that causes the InfoBar to appear to slide in.

#### Remove Flows

The user clicks the close button on x1's InfoBar view. The InfoBar view
communicates, via its InfoBarContainer back through to the selected TabContents
telling it to remove the associated InfoBarDelegate. The TabContents removes the
delegate from its list and broadcasts NOTIFY_TAB_CONTENTS_INFOBAR_REMOVED. The
InfoBarContainer observes this message and begins an animation that causes the
associated InfoBar to appear to slide out. When the slide out animation is
completed, the InfoBar calls through to its delegate saying that the InfoBar is
about to be deleted. The InfoBarDelegate implementation can use this as an
opportunity to delete itself if it is not managed some other way.

#### Tab Switch Flows

When the user switches tabs, the BrowserView receives a TabSelectedAt function
call via its implementation of TabStripModelObserver. The BrowserView calls the
ChangeTabContents method on the InfoBarContainer, which causes it to reset its
selected TabContents value, and remove and add observers from the old and new
TabContents. It also uses this opportunity to update its visual state, removing
the child views associated with the old TabContents and constructing a new set
of InfoBar views for the newly selected TabContents. InfoBar views are designed
to be destroyed and re-created every time tab selection changes. When InfoBar
views are created in this mode, there is no animation as they are added and
removed.

Another important note: The InfoBarContainer's active TabContents is set to NULL
whenever a TabContents is detached from a window, much as the BrowserView's
TabContentsContainer's active TabContents is set to NULL. This is done because
when you drag the last tab out of a window, that window is destroyed, which
results in WM_NCDESTROY being sent to the frame and the focus manager to be shut
down. However the view hierarchy is not destroyed until OnFinalMessage. Because
both InfoBarContainer and TabContentsContainerView configure the focus manager,
this shutdown behavior means that both of these views may try and configure the
FocusManager after it has been destroyed.

A possible improvement to this situation would be to have the RootView delete
its view hierarchy when the window is destroyed, rather than waiting for
OnFinalMessage.

#### InfoBar Uniqueness

The InfoBarContainer supports showing of multiple child InfoBar at the same
time. This allows client code to queue up multiple notices that the user can
respond to at their leisure. Sometimes however client code may wish there to be
only one InfoBar for a specific type of notice. The InfoBarDelegate has an
Equals method that can be implemented to support this functionality. Consider
the AlertInfoBarDelegate, which is used to display a simple text string. The
TabContents will never allow two AlertInfoBarDelegates with the same text string
to be shown. AlertInfoBarDelegate overrides InfoBarDelegate::EqualsDelegate to
test equality by comparing text strings with another AlertInfoBarDelegate. This
ensures that multiple InfoBars with the same string are not added.

#### InfoBar Layout

The InfoBarContainer overrides views::View::GetPreferredSize to communicate to
the BrowserView its total size during Layout. When InfoBars are added or
removed, or their height changes due to animations, the InfoBarContainer tells
the BrowserView to update its Layout.

#### InfoBar Expiration

By default, InfoBars "expire" (automatically close) after a reload or a
navigation to the TabContents that they were opened within. Delegates can
override the ShouldExpire method to customize this behavior.

#### InfoBar View Ownership

InfoBar views are not owned by the view hierarchy. They automatically delete
themselves after they are removed from the View hierarchy. This is not done
immediately but after a return to the message loop since the stack of View
hierarchy changed notifications must be allowed to completely unwind.
