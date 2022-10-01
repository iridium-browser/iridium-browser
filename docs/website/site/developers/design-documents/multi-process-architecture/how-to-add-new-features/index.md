---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/multi-process-architecture
  - Multi-process Architecture
page_name: how-to-add-new-features
title: How to Add New Features (without bloating RenderView/RenderViewHost/WebContents)
---

**Problem**

Historically, new features (i.e. autofill to pick an example) have been added by
bolting on their code to RenderView in the renderer process, and RenderViewHost
in the browser process. If a feature was handled on the IO thread in the browser
process, then its IPC messages were dispatched in BrowserMessageFilter.
RenderViewHost would often dispatch the IPC message only to call WebContents
(through the RenderViewHostDelegate interface), which would then call to another
piece of code. All the IPC messages between the browser and renderer were
declared in a massive render_messages_internal.h. Touching each of these files
for every feature meant that the classes became bloated.

**Solution**

We have added helper classes and mechanisms for filtering IPC messages on each
of the above threads. This makes it easier to write self contained features.

**Renderer side:**

If you want to filter and send IPC messages, implement the RenderViewObserver
interface (content/public/renderer/render_view_observer.h). The
RenderViewObserver base class takes a RenderView and manages the object's
lifetime so that it's tied to RenderView (this is overridable). The class can
then filter and send IPC messages, and additionally gets notifications about
frame loading and closing, which many features need. As an example, see
ExtensionHelper (extensions/renderer/extension_helper.h).

If your feature has part of the code in WebKit, avoid having callbacks go
through WebViewClient interface so that we don't bloat it. Consider creating a
new WebKit interface that the WebKit code calls, and have the renderer side
class implement it. As an example, see WebAutoFillClient
(third_party/WebKit/public/WebAutoFillClient.h).

**Browser UI thread:**

The WebContentsObserver (content/public/browser/web_contents_observer.h)
interface allows objects on the UI thread to filter IPC messages and also gives
notifications about frame navigation. As an example, see TabHelper
(chrome/browser/extensions/tab_helper.h).

**Other browser threads:**

To filter and send IPC messages on other browser threads, such as IO/FILE etc,
implement BrowserMessageFilter interface
(content/public/browser/browser_message_filter.h). The RenderProcessHostImpl
object creates and adds the filters in its CreateMessageFilters function.

In general, if a feature has more than a few IPC messages, they should be moved
into a separate file (i.e. not be added to render_messages_internal.h). This
also helps with filtering messages on a thread other than the IO thread. As an
example, see content/common/database_messages.h. This allows their filter,
DatabaseMessageFilter, to easily send the messages to the file thread without
having to specify their IDs in multiple places.

void DatabaseMessageFilter::OverrideThreadForMessage( const IPC::Message&
message, BrowserThread::ID\* thread) { if (IPC_MESSAGE_CLASS(message) ==
DatabaseMsgStart) \*thread = BrowserThread::FILE; }
