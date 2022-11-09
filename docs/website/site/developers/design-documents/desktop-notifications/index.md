---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: desktop-notifications
title: Desktop Notifications
---

**See also: [API
Specification](/developers/design-documents/desktop-notifications/api-specification)
for notifications**

**Design for desktop notifications**
The Desktop Notifications API allows script to request a small toast to appear
on the desktop of the user. The contents of the toast can either be specified as
iconUrl/title/text strings, or as a URL to a remote resource which contains the
contents of the toast.
In Chromium/WebKit, the API is provided to scripts in WebCore by injecting into
WebKit (via the Chrome/ChromeClient objects) an implementation of a
NotificationPresenter interface, which contains methods to show and cancel
toasts, as well as to check and request permission to show toasts. These calls
flow via IPC from the renderer process to the browser process, where the toasts
are actually created on the screen using a small HtmlView.
**Event feedback**
Several events flow in the opposite direction: when a notification is displayed
(it may not be immediate if the screen is cluttered), closed, or encounters an
error, an event is fired which can be listened for by javascript. These events
are detected in the browser process and flow back to the renderer instance which
owns the notification object. Tracking is done by notification-id counters in
each renderer process, and a map within the browser process which associates a
toast on the screen with the corresponding renderer-process-id and
notification-id. Since the renderer process chooses the id, the IPC involved can
be asynchronous.
**Lifetime considerations**
If a page which is showing a desktop notification is closed, the toast should
not be automatically removed from the screen. However, as the javascript
Notification object has gone out of scope, Chromium must not attempt to invoke
events on it. The NotificationPresenter interface supports this via a
notificationObjectDestroyed() method which must be called from WebCore when a
Notification object is destroyed.
For this reason, the renderer process's NotificationPresenter keeps a map of its
notification-id values to Notification object pointers; if a Notification
pointer goes out of scope, the NotificationPresenter is called and removes that
pointer from the map. If the browser subsequently calls with an event on that id
it will be ignored.

If a renderer process crashes, the notifications will persist in the browser
process and on the screen, but events intended to reach that renderer process
will be filtered out in the browser when normal process-lookup fails.
**Permissions**
Desktop notification permission is assigned on an origin basis. Chromium stores
two PrefLists in the user's profile to accomplish this: OriginsAllowed and
OriginsDenied. A script may request permission on a given origin through a JS
API which is routed through the NotificationPresenter interface, into the
browser process via IPC, and results in an InfoBar offering "Allow" or "Deny".
The origin is then stored in the correct PrefList.
When a notification is to be shown, permission must first be checked: this is
handled in the same way, invoking the NotificationPresenter in the renderer,
jumping via synchronous IPC to the browser, checking the PrefLists and
returning.
**Workers' notifications**
The same API is available to worker processes and mirrors the behavior for
renderers, with IPC messages using a "worker" flag for correct routing of event
feedback.
**Mac/Linux integration**
On Mac and Linux, we considered to use 3rd party notification libraries, but
have determined that having a unified HTML-capable interface is a better course,
so Chrome will render HTML notifications on these platforms as well.
**Deployment plan**
Presently the desktop notifications API will be disabled by default, but can be
enabled with the command-line switch --enable-desktop-notifications.
*Browser Process Object Flow*
==============
RenderViewHost
v (receives IPC from renderer)
v
DesktopNotificationService
v (object owned by ProfileImpl which handles requests for notifications, also
"upcasts" text notification to HTML based on template)
v
v posts a task
v
NotificationUIManager
v (global object owned by BrowserProcess which manages the space on the desktop
for notification balloons)
v
BalloonCollection
v (set of balloons on the screen, handles animations as balloons are added and
removed)
v
Balloon
(mini window, draws notification chrome and contains HTML contents view,
receives "dismiss" click from the user)
*Renderer Process Object Flow*
==============
Calling from JavaScript to create a notification:
NotificationCenter object in WebCore
v (returned by DOMWindow::webkitNotifications()).
v
v V8 bindings
v
WebKit::NotificationPresenterImpl
v (WebKit API object which adapts between WebCore::NotificationPresenter
interface and matching WebKit API interface)
v
RendererNotificationPresenter
(implements WebKit::WebNotificationPresenter interface, object owned by
WebViewDelegate implementation; sends IPC messages to browser, allocates IDs,
stores map of ID-&gt;Notification objects)
Calling back into JavaScript to relay notification events:
RendererNotificationPresenter
v (receives IPC message from browser process regarding an event)
v
v (post task onto main thread: lookup in notification-id-&gt;Notification object
map, discard if invalid)
v
WebKit::WebNotification (PIMPL wrapper in WebKit API for WebCore::Notification
object)
v
v (invoke event listener)
v
Notification in WebCore
