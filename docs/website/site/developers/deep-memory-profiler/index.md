---
breadcrumbs:
- - /developers
  - For Developers
page_name: deep-memory-profiler
title: Deep Memory Profiler
---

## Update 13-Oct-2015:

The Deep Memory Profiler tools (dmprof) were not maintained, became
non-functional, and were **removed** from the codebase . See [issue
490464](https://code.google.com/p/chromium/issues/detail?id=490464).

## Introduction

Deep Memory Profiler (dmprof) is a 1) whole-process, 2) timeline-based and 3)
post-mortem memory profiler for Chromium. It's designed to categorize **all
memory usage in a process without any omission**. See the [Design
Doc](https://docs.google.com/document/d/1_Cw9FfzdRT2kjozkNK9cZOXedE8-1fGDqnUvdW_tpGs/edit)
if interested.

Memory bloat has been a serious issue in Chromium for years. Bloat is harder to
fix than leak and errors. We have memory checkers like
[Valgrind](/developers/how-tos/using-valgrind) and [Address
Sanitizer](/developers/testing/addresssanitizer) for leak and errors, but no
handy tools for bloat. Dmprof is an easy-to-use tool for bloat to know “who is
the memory eater”. (It would be helpful also for leak.)

### Announcement

Future announcements (e.g. command line changes) will be done in
[dmprof@chromium.org](https://groups.google.com/a/chromium.org/forum/?fromgroups#!forum/dmprof).
Subscribe it if you're interested.

document.getElementById('form39898447').submit();

### Sub-profilers

*   [C++ Object Type
            Identifier](/developers/deep-memory-profiler/cpp-object-type-identifier)
            (a.k.a. Type Profiler)

## How to Use

Dmprof profiles memory usage post-mortem. You need to 1) get memory dumps while
Chrome is running, and then 2) analyze the dumps. The analyzing script dmprof is
available in
[src/tools/deep_memory_profiler](http://src.chromium.org/viewvc/chrome/trunk/src/tools/deep_memory_profiler/)
of the Chromium source tree, or can be retrieved by downloading and running
[download.sh](http://src.chromium.org/viewvc/chrome/trunk/src/tools/deep_memory_profiler/download.sh).

### Phase 1: Build Chromium

You can use dmprof for Chromium **static builds** with some build options both
for **Release and Debug builds**. Note that it is not available for **shared
builds**.

#### Linux and ChromeOS-Chrome

*   Release builds - use **GYP_DEFINES='$GYP_DEFINES profiling=1
            profiling_full_stack_frames=1 linux_dump_symbols=1'**
*   Debug builds - use **GYP_DEFINES='$GYP_DEFINES profiling=1
            disable_debugallocation=1'**

#### Android

Be careful not to overwrite GYP_DEFINES predefined by env_setup.sh.

*   Release builds - use **GYP_DEFINES='$GYP_DEFINES profiling=1
            profiling_full_stack_frames=1 linux_dump_symbols=1
            use_allocator="tcmalloc"'**
*   Debug builds - use **GYP_DEFINES='$GYP_DEFINES profiling=1
            disable_debugallocation=1 android_full_debug=1
            use_allocator="tcmalloc"'**

#### ChromeOS / ChromiumOS

*   Release builds - use **USE='... deep_memory_profiler'**
*   Debug builds - not maintained

### Phase 2: Run Chromium

### Remember the Process ID of your target process! (c.f. about:memory)

#### Linux and ChromeOS-Chrome

Run Chromium with a command-line option --no-sandbox and the following
environment variables:

1.  HEAPPROFILE=/path/to/prefix (Files like /path/to/prefix.0001.heap
            are dumped from Chrome.)
2.  HEAP_PROFILE_MMAP=1
3.  DEEP_HEAP_PROFILE=1
4.  (If you get dumps from a standalone Chromium process periodically,
            specify HEAP_PROFILE_TIME_INTERVAL=&lt;interval seconds between
            dumping&gt;)

> `$ HEAPPROFILE=$HOME/prof/prefix HEAP_PROFILE_TIME_INTERVAL=20
> HEAP_PROFILE_MMAP=1 DEEP_HEAP_PROFILE=1 out/Release/chrome --no-sandbox`

#### Android

```none
# NOTE: the device needs to be rooted!
adb root
# Create a world-writable dump directory
adb shell mkdir -p /data/local/tmp/heap
adb shell chmod 0777 /data/local/tmp/heap
# Set the following system properties
adb shell setprop heapprof /data/local/tmp/heap/dumptest
adb shell setprop heapprof.mmap 1
adb shell setprop heapprof.deep_heap_profile 1
adb shell setprop heapprof.time_interval 10
```

#### ChromeOS / ChromiumOS

Prepare a file /var/tmp/deep_memory_profiler_prefix.txt in your device or VM
image. Its content should just have a prefix to dumped files /path/to/prefix
like:

```none
/var/tmp/heapprof
```

If you get dumps from a standalone Chromium process periodically, prepare a file
/var/tmp/deep_memory_profiler_time_interval.txt which contains just an integer
meaning the interval time to dump:

```none
60
```

### Phase 3: Make Chromium dump

#### Standalone (automatic periodical dump)

If you specified "time interval" described above, the Chromium process dumps
heap profiles periodically. It depends on the platform how to specify "time
interval".

#### WebDriver (ChromeDriver)

A WebDriver (ChromeDriver) API function is available to dump. Try
dump_heap_profile():

> `dump_heap_profile(reason='Why you want a dump.')`

Note that you may want to copy the dumps from your remote machine if you run
ChromeDriver remotely.

#### Memory Benchmarking V8 API

You can dump heap profiles manually from DevTools (or automatically with your
script) when you set the environment variables above and specify the command
line option --enable-memory-benchmarking to Chrome.

```none
chrome.memoryBenchmarking.heapProfilerDump("browser" or "renderer", "some reason")
```

#### Telemetry

You can use a Telemetry profiler to drive chrome and, if running on android, to
also fetch the files from the device. It utilizes the above Memory Benchmarking
V8 API internally. See [Telemetry
instructions](http://www.chromium.org/developers/telemetry/profiling#TOC-Memory-Profiling---Linux-Android).

### Phase 4: Get the dumps

Find the dumps at /path/to/prefix.\*.heap as specified above. Names of the dumps
include Process IDs. For example, prefix.12345.0002.heap for pid = 12345. '0002'
is a sequence number of the dump.

The dmprof analyzer script (in the Phase 5) needs the executed Chrome binary to
extract symbols. If you analyze in the same host with the Chrome process, you
don't need special things. If you analyze in a different host from the Chrome
process (it's common in Android and ChromeOS), you may need to specify the path
to the executed Chrome binary on the host. (See Phase 5 for details.)

#### Linux and ChromeOS-Chrome

Heap profiles are dumped to the directory where you specified in the environment
variable HEAPPROFILE.

#### Android

Heap profiles are dumped in the device directory where you specified in the
property heapprof. Fetch the dump from the device to your host by the following
command.

```none
adb pull /data/local/tmp/heap/ .
```

The dmprof script guesses the path of the Chrome binary on the host if the
executed binary was in an Android-specific directory (e.g. /data/app-lib/). If
the guess fails, use --alternative-dirs for the dmprof script (in the Phase 5).

#### ChromeOS / ChromiumOS

Heap profiles are dumped in the device directory where you specified in
/var/tmp/deep_memory_profiler_prefix.txt. Fetch them in a certain way like scp
from the device / VM.

The dmprof script doesn't know where is the Chrome binary on the host. Use
--alternative-dirs for the dmprof script (in the Phase 5).

### Phase 5: Analyze the dumps

Use the dmprof script to analyze with the dumped heap profiles.

1.  Run the analyzing script dmprof, and redirect its stdout to a CSV
            file. You can choose a policy with -p option. Details of policies
            are described below.
    dmprof csv /path/to/first-one-of-the-dumps.heap &gt; result.csv
2.  Copy the CSV into a spreadsheet application, for example, OpenOffice
            Calc and Google Spreadsheet.
3.  Draw a (stacked) line chart on the spreadseet for columns from
            FROM_HERE_FOR_TOTAL to UNTIL_HERE_FOR_TOTAL. (See [the
            example](https://docs.google.com/spreadsheet/ccc?key=0Ajevw6ceMEA7dHZzNHFGRlRKcHItT21XNVlhR3Y3RWc).)

```none
$ dmprof csv dmprof.03511.0001.heap > dmprof.03511.result.csv
```

Or, use this to generate a local html with a graph and a data table:

```none
# Generate a JSON output for your dumps.
$ tools/deep_memory_profiler/dmprof json /path/to/first-one-of-the-dumps.heap > FOO.json
# Convert the JSON file to a HTML file.
$ tools/deep_memory_profiler/graph.py FOO.json > graph.html
```

In case of Android or ChromeOS, you may need to use --alternative-dirs for the
dmprof script to specify the path of the Chrome binary on the host instead of
the path on the device.

#### Android

```none
$ dmprof csv --alternative-dirs=/data/app-lib/com.google.android.apps.chrome-1@/home/self/chrome/src/out/Release/lib dmprof.03511.0001.heap
Assuming /data/app-lib/com.google.android.apps.chrome-1 on device as /home/self/chrome/src/out/Release/lib on host
...
```

#### ChromeOS / ChromiumOS

```none
$ dmprof csv --alternative-dirs=/opt/google/chrome@/home/self/chromeos/chroot/build/x86-generic/opt/google/chrome dmprof.03511.0001.heap
Assuming /opt/google/chrome on device as /home/self/chromeos/chroot/build/x86-generic/opt/google/chrome on host
...
```

### Phase 6: Drill down into components / backtraces

Given a specific component listed in the graph above, sue the following command
to

## ```none
# HEAP_FILE => TCMalloc .heap file
# COMPONENT => One of the components displayed in the graph.
# POLICY => the json file used to generate the graph.
tools/deep_memory_profiler/dmprof expand HEAP_FILE POLICY COMPONENT 32
```

## Caveats

### Linux / ChromeOS / Android

*   The total size can be different from RSS reported by ps and
            /proc/.../stat iin some cases.
    *   RSS by ps and stat doesn't include a part of memory usage. For
                example, the page tables, kernel stack, struct thread_info, and
                struct task_struct (from man ps). The same difference is in
                smaps and pmap.
    *   Deep Memory Profiler can double-count page frames (physical
                pages) which are mapped from different virtual pages in the same
                process. It hardly happens in Chrome.

## How to read the graph

Graphs are typical stacked ones which classifies all memory usage into some
components. We have four kinds of graphs from each execution dump based on four
built-in "policies": l0, l1, l2, t0.

We're preparing more breakdown policies. If you'd like to add a criteria for
your component, 1) modify policy.\*.json files, 2) modify polices.json and add a
policy file or 3) report it to dmprof@chromium.org.

**Common components**

<table>
<tr>
<td> mustbezero & unhooked-absent</td>
<td> Should be zero.</td>
</tr>
<tr>
<td> unhooked-anonymous</td>
<td> VMA is mapped, but no information is recorded. No label is given in /proc/.../maps.</td>
</tr>
<tr>
<td> unhooked-file-exec</td>
<td> VMA is mapped, but no information is recorded. An executable file is mapped.</td>
</tr>
<tr>
<td> unhooked-file-nonexec</td>
<td> VMA is mapped, but no information is recorded. A non-executable file is mapped.</td>
</tr>
<tr>
<td> unhooked-file-stack</td>
<td> VMA is mapped, but no information is recorded. Used as a stack.</td>
</tr>
<tr>
<td> unhooked-other</td>
<td> VMA is mapped, but no information is recorded. Used for other purposes.</td>
</tr>
<tr>
<td> no-bucket</td>
<td> Should be small. Out of record because it is an ignorable small blocks.</td>
</tr>
<tr>
<td> tc-unused</td>
<td> Reserved by TCMalloc, but not used by malloc(). Headers, fragmentation and free-list.</td>
</tr>
</table>

**Policy "l0"**

This policy applies the most rough classification.

<table>
<tr>
<td> mmap-v8</td>
<td> mmap'ed for V8. It includes JavaScript heaps and JIT compiled code.</td>
</tr>
<tr>
<td> mmap-catch-all</td>
<td> mmap'ed for other purposes.</td>
</tr>
<tr>
<td> tc-used-all</td>
<td> All memory blocks allocated by malloc().</td>
</tr>
</table>

**Policy "l1"**

It breaks down memory usage into relatively specific components.

<table>
<tr>
<td> mmap-v8-heap-newspace</td>
<td> JavaScript new (nursery) heap for younger objects.</td>
</tr>
<tr>
<td> mmap-v8-heap-coderange</td>
<td> Code produced at runtime including JIT-compiled JavaScript code.</td>
</tr>
<tr>
<td> mmap-v8-heap-pagedspace</td>
<td> JavaScript old heap and many other object spaces.</td>
</tr>
<tr>
<td> mmap-v8-other</td>
<td> Other regions mmap'ed by V8.</td>
</tr>
<tr>
<td> mmap-catch-all</td>
<td> Any other mmap'ed regions.</td>
</tr>
<tr>
<td> tc-v8</td>
<td> Blocks allocated from V8.</td>
</tr>
<tr>
<td> tc-skia</td>
<td> Blocks allocated from Skia.</td>
</tr>
<tr>
<td> tc-webkit-catch-all</td>
<td> Blocks allocated from WebKit.</td>
</tr>
<tr>
<td> tc-unknown-string</td>
<td> Blocks which are related to std::string.</td>
</tr>
<tr>
<td> tc-catch-all</td>
<td> Any other blocks allocated by malloc().</td>
</tr>
</table>

**Policy "l2"**

It tries to breakdown memory usage into specific components. See
[policy.l2.json](http://src.chromium.org/viewvc/chrome/trunk/src/tools/deep_memory_profiler/policy.l2.json?view=markup)
for details, and report to dmprof@chromium.org to add more components.

<table>
<tr>
<td> mmap-v8-heap-newspace</td>
<td> JavaScript new (nursery) heap for younger objects.</td>
</tr>
<tr>
<td> mmap-v8-heap-coderange</td>
<td> Code produced at runtime including JIT-compiled JavaScript code.</td>
</tr>
<tr>
<td> mmap-v8-heap-pagedspace</td>
<td> JavaScript old heap and many other spaces.</td>
</tr>
<tr>
<td> mmap-v8-other</td>
<td> Other regions mmap'ed by V8.</td>
</tr>
<tr>
<td> mmap-catch-all</td>
<td> Any other mmap'ed regions.</td>
</tr>
<tr>
<td> tc-webcore-fontcache</td>
<td> Blocks used for FontCache.</td>
</tr>
<tr>
<td> tc-skia</td>
<td> Blocks used for Skia.</td>
</tr>
<tr>
<td> tc-renderobject</td>
<td> Blocks used for RenderObject.</td>
</tr>
<tr>
<td> tc-renderstyle</td>
<td> Blocks used for RenderStyle.</td>
</tr>
<tr>
<td> tc-webcore-sharedbuf</td>
<td> Blocks used for WebCore's SharedBuffer.</td>
</tr>
<tr>
<td> tc-webcore-XHRcreate</td>
<td> Blocks used for WebCore's XMLHttpRequest (create).</td>
</tr>
<tr>
<td> tc-webcore-XHRreceived</td>
<td> Blocks used for WebCore's XMLHttpRequest (received). </td>
</tr>
<tr>
<td> tc-webcore-docwriter-add</td>
<td> Blocks used for WebCore's DocumentWriter.</td>
</tr>
<tr>
<td> tc-webcore-node-and-doc</td>
<td> Blocks used for WebCore's HTMLElement, Text, and other Node objects.</td>
</tr>
<tr>
<td> tc-webcore-node-factory</td>
<td> Blocks created by WebCore's HTML\*Factory.</td>
</tr>
<tr>
<td> tc-webcore-element-wrapper</td>
<td> Blocks created by WebCore's createHTML\*ElementWrapper.</td>
</tr>
<tr>
<td> tc-webcore-stylepropertyset</td>
<td> Blocks used for WebCore's StylePropertySet (CSS).</td>
</tr>
<tr>
<td> tc-webcore-style-createsheet</td>
<td> Blocks created by WebCore's StyleElement::createSheet.</td>
</tr>
<tr>
<td> tc-webcore-cachedresource</td>
<td> Blocks used for WebCore's CachedResource.</td>
</tr>
<tr>
<td> tc-webcore-script-execute</td>
<td> Blocks created by WebCore's ScriptElement::execute.</td>
</tr>
<tr>
<td> tc-webcore-events-related</td>
<td> Blocks related to WebCore's events (EventListener and so on)</td>
</tr>
<tr>
<td> tc-webcore-document-write</td>
<td> Blocks created by WebCore's Document::write.</td>
</tr>
<tr>
<td> tc-webcore-node-create-renderer</td>
<td> Blocks created by WebCore's Node::createRendererIfNeeded.</td>
</tr>
<tr>
<td> tc-webcore-render-catch-all</td>
<td> Any other blocks related to WebCore's Render.</td>
</tr>
<tr>
<td> tc-webcore-setInnerHTML-except-node</td>
<td> Blocks created by setInnerHTML.</td>
</tr>
<tr>
<td> tc-wtf-StringImpl-user-catch-all</td>
<td> Blocks used for WTF::StringImpl.</td>
</tr>
<tr>
<td> tc-wtf-HashTable-user-catch-all</td>
<td> Blocks used for WTF::HashTable.</td>
</tr>
<tr>
<td> tc-webcore-everything-create</td>
<td> Blocks created by WebCore's any create() method.</td>
</tr>
<tr>
<td> tc-webkit-from-v8-catch-all</td>
<td> Blocks created by V8 via WebKit functions.</td>
</tr>
<tr>
<td> tc-webkit-catch-all</td>
<td> Any other blocks created by WebKit.</td>
</tr>
<tr>
<td> tc-v8-catch-all</td>
<td> Any other blocks created in V8.</td>
</tr>
<tr>
<td> tc-toplevel-string</td>
<td> All std::string objects created at the top-level.</td>
</tr>
<tr>
<td> tc-catch-all</td>
<td> Any other blocks by malloc().</td>
</tr>
</table>

**Policy "t0"**

It classifies memory blocks based on their type_info.

<table>
<tr>
<td> mmap-v8</td>
<td> mmap'ed for V8. It includes JavaScript heaps and JIT compiled code.</td>
</tr>
<tr>
<td> mmap-catch-all</td>
<td> mmap'ed for other purposes.</td>
</tr>
<tr>
<td> tc-std-string</td>
<td> std::string objects.</td>
</tr>
<tr>
<td> tc-WTF-String</td>
<td> WTF::String objects.</td>
</tr>
<tr>
<td> tc-no-typeinfo-StringImpl</td>
<td> No type_info (not allocated by 'new'), but allocated for StringImpl.</td>
</tr>
<tr>
<td> tc-Skia</td>
<td> Skia objects.</td>
</tr>
<tr>
<td> tc-WebCore-Style</td>
<td> WebCore's style objects.</td>
</tr>
<tr>
<td> tc-no-typeinfo-other</td>
<td> Any other blocks without type_info.</td>
</tr>
<tr>
<td> tc-other</td>
<td> All objects with other type_info.</td>
</tr>
</table>

## Cases

*   <http://crrev.com/166963>
