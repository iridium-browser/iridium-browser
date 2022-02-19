---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/sync
  - Sync
page_name: syncable-service-api
title: Syncable Service API
---

### EVERYTHING BELOW IS OUTDATED! Go here instead: <https://chromium.googlesource.com/chromium/src/+/HEAD/docs/sync/model_api.md>

### <http://go/syncapi> (also, see <http://go/syncapipreso> \[Googlers only, but a PDF of the presentation is attached to this document\])

#### Background

As sync continues to expand to handle more data types, the need for an easier
and more scalable way for Chrome services to interact with sync is becoming more
apparent. This document proposes a new API with the following goals:

*   A Chrome service should be able to use this API to sync its data
            without having to know the details of the sync code base and
            protocol.
*   This API should encourage new Chrome services to define its
            interactions with sync up front.
*   The "business logic" for syncing a service's data should live in the
            service itself.

#### SyncableService Interface and related classes

First, the message types that Chrome services will use to talk to sync (and vice
versa):

```none
enum SyncType { BOOKMARKS, PREFERENCES, ... };
class SyncData {
 public:
  SyncType sync_type();
  // The service-specific data is in a [protobuf extension](http://code.google.com/apis/protocolbuffers/docs/proto.html#extensions) of [EntitySpecifics](https://cs.chromium.org/chromium/src/components/sync/protocol/sync.proto?l=69)
  // which corresponds to sync_type().
  [EntitySpecifics](https://cs.chromium.org/chromium/src/components/sync/protocol/sync.proto?l=69) specifics();
};
class SyncChange {
 public:
  enum ChangeType { ADD, UPDATE, DELETE };
  ChangeType change_type();
  // If change_type is ADD or UPDATE, this contains the new data; if
  // change_type is DELETE, this contains the data right before deletion.
  SyncData sync_data();
};
SyncData CreateSyncData(SyncType sync_type, [EntitySpecifics](https://cs.chromium.org/chromium/src/components/sync/protocol/sync.proto?l=69) specifics);
// |tag| should be a unique data-type specific ID that can be used to
// prevent duplicate entries for the same object, e.g. for extensions,
// it would be the extension ID.
SyncChange CreateSyncChange(SyncChange::ChangeType type, SyncType sync_type, [EntitySpecifics](https://cs.chromium.org/chromium/src/components/sync/protocol/sync.proto?l=69) specifics, string tag);
```

Both `SyncData` and `SyncChange` are immutable, thread-safe, and cheaply
copyable/assignable.

Then, the `SyncChangeProcessor` interface, which is implemented by sync classes
(and also by local services—see below):

```none
interface SyncChangeProcessor {
  void ProcessSyncChanges(SyncType type, vector<SyncChange> changes);
};
```

When `ProcessSyncChanges` is called on a sync class, it processes the given list
of sync changes for the given type, which eventually gets propagated to the sync
server and then to other clients.

Finally, the `SyncableService` interface, which Chrome services should
implement:

```none
interface SyncableService inherits SyncChangeProcessor {
  // Inherited from SyncChangeProcessor.
  void ProcessSyncChanges(SyncType type, vector<SyncChange> changes);
  vector<SyncData> GetAllSyncData(SyncType type);
  bool MergeDataAndStartSyncing(SyncType type, vector<SyncData> sync_data_to_merge, SyncChangeProcessor sync_change_processor);
  void StopSyncing(SyncType type);
};
interface [ProfileKeyedService](http://codesearch.google.com/codesearch/p#OAMlx_jo-ck/src/chrome/browser/profiles/profile_keyed_service.h&exact_package=chromium) inherits SyncableService {
  ...
};
```

Some Chrome services map to multiple sync data types, hence the need for a
`SyncType` parameter to all `SyncableService` methods (e.g., `ExtensionService`
handles both extensions and apps, and `WebDataService` handles both autofill and
autofill profiles).

A `SyncableService` is also a `SyncChangeProcessor`. When `ProcessSyncChanges`
is called on a Chrome service, it must process the given list of sync changes
for the given type (which came from other clients via the sync server).

When `GetAllSyncData` is called, it must return a list of `SyncData`s which
represents the currently known local data for the given type. `GetAllSyncData`
may be called at any time, even before syncing starts; it should always return
an accurate representation of the local state, as it may be used for diagnostics
and verification, e.g. sync may call `GetAllSyncData` after it calls `StartSync`
and verify that the local state is equivalent to the remote state.

When `MergeDataAndStartSyncing` is called on a Chrome service, it must do an
"initial sync": it must merge `sync_data_to_merge` with its local data and/or
push changes to `sync_change_processor` such that, after the function returns,
the local sync state (i.e., the return value of `GetAllSyncData`) matches the
state represented by `sync_data_to_merge` plus any changes pushed to
`sync_change_processor`. It should also store the given `sync_change_processor`
reference and use it to send changes for `type` until `StopSyncing` is called
for `type`.

A Chrome service can assume that all `SyncableService` methods are called on the
same thread, usually its "native" thread.

`SyncChange`s can be assumed to be processed atomically by sync, i.e. a Chrome
service will never receive a "partial" `SyncChange.`

`SyncableService` should be a super-interface of
`[ProfileKeyedService](http://codesearch.google.com/codesearch/p#OAMlx_jo-ck/src/chrome/browser/profiles/profile_keyed_service.h&exact_package=chromium)`.
Most Chrome services should implement `SyncableService`, either directly or via
`ProfileKeyedService`.

#### How to write a new sync-aware Chrome service

1.  **Important**: Loop in the sync team early! Send an e-mail to
            chrome-sync-dev@google.com explaining what you want to sync. We're
            happy to help.
2.  You will need to add your type to the `SyncType` enum as well define
            a protobuf to use for your data and make it an
            [extension](http://code.google.com/apis/protocolbuffers/docs/proto.html#extensions)
            of
            [EntitySpecifics](http://codesearch.google.com/codesearch/p#OAMlx_jo-ck/src/chrome/browser/sync/protocol/sync.proto&exact_package=chromium).
3.  Except for some special cases, you will probably need to make your
            service inherit from `ProfileKeyedService`. Make sure your service
            has one thread (not necessarily the UI thread) that it can use to
            communicate with sync.
4.  Write unit tests to:
    1.  Make sure `GetAllSyncData` accurately reflects your service's
                state.
    2.  Make sure that `ProcessSyncChanges` handles `SyncChanges`
                intelligently (even erroneous ones).
    3.  Make sure that calling `MergeDataAndStartSyncing` on an instance
                of your service with another instance's sync state and a
                reference to that instance as a `SyncChangeProcessor` brings
                both instances to the same state. In other words, the following
                should hold:

        ```none
        SyncType my_sync_type = ...;
        SyncableService my_service = ...;
        SyncableService my_other_service = ...;
        my_service.MergeDataAndStartSyncing(my_sync_type, my_other_service.GetAllSyncData(), my_other_service);
        EXPECT_EQ(my_service.GetAllSyncData(), my_other_service.GetAllSyncData());
        ```

5.  Write sync integration tests for your service. See
            <http://go/syncautomation> for details.

#### Suggested migration for existing synced Chrome services

1.  Make the service "consume" `SyncChange`s, i.e. implement
            `ProcessSyncChanges`. This most likely involves moving some logic
            from the change processor to the service itself.
2.  The change processor is most likely listening to notifications from
            the service and using the notification's details or querying the
            service directly to find out what changed. Make the service
            "produce" `SyncChange`s: i.e., when the change processor receives a
            notification from the service, it should only have to query the
            service to get one or more `SyncChange`s to process. This most
            likely involves moving the rest of the logic from the change
            processor to the service itself.
3.  Make the service inherit from `SyncableService`; this most likely
            involves moving the logic from the model associator to
            `MergeDataAndStartSyncing`. The model associator itself can stick
            around and just call `MergeDataAndStartSyncing/StopSyncing` from
            `Associate`/`DisassociateModels`.

#### Implementation Notes

See the PDF attached for an overview of the Syncable Service API.

`SyncType` should just be `syncable::ModelType`.

Internally, a `SyncData` will contain a `SyncEntity` protobuf. Or rather, a
thread-safe ref-counted pointer to an immutable one, so `SyncData`/`SyncChange`
can be cheaply copyable and passable across threads. TODO: We should try to make
`sync_data.h`/`sync_change.h` not include our protobuf header files.

Some special handling is needed for bookmarks; currently, it is the only data
type which uses sync's support for hierarchies. The most practical solution for
this is probably to just add bookmark-specific accessors to `SyncChange` for id,
`first_child_id`, and `successor_id`.

Extensions and apps may need to use sync's support for ordering, i.e. it may
also need to use `successor_id`.

There will most likely be one instance per thread of `SyncChangeProcessor` which
handles all the data types which live on that thread.

An earlier API proposal by Nicolas Zea is
[here](https://docs.google.com/a/google.com/document/d/1-Ky4lhCTIN_9cTef2zZsIh3pBALQUPj-3Hh2BpPkX20/edit?hl=en)
(available only to Googlers).

[Life of a Sync Item
Update](https://docs.google.com/Doc?docid=0Aa_pnb1d4_gnZDUzOWp4bV80ZDhjeng1ZzU&hl=en)
is useful for understanding how exactly sync works behind the scenes.

#### Future plans

Support may be added to enable `SyncableService`s to talk to
**chrome://sync-internals**. This will most likely involve passing some sort of
Javascript bridge object in `MergeDataAndStartSyncing` that a `SyncableService`
can use to raise events, and/or an additional method in `SyncableService` to
enable it to respond to Javascript queries.