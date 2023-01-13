---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: out-of-memory-handling
title:  Chrome OS Out of Memory Design
---

August 2011 \
jamescook@, gspencer@, ups@

Chromebooks have a limited amount of physical RAM (1 - 2 GB) and do not use
swap. Currently, when the machine runs out of memory the kernel’s OOM killer
runs and kills a process. Usually this is a renderer process, which results in
one or more tabs being killed. If that renderer is running the frontmost tab,
the user sees a “He’s dead, Jim” page informing the user that the tab was
killed. If it is running background tabs, those tabs are reloaded when the user
switches to them.

## Background

See the “Where does our memory go?” appendix below for some discussion of our
current memory usage.

## Proposal

Ideally, the user should never have to worry about running out of memory. We
should be able to determine when we’re getting low on RAM and take actions in
userspace to reduce our footprint. We propose to continue closing tabs to free
up RAM, but to start with closing individual tabs rather than the whole set
owned by one renderer. We like this direction because:

*   The user experience is better than killing a process - one thing dies at a
    time, rather than a seemingly random group.
*   This strategy is commonly used on phones and tablets.
*   We can close tabs quickly, so the user experience should be Chrome running
    fast, dropping data, and continuing to run fast, rather than a gradual
    slowdown we would anticipate from trying to compress/hibernate the state of
    a tab.
*   We can try to be intelligent about which tabs we close, preferring those
    which are rarely or never used, or which are unlikely to have user-created
    state.
*   We know that closing a tab will rapidly return a large block of memory to
    the OS, as opposed to flushing caches, running garbage collection, etc.
    which return variable amount of RAM and may quickly result in the tab
    reclaiming it.
*   The strategy is relatively simple - we already know how to close tabs and
    automatically reload them.

We should kill renderer processes only as a last resort, and even then avoid
killing the process associated with the page the user is viewing. The user
should never see “He’s dead Jim” unless the page he is looking at is consuming
all available memory on the device.

We hypothesize that if we can avoid showing “He’s dead Jim” the majority of
users will not notice the page kills. If users find this objectionable, we can
try other strategies, like swap-to-memory (zram) or blocking the creation of new
tabs when we’re out of RAM.

## Technical Design

We’re going to build a “double-wall” for low-memory states: A “soft wall” where
the machine is very low on memory, which triggers a user space attempt to free
memory, and a “hard wall” where the machine is out of memory, which triggers the
normal kernel OOM killer. In both cases we’ll attempt to minimize impact on the
user.

### For the “soft wall”

It takes memory to free memory. In particular, while chrome is trying to free up
memory processes will continue to consume it. So we need to trigger the “soft
wall” while there is still some free memory available.

The kernel will reserve 10 MB of memory for the wall. It will provide a special
file descriptor to allow user space processes to monitor for a low-memory event.
The chrome browser process will monitor that file descriptor for the event.
(We’ll keep the size in a constant, and maintain UMA statistics on whether it
was a large enough buffer space.)

When an allocation occurs that eats into this reserve, the kernel will
temporarily “move the wall back”, allowing a memory page to be used and the
allocation to succeed. It will send an event on the file descriptor and start a
timer for 200 ms. It will listen on the file descriptor for a “done” event from
chrome.

When the soft wall is reached, Chrome will evaluate tabs to decide which one(s)
to close, and in what order, in order to free up memory. Note that closing a tab
in this mode is different from the user closing a tab because we can’t run the
“onclose” javascript handlers that we would normally run (they might do
anything: open a modal dialog, for instance). We are investigating how to
implement this - we may choose to navigate forward to about:blank, which frees
most of the memory used by the page and allows us to navigate back when the user
selects the tab. This saves their scroll position, form data, etc.

The chrome browser process will iterate through its list of tabs to choose one
to close or blank out. The browser prefers closing tabs in this order:

*   A background tab which the user has opened, but never seen (e.g., a
    control-click on a link)
*   A background tab into which the user has never typed/clicked/scrolled
*   Any background tab (ordered by most recently clicked in buckets of 10 min,
    then by memory consumption)
*   The foreground tab

Within each group, tabs are ordered by the last time that the user clicked on
them, preferring to close tabs that haven't been clicked on in the longest
amount of time. If two tabs were clicked on within 10 minutes of each other,
then the one with the largest memory use is more likely to be closed.
After closing the tab, if the renderer is still running (it has other tabs open)
it tells tcmalloc to release freed memory back to the OS immediately. The
browser process then tells the kernel it is done. The kernel then resets its
memory reserve back to 10 MB.

### The “hard wall”

Our hard wall is the OOM process killer. We hit the hard wall in two situations:

1.  During chrome’s memory cleanup, we still run out of physical memory
    (a fast leak)
2.  Chrome doesn’t send the done event promptly

If Chrome sends the done event, but didn’t free enough memory, we send the soft
notification again, and rely on the timeout to do the hard kill. Per ups@ this
is easier to implement in the kernel and more robust.

We cannot do any userspace work when we hit the hard wall. However, we can use
the kernel’s oom_score_adj property on processes to give it guidance as to which
process to kill. We set this score at startup for OS-related processes to -1000.
For Chrome related processes, we set an initial score as outlined below. For
renderers, we start them with an initial score of 300 and adjust the score once
every 10 seconds (from within the browser process).

A higher oom_score_adj makes a process more likely to be killed in OOM
situations: you can think of it as a percentage of memory. Setting a
oom_score_adj value of 500, for example, is roughly equivalent to allowing the
remainder of tasks sharing the same system, cpuset, mempolicy, or memory
controller resources to use at least 50% more memory. A value of -500, on the
other hand, would be roughly equivalent to discounting 50% of the task's allowed
memory from being considered as scoring against the task. A score of -1000 makes
a process unkillable by the OOM killer.

The current Linux OOM killer algorithm only considers allowed memory usage, and
not run time or process start time in its “badness” score. Note that this
algorithm does not guarantee that the processes will be killed in the order
specified, it is only a hint. Please check out [this
description](https://www.kernel.org/doc/html/latest/filesystems/proc.html#chapter-3-per-process-parameters)
for more detailed information.

By default all processes started by init on ChromeOS are set to an initial value
of -1000, which is inherited by sub-processes, so that only processes which we
explicitly set to higher values will be killed by the OOM killer. We score
processes as follows:

| Score | Description |
| ----- | ----------- |
| -1000 | Processes are never killed (Upstart uses `oom score never`) |
| -1000 | Linux daemons and other processes |
| -1000 | CrOS daemons that are critical to the system (dbus) |
|  -900 | CrOS daemons that are needed to auto-update, but can restart |
|  -100 | CrOS daemons that can recover (shill, system metrics) |
|  -100 | Android system processes |
|     0 | Chrome browser and zygote |
|   100 | Plugins, NaCl loader |
|   200 | Chrome GPU process, workers, plugin broker process |
|   300 | Chrome extensions |
| 300-1000 | Chrome renderers |
|  1000 | Processes that are killed first |

The renderer scoring will follow the algorithm for tab closing above. However,
since each renderer process might be servicing multiple tabs, we select the
lowest score (least likely to be killed) of all the tabs for each renderer. This
ensures that the renderer showing the tab the user is viewing has a low score,
even if other tabs in the same renderer have a higher score.

### “Why not just garbage collect?”

Well, we tried that. In purging memory, we used the MemoryPurger, and even added
Webkit cache and font cache, and notification of V8 that we’re low on memory to
the list of things that the memory purger pushes out, and found that for a
browser with 5-10 “real world” tabs open, we save single-digit megabytes, and
they rapidly fill up again (for instance, after 10s, 30% of the memory recovered
appears to be used again). With the exception of V8, it takes only 20ms or so to
do the purge, but it’s not very effective. V8 adds another 700ms or so to the
time it takes to purge.

## Appendix: “Where does our memory go?”

Memory consumption is notoriously tricky to measure and reason about, as Evan
recently noted in his blog post “[Some things I’ve learned about
memory](http://neugierig.org/software/blog/2011/05/memory.html)”. For a release
image of Chrome OS with the stock extensions installed, after logging in and
loading `about:blank`, `/proc/meminfo` looks like:

```
MemTotal: 1940064 kB &lt;---- ~2 GB minus kernel
MemFree: 1586544 kB
Buffers: 3640 kB
Cached: 248344 kB
Active: 110848 kB
Inactive: 210648 kB
Active(anon): 77216 kB \\____ ~120 MB dynamically allocated “anonymous memory”
Inactive(anon): 48744 kB /
Active(file): 33632 kB \\____ ~200 MB file-backed executables, mmap’d files,
etc.
Inactive(file): 161904 kB / some of which can be released
...
AnonPages: 69512 kB
Mapped: 85740 kB
Shmem: 56448 kB &lt;---- ~50 MB shared memory, mostly from video driver
Slab: 15096 kB
SReclaimable: 6476 kB
SUnreclaim: 8620 kB
KernelStack: 1288 kB
PageTables: 3380 kB
After opening gmail.com, calendar.google.com, and
[espn.go.com](http://espn.go.com/) (image and Flash heavy), /proc/meminfo
changes as follows:
Active(anon): 277856 kB \\___ ~390 MB dynamically allocated, up from ~120
Inactive(anon): 111440 kB /
Active(file): 50912 kB \\___ ~220 MB file-backed, up from ~200
Inactive(file): 174260 kB /
...
Shmem: 140012 kB &lt;--- ~140 MB, up from ~50
```

File-backed memory doesn’t change much, because most of it is executables and
mmap’d resource files that don’t change. Renderers account for most of the
change in dynamically allocated memory, though the browser process grows a
little for each new renderer. Both Flash and WebGL content will cause the video
driver to consume additional memory. (Of note, R12 had a bug where video streams
would cause the video driver’s memory consumption to grow rapidly, which was the
primary cause of “He’s dead Jim” in the cases I investigated.)

From a userspace perspective, most of the renderer’s memory consumption is
images and strings, depending on the source site. For example, tcmalloc heap
profiling of a renderer on gmail.com shows the top 5 consumers:

```
Total: 27.5 MB
15.7 57.0% 57.0% 15.7 57.0% WTF::fastMalloc
4.3 15.5% 72.5% 4.3 15.5% pixman_image_create_bits (inline)
4.1 15.0% 87.5% 4.1 15.0% std::string::_Rep::_S_create (inline)
0.8 2.9% 90.4% 0.8 2.9% v8::internal::Malloced::New
0.6 2.2% 92.6% 0.6 2.2% sk_malloc_flags
```

The graphical profile (see attachment "pprof.renderer.gmail.svg") shows
`WTF::fastMalloc` is allocating strings, mostly for `HTMLTreeBuilder`.
For `espn.go.com`, an image heavy site, the heap profile looks like:

```
Total: 26.8 MB
11.4 42.6% 42.6% 11.4 42.6% WTF::fastMalloc
10.7 40.0% 82.5% 10.7 40.0% sk_malloc_flags
2.8 10.3% 92.8% 2.8 10.3% v8::internal::Malloced::New
0.2 0.7% 93.5% 0.2 0.7% WebCore::Text::create
0.2 0.6% 94.1% 0.2 0.8% SkGlyphCache::VisitCache
```

The graphical profile (see attachment "pprof.renderer.espn.svg") shows
`WTF::fastMalloc` is still strings, but now `sk_malloc_flags` is allocating large
image buffers, from WebCore::PNGImageDecoder and JPEGImageDecoder.
Work is underway to reduce WebKit memory consumption for 8-bit strings, see
<https://bugs.webkit.org/show_bug.cgi?id=66161>

Some WebKit fixes recently landed to make decoded image pruning more efficient:
<https://bugs.webkit.org/show_bug.cgi?id=65859>

All that said, users will always be able to fill available memory by opening a
large number of tabs, or using web sites that bloat with DOM nodes or JS
objects. We will always need some mechanism to handle the out-of-memory case.

## Items for engineering review (added September 6):

A. We can take three general approaches in OOM situations:

1.  Stop the user from consuming more RAM. (Warn them with a butter-bar,
    then eventually block them from opening more tabs, etc.)
2.  Spend CPU to simulate having more RAM (zram memory compression,
    process hibernation?)
3.  Throw away data we can quickly/safely regenerate

We’re afraid 1 is annoying and 2 may be slow. We’re proposing trying 3, and we
can easily fall back to 1 if it doesn’t work out.

B. On more reflection, the soft wall should probably be more than “one tab’s
worth” of memory away, perhaps 25 or 50 MB. That will give us much more time to
do work. How do we figure out the right value?

C. This design intentionally does not address Chrome’s baseline memory usage or
cases where Chrome might leak or bloat. We’ve got extra instrumentation in R14
to help find end-user cases where Chrome on Chrome OS uses a lot of RAM. We’d
like to do something simple with the OOM case, and spend more time
finding/fixing specific cases of bloat.

D. We could add some sort of subtle notification when Chrome hits the soft wall.
Glen proposed badging the wrench icon and providing a menu item like “Chrome
needs attention” that would take you to the task manager and show you processes
that are eating a lot of RAM or CPU. This might help power users find and report
cases of bloat. Don’t - log it and learn.

E. We can probably do testing of the signals sent from the kernel with autotest.
The correctness of the kill-priority algorithm can be tested with unit and
browser tests. The discard/kill process itself can be tested with browser tests.

## Notes from design review (added September 6):

Overall, we want to build something simple, get it in the field, collect
log/histogram data and iterate to ensure we’re killing the right tabs. We don’t
want to add any UI - managing memory is our problem, not the user’s problem.

Action: Implement an experiment for Chrome users where we test our killing
algorithm by testing which tab the user is most likely to close or never visit
again.

Action: Log how much memory you get back from each close/kill so that you can
start to develop an algorithm for which tab might get you the most memory back.
GPU process can run out of aperture space and the GL calls can fail. Must
reclaim memory by closing GPU intensive tabs. Probably needs a GPU-specific
implementation, not part of this feature.

Action: Add a global notification that we are low on memory, and different
components can respond appropriately.

Perhaps it would be good to implement logging per user to determine what their
patterns are to feed into the algorithm.

What to do about parent/child tabs (gmail tearoffs, etc.) Can we restore this
connection?

Decision: delay implementing zram until we know more about whether we want it
for extending memory.

Action: Implement an about: page that describes tab close/kill order at the
moment. Could also show historical kill/close information.

Action: Design an experiment for the size of the “soft wall” memory padding.
