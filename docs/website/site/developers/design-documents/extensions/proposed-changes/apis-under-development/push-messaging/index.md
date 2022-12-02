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
page_name: push-messaging
title: Push Messaging
---

**You’re about to propose a new extension API, what a marvelous road you have
ahead of you. Before you venture forth, though, we’d like to get a sense for
your new API and offer you some guidance on your way. Fill out this proposal and
post it und**er [For Developers‎ &gt; ‎Design Documents‎ &gt; ‎Extensions‎ &gt;
‎Proposed & Proposing New Changes‎ &gt; ‎API
Proposals](/developers/design-documents/extensions/proposed-changes/apis-under-development)
**in the list of proposed APIs, then include a link in your Extension Review
request in go/extensionsreview.**

Proposal Date
2012/07/31
Who is the primary contact for this API?
dimich@

Who will be responsible for this API? (Team please, not an individual)

dcheng@, dimich@, petewil@

Overview
Web apps have the inherent limitation that they're not able to do much when they
aren't running. Today, extensions/apps use persistent background pages to do
interesting work even if they're not in the foreground; for example, they can
use it to poll the server for new emails and download them to a local store when
they're received. From a performance/efficiency standpoint, this is less than
desirable since it requires a mostly idle background page to just hang around
waiting for work. With new Chrome packaged apps and extension APIs that allow
background pages to be unloaded on idle, this will no longer be possible. As a
result, we believe it'd be useful to offer an API that would allow a remote
server to wake up a web app that's not currently running to do useful work. We
plan on implementing this API on top of the cache invalidation service that
Chrome is already using for Sync.
Use cases
Gmail/Google Docs: instead of polling the server for updates, they could
subscribe to push messages for their service. When there's actually an update,
then they can use the event to wake up their background page and update their
local store.
Google Talk: a push message would be triggered for the start of a new chat; this
would allow Google Talk to spawn a new chat window even when it's currently
unloaded.
Do you know anyone else, internal or external, that is also interested in this
API?
Google Talk is the most interested client, though it could be useful for Gmail
and Google Docs as well.
Could this API be part of the web platform?
It would be difficult to standardize this in the web platform because it depends
on many other server-side components such as:

*   Authentication
*   The invalidation mechanism used
*   The features supported

Do you expect this API to be fairly stable? How might it be extended or changed
in the future?
We expose only a small subset of the cache invalidation service that Chrome uses
internally. The main limitation today is that we have a list of hardcoded
subchannels that we register on behalf of extensions; in the future, it's
possible that we'd allow extensions to specify their own subchannel names. We'd
likely have to keep the hardcoded subchannels around forever though, to avoid
breaking extensions that use those subchannels.

**If multiple extensions used this API at the same time, could they conflict with each others? If so, how do you propose to mitigate this problem?**
They cannot conflict with each other, since we do not allow extensions to
specify any part of the object IDs registered on their behalf.
List every UI surface belonging to or potentially affected by your API:
No UI elements are involved.
**Actions taken with extension APIs should be obviously attributable to an
extension. Will users be able to tell when this new API is being used? How?**

Invalidations may trigger actions that result in UI elements being surfaced.
Since we add no new UI elements ourselves, and existing ones should be obviously
attributable, the existing UI should be sufficient.

How could this API be abused?
See below.
Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you
could commit with your API (if you’ve got good ones, feel free to add more):
1) The server could trigger a lot of invalidations. Presumably this would
eventually cause the app to get throttled on the server though...
2) An extension could claim it will manually acknowledge an invalidation and
never acknowledge it, allowing it to be woken up regularly.
3) An app writer could always add the pushMessaging to his permissions list,
whether or not he used it, causing the extension API to always register object
IDs for that extension even though they are not used.
What security UI or other mitigations do you propose to limit evilness made
possible by this new API?
To prevent extensions from being able to snoop on invalidations for other
users/extensions, we do not allow the extensions to actually specify their own
object IDs--we generate object IDs for an extension based on the signed-in user
+ the extension ID + the subchannel.

In addition, to reduce server load, we always acknowledge receipt of an
invalidation immediately when we receive it from the cache invalidation server.
We re-dispatch invalidations to the extension ourselves and use a backoff policy
to prevent a faulty/malicious app from causing its background page to be woken
up excessively.
**Alright Doctor, one last challenge:**
**Could a consumer of your API cause any permanent change to the user’s system using your API that would not be reversed when that consumer is removed from the system?**
**No. We unregister all object IDs registered on behalf of an extension when it is uninstalled.
How would you implement your desired features if this API didn't exist?
The user would have to make sure the app was constantly open for any
"background" functionality to work as expected. In the case of Google Talk, this
would require that the roster window always be opened.
Draft API spec
API reference: chrome.pushMessaging

Methods:

*getChannelId*

*chrome.pushMessaging.getChannelId(function callback)*

Retrieves the channel ID associated with an extension. This contains the
obfuscated user ID combined with the extension ID; the typical usage is for the
extension to send the channel ID to its app server to allow the app server to
trigger push messages.

*callback ( function )*

Called when channel ID has been retrieved.

*Callback function:*

The callback parameter should specify a function that looks like this:

*function(string channelID) {...};*

*channelID ( string )*

Contains the channel ID of the extension.

Events:

*onMessage*

*chrome.pushMessaging.onMessage.addListener(function(Message message) { ... });*

Fired when a push message is received.

*Listener parameters:*

*message ( Message )*

The data associated with the message.

Types:

*Message*

*( object )*

Stores data about a push message.

*subchannel ( number )*

The subchannel the push message was received for.

*payload ( string)*

The payload associated with the push message. Delivery of the payload is not
guaranteed; failure to deliver the payload will result in value being set to an
empty string.

*willAcknowledge ( function )*

An extension can call this to indicate that it will manually acknowledge the
receipt of the message. This is useful if the extension wants to do something
asynchronous in response to a push message and wants it to be re-delivered if it
isn't successfully handled for some reason.

*acknowledge ( function )*
Allows an extension which called willAcknowledge() to acknowledge receipt of the
push message.
Open questions
willAcknowledge() and acknowledge() may not be possible to implement.
