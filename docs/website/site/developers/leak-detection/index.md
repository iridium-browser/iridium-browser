---
breadcrumbs:
- - /developers
  - For Developers
page_name: leak-detection
title: Leak Detection
---

When launching a new feature it is important to consider doing manual inspection
to look for memory leaks since some types of leaks will not easily show up
during other testing, especially if they are slow leaks. This can avoid bugs
which are otherwise difficult to detect.

You can look for memory leaks on Windows using ETW heap tracing using
[UIforETW](https://randomascii.wordpress.com/2015/09/01/xperf-basics-recording-a-trace-the-ultimate-easy-way/),
an open-source wrapper for Microsoft's ETW/WPT toolkit. To set up for heap
tracing you need to download and run UIforETW, then:

*   Click on Settings and set "Heap-profiled processes" to chrome.exe,
            check "Chrome developer", then close Settings
*   Change the tracing type (just below "Input tracing") to "Heap
            tracing to file"
*   Close all unnecessary programs including other copies of Chrome in
            order to reduce the trace size and noise
*   If you need to record allocations that happen during startup then
            start tracing and then launch Chrome. If not then you should launch
            Chrome and wait for its startup activity to calm down. If you launch
            Chrome with a minimal set of tabs then this will reduce the trace
            size and noise
*   When you are ready to test your feature then type Ctrl+Win+R (with
            any window active) or click "Start Tracing" in UIforETW
*   Run your test a few times, let Chrome go idle again, and then type
            Ctrl+Win+R (with any window active) or click "Save Trace Buffers" in
            UIforETW

Tips for efficient tracing:

*   If you want to just heap-profile a renderer process then you should
            launch Chrome *before* setting the tracing type. This will ensure
            that heap tracing is disabled for the browser, GPU, utility,
            extension, crashpad, and watcher processes. Only processes launched
            *after* you change the tracing type to "Heap tracing to file" will
            be heap traced, and only allocations/frees done after you start
            tracing will be recorded.
*   If you are only going to be looking at heap data then uncheck
            "Context switch call stacks" and "CPU sampling call stacks" in order
            to keep the trace as small as possible

These trace analysis instructions are very terse because a full explanation
would require many pages. If you need assistance then please contact
brucedawson@chromium.org.

If you have Python installed then IdentifyChromeProcesses.py will be run and a
list of Chrome process types and PIDs will be printed, which helps with
identifying the browser process, etc. If you need to identify a specific render
process PID then use Chrome's Task Manager. You can copy the trace to a
different machine for analysis, and you can manually run
IdentifyChromeProcesses.py on that machine by right-clicking on the trace name
and selecting Scripts-&gt; Identify Chrome Processes.

Open the trace in WPA. The default view is designed for analyzing performance
rather than allocations so you will want to use HeapAnalysis.wpaProfile from
UIforETW's bin directory (available in versions published in September 2018 or
later). You can apply this profile by going to the Profiles menu in WPA,
selecting Apply... and then browsing to the .wpaProfile file. Load symbols
(Trace-&gt; Load Symbols) and you can then drill down into the allocations for
the process of interest. The Type column will usually list AIFI and AIFO types.
AIFI means Allocated Inside and Freed Inside - meaning that the allocation was
transient within the visible time range. AIFO means Allocated Inside and Freed
Outside meaning that the memory was allocated but not freed within the visible
time range. AIFO allocations are potential leaks. You can drill down through the
stack, sorting by Count, or Size, or you can search for particular symbols by
typing Ctrl+F when the Stack column is selected.

The allocation times for the selected portion of stack will be highlighted in
the graph area, giving clues to patterns. These can be compared against input
events emitted by UIforETW.exe.

It is also possible to use Visual Studio's heap profiler. This is particularly
convenient for profiling the browser process at startup.
