---
breadcrumbs:
- - /developers
  - For Developers
page_name: memory-usage-backgrounder
title: Memory Usage Backgrounder
---

Chromium Memory Usage

Here is some background information if you are measuring memory in Chromium.

**Multi-Process Model Background**

To understand Chromium's memory usage, let's understand the multi-process model.
Unlike other browsers, Chromium is divided into multiple processes. When
Chromium starts up, it will initially have two processes. One process is the
**browser process** which controls the main browser functionality, and the other
is the initial **renderer process**, which runs the Blink rendering engine and
JavaScript (V8). Each time you open a new tab in Chromium, you'll likely get a
new renderer process. With typical browsing, it is common to see 5-7 chrome.exe
processes active. Further, if you utilize plugins, apps, or extensions, they may
also execute within independent processes. All of Chromium's processes, whether
it is a browser process, a renderer process, or a plugin process, will show
under the Task Manager as "chrome.exe".

Here is a screenshot of what you might see from Windows 8 running Chromium:

[<img alt="Windows task manager displaying multiple Chrome processes."
src="/developers/memory-usage-backgrounder/taskmanager.jpg">](/developers/memory-usage-backgrounder/taskmanager.jpg)

If you were to look at the above table on Windows XP, the numbers for Memory
would be significantly larger. This is because Windows Vista and higher has
updated the primary metric for measuring process memory since Windows XP. Both
operating systems primarily measure a process' **working set** - that is the
number of bytes held in physical memory on behalf of that process. However, the
working set is made up of three distinct components:

*   **Private Working Set**
    Resident pages which are private only to this process
*   **Shareable Working Set**
    Resident pages which may be shared with other processes.
*   **Shared Working Set**
    Resident pages which are currently being shared with other processes. This
    is a subset of the Shareable Working Set.

In Windows XP, each process is measured by its total working set. In Windows
Vista the definition was changed to just be the private working set. This
actually makes more sense, because the total working set will change based on
what other processes are currently running. Thus, if you want to measure a
reproducible metric for memory, the Private Working Set is generally a better
measure.

**How to Measure Memory**

It turns out that there are several ways to look at memory. For Chromium, we
really wanted to find a measure which is a true reflection of the actual system
resources used by the application. We couldn't use the Total Working Set,
because this measure double-counts the shared memory used by each process.
Almost all windows applications benefit from at least some amount of shared
memory, and Chromium is no exception. For example, each process typically loads
several Windows DLLs (such as kernel32, ntdll, user32, and more), and these DLLs
are shared by Windows across all running processes; double counting this memory
within Chromium would make it appear to be 10-50MB larger than it really is.

The primary metric we selected for Chromium is a measurement which is similar to
what Windows uses. We measure the working set. However, unlike Windows Task
Manager, we consider which memory is shared and which is not. We also wanted to
make sure that if Chromium loads DLLs which no other process uses, that those
are counted as memory used specifically by Chromium. The final algorithm for
"Private" size is: Private Working Set + Shareable Working Set - Shared Working
Set. This is close to Window XP's measurement of memory, except for that it also
accounts for shared memory.

The simplest way to look at total memory used by Chromium is to use the
**Chromium about:memory** feature. Simply type 'about:memory' into Chromium's
URL bar, and you might see something like this:

[<img alt="Chrome&#39;s about:memory page, displaying the breakdown of memory
among tabs and other processes."
src="/developers/memory-usage-backgrounder/aboutmemory.jpg">](/developers/memory-usage-backgrounder/aboutmemory.jpg)

At the top of the page is a summary of the overall memory usage. If you just
want a single number for Chromium's memory usage, the Private column is the best
figure which aggregates all memory used by all Chromium processes.

The rest of the numbers on the page provide more detailed breakdowns about
memory usage within Chromium. The Summary section provides aggregate information
across processes used by the browser, and the Processes section enumerates the
memory usage used by each active chrome.exe process.

When the about:memory page is loaded, Chromium also looks for other browsers
(Firefox, Opera, Safari, or IE) that are running and includes them within the
summary for a quick comparison. We use the same metric for measuring each
browser (including IE8's multi-process mode).

**How to Measure Memory Part 2**

Looking at an application's Working Set is not the only way to measure memory.
While it does provide a good general measure, what about other system resources
consumed which may not show up as part of a process' working set? As it turns
out, there are system resources allocated by any application. A common resource
consumed is GDI memory; this memory will show up in the system's commit charge
and may not be reflected in the working set.

To measure this, we need to look at the system's **Total Commit Charge**. The
total commit charge measures the total amount of memory used by all applications
and the system itself. Because this measurement is a system-wide measurement
rather than a per-process measurement, it is much trickier to measure. It is
also sensitive to any background processes or services which may run unbeknownst
to the user while executing the test. However, the basic procedure is this:

*   shut down any unnecessary services
*   reboot
*   measure the Total Commit Charge of the system (1)
*   run the application test (chrome.exe)
*   measure the Total Commit Charge of the system (2)
*   close the application
*   measure the Total Commit Charge of the system (3)

To verify that the test was valid, measurements (3) and (1) should be almost
identical. The "Total Commit Charge" will be the difference between measurement
(2) and (1).

We've executed these tests for Chromium, and it scores reasonably well. These
tests are complicated, and results vary greatly depending on the operating
system version, video drivers installed, and hardware configuration.

**Weaknesses of a Multi-Process Model on Memory Usage**

Using a multi-process model within the browser offers benefits for reliability,
robustness, and security of the browser. Those benefits drove our design toward
the multi-process model, and you can read more about them
[here](https://sites.google.com/a/google.com/the-chrome-project/developers/design-documents/process-models).

However, using multiple processes is somewhat at odds with building a
lightweight browser. First off, each process does have some amount of overhead.
The process overhead turns out to be relatively small, however, once you've
accounted for the shared memory properly. The more significant handicaps are the
replicated internal components of a browser, such as caches, JavaScript VM
heaps, and internal data structures which must be duplicated inside multiple
processes. JavaScript is particularly troublesome because of its garbage
collected heap. Heaps are generally relatively large and must be replicated
across each browser process.

How can Chromium overcome these deficiencies? In short, it can't :-) All we can
do is to make everything else that much smaller, so that the effects of being
multi-process is minimized. As a result, there are degenerate cases where
Chromium uses a lot more RAM than other browsers. The case which is worst is
where many tabs are open, each to separate domains with large amounts of
JavaScript. But for typical usage, we think Chromium fares well at balancing the
benefits of multiple processes and also maintaining memory usage at levels which
is lower than some popular browsers.

Note to techies: Check out single process mode. To see how Chromium would fare
if it were not a multi-process model, you can ditch the multi-process model and
run Chromium in 'single process mode' (use the --single-process command line
option).

**Benefits of a Multi-Process Model on Memory Usage**

Despite the obvious negative impact of multiple processes on memory usage, it
turns out there are some unique benefits which in the long run may dramatically
overshadow the downsides. These are benefits which are simply not available to
single-process model browsers.

The first benefit is that the multi-process browser more readily reclaims unused
memory than single-process models. Here is a quick test to run at home. Run a
Chromium browser, and also a single-process browser side by side. Open up
several tabs - say 10 tabs to different sites. Use each browser a bit. Then,
close 9 of the tabs and look at the memory usage of the browser. What you'll
notice is that while Chromium is able to give back almost all of the memory used
for those tabs, the single-process browsers cannot. Those same data structures,
caches, and Javascript heaps which must be replicated in the multi-process model
cannot be purged in the single-process model! This has a dramatic impact on your
browsing experience over time. With Chromium, when you close a tab, it's
resources are completely flushed.

Another benefit of the multi-process model is that Chromium can actively help
the operating system when memory is tight. Windows already has a feature where
when a foreground application is minimized it will tell Windows that the
application's memory can be reclaimed if necessary. As long as there is plenty
of RAM, this has no effect on system performance. However, when RAM is tight,
the OS knows to take pages from the minimized application rather than the
applications which the user may still be using. Similarly, Chromium employs this
exact same model for active tabs. It is quite common for some users to have
12-15 tabs open concurrently, but only 2-3 of those tabs to be actively in use.
Some of those tabs may not have been touched for days! As you use Chromium, it
will actively tell Windows that the tabs which are not in use should be paged
out first. Single-process browsers cannot differentiate which memory is in use
by which tabs.

Finally, a third benefit to the multi-process browser's memory usage is the
ability to selectively cleanup unused memory. Below is a screenshot of
Chromium's Task Manager. To activate the Task Manager, click the menu button in
the upper right -&gt; More tools -&gt; Task Manager (or just press Shift-Esc). A
screenshot is included below:

[<img alt="Chrome&#39;s task manager showing the CPU and memory usage of its
tabs and processes."
src="/developers/memory-usage-backgrounder/chrometaskmanager.jpg">](/developers/memory-usage-backgrounder/chrometaskmanager.jpg)

In the unfortunate event that your browser is using more memory than you'd like
it to, you can use Chromium's Task Manager to get a glimpse at which tabs are
using the most memory. If you spot an errant plugin process or tab using more
than you want, you can kill it using the "End process" button, and instantly
reclaim that memory. Your browser will keep running. Without the help of
multiple processes, this would not be possible.

Helping the Operating System
Windows today uses several hints to help the operating system manage memory. One
of these is reducing the working set when a process is not in use. For example,
if you minimize a windows application, Windows will automatically release the
working set of that application to the OS. If there is plenty of RAM, this has
little effect; however, when memory is tight, the OS uses this information to
decide which pages to page out first. (You can test this yourself - run Outlook,
and check its memory usage. Then minimize it. Watch the working set shrink to a
very small size).

Just as Windows can do this for desktop applications, a multi-process browser
can do this for tabs. When you navigate away from a tab, Chrome uses this as a
hint that the resource for that tab are less important than your foreground tab.
Chrome will lower the priority of the now backgrounded tab and also give back a
portion of the working set for that tab to the OS.

Giving back pages to the OS when there are no RAM constraints is somewhat of a
no-op; but when other applications need the memory, Chrome is "nice" and yields
its memory so that foreground applications (Chrome or any other app) can be
responsive. Users on low memory machines definitely notice the difference.
Imagine having 15 tabs open, but only being "active" in 2 of them. You'd rather
have those two be snappy than have all 15 compete for RAM.

Likewise, if you leave Chrome idle for periods of time, it will attempt to give
back RAM to the operating system. You aren't using it, so giving it back helps
other applications.

Partial TODO List For Reducing Memory Usage

Here is a partial list of things we can do to improve memory usage. Most of
these apply to Blink and Chromium; if possible, please try to fix within the
Blink project for maximal re-use of code.

Font management: The way fonts are cached in memory is fairly sparse.

Strings: Duplicate strings are bad. All strings in Blink/Chromium are unicode
strings, even if the string could be encoded with a single-byte encoding. This
has been measured to add up to megabytes of extra space.

History subsystem: The SQLite DB for history could be optimized. This will
particularly help keep Chromium lean with usage-over-time.

In Memory Cache: Chrome currently uses a shared memory cache across all
processes. Maximum size is currently 32MB, partitioned across the active
processes.

Javascript: Since Chrome renderers each generally have their own JavaScript VM,
reducing memory usage in Javascript pays benefits repeatedly. See the V8 project
for more information.

Tools: Better real-time tools is always needed. The memory_watcher project is
currently used for debugging.
