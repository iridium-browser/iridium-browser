---
breadcrumbs:
- - /developers
  - For Developers
page_name: memory-bloat
title: Efforts against Memory Bloat
---

This page surveys and organizes efforts against memory bloat in Chrome.

## Memory Bloat

There are two main types of memory issues. One is *memory safety* issues like
use-after-free and buffer-overflow. Many tools are your friends like
[ASAN](/developers/testing/addresssanitizer) and [Dr.
Memory](/developers/how-tos/using-drmemory) for memory safety issues. Other
memory issues are *memory bloat* issues that simply mean too large memory usage.
We need completely different efforts and toolsets against memory bloat issues
than memory safety issues.

### Leaks

Leaks are a part of memory bloat issue. Some leaks can be found by some memory
safety tools like Dr. Memory, especially in case malloc'ed objects remain to the
end. We however have many other types of practical leaks. For example,
forgetting to release a reference-counted pointer to a bulk of objects may
practically cause a leak. It cannot be caught as it's eventually released at the
very end. Such leaks are observed by tools against memory bloat issues.

### Which is Bloating: Native or JavaScript?

Memory bloat issues are usually observed either in two places: native (C++) heap
and/or JavaScript heap. Your first action is to identify which is bloating. A
JavaScript value window.performance.memory (visible from DevTools) would help to
identify. Chromium is usually blamed when the bloat is observed in native heap,
and the Web application is usually in charge of bloat in JavaScript heap. The
difficulty is that it's not always, but identifying which is bloating must help
to track the issue.

### File a Bug

Please file a bug with a
"[Performance-Memory](https://code.google.com/p/chromium/issues/list?q=Performance%3DMemory)"
label for memory bloat issues.

## Efforts and Tools

### Categorizing the Efforts

Memory bloat issues are complicated, and we need to observe them from various
viewpoints: testing, reproducing, breaking-down, leaking... The following is a
trial to categorize existing efforts. Feel free to edit the table for better
categorization.

<table>
<tr>
<td>Testing and Benchmarking</td>
<td><a href="/developers/memory-bloat#TOC-Chrome-Endure">Chrome Endure</a></td>
<td><a href="http://build.chromium.org/p/chromium.webkit/builders/WebKit%20Linux%20Leak">LeakDetector on Bots</a></td>
</tr>
<tr>
<td>Runtime Bloat Detection</td>
<td>MemoryPressureListener</td>
</tr>
<tr>
<td>Real-world Data Collection</td>
<td><a href="/developers/memory-bloat#TOC-about:memory-internals">about:memory-internals</a></td>
<td> <a href="/developers/memory-bloat#TOC-WANTED:-UMA-based-tool">WANTED: UMA-based tool</a></td>
</tr>
<tr>
<td>Top-down Analysis</td>
<td><a href="/developers/memory-bloat#TOC-about:profiler">about:profiler</a></td>
<td> <a href="/developers/memory-bloat#TOC-about:tracing-memory-snapshots"> about:tracing memory snapshots</a></td>
<td> <a href="/developers/memory-bloat#TOC-Deep-Memory-Profiler">Deep Memory Profiler</a></td>
<td> <a href="/developers/memory-bloat#TOC-Memory_Watcher">Memory_Watcher</a></td>
<td> <a href="/developers/memory-bloat#TOC-memdump-Android-only">memdump</a> (Android only) </td>
<td>LeakDetector</td>
</tr>
<tr>
<td>Bottom-up Analysis</td>
<td><a href="/developers/memory-bloat#TOC-LeakSanitizer">LeakSanitizer</a></td>
<td> <a href="/developers/memory-bloat#TOC-Leak-Finder-for-JavaScript">Leak Finder for JavaScript</a></td>
<td> <a href="/developers/memory-bloat#TOC-WANTED:-Tracking-reference-counted-pointers">WANTED: Tracking reference-counted pointers</a></td>
</tr>
<tr>
<td>Life Prolongation</td>
<td><a href="/developers/memory-bloat#TOC-Tab-Discarder">Tab Discarder</a></td>
<td> Memory Purger</td>
</tr>
<tr>
<td>DevTools: for Web-application side</td>
</tr>
</table>

**Feel free to add your efforts in the list. The description is nice to have its
advantage and also DISADVANTAGE (limitation) so that we can easily understand
what is missing and needed to reduce memory usage. Just a plan is welcome in the
list!**

### Testing and Benchmarking

> #### [Chrome Endure](/system/errors/NodeNotFound)

> (to be written...)

### Runtime Bloat Detection

> #### MemoryPressureListener

> Implemented <http://crbug.com/246125> to replace Memory Purger.

### Real-world Data Collection

Memory bloat issues are happening in the real world, but it is hard to reproduce
them in our "lab" (tests and builders). The efforts are to reproduce real-world
memory bloat issues by collecting more real-world memory usage data.

> #### about:memory-internals

> A page that summarizes memory-related information for easier reporting from
> users for memory bloat. It's similar in concept to about:network-internals.
> The page can include detailed information (extensions, process status and
> more) for a specific bloating situation, but less cases (for example than UMA)
> because it's not automatic. This page should help us get a better
> understanding of real-world memory issues reports. It might be kicked by
> observing memory pressure or memory bloat (see MemoryPressureListener).

> #### WANTED: UMA-based tool

> While memory-internals is a tool to get detailed information of a specific
> case, we have had a tool to get rough information from all over the world,
> UMA. It is however difficult to get correlation between memory usage and
> something from UMA. For example, "Users using this extension have a tendency
> to bloat." or like that. We need a tool to find such correlation easily.

### Top-down Analysis

"Top-down" means breaking down the entire (or a large part of) memory usage into
some parts.

> #### [about:profiler](/developers/threaded-task-tracking)

> If you set a CHROME_PROFILER_TIME environment variable to 1, the
> about:profiler page will then show you the amount of memory, in bytes,
> allocated in each task, listed in the "duration" columns. You can take a
> couple of snapshots, show the diff and group by process to find out where the
> memory is going. Note: confirmed to work on Windows by setting a user variable
> from the system properties panel.

> #### [about:tracing memory snapshots](/system/errors/NodeNotFound)

> An experimental tool that reports on tcmalloc heap memory usage using the
> about:tracing TRACE_EVENT macros to generate stacks (instead of real stacks
> with symbols). Works with release builds without symbols with minimal
> performance hit, but provides very non-specific stack data. Contact:
> jamescook@, nduca@

> #### [Deep Memory Profiler](/developers/deep-memory-profiler)

> A native (C++-level) memory profiler to breakdown the whole-process memory
> usage of a long-running process. The goal is to observe and visualize physical
> memory usage deeper and deeper (e.g. mapped files, mmap and malloc backtraces,
> C++ object types and more). The limitation is that it works post-mortem and
> needs a special build (Release with some build options or Debug). It works for
> Linux and Android for now. Contact: [the dmprof
> group](https://groups.google.com/a/chromium.org/forum/?fromgroups#!forum/dmprof).

> #### [Memory_Watcher](/developers/memory_watcher)

> (to be written...)

> #### [memdump](/developers/testing/multi-process-ram-usage-analysis-on-android) (Android only)

> Multi-process aware version of /proc/&lt;PID&gt;/smaps.

### Bottom-up Analysis

"Bottom-up" means finding individual specific memory problems like leaks, cyclic
reference and else. We have some existing leak finding tools like Massif (in
[Valgrind](/system/errors/NodeNotFound)) and
[heap-checker](/system/errors/NodeNotFound) (in gperftools)

> #### [LeakSanitizer](/developers/testing/leaksanitizer)

> #### LeakSanitizer is an experimental tool that works similarly to Heap Leak Checker. LeakSanitizer is fast and can be used on top of ASan.

> **[Leak Finder for
> JavaScript](https://code.google.com/p/leak-finder-for-javascript/)**

> In JavaScript you cannot have "memory leaks" in the traditional sense, but you
> can have objects which are unintentionally kept alive and which in turn keep
> alive other objects and ends up in hogging a huge chunk of memory. Leak Finder
> for JavaScript detects objects which are considered "memory leaks" according
> to predefined leak definitions.

> #### WANTED: Tracking reference-counted pointers

> Reference-counted pointers can be sources of "hidden" memory leaks as
> described above. Some good thing to track such "hidden" leaks by
> reference-counted pointers would help to eliminate memory leaks.

### Life Prolongation

> #### [Tab Discarder](/chromium-os/chromiumos-design-docs/tab-discarding-and-reloading)

> c.f. [Chrome OS out-of-memory design
> doc](http://www.chromium.org/chromium-os/chromiumos-design-docs/out-of-memory-handling)

> #### Memory Purger

> To be removed: discussed in <http://crbug.com/98238>.

### [DevTools](https://developers.google.com/chrome-developer-tools/): for Web-application side

[**Timeline Memory
view**](https://developers.google.com/chrome-developer-tools/docs/timeline#memory_mode)

> Here you can track number of live DOM Nodes, Documents and JS event listeners
> in the inspected render process.

> **[JavaScript Heap
> Profiler](https://developers.google.com/chrome-developer-tools/docs/heap-profiling)**

> Allows to take JS heap snapshots, analyze memory graphs and compare snapshots
> with each other.

> **[JavaScript Object Allocation
> Tracker](http://www.youtube.com/watch?v=x9Jlu_h_Lyw)**

> Dynamic version of the JS heap profiler that allows you to see JS object
> allocation in real time.

## Analysis Reports and Case Studies

Let's collect public reports of any kind of memory usage analysis.
