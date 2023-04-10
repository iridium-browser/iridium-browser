---
breadcrumbs:
- - /omnibox-history-provider
  - 'Omnibox: History Provider'
page_name: caching
title: Omnibox History Quick Provider Index Caching
---

## **Why Caching is Important in this Case**

The internal indexes for the HQP can take some time to build from scratch. This
is done by querying all records in the the URL visit history database. Instances
of this database have grown to have tens of thousands of entries reaching over
100 MB in size. Building the HQP indexes at startup from such a large database
would delay the availability of this feature noticeably and would have a
noticeable impact on the responsiveness of the browser.

The objective then is to eliminate having to rebuild the internal indexes used
by the HQP. Once that index data has been built, the HQP saves it into a
profile-specific cache file. (The name of this file is 'History Provider
Cache'.) At profile startup, this cache file is read and used to repopulate the
HQP index data. At profile shutdown the HQP index data is written to the cache
file.

Initially, Pickle was being used as the binary encoding mechanism for recording
the cache file and the transaction file. During subsequent discussions, some
discomfort was expressed with Pickle as its primary client is IPC, not large
file encoding. As a result, the encoding mechanism was changed to Protocol
Buffers which is where the implementation stands today. There are drawbacks to
both Pickle and Protocol Buffers as the cache is only updated at browser
shutdown.

A snapshot-based approach, as currently used, would benefit from having a
transaction journal file into which new, changed and deleted visits would be
record. This transaction file would be kept constantly updated and then used at
profile startup to make adjustments to the HQP index as restored from the most
recent cache file snapshot. A new snapshot would then be recorded for the next
profile startup.

Before the transaction journal approach was begun, however, it was decided that
moving to a SQLite-based caching mechanism would be prudent. Doing so would
eliminate the need for a transaction journal inasmuch as the SQLite-based cache
would *always* be up-to-date. Effort to implement the SQLite caching has been
ongoing but has not yet incorporated into the HQP.

The current implementation **does not** employ a transaction journal, thus there
is some risk that historical information about site visits subsequent to profile
startup could be lost if the application crashes.

### **The Situation Today**

Currently, all cache file operations are performed on the main thread. This
could slowdown startup though no noticeable impact has been reported by users.
Nevertheless, it is important to move all file operations off of the main
thread. This leads us to...

#### **The Properly Threaded Solution**

Modifications to the HQP index caching code are underway such that all cache
file and database operations will be performed on the appropriate thread.

==Startup/Restoring==<img alt="image"
src="/omnibox-history-provider/caching/Startup.png" height=320 width=307>

The illustration to the right shows the threading model for profile
initialization. For a larger image [click
here](/omnibox-history-provider/caching/Startup.png).

Three threads are involved: the main (or UI) thread, the FILE thread, and the
History Database thread.

If there is a HQP cache file and that file can be successfully read then the
internal index data for the HQP is reconstituted from the contents of that cache
file and swapped into the HQP. The file reading and internal index data
reconstitution is performed on the FILE thread; swapping in the new index data
is performed on the main thread. Swapping of the index data on the main thread
is safe since all other accesses to the HQP's internal data occurs on the main
thread.

If there is no HQP cache file or that file is unreadable for some reason then
the history database (The database file itself is named 'History'.) is queried
on the history thread
and the internal index data for the HQP is reconstituted using the records from
that database. Once this operation has completed the HQP index data is swapped
in on the main thread. Note that if there is no history database, the database
is corrupt, there are no qualifying records in the database, or any other error
occurs that a new but empty HQP index data is swapped in.

The observer, if any, is notified on the main thread. (Currently, the only
observer is in unit_tests.)

==Shutdown/Saving==

<img alt="image" src="/omnibox-history-provider/caching/Shutdown.png" height=140
width=320>The illustration to the left shows the threading model for shutdown.
For a larger image [click here](/omnibox-history-provider/caching/Shutdown.png).

When the profile is shutdown, a copy of the HQP's index data is made on the main
thread and then written to the cache file on the FILE thread. Once again, making
the copy of the index data on the main thread affords safety as all other
accesses to that index data are performed on the main thread.

The observer, if any, is notified on the FILE thread. (Currently, the only
observer is in unit_tests.)

The history database thread is not involved in shutdown.

## Excruciating Detail

The illustration below gives a detailed image showing all classes and functions
involved in the profile startup/cache restoral operation. For a larger image
[click here](/omnibox-history-provider/caching/StartupDetail.png).

[<img alt="image" src="/omnibox-history-provider/caching/StartupDetail.png"
height=370 width=400>](/omnibox-history-provider/caching/StartupDetail.png)

The illustration below a detailed image showing all classes and functions
involved in the profile shutdown/cache saving operation. For a larger image
[click here](/omnibox-history-provider/caching/ShutdownDetail.png).

[<img alt="image" src="/omnibox-history-provider/caching/ShutdownDetail.png"
height=300 width=400>](/omnibox-history-provider/caching/ShutdownDetail.png)
