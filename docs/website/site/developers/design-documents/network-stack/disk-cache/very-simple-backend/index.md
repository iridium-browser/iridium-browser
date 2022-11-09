---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/network-stack
  - Network Stack
- - /developers/design-documents/network-stack/disk-cache
  - Disk Cache
page_name: very-simple-backend
title: Very Simple Backend
---

[TOC]

## Summary

Proposed is a new backend for the disk cache, conforming to the interface in
[Disk Cache](/developers/design-documents/network-stack/disk-cache). The new
backend is purposefully very simple, using one file per cache entry, plus an
index file. This backend will be useful as a testing baseline, as well as
dealing with IO bottlenecks impairing mobile browsing performance on some
platforms.

Compared to the standard blockfile cache, this new design has benefits and
goals:

*It is more resilient under corruption from system crashes.* The new design
periodically flushes its entire index, and swaps it in atomically. This single
atomic operation, together with cache entries being in separate files similarly
swapped in makes the implications of system crash much less serious; after
system crash Chrome will start with a stale cache, rather than have to drop its
entire cache as many users experience with the blockfile cache.
*It does not delay launching network requests.* The current cache requires
multiple context switches, and possibly blocking disk IO before launching
requests that ultimately use the network. These delays average over 25ms on
requests using the network, and about 14ms averaged over all requests on
Windows. Without context switches or blocking disk IO, this can be entirely
eliminated. On the Android platform, the slower flash controllers make these
delays significantly slower, increasing this benefit of a very simple backend.
*Lower resident set pressure and fewer IO operations.* Our disk format has
256-512byte per entry records, plus rankings & index information of ~100bytes
per entry in resident set pressure. Not all entries that are heavily used are
contiguous, so paging pressure is in practice larger. The very simple cache
stores only SMALLNUM bytes per entry in memory, contiguously, and does not
normally access the disk in the critical path of a request where not required.
As a result, the Very Simple Backend should maintain good performance even
without good OS buffer cache.

*Simpler.* The very simple cache explicitly avoids implementing a filesystem in
chrome. Files are opened and either read or written from the beginning to end.
No reallocations within files take place. The cache thread component of the very
simple cache should be shorter and easier to maintain than the
filesystem-in-chrome approach.
Together with the above benefits and goals, the new design has some non-goals:
*It is not a log structured cache.* While the IO performed by the Very Simple
Cache is mostly sequential, it is not fundamentally log structured; in
particular, the filesystem operations it performs are not log structured unless
used on a filesystem that itself is log structured.
*It is not a filesystem.* The disk cache delegates filesystem operations to the
filesystem. As filesystems improve, or change for different devices, the disk
cache will benefit simultaneously.

## Status

See [Bug 173381](https://code.google.com/p/chromium/issues/detail?id=173381) for
status on this implementation, or [track the Internals-Network-Cache-Simple
issue in
crbug](https://code.google.com/p/chromium/issues/list?q=label:Internals-Network-Cache-Simple).
For related designs covering performance tracking, see [Disk Cache Benchmarking
& Performance
Tracking](/developers/design-documents/network-stack/disk-cache/disk-cache-benchmarking).

## External Interface

[See the main disk cache design
document.](/developers/design-documents/network-stack/disk-cache#TOC-External-Interface)

## Structure on Disk

Like the blockfile disk cache, the disk cache is stored in a single directory.
There is one index file, and each entry is stored in a single file in that
directory.
An Entry Hash is a relatively short hash of the url, used for storage efficiency
in index and entry naming. Two entries with the same Entry Hash cannot be
stored. With a 40 bit per-user-salted SHA-2 of the url; collisions would occur
only at one in a million probability with a million entry cache. Each cache
entry is stored in a file named by the Entry Hash in hexadecimal, an underscore,
and the backend stream number.
The cache is stored in a single directory, with a file file 00index that
contains data for initializing an in memory index used for faster cache
performance. The index consists of entry hashes for records, together with
simple eviction information.
Format of Entry Files:

*   A magic number and version field.
*   The full url, length prefixed.
*   Chunks of data, length prefixed, compressed and checksummed.
*   An EOF record.

## Implementation

### IO Thread Operations

The public API is called on the IO thread; and the cache maintains the index
data on the IO thread as well. This allows for cache misses, and cache create
operations to be performed at low latency. The index is updated in the IO thread
as well.

### Worker Pool Operations

All IO operations in the simple cache are performed asynchronously on a worker
thread pool.

To reduce the critical path cost of creating new entries, the simple cache will
keep a pool of newly created entries ready to move into final place; this
permits the IO thread to update its index and provide a new entry at zero
latency, with some cost in renames-into-place occurring later in the entries
life cycle.

### Index Flushing and Consistency Checking

The index is flushed on shutdown, and periodically while running to guard
against system crashes. On recovery from a system crash, the browser will have a
stale index, and thus will need to periodically iterate over the cache directory
to find entries in the directory not mentioned in the idex.

### Operation without Index

If startup speeds and startup IO is too costly, note that the simple backend can
operate without the IO thread index by directly opening files in the directory.

### Code Location

[src/net/disk_cache/simple](https://chromium.googlesource.com/chromium/src/+/HEAD/net/disk_cache/simple/)

## Potential Improvements

### HTTP Metadata in Index

Request conditionalization, freshness lifetime, and HTTP validator information
could be stored in the index on the IO thread, greatly reducing latency required
to serve requests.

### Single File per Entry

It's arguably not a very simple idea, but in practice combining the headers and
the entry data into the same file makes atomic filesystem operations (renames,
etc...) easier, while also making writes/reads of a single entry generally more
sequential.
