---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: indexeddb
title: IndexedDB Design Doc
---

**This is quite outdated. The fundamental design is still pretty similar to what's described here, but this document is now of limited usefulness.**
Please send any feedback you might have to *Jeremy Orlow
&lt;jorlow@chromium.org&gt;* or *chromium-dev@chromium.org*.

**Introduction:**

The IndexedDB API implements a persistent (across browser/machine restarts)
database that is quite stripped down. It is built upon "Object Stores" (btrees
of key-&gt;value pairs) and Indexes (btrees of key-&gt;object store record). It
includes an async API for use in pages and both an async and a sync API for
workers.

The latest editors draft of the spec can be found here:
<http://dev.w3.org/2006/webapi/WebSimpleDB/> And there's a lot of discussion
currently happening on public-webapps@w3.org.

At the time of this writing the following are major changes that will probably
happen to the spec in the near term:

1.  The async API will probably be changed to a callback based one.
2.  The async API should be available from workers.
3.  Composite indexes will probably be added.
4.  External indexes will probably be removed (indexes not attached to a
            object store).
5.  Inverted indexes will hopefully be added so it's possible to
            efficiently implement full text search.

*Note: Most of this design doc is talking about work inside WebKit/WebCore since
most of the implementation will exist there. At the bottom, there is discussion
of Chromium specific plumbing.*

**Threading model:**

All database processing will happen on a background thread. The reasons are as
follows: Both workers and pages will be able to access IndexedDB and thus it's
possible that two different WebKit threads may have the same database open at
the same time. Database access will result in file IO and we need to ensure that
we never block the main thread. There's no way to re-use worker threads (even if
we wanted to) because their lifetimes may be shorter than that of the opened
database. In addition, sharing things across threads is particularly difficult
and dangerous in WebKit. So, the easiest way to mitigate this is to do all
database work on its own thread.

Up for debate is whether we should have one IndexedDB thread, one per origin, or
one per database. Originally I was leaning towards one per database since
threads are cheap and we should let the OS do its thing in terms of IO
scheduling, but this may be excessive. Either way, I'd like to design so that we
*can* split things up thread wise if we choose to in the future.

A side effect of having all work take place on a background thread is that we
can share almost all of the code between the sync and async versions of the API.
To make this simpler, I'm planning on creating a class like what follows to link
code running on a background thread to either the sync or async messages
happening on the main/worker threads.

// This is a class that abstracts away the idea of having another thread offer a

// "promise" to give a particular return value, go off and do the processes, and
then

// send a message which either triggers asynchronous callbacks or unblocks the
thread

// and returns a result. This class is not threadsafe, so be sure to only call
it from

// the thread it's created on.

template &lt;typename ResultType&gt;

class AsyncReturn&lt;ResultType&gt; : RefCounted&lt;ResultType&gt; {

public:

// Provide async callbacks to be used in the event of a success or error.

// You should only call this or syncWaitForResult, and you should only call

// either one once.

void setAsyncCallbacks(PassOwnPtr&lt;ScriptExecutionContext::Task&gt;
successCallback,

PassOwnPtr&lt;ScriptExecutionContext::Task&gt; errorCallback);

// Block execution until a result or an exception is returned. This will run

// a WorkerRunLoop (sync is only available on workers) with the mode set to only

// process tasks from our background thread. You should only call this or

// setAsyncCallbacks, and you should only call either one once. If an exception

// is set, the returned pointer will be 0.

PassOwnPtr&lt;ResultType&gt;
syncWaitForResult(OwnPtr&lt;IDBDatabaseException&gt;\*);

// Called on success. If there's anything in the queue, starts a one shot timer

// on m_timer.

void setResult(PassOwnPtr&lt;Type&gt;);

// Called on error. If there's anything in the queue, starts a one shot timer on

// m_timer.

void setError(PassOwnPtr&lt;IDBError&gt;);

private:

// Function called by our timer. Calls the proper callback.

void onTimer(Timer\*);

// Only one of the following should ever be set. These are necessary even in

// sync mode because it's possible for syncWaitForResult to be called after

// the response has reached us. These are zeroed out again when passed to the

// callback or returned via syncWaitForResult.

OwnPtr&lt;Foo&gt; m_result;

OwnPtr&lt;IDBDatabaseError&gt; m_error;

// When this fires, we'll call the proper callback. Only for async usage.

Timer m_timer;

// Only one of these is ever used and only during async usage.

OwnPtr&lt;ScriptExecutionContext::Task&gt; m_successCallback;

OwnPtr&lt;ScriptExecutionContext::Task&gt; m_errorCallback;

// Used so we can assert this class is only used in one way or another.

bool m_asyncUsage;

bool m_syncUsage;

};

**Class structure:**

In the spec, each interface has an async and a sync counterpart. In most cases,
they share a common base class. The sync version has a "Sync" suffix and the
async version has a "Request" suffix. Thus the following is fairly common:

interface Foo

Interface FooRequest : Foo

interface FooSync : Foo

In most cases, FooRequest and FooSync are almost identical except in terms of
the return types. For sync, the methods usually return values. For the other,
they call callbacks which pass the values in as a parameter. (The current event
based API has you set event handlers that are shared by all requests and make a
call. So it is like the callback interface except only one call can be inflight
at a time. Plus the syntax is ugly.)

In WebKit, my plan is to call the base class FooBase, but otherwise implement
the IDLs exactly as specced. So, I'll implement things as if the structure was
as follows:

interface FooBase

interface FooRequest : FooBase

interface FooSync : FooBase

FooRequest and FooSync are very similar. The only difference is in their return
values and whether they block. Since we're doing all real work on the background
thread anyway, it makes sense for our sync and async APIs to run the same code
and simply use the above AsyncReturn class to signal completion in both cases.

Both FooRequest and FooSync will have a pointer to a Foo class. This Foo class
will have all the same methods as FooRequest and FooSync except it'll return an
AsyncReturn&lt;Type&gt; instance. FooSync will then call syncWaitForResult()
which will run the WorkerRunLoop with the mode set to only accept messages from
a background thread (much like WebSockets and sync XHR) while waiting for the
response. FooRequest will call setAsyncCallbacks and use those to call the
proper javascript callback when the result is ready.

For browsers that use multiple processes (like Chromium) there needs to be some
way to detach the central backend from various frontends (in Chromium terms, the
browser process and the various renderer processes). We'll do this by making all
the Foo classes abstract interfaces and having the actual implementations be
FooImpl classes.

Each Foo class will have a .cpp file with a single static method in it: a
constructor. For normal WebKit, that will call FooImpl::create. For
multi-process implementations, they can omit this file from their build and
instead have Foo::create return a proxy object which implements the Foo
interface but which actually sends messages across processes.

So, to recap, for each type of interface in the IDLs (for example,
IDBObjectStore____) we'll have the following classes:

*   IDBObjectStoreBase -&gt; "IDBObjectStore" in the IDL; just a base
            class with stuff shared between the next two.
*   IDBObjectStoreRequest -&gt; The same in the IDL; the async version
            of the class. This and the next one do little more than have a
            reference to IDBObjectStore and call methods on it.
*   IDBObjectStoreSync -&gt; The same in the IDL; the sync version of
            the class.
*   IDBObjectStore -&gt; Interface for IDBObjectStoreImpl; methods will
            pretty much match what's exposed in IDL.
*   IDBObjectStoreImpl -&gt; Implements IDBObjectStore; lives almost
            entirely on the background thread; will send a message to the
            AsyncReturn instance on completion.

Because all objects either are tied 100% to the thread they're created on
(FooSync/FooRequest/FooBase) or exist completely on the background thread (Foo),
object lifetimes and how they're destroyed should be fairly easy. This is even
true in the face of the page cache. When a page is suspended, we should pause
all of it's callbacks. Ideally, we'd cancel all currently running operations and
restart if/when the page comes back, but it's not necessary for correctness. All
in all, the threading model should make page cache integration fairly
straightforward. During early development, however, we'll probably just have
canSuspend return false to keep things simple.

When we open an indexed database, we'll need to call the chrome client to see if
it's allowed.

It may make sense to take some parts of workers (like GenericWorkerTask and
WorkerRunLoop) and make them a tad more generic and then move them into
Platform.

**Bindings:**

There are currently no examples of methods that take callbacks that aren't
implemented as custom functions. I fear this means I have a lot of custom
binding writing in my future. Hopefully there's some way to modify the code
generators to avoid this, though.

To keep things simple, during the bootstrapping process I'll only be working on
V8 bindings. Once things start to stabilize API/Spec/code wise, I'll take a shot
at JSC bindings. If they're not fairly straightforward, I might see if someone
from the JSC side of the world is interested in helping.

**Btree implementation**/persistance**:******

I often say "btree" but there are many possible ways to implement data
persistence. Either way, it's out of scope for this document at the moment. The
most important thing is to get an implementation working with or without
persistance. Once we're getting close to that goal, then we can start thinking
about this. In the mean time, it's most important to get good feedback to the
working group since WebKit already has a good data persistence layer
(WebSQLDatabase) and there is no indication anyone else will be shipping an
implementation soon.

**Chromium side of the implementation:**

The communication from WebCore in the renderer to WebCore in the browser is all
fairly straightforward (and will look much like how we implemented
LocalStorage/SessionStorage), but the reverse needs to happen completely via
async callbacks/tasks. To handle this cleanly/efficiently, we'll need to add
some notion of "tasks" to the Chromium WebKit API (like
WebCore::GenericWorkerTask and Chromium's Task/RunnableMethod classes). We'll
also have FooProxy classes in WebCore that implement the Foo interface and which
proxy all calls through to the WebKit layer, much like in
LocalStorage/SessionStorage.

Here are the various layers of calls (from renderer to browser):

WebCore::FooProxy (implements WebCore::Foo)

WebKit::RendererWebFooImpl (implements WebKit::WebFoo)

&lt;-- IPC --&gt;

Foo

WebKit::WebFooImpl (implements WebKit::WebFoo)

WebCore::FooImpl (implements WebCore::Foo)

Note the symmetry and how we're essentially just trying to connect WebCore (in
the renderer) to WebCore (in the browser process).

In addition to the standard set of classes, we'll need a couple special ones.

*   **WebKit::WebAsyncReturn&lt;&gt;** Much like
            WebCore::AsyncReturn&lt;&gt; except without any of the synchronous
            waiting functionality. Returned from all methods.
*   **WebKit::WebTask** Much like WebCore::GenericWorkerTask or
            Chromium's Task. Passed into
            WebKit::WebAsyncReturn&lt;&gt;::setAsyncCallbacks().
*   **IndexedDBDispatcher** Lives in the renderer and handles all the
            reply messages from the browser process. Each outgoing request is
            assigned an ID so that we know which WebAsyncReturn object to match
            it up with. There's one per renderer process.
*   **IndexedDBDispatcherHost** Lives in the browser and handles all the
            messages from the renderer process. It has a reference to the
            IndexedDB context and can send reply messages back to the renderer
            process. There's one per ResourceMessageFilter.
*   **IndexedDBContext** Keeps track of all the IDs for the various
            WebKit wrappers and contains a hirarchy of Chromium objects which
            wrap the WebKit objects which wrap the WebCore implementations. The
            context is connected to the profile (and threadsafe ref counted).

Now that you're acquainted with the objects, lets run through an example. In
this case, what happens when IDBDatabaseRequest::openObjectStore() is called:

*   In WebCore inside the renderer process, openObjectStore will be
            called on an IDBDatabaseProxy object which implements the
            WebCore::IDBDatabase interface.
*   The proxy will call RendererWebIDBDatabaseImpl::openObjectStore().
            (An implementation of the WebIDBDatabase interface.)
*   RendererWebIDBDatabaseImpl::openObjectStore() sends an async IPC to
            the browser process with the RendererWebIDBDatabaseImpl's ID.
*   It then creates a
            RendererWebAsyncReturnImpl&lt;WebIDBObjectStore&gt; object and
            registers it with the IndexedDBDisptcher.
*   RendererWebIDBDatabaseImpl::openObjectStore() then returns that
            WebAsyncReturn&lt;WebIDBObjectStore&gt; object to its caller
            (IDBDatabaseProxy).
*   IDBDatabaseProxy::openObjectStore() would then instantiate an
            AsyncReturn&lt;IDBObjectStore&gt; object and two WebTasks.
*   Both WebTasks would have pointers to the AsyncReturn&lt;&gt; object.
            One would set the result. The other would set the error.
*   WebAsyncReturn&lt;&gt;::setAsyncCallbacks() would be called with the
            two tasks.
*   IDBDatabaseProxy::openObjectStore() would then return the
            AsyncReturn&lt;&gt; object to its caller.
*   That caller would similarly create tasks and pass them into the
            AsyncReturn&lt;&gt; (but that's just what happens normally; it's not
            Chromium speciifc).

So now there's an async message in flight to the browser process and a chain of
callbacks/tasks set up to fire a callback or allow execution to continue inside
the browser process.

*   On the IO thread in the browser, the message comes in. An
            IndexedDBDispatcherHost receives the message. Each method detects
            whether it's on the IO thread and, if so, redirects it to the WebKit
            thread.
*   The message would then call IndexedDBContext::getIndexedDatabase()
            with the WebIDBDatabase's ID.
*   If the ID doesn't exist, we kill off the renderer since at best
            there's a major bug or memory corruption, and at worst the renderer
            is compromised.
*   We then call Chromium's IDBDatabase::openObjectStore() method. (Note
            that the name is the same as another object in WebCore. That's fine.
            They're in separate namespaces and since the WebKit layer is in
            between, it isn't confusing in practice.)
*   Chromium's IDBDatabase::openObjectStore then calls
            WebIDBDatabaseImpl::openObjectStore(). (The WebIDBDatabaseImpl is an
            implementation of WebIDBDatabase that lives in WebKit/chromium/src
            and provides entry back into WebCore's implementation of the
            IndexedDB implementation)
*   WebIDBDatabaseImpl::openObjectStore() then calls
            WebCore::IDBDatabase::openObjectStore(). (Which, via the virtual
            method call, actually calls
            WebCore::IDBDatabaseImpl::openObjectStore().)
*   WebCore's IndexedDB implementation does its thing and returns an
            AsyncReturn&lt;IDBObjectStore&gt; object.
*   WebIDBDatabaseImpl then creates a
            WebAsyncReturnImpl&lt;WebIDBObjectStore&gt; object that has a
            pointer to the AsyncReturn&lt;&gt; object and returns it.
*   Chromium's IDBDatabase::openObjectStore() then creates two WebTasks.
            One for a success/result and one for an error. These WebTasks wrap
            Chromium Tasks/RunnableMethods.
*   When the WebTasks get a response, they'll wrap it in a
            WebIDBDatabaseError or a WebIDBObjectStore as appropriate. They'll
            then pass the result on to the appropriate WebTask.
*   The appropriate Chromium Task will then be called which will fetch
            the result. If it's a WebIDBObjectStore, it'll be added to the
            IndexedDBContext and an ID will be assigned.
*   Either the serialized WebIDBDatabaseError or the WebIDBObjectStore's
            ID will be sent back via the IndexedDBDispatcherHost via an async
            message. Either way, it'll include an ID that was sent with the
            original async request.

So now a reply message is en route to the renderer.

*   The IndexedDBDispatcher receives the message. Based on the ID, it
            figures out which WebAsyncReturn&lt;&gt; object is associated with
            it.
*   It'll create a RendererWebIDBDatabaseErrorImpl or a
            RendererWebIDBObejctStoreImpl to wrap the result in. If it's the
            latter, it'll be given the ID the browser process created.
*   The object is passed into either WebAsyncReturn&lt;&gt;::setResult()
            or ::setError.
*   That then either converts the WebIDBDatabaseError object into a
            WebCore::IDBDatabaseError or wraps the WebIDBObjectStore in an
            WebCore::IDBObjectStoreProxy and passes the result into
            WebCore::AsyncReturn&lt;&gt;::setResult() or ::setError().
*   And the rest proceeds as it normally would in WebCore.

This same process happens for each object type--except for IndexedDatabase since
it's a singleton and the entrypoint to the whole API. It'd be created as
follows:

*   WebCore::IndexedDatabase::create() is normally part of
            WebCore/storage/IndexedDatabase.cpp, but Chromium won't compile that
            file. Instead, the file with our WebCore::IndexedDatabaseProxy will
            contain the WebCore::IndexedDatabase::create() implementation.
            (Normally WebCore::IndexedDatabase::create() would just call
            WebCore::IndexedDatabaseImpl::create().)
*   WebCore::IndexedDatabase::create() would call
            WebCore::IndexedDatabaseProxy::create() through the ChromiumBridge.
*   IndexedDatabaseProxy::create() would call
            WebKitClient::CreateIndexedDatabase() which would create a
            RendererWebIndexedDatabaseImpl and return is synchronously.
            IndexedDatabaseProxy would then wrap that class.
*   The browser process would create its own
            IndexedDatabase/WebIndexedDatabase/WebCore::IndexedDatabase lazily
            on first use.

Whenever a Proxy object is deleted, it should delete the wrapper it owns. When
the RendererFooImpl is deleted, it should send a message to the browser and the
chain of deleting should continue.
