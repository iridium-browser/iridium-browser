---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: sync
title: Sync
---

**Design docs**

[Embassy](/developers/design-documents/sync/embassy) (using native model state
to make model association + shutdown robust against change processing failures)

[Life of a Sync Item
Update](https://docs.google.com/Doc?docid=0Aa_pnb1d4_gnZDUzOWp4bV80ZDhjeng1ZzU&hl=en)

[Syncable Service API](/developers/design-documents/sync/syncable-service-api)

[Sync Data Best
Practices](/developers/design-documents/sync/sync-data-best-practices)

[Client side
architecture](http://docs.google.com/Doc?docid=0Aa_pnb1d4_gnZDUzOWp4bV8yNXg2OW5oZGI&hl=en)

[A look at extensions / sync
interaction](http://docs.google.com/Doc?docid=0Aa_pnb1d4_gnZDUzOWp4bV8wZHNzdHdnczI&hl=en)

[Extensions quota service
proposal](http://docs.google.com/Doc?docid=0Aa_pnb1d4_gnZDUzOWp4bV8xZnhqaHJmZjM&hl=en)

[Unified Sync And Storage
proposal](/developers/design-documents/sync/unified-sync-and-storage-overview)

***What***

A library that implements the client side of our sync protocol, as well as the
Google server-side infrastructure to serve Google Chrome users and synchronize
data to their Google Account.

The goals for this protocol include:

*   Store/sync user's bookmarks in a way that may be extended to
            additional data types.
*   Allow the user to connect to the server from multiple clients
            simultaneously.
*   Changes the user makes in one client should be immediately reflected
            in other clients connected to the server.
*   Allow the user to make changes to her bookmarks even if the server
            is unreachable, such that changes made while offline are synced with
            the server at a later time.
*   Resolve bookmark data conflicts on the client without prompting the
            user.
*   Provide a web interface to access stored / synced bookmarks, likely
            via the docs.google.com doclist.
*   Standards compliant (e.g xmpp) client/server messaging.

*Where*

*The code lives in the Chromium repository, and is rooted at chrome/browser/sync.*

*chrome/browser/sync/*

Top level directory containing the entry point used by other chrome code, such
as profile.h.

For example, profile_sync_service.h/cc

*The rest of this stuff will live in 'browser_sync' namespace.*

*...sync/glue*

Stuff needed to embed the engine into Chrome.

For example,

*   http_bridge.h/cc - a bridge from the sync engine into Chrome's HTTP
            stack.
*   bookmark_model_worker.h/cc - makes sure the BookmarkModel is only
            used on the UI loop,
    even though the engine runs on it's own thread.

*...sync/engine*

The core sync engine parts and "business" logic, because it's business time.

*...sync/syncable*

The domain of all things syncable on the client. The definitions of what a
bookmark or a folder look like in sync-land, as well as the code responsible for
maintaining the local cache of the cloud state, are found in here. The code to
persist the local cache using sqlite is also found here.

*...sync/protocol*

Contains our protobuf specification of the messages sent between clients (the
browser) and sync servers.

**How**

To make the sync infrastructure scale to millions of users, we decided to
leverage existing XMPP-based Google Talk servers to give us "push" semantics,
rather than only depending on periodically polling for updates. This means when
a change occurs on one Google Chrome client, a part of the infrastructure
effectively sends a tiny XMPP message, like a chat message, to other actively
connected clients telling them to sync. To put that gain into perspective,
consider a 3 minute polling interval. 3 minutes is far from real time, or
"immediately" as our goal was stated. But already, at the very least, every 3
minutes every client needs to ask the server if anything changed. Even with just
one thousand users, we're already talking about a server having to handle a poll
request every 0.18 seconds on average (or roughly 5.6 queries per second). And
that's just when nothing is happening! Using XMPP pushes, the sync servers don't
need to waste cycles for no reason.