---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/sync
  - Sync
page_name: sync-diagnostics
title: Sync diagnostics (Needs update)
---

### <http://go/aboutsync>

### Overview

*   Located in **about:sync** (or **chrome://sync-internals**)
*   Inspired by **chrome://net-internals**
*   In M35 - Notifications tab has been branched off to
            **chrome://invalidations**.
*   Tabs:
    *   About tab: "Classic" about:sync page, concise status overview
    *   Data tab: dump debugging data to text for copy & paste
    *   Notifications tab: Keeps track of incoming sync notifications
    *   Events tab: logs for sync-related events
    *   Sync Node Browser tab: browse all synced data
    *   Search tab: Searching for strings in all synced data
    *   Traffic tab: Text dump all of the sent and received messages to
                and from chrome sync servers.

### **About tab**

*   "Classic" about:sync page
*   Updates on the fly
    *   Try moving a bookmark, or stopping and starting sync
*   Some stuff will eventually be moved out, but page is still useful as
            an overview

### Data tab

*   Dump to text button
    *   Dumps all info from about and events tab
        *   But can also select datatypes info to dump
    *   Can be used to tell users to:
        *   Open about:sync
        *   Repro the problem if possible
        *   Wait a while (15-30 seconds)
        *   Go to data tab
        *   Dump info, paste into bug (or email privately if there's
                    sensitive info)
    *   Event log will probably be most useful part
    *   Redacts sensitive info
*   Room for improvement
    *   Automatically highlight dumped text, add "Copy to clipboard"
                button
    *   Button to add to an existing bug, or create a new one

### Notifications tab (up to M35)

*   If a notification is received when the page (not just the tab) is
            open, a counter will increment (one for each data type)
    *   Try moving a bookmark, setting a preference, or stopping and
                starting sync
    *   Takes ~5-10 seconds for a notification to bounce back
    *   Total Count of invalidations per each data type
    *   Session Counters reset if you reload
*   Room for improvement
    *   More detailed connection stats
        *   How long has connection been up? Which XMPP server/port? How
                    long ago was the last message sent/received?

### Events tab

*   Shows all sync events (i.e., functions in SyncManager::Observer and
            incoming notifications) that happen as long as the page is open
    *   Try moving a bookmark, or starting sync (dumps lots of data!)
    *   Doesn't quite log everything yet
    *   Only logs events after backend is initialized
    *   Resets if you reload
*   Room for improvement
    *   Store last N events (C++ side) so you can open it right when you
                hit a problem and still get some useful info
    *   Also log requests/responses to HTTP server
    *   Perhaps more logging for sync setup events
    *   Better display ([82866](http://crbug.com/82866))
    *   Start logging even before backend is initialized (tricky)

### Sync node browser tab

*   Shows sync nodes
    *   Sync nodes are structured in a tree.
    *   Each data type has a top-level node which is a child of the root
                node
    *   Only bookmarks sub-tree has true tree structure
        *   Everything else is just a list under the top-level node
    *   Use links to navigate tree: parent, first child, predecessor,
                successor
    *   Ignore specifics information for folder
*   Tree view version
    *   Tree visualized on the left, details of selected node on the
                right
*   Does *not* auto-update; you need to refresh
    *   Be careful, you may lose logged events; you can always open a
                new about:sync tab instead
*   Room for improvement
    *   Implement auto-updating (hard)
    *   Add a refresh button (easier)

### Search tab

*   Searches expressions in all the nodes
*   Supports Regex over the JSONified version of each node
*   Has quick searches to provide for easy access to interesting nodes.
            So far:
    *   Unapplied updates
    *   Unsynced items
    *   Delted items
    *   Conflicted items (unapplied and unsynced)

### Network-level debugging

*   Sync talks to the sync server via POSTing to clients4.google.com.
*   Sync talks to the notification servers via XMPP (talk.google.com).
*   Both sockets should show up in **chrome://net-internals** (after
            [82365](http://code.google.com/p/chromium/issues/detail?id=82365) is
            fixed).

### Room for improvement

*   Data type-specific tabs
    *   Model associators / change processors have access to the
                SyncService, so shouldn't be too hard to emit events or
                reply to calls
    *   Probably easier for things that live on the UI thread
*   Some way to capture VLOG() events
*   Should be obvious when sync isn't working
*   Use HTML notifications for events
*   Front-end for memory usage info (Lingesh is working on backend)
