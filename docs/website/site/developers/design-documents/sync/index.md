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

[Model API](/developers/design-documents/sync/model-api)

[ClientTagBasedModelTypeProcessor](/developers/design-documents/sync/client-tag-based-model-type-processor)

[Protection against data override by legacy clients](/developers/design-documents/sync/old-sync-clients-data-override-protection)

[Sync diagnostics](/developers/design-documents/sync/diagnostics) (Needs update)

[Sync Data Best
Practices](/developers/design-documents/sync/sync-data-best-practices) (Needs update)

[Unified Sync And Storage
proposal](/developers/design-documents/sync/unified-sync-and-storage-overview)
 (Needs update)

***What***

A library that implements the client side of our sync protocol, as well as the
Google server-side infrastructure to serve Google Chrome users and synchronize
data to their Google Account.

The goals for this protocol include:

*   Store/sync different kinds of "data types", e.g. bookmarks.
*   Allow the user to connect to the server from multiple clients
            simultaneously.
*   Changes the user makes in one client should be immediately reflected
            in other clients connected to the server.
*   Allow the user to make changes to their data even if the server is
            unreachable, such that changes made while offline are synced with
            the server at a later time.
*   Resolve data conflicts on the client without prompting the user.

*Where*

TODO(crbug.com/1006699): Nowadays we have directories with reasonable meaning
and dependencies between them. Describe that here, just like the password
manager [folks](https://source.chromium.org/chromium/chromium/src/+/91bcc3658d7f81fde685523091ca94755419a708:components/password_manager/README.md).
