---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/sync
  - Sync
page_name: sync-data-best-practices
title: Sync Data Best Practices (Needs update)
---

[The new sync API](/developers/design-documents/sync/syncable-service-api) uses
protobufs to communicate with Chrome services, which is nice because protobufs
were written to be robust against protocol changes (see [protobuf
docs](http://code.google.com/apis/protocolbuffers/docs/overview.html) for
details). However, once you start syncing data, changing your protobuf format
isn't completely painless; not only can sync users upgrade from one Chrome
version to another, but they may have different Chrome versions running at the
same time! Fortunately, there are some best practices to help make it easy:

*   Avoid using "explicit version numbers" in your protobuf; instead,
            have your code test for the existence of a field to determine what
            to do. Protobufs were written precisely to avoid version-specific
            logic, and testing for fields is more robust.
*   Adding a new field is the simplest case. Old clients will simply
            ignore the unknown new field, and when old clients send up new data,
            the sync backend preserves unknown fields so new clients can still
            use them.
*   Removing an old field is also pretty simple. Simply stop populating
            the field. Old clients will continue to use it, and new clients will
            ignore it. Once the Chrome stable version moves past the last
            version to use the old field, (where "moves past" means something
            like "the number of users using a version of Chrome older than the
            stable version drops below 1%) then you can remove the field
            entirely via a server-side map-reduce.
*   Avoid repurposing existing fields. Instead, add a new field for the
            new data and stop populating the old field, although continue to
            read it if the new field isn't present. Then, when the stable
            version of Chrome moves past the last version that creates old data,
            add code to migrate the old field to the new field (preferably on
            the server side, as adding substantial migration code might cause
            sync traffic spikes on version upgrades). Finally, once the stable
            version of Chrome becomes the first Chrome version with the
            migration code, you can remove data in the old field.
