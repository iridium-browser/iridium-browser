---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/sync
  - Sync
page_name: embassy
title: embassy
---

<div class="two-column-container">
<div class="column">

**embassy: making model association + shutdown robust against change processing failures**

**tim@chromium.org**

**1. Problems**

1.  **The native model and "local" sync model are persisted separately
            and hence can diverge.**
2.  **Shutdown must wait for any change applied to "local" sync model to
            be applied to native. (via ScopedAllowWait)**
3.  **Extra memory, as every synced object has a "sync shadow" in memory
            at all times.**

**This document discusses these problems and some methods to address them in stages. If implemented, the proposals could make way to remove use of ScopedAllowWait in sync code, like task flushing / blocking in UIModelWorker::Stop. See Future section below for details.**
**1.1 Dual databases**
**Problem 1 above occurs because native models, such as the BookmarkModel, are written to disk at independent intervals and as part of independent transactions with respect to the syncable::Directory. This leads to the Great Sync Nemesis - ‘back from the dead’ bugs.**
**1.2 Synchronizing shutdown**
**Although in practice corruption due to #1 should be rare, we go to great lengths in the browser to ensure back-from-the-dead issues don’t arise in other ways. One of those ways is Problem 2. If ApplyUpdates has completed, the WriteTransaction destruction will try to propagate changes across to native models via ChangeProcessors. If that propagation doesn’t happen, any deletes that were applied to sync won’t get applied to native, so those items will be recreated on restart by model association. To avoid this, at shutdown we join the UI thread with the sync thread before stopping change processors. Aside from being complicated, this behavior reduces Chrome’s ability to parallelize shutdown tasks, hence can lead to jank and slowness ([see bug 19757](/developers/design-documents/aura)).**
**2 Proposal**
**The proposal involves installing some sync state in native data model territory. Kind of like embassies… hence the nickname. It is made up of three mostly independent parts.**
**2.1 bookkeeping. Problems 1 & 2 could be handled with extra bookkeeping when persisting writes to native models and the Directory, providing a form of transactional semantics. Writes to each can be kept in lock step via a monotonically increasing timestamp or checksum, that would be incremented and persisted with writes to the native database. Then at any point (particularly after restart) it is possible to determine if changes to the Directory were not reflected in the native model prior to shutting down. We could rely on such detectability on restart, rather than having to wait on shutdown.**
**There are some twists. First, a single timestamp only lets us detect if something is out of sync, and not what items. Second, there are two failure modes - a) we lose a race with shutdown and don't even attempt application on the native model (though changes were successfully applied to the sync database), and b) writing / persisting to the native model fails for some reason (can happen at any time).**
**With (a), it would normally be the case that the BookmarkModel (e.g.) and sync DB could be at most one timestamp apart, because of the way we signal the SyncScheduler to shutdown. Namely, we make a best effort to never open a WriteTransaction once exit has been requested, so the transaction would either succeed, or we would shutdown before the last open transaction could native-apply gracefully. But we have to worry about (b) because that write can actually fail if it is attempted. Meaning we do have to consider the case where the native model and sync DB are out of sync by more than one timestamp increment.**
**We can address this, as well as tell which items failed to apply (and hence trust the sync model on restart\*\*) in the following manner.**
**2.1.1 For edits (\*not\* deletes):**
**When a transaction closes:**

1.  **Compute timestamp tsyncdb, and stamp every item (in the sync db
            only) that was mutated.**
2.  **Store that single timestamp value as a datatype global "max
            timestamp" in the sync DB as well.**
3.  **Attempt to write this single timestamp value into native model
            metadata once the changes have been persisted there. Call it
            tnative.**

**On restart,**

1.  **Compare tnative and tsyncdb. If tnative = tsyncdb, perform normal
            model association.**
2.  **If tnative &lt; tsyncdb the last shutdown was not clean. During
            ModelAssociation, if any item read from the sync DB has a timestamp
            in the range \[tnative, tsyncdb\], trust the sync DB item.
            Otherwise, trust native (normal ModelAssociation).**
3.  **tnative &gt; tsyncdb shouldn't happen if implemented correctly,
            but it may be difficult to do so (to wait for success on sync
            persist before dispatching) since we may not have time at shutdown.
            If this does happen, performing normal model association is the
            right thing to do. The previous change will either be in an
            unapplied state or we will download it again.**

**Realistically we expect the logic in (2) to be needed rarely, but it should be robust - e.g. it handles cases such as server-deleted item that didn't get applied to native being re-added on model association.**
**\*\* This still doesn't gracefully handle hand-edits to bookmarks.js in the presence of an unclean shutdown. This is not a top concern.**
**2.1.2 For deletes:**
**If a tombstone is marked as applied in the sync database, it will not be re-loaded into memory from disk on a subsequent restart (see DropDeletedEntries). This means the above approach (2.2.1.1) can't work for deletes, because there is no durable item to stamp with t.**
**So, we can either**

1.  **choose to keep deleted entries around, tag them with tsyncdb, and
            avoid dropping them until model association completes.**
2.  **keep a "lookaside table" / journal of deleted items (keyed by
            local_external_id), updated in the same transaction as the deletes
            themselves (sync side), cleared when model association completes.**

**(2) has the advantage of not changing any expectations on the sync side and also not loading unnecessary EntryKernels, since this journal is purely bookkeeping for the embedder, not for sync itself.**
**\*Note re “when model association completes” - if failures with model association could stilll somehow miss the delete, we could avoid purging the sync item in either of the two cases above until an additional 'consumed' bit was set. This bit would get set on receipt of successful application (happy case) OR when model association consumes the item (likely deleting a local item). I don't think this necessary.**
**Bookkeeping alone does not address Problem 3 at all, so on to the next part.**
**2.2 offloading. (optional) P3 could be partially addressed by permitting groups other than GROUP_UI to offload associated sync entries to disk. This exploits the fact that groups with high numbers of items (and hence higher memory usage) don't typically have the requirement that they be readily available in memory. For these groups, synchronous trips to sqlite to fetch items is normal operation for the native model (e.g. autofill), and so it isn't an issue for sync to do the same.**
**Some work was done in this area before. The investigation included analyzing sync memory footprint and prototyping a mechanism to offload data. At that time, the analysis did not show enough of a memory dent to warrant the extra complexity. But recent heap profiling of chrome startup has demonstrated a larger impact in sync allocations than experiments using the original method would suggest. The data is different enough that this needs to be revisited, especially with the new importance of mobile.**
**2.3 reducing shadow size (optional) Bookkeeping + offloading together would partially address each problem from sec. 1, but some types (GROUP_UI, at least) would still suffer from sync shadows. This has some consequences, particularly on mobile. As an enhancement to the two features above, we could also remove the copy of the EntitySpecifics from the local sync DB and store that only at the native site. This (in tandem with bookkeeping) avoids back-from-the-dead issues and the awkwardness of handling tombstones (sec. 3.1, #2).**
**3. Alternatives considered**
**3.1 At a high level, the initial thinking was to shard the local portions (non SERVER_) across native models in Chrome. This would address all the issues listed above, although we would lose a few things from today's world -**

1.  **Native schemas are agnostic and have no sync state or business
            logic duplicated for each data type.**
2.  **Tombstone handling (follows from 1). Sync must set a bit (vs deep
            remove) when user deletes an entry.**
3.  **Encryption handling?**

**These are non-trivial sacrifices. Although this high level idea made sense at first, the current proposal does not have the same drawbacks.**
**3.2 Another idea is to change all model association implementations to behave differently if an item has ever been associated before. For example, if we're re-associating a native item that previously had a sync item counterpart (which could be tracked by a bit in the native model), but that sync item no longer exists, a case could be made that the native item should be deleted rather than pushed into sync. Similarly for edits, each data type could choose to do something different in the "re-associating" case. Bookmarks (or any type that can be modified before association occurs) would be more challenging to support with this, however, as would the post-unrecoverable-error model association run.**
**A significant downside to this is that it changes our happy-case paradigm of “never delete data, favor duplication”. We strayed from this with search engines, and ran into a lot of trouble. Data loss is \*much\* worse than duplication. Any logic that deletes something should be used \*extremely\* rarely. Moreover, if it happens, it should be based on a single source of information and code for all data types in a consistent and prescriptive manner -- the sync engine should do the dirty work to ensure correctness (versus every model association implementation doing their own thing). Thus, the bookkeeping described in section 2.1 is preferable.**
**It also may be more sensitive to changes that result from addressing crbug.com/80194.**
**4. Future**
**4.1 Remove ScopedAllowWait in glue**
**Once restart and model association logic is sophisticated enough to deal with half-applied change scenarios described above, we don't have to rely on blocking the UI on ModelSafeWorkers to ensure changes are propagated at shutdown. For example, we could "jettison" the sync engine at shutdown (perhaps via a CancellationFlag), so that it no longer attempts to forward work on to ChangeProcessors and terminates in isolation.**
**One thing to note, however, is that the approaches described above are best-effort for "late" native side changes. If any change occurs to a native model after the sync engine is "jettisoned", and it so happens that changes to that data type occurred on the sync side (that were not propagated due to jettison), the sync changes will trump the native changes on restart. This is likely only a practical concern for bookmarks, due to hand-editability of bookmarks.js. Late changes on both sides should be rare enough that this isn't a big issue, but we can add UMA to back up this speculation.**
**4.2 Back-from-the-dead & UnrecoverableErrors**
**Another place back-from-the-dead issues can crop up is with unrecoverable errors. If a data type experiences an unrecoverable error during model association or change processing, sync mechanisms will disable that type for the given client so that others can continue syncing normally. On browser restart, the mechanism will try to restart that type (unless the user explicitly disabled it). Let's call the time between when the error is hit and when a successful data type restart occurs the "down time" for that type. The problem is that any changes occurring to that type on other clients during "down time" will not be applied to the client that suffered the error. If any of those changes were, e.g. delete bookmark G, then the first successful model association after down-time on the affected client will bring G back, since it will still exist in the local bookmark model but not in sync.**
**Currently, disabling a datatype T means the sync database is purged of items with type T. Therefore, re-enabling it on a later restart will mean downloading a snapshot of T. In our bookmark example, this dataset won't have G. One option mentioned here for relation to the embassy proposal above might be to not purge the sync db on such an error, but keep things syncing silently (without any ChangeProcessor installed for that type). The bookkeeping would then be able to tell us, when model associating after down-time, that G was deleted and we should not it add it back. Leaving a type in the "syncing silently" state is fairly complex.**
**A (better?) alternative may be to keep a copy of the progress marker for T
(taken just before purging), and use this along with some server support to
recover from this case.**

</div>
<div class="column">

</div>
</div>