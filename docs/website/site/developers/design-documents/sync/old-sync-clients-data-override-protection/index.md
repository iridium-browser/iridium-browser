---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/sync
  - Sync
page_name: old-sync-clients-data-override-protection
title: Protection against data override by old Sync clients
---

[TOC]

## Overview

### Problem

This document outlines the necessary steps to prevent data loss scenarios with
[Sync's Model API][SyncModelApi] for **multi-client** Sync users. A typical
problematic scenario is as follows:
1. New proto field `F` is introduced in data [specifics][DataSpecifics] (e.g.
   [`PasswordSpecifics`][PasswordSpecifics]).
2. Client `N` (a newer client) submits a proto containing the introduced field `F`.
3. Client `O` (an older client) receives the proto, but doesn’t know the field `F`,
   discarding it before storing in the local model.
4. Client `O` submits a change to the same proto, which results in discarding
   field’s `F` data from client `N`.

[DataSpecifics]: https://www.chromium.org/developers/design-documents/sync/model-api/#specifics
[PasswordSpecifics]: https://cs.chromium.org/chromium/src/components/sync/protocol/password_specifics.proto
[SyncModelAPI]: https://www.chromium.org/developers/design-documents/sync/model-api

### Solution

To prevent the described data loss scenario, it is necessary for the old client
(client `O` above) to keep a copy of a server-provided proto, including
**unknown** fields (i.e. fields not even defined in the .proto file at the time
the binary was built) and **partially-supported** fields (e.g. functionality
guarded behind a feature toggle). The logic for caching these protos was
implemented in [`ModelTypeChangeProcessor`][MTCP].

To have this protection for a specific datatype, its
[`ModelTypeSyncBridge`][MTSB] needs to be updated to include the cached data
during commits to the server (more details in the [`Implementation`](#implementation)).

[MTCP]: https://cs.chromium.org/chromium/src/components/sync/model/model_type_change_processor.h

### Checklist

To implement this solution, a Sync datatype owner should follow these steps:
1. Override [`TrimAllSupportedFieldsFromRemoteSpecifics`][TRSFC] function (see this
   [`section`](#trimming)).
2. [Optional] Add DCHECK to local updates flow (see this [`section`](#safety-check)).
3. Include unsupported fields in local changes (see this
   [`section`](#local-update-flow)).
4. Redownload the data on browser upgrade (see this [`section`](#browser-upgrade-flow)).
5. [Optional] Add sync integration test (see this [`section`](#integration-test)).

The result of these steps is that:
* Local updates will carry over unsupported fields received previously from the
  Server.
* Initial sync will be triggered if upgrading the client to a more modern
  version causes an unsupported field to be newly supported.

[TRSFC]: https://cs.chromium.org/chromium/src/components/sync/model/model_type_sync_bridge.h;l=215;drc=0c829042793ff12fd42c7378fc57ed91a33f1eb2

## Implementation

### Trimming

Storing a full copy of a proto may have performance impact (memory, disk). The
Sync infrastructure allows and encourages to trim proto fields that do not need
an additional copy (if the field is already well supported by the client).

Trimming is a functionality that allows each data type to specify which proto
fields are **supported** in the current browser version. Any field that is
**not supported** will be cached by the [`ModelTypeChangeProcessor`][MTCP] and
can be used during commits to the server to prevent the data loss.

Fields that should **not** be marked as supported:
* Unknown fields in the current browser version
* Known fields that are just defined and not actively used, e.g.:
    * Partially-implemented functionality
    * Functionality guarded behind a feature toggle

[`TrimAllSupportedFieldsFromRemoteSpecifics`][TRSFC] is a function of
[`ModelTypeSyncBridge`][MTSB] that:
* Takes a [`sync_pb::EntitySpecifics`][EntitySpecifics] object as an argument.
* Returns [`sync_pb::EntitySpecifics`][EntitySpecifics] object that will be
  cached by the [`ModelTypeChangeProcessor`][MTCP].
* By default trims all proto fields.

To add a data-specific unsupported fields caching, override the trimming
function in the data-specific [`ModelTypeSyncBridge`][MTSB] to clear all its
supported fields (i.e. fields that are actively used by the implementation
and fully launched):

```cpp
sync_pb::EntitySpecifics DataSpecificBridge::TrimAllSupportedFieldsFromRemoteSpecifics(
   const sync_pb::EntitySpecifics& entity_specifics) const {
 sync_pb::EntitySpecifics trimmed_entity_specifics = entity_specifics;
 {...}
 trimmed_entity_specifics.clear_username();
 trimmed_entity_specifics.clear_password();
 {...}
 return trimmed_entity_specifics;
}
```

[EntitySpecifics]: https://cs.chromium.org/chromium/src/components/sync/protocol/entity_specifics.proto
[MTCP]: https://cs.chromium.org/chromium/src/components/sync/model/model_type_change_processor.h
[MTSB]: https://cs.chromium.org/chromium/src/components/sync/model/model_type_sync_bridge.h
[TRSFC]: https://cs.chromium.org/chromium/src/components/sync/model/model_type_sync_bridge.h;l=215;drc=0c829042793ff12fd42c7378fc57ed91a33f1eb2

### Safety check
Forgetting to trim fields that are supported might result in:
* I/O, memory overhead (caching unnecessary data)
* Unnecessary sync data redownloads on browser startup (more details below)

To prevent this scenario, add a check that:
* Takes a local representation of the proto (containing supported fields only)
* Makes sure that trimming it would return an empty proto

This should be done before every commit to the Sync server:

```cpp
DCHECK_EQ(TrimAllSupportedFieldsFromRemoteSpecifics(datatype_specifics.ByteSizeLong()), 0u);
```

### Local update flow
To use the cached unsupported fields data during commits to the server, add the
code that does the following steps:
1. Query cached [`sync_pb::EntitySpecifics`][EntitySpecifics] from the
   [`ModelTypeChangeProcessor`][MTCP]
   ([`Passwords example`][PasswordCacheQuery]).
2. Use the cached proto as a base for a commit and fill it with the supported
   fields from the local proto representation
   ([`Passwords example`][PasswordProtoMerge]).
3. Commit the merged proto to the server
   ([`Passwords example`][PasswordMergeCommit]).

[MTCP]: https://cs.chromium.org/chromium/src/components/sync/model/model_type_change_processor.h
[PasswordCacheQuery]: https://source.chromium.org/chromium/chromium/src/+/main:components/password_manager/core/browser/sync/password_sync_bridge.cc;l=972-979;drc=e95ee489e7a4aee5408d8bb0e13bebc61adcca0d
[PasswordMergeCommit]: https://source.chromium.org/chromium/chromium/src/+/main:components/password_manager/core/browser/sync/password_sync_bridge.cc;l=366-374;drc=12be03159fe22cd4ef291e9561762531c2589539
[PasswordProtoMerge]: https://source.chromium.org/chromium/chromium/src/+/main:components/password_manager/core/browser/sync/password_proto_utils.cc;l=226-267;drc=32d86114a7d841e2b1b041b0d8b5434930164f17

### Browser upgrade flow
To handle the scenario when unsupported fields become supported due to
a browser upgrade, add the following code to your data-specific
[`ModelTypeSyncBridge`][MTSB]:
1. On startup, check whether the unsupported fields cache contains any field
   that is supported in the current browser version. This can be done by using
   the trimming function on cached protos and checking if it trims any fields
   ([`Passwords example`][PasswordCacheCheck]).
2. If the cache contains any fields that are already supported, simply force
   the initial sync flow to deal with any inconsistencies between local and
   server states
   ([`Passwords example`][PasswordInitialSync]).

It’s important to implement the trimming function correctly, otherwise the client
can run into unnecessary sync data redownloads if a supported field gets cached
unnecessarily.

If the trimming function relies on having data-specific field present in the
[`sync_pb::EntitySpecifics`][EntitySpecifics] proto ([`example`][PasswordCheckExample]),
make sure to skip entries without these fields present in the startup check (as
e.g. cache can be empty for entities that were created before this solution
landed). This can be tested with the following [`Sync integration test`][SyncStartupTest].

[MTSB]: https://cs.chromium.org/chromium/src/components/sync/model/model_type_sync_bridge.h
[PasswordCheckExample]: https://cs.chromium.org/chromium/src/components/password_manager/core/browser/sync/password_sync_bridge.cc;l=904;drc=d149054a527b4fae61477ce9c338aeff64273d06
[PasswordCacheCheck]: https://source.chromium.org/chromium/chromium/src/+/main:components/password_manager/core/browser/sync/password_sync_bridge.cc;l=981-1008;drc=e95ee489e7a4aee5408d8bb0e13bebc61adcca0d
[PasswordInitialSync]: https://source.chromium.org/chromium/chromium/src/+/main:components/password_manager/core/browser/sync/password_sync_bridge.cc;l=284-296;drc=e95ee489e7a4aee5408d8bb0e13bebc61adcca0d
[SyncStartupTest]: https://chromium-review.googlesource.com/c/chromium/src/+/3773600

### Integration test
Add a [`Sync integration test`][SyncCachingTest] for the caching / trimming flow.

[SyncCachingTest]: https://chromium-review.googlesource.com/c/chromium/src/+/3638012

## Limitations

### Sync support horizon
The proposed solution is intended to be a long-term one, but it will take some
time until it can be used reliably. This is due to the facts that:
* Browser clients need to actually have the version with the mentioned
 [`implementation`](#implementation) merged.
* Sync supported version horizon is pretty long (multiple years).

### Deprecating a field
Deprecated fields should still be treated as supported to prevent their
unnecessary caching.

### Migrating a field
This requires client-side handling as the newer clients will have both fields
present and the legacy clients will have access to the deprecated field only.
Newer clients should:
* Keep filling the deprecated field for legacy clients to use
* Add a logic to pick a correct value from a deprecated and new field to
  account for updates from legacy clients

### Repeated fields
No client-side logic is required - the solution will work by default.

### Nested fields
Protecting nested fields is possible, but requires adding client-side logic to
trim single child fields or the top level field if none of the child fields are
populated (Passwords notes [`example`][NotesExample]).

[NotesExample]: https://source.chromium.org/chromium/chromium/src/+/main:components/password_manager/core/browser/sync/password_proto_utils.cc;l=29-64;drc=0495165c40e1bfad00d7a84474cfb8025e6d4a7c