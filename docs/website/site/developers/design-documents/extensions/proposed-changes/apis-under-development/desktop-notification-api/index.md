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
page_name: desktop-notification-api
title: Desktop Notification API
---

Proposal Date

January 2013.
Who is the primary contact for this API?

Justin DeWitt (dewittj)

Who will be responsible for this API? (Team please, not an individual)

The Extensions team will own the API, but the Notifications Frontend team will
own the implementation (i.e., the actual desktop notification code) behind it.

Overview

This API provides rich desktop notifications to Chrome apps and extensions.

Today, Chrome implements an [outdated version of the W3C spec for HTML5 desktop
notifications](http://www.chromium.org/developers/design-documents/desktop-notifications/api-specification).
The functionality proposed here is generally similar, but significantly more
powerful. We have yet addressed the question whether this API will replace,
enhance, or remain entirely independent of any Chrome implementation of the W3C
spec.

Use cases

Desktop notifications provide a convenient, non-intrusive method of notifying
users of events, particularly when those events originate from a background web
app. Today, a web app has unsatisfactory options to notify a user of important
information:

1.  Raise a JavaScript alert, which might or might not bring a
            backgrounded window to the foreground. In the best case, where the
            user does see the alert, the result is text-only and modal.
2.  Change the window title and hope that enough of the window
            (typically the tab) is prominent. An example is Gmail's "Inbox (1)"
            title, originally designed to exploit the real estate available on a
            background tab in a tabbed browser.
3.  Try to play a sound.
4.  Develop and support a native notifier application that sits in the
            taskbar.

An enumeration of specific use cases isn't needed. Today's web apps are likely
to be one of many of a user's open browser tabs, only one of which is active at
a time, any of which might have legitimate reasons to present urgent information
to the user (new mail, new instant message, completion or failure of
long-running process, scheduled reminders).

Do you know anyone else, internal or external, that is also interested in this
API?

Yes.
Could this API be part of the web platform?

Yes.

Do you expect this API to be fairly stable? How might it be extended or changed
in the future?

We expect many more notification layouts in the near future, as well as more
structured data supporting those layouts. Our hope is to design a flexible API
that will accommodate those changes without breaking the API or substantially
altering its shape.

**If multiple extensions used this API at the same time, could they conflict with each other? If so, how do you propose to mitigate this problem?**
Our "message center," which is the container that will display notifications,
will intelligently manage the stream of notifications produced by API consumers.
It will also give control to users in the form of per-app mute, a global on/off
switch, and quiet hours. As no app or extension should be able to control these
settings, they won't be part of the API.
List every UI surface belonging to or potentially affected by your API:

We'll separately mail out our message-center mocks. For purposes of this
document, a notification area or taskbar item in the host OS will expand to
display a rectangular message center. We don't expect the UI of Chrome proper to
change.
**Actions taken with extension APIs should be obviously attributable to an
extension. Will users be able to tell when this new API is being used? How?**

Users can currently attribute a notification to a particular extension by
right-clicking on it. This names the extension and gives the user the ability to
disable notifications from the extension.

How could this API be abused?

*   Idle detection. Raise a notification and see whether the user
            dismisses it.
*   Create a large number of notifications in a short period of time.
*   Create a large number of notifications over a long period of time.
*   Raise a notification scaring the user ("Warning, system compromised,
            unplug from wall immediately").
*   Raise a notification embarrassing the user during a presentation
            ("Test results received from Fungus-B-Gone Clinic"). We have
            implemented a mode "Quiet Mode" that prevents new notifications from
            being displayed on screen to mitigate this.
*   Raise a notification demanding credentials, then steal them. Because
            the notification appears to originate outside Chrome, it might carry
            more clout than a typical evil website's phishing plea.
*   Impersonate another app or extension.

Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you
could commit with your API (if you’ve got good ones, feel free to add more):

1.  Pretend to be Gmail. Use the Gmail icon. Put up a notification that
            sends the user to a malware site that steals Gmail credentials.
2.  Send just enough inane notifications that the user gives up and
            disables notifications entirely. The user is known to be an oncall
            ops person at a targeted company. Destroy the company's servers and
            use the delay caused by the lost notifications to escape the
            country.
3.  Pretend to be an instant-message service. Send "Becky: what's our
            bank account #? I don't have it handy. \[Reply\]" All you need is
            one husband in a hurry whose wife is named Becky for this attack to
            be profitable. (This is really the same as #1; I'm out of ideas.)

What security UI or other mitigations do you propose to limit evilness made
possible by this new API?
For the impersonation case, see answer to the "obviously attributable" question.
For local DOSing, see answer to the conflicting extensions question.
**Alright Doctor, one last challenge:**
**Could a consumer of your API cause any permanent change to the user’s system
using your API that would not be reversed when that consumer is removed from the
system?**

No.

How would you implement your desired features if this API didn't exist?

We'd use the existing desktop notification API, which unfortunately is reducing
its feature set (no HTML layout option). Or we'd advise applications to use
something like Growl, which wouldn't work on ChromeOS.

**Draft Manifest Changes**

A "notifications" permission, which is merged with the [existing
**notifications**
permission](http://developer.chrome.com/extensions/declare_permissions.html)
that enables HTML5 desktop notifications without a runtime permissions check.

**Draft API spec**
**Note that this API approximately matches what's in the [Chromium repository](https://code.google.com/searchframe#OAMlx_jo-ck/src/chrome/common/extensions/api/experimental_notification.idl&exact_package=chromium&l=5). The source code is authoritative.**

```none
namespace notifications {  enum TemplateType {    // icon, title, message    simple,    // icon, title, message, expandedMessage, up to two buttons    basic,    // icon, title, message, expandedMessage, image, up to two buttons    image,    // icon, title, message, items, up to two buttons    list  };  dictionary NotificationItem {    // Title of one item of a list notification.    DOMString title;    // Additional details about this item.    DOMString message;  };  dictionary NotificationButton {    DOMString title;    DOMString? iconUrl;  };  dictionary NotificationOptions {    // Which type of notification to display.    TemplateType type;    // Sender's avatar, app icon, or a thumbnail for image notifications.    DOMString iconUrl;    // Title of the notification (e.g. sender name for email).    DOMString title;    // Main notification content.    DOMString message;    // Priority ranges from -2 to 2. -2 is lowest priority. 2 is highest. Zero    // is default.    long? priority;    // A timestamp associated with the notification, in milliseconds past the    // epoch (e.g. Date.now() + n).  Currently unimplemented, but planned for    // a forthcoming release.    double? eventTime;    // Text and icons of the notification action button.    NotificationButton[]? buttons;    // Image thumbnail for image-type notifications    DOMString? imageUrl;    // Items for multi-item notifications.    NotificationItem[]? items;  };  callback CreateCallback = void (DOMString notificationId);  callback UpdateCallback = void (boolean wasUpdated);  callback DeleteCallback = void (boolean wasDeleted);  interface Functions {    // Creates and displays a notification having the contents in |options|,    // identified by the id |notificationId|. If |notificationId| is empty,    // |create| generates an id. If |notificationId| matches an existing    // notification, |create| first deletes that notification before proceeding    // with the create operation. |callback| returns the notification id (either    // supplied or generated) that represents the created notification.    static void create(DOMString notificationId,                       NotificationOptions options,                       CreateCallback callback);    // Updates an existing notification having the id |notificationId| and the    // options |options|. |callback| indicates whether a matching notification    // existed.    static void update(DOMString notificationId,                       NotificationOptions options,                       UpdateCallback callback);    // Given a |notificationId| returned by the |create| method, clears the    // corresponding notification. |callback| indicates whether a matching    // notification existed.    static void clear(DOMString notificationId, DeleteCallback callback);  };  interface Events {    // The notification closed, either by the system or by user action.    static void onClosed(DOMString notificationId, boolean byUser);    // The user clicked in a non-button area of the notification.    static void onClicked(DOMString notificationId);    // The user pressed a button in the notification.    static void onButtonClicked(DOMString notificationId, long buttonIndex);  };};  
```
