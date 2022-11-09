---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: cookie-split-loading
title: cookie-split-loading
---

Currently, the CookieMonster cookie store is initialized by reading in the
cookies from the backing store in a large lump on first access. This is bad for
several reasons:

*   It violates the principle that we shouldn't do blocking operations
            on the IO thread.
*   It contributes to perceived user startup time in if a session is
            being restored (which requires URL accesses, which result in
            CookieMonster accesses).
*   It contributes to variation in web page loading, as the first web
            page takes substantially longer than later ones.

This problem is tracked in [Issue 52909](http://crbug.com/52909). This document
sketches out a proposed solution to this problem.

**Requirements**

*   Minimize startup time (including startup time which includes session
            restore, i.e. CM access).
*   Minimize stalling of individual set/get cookies.

I believe that these requirements imply:

*   Basic startup cannot be gated on loading all cookies from persistent
            backing store.
*   Individual page loads cannot be gated on loading the entire cookie
            store from persistent backing store. Ideally, individual page loads
            would only load the cookies from persistent backing store that are
            needed for that page.
*   Full persistent store load should be initiated at startup, but in
            the background not blocking any processes (as having cookies loaded
            in memory will result in absolute minimum latency).

**Notes on synchronous vs. asynchronous interfaces to the CookieMonster**

(See [Issue 68657](http://crbug.com/68657))

Generally, services on the IO thread provide asynchronous interfaces with
callbacks, to avoid the IO thread becoming a bottleneck in providing services
for other threads. For all requests after the initial one, the CookieMonster
follows this rule and does not block; all information required for all
CookieMonster APIs is in memory. However, the first call to the CookieMonster
triggers loading in of the entire persistent backing store, done on-thread,
which means that the IO thread blocks on disk operations on the first request to
a CookieMonster with a backing store.

Giving the CookieMonster an asynchronous interface is a large task, not because
of the internal structure of the Cookie, but because of the large number of
callers, all of whom would need to be adapted to the new interface. Issue
&lt;xxx&gt; tracks this task.

In an ideal world, the current design would not be dependent on async
CookieMonster interfaces. However, a key part of this design is breaking up the
loading of the backing store into indepedent pieces, using Task objects, which
are spawned off asynchronously in the backing in the absence of incoming
reuqests to laod the backing store database. Combining this with a synchronous
interface for those reuqests that need backing store loads would mean doing one
of:

*   Loading data inline (in the IO thread) when it is needed for a
            particular reuqest. This would probably mean that all load tasks
            would need to be on the IO thread.
*   Completely blocking any IO thread activity while waiting for
            responses from the DB thread for data load-in. This violates one of
            the basic principles of the IO thread (though in theory it's no
            worse than what we're doing now).
*   Running nested message loops, and carefully tracking which requests
            corresponded to which nesting level of the message loop. If request
            B were run while request A had an outstanding load message to the DB
            thread, this means that request A would be blocked on the completion
            of request B (because the stack couldn't unwind until that point).

None of these options is particularly appealing. Thus it's preferred that this
work be done after the CookieMonster has been converted over to having an async
interface. The rest of this design assumes such an interface.

\[Note that it would be relatively easy to layer an asynchronous interface (if
not behavior) on top of the current synchronous interface; the async trigger
would wrap the sync call and a PostTask(self) for the callback. This could allow
incremental changes to the various callers. When all callers had been converted,
the sync interface could be removed and the async interface made to behave in a
non-blocking manner. \]

**High Level Design**

The cookie monster will get an additional data structure, indexed by domain key
(currently eTLD+1) indicating whether or not a particular domain key's set of
values has been loaded. Incoming requests (either set of get) will trigger the
loading of the domain key(s) required for those requests if they have not yet
been loaded. In these cases, the incoming request will block (as it currently
does if the backing store hasn't been loaded). All actual database accesses will
occur on the DB thread.

At CookieMonster construction, a background operation will be invoked to load
the entire cookie store. This will run on the database thread, and will break
down into a series of tasks (T) marks each separate task:

*   (T) Get the list of domain keys contained in the persistent backing
            store.
*   For each key:
    *   (T) Load all data from that key into memory
    *   (T -- IO thread) Incorporate that loaded data into the cookie
                monster and mark that key as loaded.

These are broken up into indiviudal tasks both to avoid freezing the DB thread,
and to allow loads of specific keys triggered by particular web page access to
"jump the queue" and get pulled in earlier.

**Routine Flow Outline**

The primary routines and data structures involved in this design are sketched
out below.

[<img alt="image"
src="/developers/design-documents/cookie-split-loading/objects.svg">](/developers/design-documents/cookie-split-loading/objects.svg)

There are two basic "ropes" (since they're potentially cross-thread) of routine
invocation: the initial spawn of loading of all of the cookies, and the
load-on-demand triggerred by specific requests to the cookie monster.

The initial spawn occurs from the CookieMonster constructor -&gt;
SQLitePersistentCookieStore::SpawnFullLoad -&gt;
SQLitePersistentCookieStore::BackingStore::SpawnFullLoad. Once the rope has
reached the DB thread, a full list of all the keys in the SQLite DB is loaded
and stored in BackingStore. Then SpawnFullLoad does a self-dispatch (to the DB
thread) of a ChainLoadKeyCookies task for the first key in the list.
ChainLoadKeyCookies will call LoadKeyCookies (which will load in the cookies
associated with that key from the backing store and notify the CookieMonster of
that load) and will then self-dispatch another instance of ChainLoadKeyCookies.
When the last entry in the key list is loaded, ChainLoadKeyCookies will return
without any action. This will (incrementally, allowing other tasks to run on the
DB thread) load all the cookies.

If a request comes into the CookieMonster while this chain is executing, it will
check all_loaded_ and keys_loaded_ to detemrine that the relevant data for the
request has not been loaded (if the relevant data has already been loaded, it
will execute the in-memory lookup inline as current and call the callback with
the information). It will then register the callback and operation information
into the key-&gt;callback mapping. If that is the first registration of that
key, it will call SQLitePersistentCookieStore::LoadKeyCookies, which will
dispatch an invocation to the DB thread of
SQLitePersistentCookieStore::BackingStore::LoadKeyCookies. If LoadKeyCookies
determines that the key requested has already been loaded (race with above
chain), it will simply exit.

The tricky part of this design is making sure that there are no races that
result in dangling requests. The invariants that guarantee that this doesn't
happen are:

*   The first entry in the keys-&gt;callback table is always correlated
            with a LoadKeyCookies dispatch to the DB thread.
*   That entry/dispatch will only happen if no previous CompleteLoad for
            that key has already arrived.
*   The only case in which a LoadKeyCookies dispatch will not result in
            a later CompleteLoad dispatch is if a previous CompleteLoad dispatch
            for that cookie has already been sent.

So it's possible for a LoadKeyCookies() dispatch to not result in a
CompleteLoad() being sent back, but only if there's already one in transit.
Thus, if a callback is registered in the keys-&gt;callback table, it will be
executed with maximum latency round trip to DB thread + queue wait time + load
time.

**Object Lifetime Relationships**

\[Will Chan take note :-}.\]

Both CookieMonster and PersistentCookieStore are RefCountedThreadSafe objects.
The CookieMonster keeps a reference to the PersistentCookieStore in its store_
pointer; this guarantees that the store lives as long as the Monster does.

Both SpawnFullLoad and LoadKeyCookies will take as arguments scoped_refptrs on
the CookieMonster which the PersistentCookieStore (or its backing store) will
use for callbacks. These references will guarantee that the CookieMonster stays
alive for the duration of the callback.

If the CookieMonster needs to be torn down, it will call an abort() routine on
the PersistentCookieStore, which will break the chain load and release the
scoped_refptr&lt;&gt; held on the CookieMonster by that chain load.
