---
breadcrumbs:
- - /developers
  - For Developers
page_name: memory_watcher
title: Memory_Watcher
---

**Memory_Watcher is no longer supported. Either tcmalloc tools or UMDH on
Windows are good alternatives.**

**Document kept for historical interest**

---

**OVERVIEW**

Memory watcher is a tool that helps to attribute blame for memory usage on a
windows build. It was originally coded predominantly by Mike Belshe, and was
recently ressurected (and made to run on Vista).

Memory_watcher provides a dump for each Chromium process, of the aggregate
information on all memory that is currently in use. It gathers this information
by recording stack traces whenever any form of dynamic memory allocation is
requested (i.e., malloc, HeapAlloc, VirtualAlloc, GlobalHeapAlloc,
LocalHeapAlloc). The dump for each process includes, for each stack that was
used above an allocation, a count of the number of still-live allocations, and
the total byte counts for those still-live regions. The dumped output is in
human readable format (ASCII). The dump is sorted so that the stacks that
induced the largest byte counts are listed first. Note that call counts and
byte-allocation tallies are not included for memory that was freed (or
decommitted). \[Future versions may tally stack traces that thrash through
allocations and frees in a second per-process dump file\]

A second included tool can process the dump file from any of the processes
(i.e., either the browser process, or any of the renderer processes) and
attribute blame to roughly 20 distinct users of memory. This latter tool will
evolve over time to be more precise (and hopefully helpful).

**HOW TO BUILD AND RUN CHROMIUM WITH THE MEMORY_WATCHER TOOL ACTIVE**

We suggest that you build Chrome using the "Purify" setting, rather than Debug
or Release. Debug may have different allocation results from the Release, and
Release has cryptic stacks. You'll probably get the best results using purify,
which turns off the DEBUG macros, but avoids optimizations that obscure the
stacks.

After you've built chrome, select and build the tools-&gt;memory_watcher module
(right click on it in the "Solution Explorer" tab, and select "Build").

Currently, memory_watcher works only with the Windows Default allocator (the
amount of allocated memory should be identical to what happens with other
allocators, as only the varying overhead, and fragmentation impact, will not be
tallied). To insure that you are using that allocator, you need to either set an
environment variable (set CHROME_ALLOCATOR to "WINHEAP" and not to "JEMALLOC",
"WINLFH", or "TCMALLOC"; don't set CHROME_ALLOCATOR_2), or you can more
thoroughly change the default by editing
src/thirdparty/tcmalloc/allocator_shim.cc. To change the default in that source
code, you should change the line that reads:

// This is the default allocator.

static Allocator allocator = **TCMALLOC**;

to:

// This is the default allocator.

static Allocator allocator = **WINHEAP**; // Equivalent to setting environment
variable to "WINHEAP".

Also change the specification of the subprocess allocator by changing:

char\* secondary_value = secondary_length ? buffer : "**TCMALLOC**";

to:

char\* secondary_value = secondary_length ? buffer : "**WINHEAP**";

After making those changes, you'll need to rebuild again, but it will complete
relatively quickly.

Finally, you'll need to modify the command line arguments used for starting
chrome. You'll need the command line:

--memory-profile --no-sandbox --js-flags="--noprof"

You are now ready to run chrome, and gather stats. Chrome will run a little
slower, but it is doing a lot more work on each allocation (i.e., it is
recording stack traces). The directory that you use to run chrome will be the
directory where the stack dumps appear. If you are running under a vanilla setup
with MSVC, this will typically be the src/chrome directory, and NOT for instance
the src/chrome/Purify directory.

**HOW TO GET CHROME TO DUMP THE LIST OF TRACES FOR CURRENTLY ALLOCATED MEMORY**

When you have reached an interesting point that you would like to understand,
you can press:

Ctrl-Alt-d

You can only do this once during a run of Chrome. Pressing it again and again
will do nothing :-/.

It takes a while to complete a dump. The dump files will accumulate in files
named memory_watcher.logNNNN.tmp, where NNNN is the process id of the dumping
process. When the dump is complete, the file will be renamed to
memory_watcher.logNNNN. Please do not terminate Chrome while the dumping is in
progress, or you will get an incomplete dump.

**HOW CAN I EXAMINE THE DUMP FILE**

The easiest way is with a text editor, as it is plain ASCII. Unfortunately, it
is VERY large, and there will typically be many distinct stacks. To get a
summary of any individual dump file, run the script file
src/tools/memory_watcher/scripts/summary.pl with the dump file piped in as
standard input. For example:

./summary.pl &lt;memory_watcher.logNNNN

If you have a number of process dumps, it may be convenient to run something
like the following in an Msysgit bash window:

for i in memory_watcher.log???? ; do echo summary.pl from file $i; ./summary.pl
&lt;$i; echo; done

**CAVEATS**

1) The total memory listed appears smaller than I would expect, based on other
memory tools. I don't yet have the complete explanation for this, but some of
the missing memory is surely static initialized data, and code space. Still,
that cannot account for what is missing, and this tool will hopefully get better
over time.

2) The summary.pl file is NOT perfect. It merely attempts to find an
"interesting" stack frame, and use that to decisively blame the stack on a
specific module. Over time, it will get better. If you think that some dump
should be refined in greater detail, and some attribution is too coarse, take a
look inside the summary.pl file. You can make a minor edit to cause the
contributors to any bucket-of-blame to be examined more closely, and any "large"
contributors will be listed separately (from said bucket), along with their
individual stacks. After you've made the edit (and perchance asked for details
of contributors to said bucket that are over 100,000 bytes), you should run the
summary.pl script. When you see the stacks, and realize the error of blame, you
can then add a new rule to the summary.pl file, and run it again. If you have a
great addition, please consider landing a change list with your improvements!!!

This is a work-in-progress. Please feel free to help it along, or to make
suggestions.

Thanks,

Jim Roskind \[jar at chromium dot org\]
