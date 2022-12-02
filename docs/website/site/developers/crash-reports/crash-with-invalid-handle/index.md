---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/crash-reports
  - Crash Reports
page_name: crash-with-invalid-handle
title: Crash with invalid handle
---

## What is this crash about?

### I'm seeing frequent Chrome crashes and the stack looks like this:

> \[debugger_win.cc:20 \] base::debug::BreakDebugger()

> \[scoped_handle.cc:79 \] base::win::HandleTraits::CloseHandle()

> \[file_win.cc:120 \] base::File::Close()

> \[file_stream_context.cc:184 \] net::FileStream::Context::CloseFileImpl()

> \[bind_internal.h:1169 \] base::internal::Invoker&lt;&gt;::Run()

> \[sequenced_worker_pool.cc:760 \]
> base::SequencedWorkerPool::Inner::ThreadLoop()

> \[sequenced_worker_pool.cc:508 \] base::SequencedWorkerPool::Worker::Run()

> \[simple_thread.cc:60 \] base::SimpleThread::ThreadMain()

> \[platform_thread_win.cc:78 \] base::\`anonymous namespace'::ThreadFunc()

> \[kernel32.dll + 0x0004ee1b \] BaseThreadInitThunk

> \[ntdll.dll + 0x000637ea \] __RtlUserThreadStart

> \[ntdll.dll + 0x000637bd \] _RtlUserThreadStart

or like this:

> \[debugger_win.cc:20 \] base::debug::BreakDebugger()

> \[scoped_handle.cc:79 \] base::win::HandleTraits::CloseHandle()

> \[scoped_handle.h:52 \]
> base::win::GenericScopedHandle&lt;&gt;::~GenericScopedHandle&lt;&gt;()

> \[handle_watcher.cc:271 \] mojo::common::WatcherThreadManager::StopWatching()

> \[handle_watcher.cc:434 \]
> mojo::common::HandleWatcher::SecondaryThreadWatchingState::~SecondaryThreadWatchingState()

> \[chrome.dll + 0x00238a03 \]
> mojo::common::HandleWatcher::SecondaryThreadWatchingState::\`scalar deleting
> destructor'()

The relevant part is the first three lines that show that something is being
closed and there is an intentional crash because there is an error closing that
handle.

These stacks point to a form of corruption in the past, which is only detected
when the invalid handle is being closed. Like most forms of corruption, it is
not safe to continue execution at this point, but at the time of the crash there
is not enough information to know when (and where) the corruption was
introduced.

### I am seeing Chrome hangs with stacks like this one:

> \[ntdll.dll + 0x00040da8 \] ZwWaitForSingleObject

> \[KERNELBASE.dll + 0x00001128 \] WaitForSingleObjectEx

> \[KERNELBASE.dll + 0x000010b3 \] WaitForSingleObject

> \[platform_thread_win.cc:222 \] base::PlatformThread::Join()

> \[thread.cc:135 \] base::Thread::Stop()

> \[..._process_sub_thread.cc:26 \]
> content::BrowserProcessSubThread::~BrowserProcessSubThread()

> \[chrome.dll + 0x0043dcb8 \] content::BrowserProcessSubThread::\`scalar
> deleting destructor'()

> \[browser_main_loop.cc:829 \]
> content::BrowserMainLoop::ShutdownThreadsAndCleanUp()

> \[browser_main_runner.cc:146 \] content::BrowserMainRunnerImpl::Shutdown()

> \[browser_main.cc:28 \] content::BrowserMain()

This is another manifestation of the same underlying issue: once a handle is
corrupt, using it can do anything. If the OS detects that the handle is invalid,
the operation generally fails right away. However, if the handle now points to
something else, operations may appear to work but do something generally
unexpected. The code in this example is simply waiting on the wrong object, so
most likely it will never be signaled.

## What to do about it

We have better detection baked into the 32-bit builds of Dev and Canary builds
of Chrome. In that case, the crash stack will hopefully morph into something
that points to the code at fault.

### For users:

*   Consider installing a
            [Canary](https://www.google.com/chrome/browser/canary.html?platform=win)
            or
            [Dev](https://www.google.com/chrome/browser/index.html?extra=devchannel&platform=win)
            Channel Build (Windows only. See
            [this](/getting-involved/dev-channel) for information about what a
            channel is and what to do before installing a new one).
*   Search your system for malware, and try removing software that
            interacts directly with Chrome by injecting DLLs. In the past we
            have seen correlation between 3rd party software loaded into Chrome
            and problems with invalid handles. You can get a list of all DLLs
            loaded into the browser by navigating to about:conflicts (note that
            most likely there are no detected conflicts even if a DLL is causing
            trouble).

### For bug triage:

*   Close bugs that are reported on the Stable or Beta builds, as there
            is very little to do. Feel free to point to this page.
*   If the report comes directly from the stability result of a given
            version (top x on the crash server), search for a similar report and
            merge (it may be closed as WontFix). Avoid merging into a
            user-reported bug (there is a similar report somewhere for something
            that is internal
*   The ScopedHandle verifier is far from perfect, so even with a 32-bit
            build canary build it is bound to generate dumps that follow the
            same pattern of dumps from Stable. Ignore those reports.

### For developers:

A Canary build will generate a properly bucketed stack dump like this:

> \[scoped_handle.cc:145 \] base::win::OnHandleBeingClosed()

> \[close_handle_hook_win.cc:27 \] \`anonymous namespace'::CloseHandleHook()

> \[shared_memory_win.cc:75 \] base::SharedMemory::~SharedMemory()

> \[resource_buffer.cc:42 \] content::ResourceBuffer::~ResourceBuffer()

> \[ref_counted.h:192 \] base::RefCountedThreadSafe&lt;&gt;::DeleteInternal()

> \[async_resource_handler.cc:99 \]
> content::AsyncResourceHandler::~AsyncResourceHandler()

> \[chrome.dll + 0x003af3ae \] content::AsyncResourceHandler::\`scalar deleting
> destructor'()

> \[...ed_resource_handler.cc:19 \]
> content::LayeredResourceHandler::~LayeredResourceHandler()

> \[chrome.dll + 0x003aebc7 \] content::BufferedResourceHandler::\`scalar
> deleting destructor'()

> \[...ed_resource_handler.cc:19 \]
> content::LayeredResourceHandler::~LayeredResourceHandler()

> \[chrome.dll + 0x003ae5f8 \] content::ThrottlingResourceHandler::\`scalar
> deleting destructor'()

> \[resource_loader.cc:104 \] content::ResourceLoader::~ResourceLoader()

It points to ~SharedMemory as the source of corruption. This means that said
code is either double closing a handle somehow (most of the time it is far from
obvious how that happens), or it is not really doing anything wrong but it is
being blamed anyway. In any case, this code should not be doing manual handle
management so it should be refactored to use ScopedHandle instead.

If there is a bug, it most likely will go away with the refactor. If there was
no bug, ScopedHandle verifier will stop blaming this code, and the overall
reliability of the verifier will go up.

*   So the take away is... refactor the code to remove manual handle
            management. This also means to modify interfaces to remove passing
            raw handles around and instead pass a ScopedHandle (or something
            that encapsulates the handle using a ScopedHandle internally).
            There's plenty of code to choose from!. See bugs
            [322664](http://crbug.com/322664), [416721](http://crbug.com/416721)
            or [417532](http://crbug.com/417532) for some examples.
*   Search the crash server for stacks containing OnHandleBeingClosed to
            get a summary of issues currently detected as problematic.
