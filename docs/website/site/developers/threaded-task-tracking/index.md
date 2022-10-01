---
breadcrumbs:
- - /developers
  - For Developers
page_name: threaded-task-tracking
title: Thread and Task Profiling and Tracking
---

**Introduction**

Within Chromium, tasks are regularly dispatched between various threads, by
"posting a task," or between processes, by receiving an IPC directive. This code
structure can make it difficult for a standard instruction profiler to
understandably attribute execution costs. It also makes it difficult to follow
the flow of code in a debugger.

At the about:profiler internal URL, Chromium provides a profiling and debugging
infrastructure: It provides answers to questions such as: What tasks are taking
a "long" time? What tasks are called "too often?" What tasks are suffering
"significant" queueing delay? How busy is each thread? etc.

This document describes some of the features supported by the about:profiler
infrastructure.

**Overview**

The infrastructure described has three basic uses:

1.  performance profiling (to identify potential performance and jank
            optimizations);
2.  learning about the codebase (see what tasks run, and when);
3.  debugging (checking to see task are called the correct number of
            times).

Posted-tasks and IPC tasks are tracked, along with their performance
characteristics, across the life-cycle of each task. Information is maintained
about where each task was constructed/posted (function name, file and line
number), along with the thread that was active when the task created. After each
task has been executed, information about the run of the task is tallied.
Information includes where it executed (thread, process, etc.), as well as
counts and durations (time, queueing time, etc.) All that information is
compactly accumulated in real time, while Chromium is running.

The accumulated performance characteristics are all available during a any
session. Additional reference snapshots can be taken at any time during the
execution of the browser. The results, as well as deltas between snapshots, can
be viewed, sorted, aggregated, and filtered, all within the about:profiler page.

For learning about the codebase, is is very educational to perform some browsing
actions, and then look and see what collection of chromium tasks were fired on
various threads. Rather than pouring over the codebase, it is often more
educational to look at these lists of tasks. The
[about:tracing](/developers/how-tos/trace-event-profiling-tool) support is also
extremely useful for more detailed investigation in this regard.

**Common Profiling Activities When Trying to Identify Performance Issues**

**Overall Performance: Task Execution Counts**

The easiest way to improve performance is to do less work. If you wish to
identify tasks that are running "too often," you should bring up the
about:profiler page, which will by default sort all entry by the number of times
each task was performed (its "count"). The page will have a header above a table
that looks like:

<table>
<tr>
<td>36,224,621</td>
<td>13,772,110</td>
<td>0</td>
<td>98,449</td>
<td>1</td>
<td>29,947</td>
<td>16 unique</td>
<td>17 unique</td>
<td>2 unique</td>
<td>19 unique</td>
<td>682 unique</td>
<td>702 unique</td>
</tr>
<tr>
Count\*\* Total run time Avg run time Max run time Avg queue time Max queue time Birth thread Exec thread Process type PID Function name Source location </tr>
<tr>
</tr>
</table>

The red asterisks in the column heading "Count" indicate that the results are
sorted by that column. The row above the column headings provides an aggregate
view of the table that appears below the he

Keep in mind that although high-frequency tasks (tasks with high "Counts") may
have a short run times, the overhead for such tasks is not included in their run
times. Overhead includes construction, posting to a queue (via locks), getting
time slices that other tasks could have enjoyed, cleaning up after a task is
executed, etc. As a result, reduction in the number of tasks, for ***very***
high frequency tasks may be very beneficial to performance.

**CPU Utilization: Where Is The Time Spent?**

If you wish to look for hot spots, or tasks that are using up a lot of the CPU
time, click on the "Total Run Time" column heading to sort on that column. All
times shown are in milliseconds.

The sorted table will highlight tasks which have total execution time that is
large. That statistic means that the product of the number of executions, and
the execution per run, was high. If that task can be made more efficient, it may
have a positive impact on overall CPU utilization. Caveat: by default, Execution
Time is measured with a wall clock, not based on activity of the CPU on the
thread. Beware of tasks that block, such as waiting for a DNS resolution.

**Memory usage and Churn**

As of M58, if chrome is launched with the flag
"--enable-heap-profiling=task-profiler", allocation metrics are captured and
aggregated to tasks. This adds a few new colums to the table.

<table>
<tr>
<td>7,888,368</td>
<td>1,084,910</td>
<td>0</td>
<td>4,635</td>
<td>44</td>
<td>600,000</td>
<td>30</td>
<td>30</td>
<td>218</td>
<td>76,455,816</td>
<td>46 unique</td>
<td>45 unique</td>
<td>4 unique</td>
<td>27 unique</td>
<td>1544 unique</td>
<td>1833 unique</td>
</tr>
<tr>
Count\*\*Total run timeAvg run timeMax run timeAvg queue timeMax queue timeAvg AllocationsAvg FreesAvg Net BytesMax allocated (outstanding) bytesBirth threadExec threadProcess typePIDFunction nameSource location</tr>
<tr>
</tr>
</table>

Of note here are the "Avg Allocations/Avg Frees", which relate the average
number of allocations/frees per task invocation. "Avg Net Bytes" reports the
average memory consumption of that individual task. Note that this can be a
positive or a negative number, as one task may allocate memory that's freed by
another, in which case the former is net positive, while the latter is net
negative. The "Max allocated (outstanding) bytes" reports the high watermark of
bytes allocated during any run of the task.

A few additional columns can be enabled by clicking on the \[columns\] link.

<table>
<tr>
<td>30</td>
<td>30</td>
<td>218</td>
<td>76,455,816</td>
<td>237,354,786</td>
<td>239,888,572</td>
<td>78,462,758,217</td>
<td>76,746,509,241</td>
<td>1,951,755,449</td>
</tr>
<tr>
Avg Allocations\*\*Avg FreesAvg Net BytesMax allocated (outstanding) bytesAllocation countFree CountAllocated bytesFreed bytesOverhead bytes</tr>
<tr>
</tr>
</table>

The "Allocation Count/Free Count" report the number of alloc/free operations
across all task invocations, and the "Allocated Bytes/Freed bytes" likewise
report the number of bytes allocated/freed across all invocations.

The "Overhead bytes" column, provided your allocator supports it, reports a
lower-bound estimate of the number of overhead bytes that go to heap metadata
and slack space in allocated blocks (due to allocation granularity). The
fraction "Overhead bytes/Allocated bytes" is a decent guesstimate for how
efficiently the task is using heap allocations - fewer, larger allocations have
lower overhead.

**Major Jank or Delay In Responsiveness**

When the browser is not responsive, the most common cause is that a task is
running too long, and has blocked the ability of some critical thread to
respond. Most obviously, this can happen on the CrBrowserMain (i.e., the UI
thread in the browser process). It is also common to see this caused by a slow
task on the Chrome_IOThread (i.e., the thread used for IPC communications with
the renderer). At times, some threads send messages to other threads to get work
results, and may be waiting (without officially "blocking") for a posted
response, so almost any blocked thread \*might\* be able to cause jank, but the
Chrome_FileThread (i.e., the thread that reads and writes from disk) is a most
common culprit.

To see what tasks have taken an unusually long time to execute, click to sort on
the "Max Run Time" column.

It is often helpful to view the results on various processes separately, as Jank
tends to be caused by delays in the Browser process. To separate out the
results, by process, find the drop-down box labeled "Group By" at the top of the
page. Select "Process Type" to cause the browser process to be isolated in its
own table. Once you select that, a second drop down will appear to the immediate
right of the first. You should then select "Exec Thread" in that pull-down. The
top of the page will then look like:

**Group by:** --- Process type PID Birth thread Exec thread Function name Source
location File name Line number --- Process type PID Birth thread Exec thread
Function name Source location File name Line number --- Process type PID Birth
thread Exec thread Function name Source location File name Line number

Separate tables will now be visible to make it more visible, for each thread,
what tasks is taking a "long time." You'll also notice that since each table
only list a single process type and a single execution thread, that the
corresponding columns are no longer shown in each table.

After you have identified a thread that appears to have abnormally slow
processes (that might cause jank), it is often helpful to see what tasks were
delayed (an that thread!) by such excesses.

To see what tasks have been forced to wait, click to sort on the Max Queue Time.
Assuming the observed execution time was long, you'll tyically see a number of
tasks that have a commensurate queuing delay.

**Quickly Drill Down**

If you know what you wan to look at (example: a specific file; a specific thead;
etc), you can instantly filter all your results to only show rows that contain
an arbitrary string. The text box at the top-right of the about:profiler page
provides automatic case-insensitive filtering.

For example, if you wanted to only see results that contain the word "browser"
(perhaps, items running on the "browser" thread), simply enter the text
"browser" (without the quotes) into the box at the top right. Similary, the box
could be used to illuminate only lines that contain "sync," or lines that
contain only "AnimationContainer::SetMinTimerInterval".

**Understanding Profiler Results from** about:profiler

When you visit the URL about:profiler, you'll get a set of lines which might
begin with the following (entry and select boxes don't seem to appear in this
documentation):

<table>
<tr>
<td><b>Group by: </b> --- Process type PID Birth thread Exec thread Function name Source location File name Line number <b>Sort by: </b> --- Count Total run time Avg run time Max run time Total queue time Avg queue time Max queue time Birth thread Exec thread Process type PID Function name Source location File name Line number Count (DESC) Total run time (DESC) Avg run time (DESC) Max run time (DESC) Total queue time (DESC) Avg queue time (DESC) Max queue time (DESC) Birth thread (DESC) Exec thread (DESC) Process type (DESC) PID (DESC) Function name (DESC) Source location (DESC) File name (DESC) Line number (DESC) --- Count Total run time Avg run time Max run time Total queue time Avg queue time Max queue time Birth thread Exec thread Process type PID Function name Source location File name Line number Count (DESC) Total run time (DESC) Avg run time (DESC) Max run time (DESC) Total queue time (DESC) Avg queue time (DESC) Max queue time (DESC) Birth thread (DESC) Exec thread (DESC) Process type (DESC) PID (DESC) Function name (DESC) Source location (DESC) File name (DESC) Line number (DESC) </td>
<td>\[snapshots\] \[columns\] </td>
</tr>
</table>

---

<table>
<tr>
<td>19,402</td>
<td>187,008</td>
<td>10</td>
<td>9,338</td>
<td>217</td>
<td>9,932</td>
<td>15 unique</td>
<td>17 unique</td>
<td>2 unique</td>
<td>18 unique</td>
<td>415 unique</td>
<td>430 unique</td>
</tr>
<tr>
Count\*\* Total run time Avg run time Max run time Avg queue time Max queue time Birth thread Exec thread Process type PID Function name Source location </tr>
<tr>
<td>990</td>
<td>249</td>
<td>0</td>
<td>28</td>
<td>7</td>
<td>363</td>
<td>Chrome_CacheThread</td>
<td>Chrome_IOThread</td>
<td>Browser</td>
<td>3036</td>
<td>disk_cache::InFlightIO::OnIOComplete</td>
<td><a href="http://chromesrc.appspot.com/?path=.%5Cdisk_cache%5Cin_flight_io.cc&line=62">in_flight_io.cc \[62\]</a></td>
</tr>
<tr>
<td>990</td>
<td>4,675</td>
<td>5</td>
<td>255</td>
<td>23</td>
<td>276</td>
<td>Chrome_IOThread</td>
<td>Chrome_CacheThread</td>
<td>Browser</td>
<td>3036</td>
<td>disk_cache::InFlightBackendIO::PostOperation</td>
<td><a href="http://chromesrc.appspot.com/?path=.%5Cdisk_cache%5Cin_flight_backend_io.cc&line=472">in_flight_backend_io.cc \[472\]</a></td>
</tr>
<tr>
<td>961</td>
<td>249</td>
<td>0</td>
<td>203</td>
<td>0</td>
<td>14</td>
<td>Chrome_IOThread</td>
<td>Chrome_IOThread</td>
<td>Browser</td>
<td>3036</td>
<td>net::SpdySession::ReadSocket</td>
<td><a href="http://chromesrc.appspot.com/?path=.%5Cspdy%5Cspdy_session.cc&line=773">spdy_session.cc \[773\]</a></td>
</tr>
<tr>
<td>831</td>
<td>4</td>
<td>0</td>
<td>1</td>
<td>18</td>
<td>282</td>
<td>CrBrowserMain</td>
<td>Chrome_IOThread</td>
<td>Browser</td>
<td>3036</td>
<td>IPC::ChannelProxy::Send</td>
<td><a href="http://chromesrc.appspot.com/?path=.%5Cipc_channel_proxy.cc&line=357">ipc_channel_proxy.cc \[357\]</a></td>
</tr>
<tr>
<td>561</td>
<td>1</td>
<td>0</td>
<td>1</td>
<td>42</td>
<td>278</td>
<td>CrBrowserMain</td>
<td>Chrome_IOThread</td>
<td>Browser</td>
<td>3036</td>
<td>IPC::ChannelProxy::Context::AddFilter</td>
<td><a href="http://chromesrc.appspot.com/?path=.%5Cipc_channel_proxy.cc&line=236">ipc_channel_proxy.cc \[236\]</a></td>
</tr>
<tr>
<td>470</td>
<td>3,480</td>
<td>7</td>
<td>1,286</td>
<td>986</td>
<td>2,960</td>
<td>Chrome_IOThread</td>
<td>CrBrowserMain</td>
<td>Browser</td>
<td>3036</td>
<td>IPC::ChannelProxy::Context::OnMessageReceivedNoFilter</td>
<td><a href="http://chromesrc.appspot.com/?path=.%5Cipc_channel_proxy.cc&line=116">ipc_channel_proxy.cc \[116\]</a></td>
</tr>
<tr>
<td>405</td>
<td>297</td>
<td>1</td>
<td>30</td>
<td>6</td>
<td>363</td>
<td>WorkerThread-\*</td>
<td>Chrome_IOThread</td>
<td>Browser</td>
<td>3036</td>
<td>base::win::ObjectWatcher::DoneWaiting</td>
<td><a href="http://chromesrc.appspot.com/?path=.%5Cwin%5Cobject_watcher.cc&line=130">object_watcher.cc \[130\]</a></td>
</tr>
<tr>
<td>404</td>
<td>0</td>
<td>0</td>
<td>0</td>
<td>0</td>
<td>0</td>
<td>Chrome_IOThread</td>
<td>Chrome_IOThread</td>
<td>Browser</td>
<td>3036</td>
<td>_IpcMessageHandlerClass::OnDataReceivedACK</td>
<td><a href="http://chromesrc.appspot.com/?path=.%5Cbrowser%5Crenderer_host%5Cresource_dispatcher_host.cc&line=408">resource_dispatcher_host.cc \[408\]</a></td>
</tr>
<tr>
<td>379</td>
<td>43</td>
<td>0</td>
<td>2</td>
<td>703</td>
<td>2,684</td>
<td>Chrome_IOThread</td>
<td>CrBrowserMain</td>
<td>Browser</td>
<td>3036</td>
<td>\`anonymous-namespace'::ChromeCookieMonsterDelegate::OnCookieChanged</td>
<td><a href="http://chromesrc.appspot.com/?path=.%5Cbrowser%5Cprofiles%5Cprofile_io_data.cc&line=95">profile_io_data.cc \[95\]</a></td>
</tr>
<tr>
<td>362</td>
<td>0</td>
<td>0</td>
<td>0</td>
<td>1,275</td>
<td>2,962</td>
<td>Chrome_IOThread</td>
<td>CrBrowserMain</td>
<td>Browser</td>
<td>3036</td>
<td>TaskManagerModel::NotifyBytesRead</td>
<td><a href="http://chromesrc.appspot.com/?path=.%5Cbrowser%5Ctask_manager%5Ctask_manager.cc&line=971">task_manager.cc \[971\]</a></td>
</tr>
<tr>
<td colspan=12>1194 rows (1184 hidden) Show noneShow moreShow all</td>
</tr>
</table>

\[Reset tracking data\]

Before describing the top line, it is significant to clarify what each of the
columns presented in any table are. The columns are:

1.  Count:: The number of times this task was observed to have run. If
            the execution thread is "Still running," then it has not completed,
            and the actual execution thread is unknown (it might not even have
            started!).
2.  Total run time: The sum of the run times for all tasks listed on
            this row.
3.  Avg run time: The quotient of the Total run time column divided the
            Count column.
4.  Max run time: The largest single Run time for the task in this row.
5.  Avg queue time: The average, over all runs of this task, of the
            queuing delay the task encountered. If this was an ordinary task,
            the queuing delay is measured from the birth (construction) of the
            task, until the task began. If this was a PostDelayedTask, then the
            delay is measured from when the task was asked to run, until it
            actually ran. For IPC tasks, the queuin delay is always listed as
            zero (TBD: make this measure interprocess latency).
6.  Max queue time: The largest singular queuing delay observed for this
            row.
7.  Birth thread: The thread on which this task was constucted.
8.  Exec Thread: The thread on which this task ran.
9.  Process type: An enumerated list of process types, currently
            including "Browser" and "Tab" (meaning Renderer). It will soon
            include GPU, Plugin, etc.).
10. PID: The operating system Process ID associated with the process.
            This can be used to (for example) distinguish various renderer
            processes.
11. Function Name: Usually the name of function where the task was
            constructed (not actually the top level function name for the task
            in this row). If this is an IP task, then it is the name of the IPC
            routine that handled the incoming request. On some compilers, this
            will templete parameters (if applicable), while on other compilers
            (gcc?), it will only include the name of the templete. In such
            cases, there may be indistinguishable rows in the profile.
12. Source location: The file and line number for the function where
            this task was created. Clicking on the link in this column will open
            a new window, displaynig the actual source code specified.

**Top Line Elements**

The first line provides pull down boxes to organize and filter what appears in
the table(s). There are currently 4 parts on this headline, such as the
following (pulldown and text entry boxes do not appear in this documentation :-(
):

<table>
<tr>
<td><b> Group by: </b> <b>Sort by: </b></td>
<td>\[snapshots\] \[columns\] </td>
</tr>
</table>

. The parts are:

1.  Pulldown box (labeled **Group by**) to select presentation of
            multiple tables, rather than one table. If an option is selected
            (such "Exec" thread) then a separate table is presented for each
            distinct value of the selected option. When an option is selected,
            then an additional pulldown is provided, to select funther
            partitioning of the tables.
2.  Pulldown box (labeled **Sort by**) to select the sort order.
            Generally, is it easier to click on the column headings to change
            the sort order, but these pulldown can be used when several levels
            of sorting are requested. For instance, you can select a primare
            sort on Count, and a secondary sort on Total Execution time.
3.  A link to enable/disable Snapshot mode. When this mode is enabled,
            additional snapshots of data can be obtained, and a delta between
            any two snapshots can be selected for examination.
4.  A link to show a more complete list of columns and options. You can
            select this, and then alter whether some columns are shown or not
            (to make the tables less wide??), or to merge similar rows (example:
            Merge all similar rows in the same browser type by not distingushing
            based on PID). By default, merging of Worker Threads and PAC (Proxy
            Auto Config) threads is performed, based on the the "merge similar
            threads" checkbox.
5.  An instant filter box. Any text typed into this box will be matched
            (case insensitive) against all the information known about the row
            (i.e., threads, function, location, etc). Only matching rows will be
            shown in the table(s).

**Table Elements**

Each table consists of 4 groups of rows. In addition, if multiple tables are
presented, after a "Group by" pulldown is selected, there may be a title for the
table. The title for table indicates what rows have been selected for
presentation in the table. For example, a selection to Group by "Exec thread"
was selected, then one possible title is "Exec thread = CrBrowserMain".

The following are the 4 groups of rows in each table (with sample lines taken
from the table shown above):

<table>
<tr>
<td>19,402</td>
<td>187,008</td>
<td>10</td>
<td>9,338</td>
<td>217</td>
<td>9,932</td>
<td>15 unique</td>
<td>17 unique</td>
<td>2 unique</td>
<td>18 unique</td>
<td>415 unique</td>
<td>430 unique</td>
</tr>
<tr>
</tr>
</table>

> 1. Summary row: This singular row presents the aggregate results for all rows
> present in the table. For example, it' Count column contains sums the Count
> column in each row, while its Max exec time columns contains the maixum of all
> Mac exec time columns in each row.

<table>
<tr>
Count\*\*Total run timeAvg run timeMax run timeAvg queue timeMax queue timeBirth threadExec threadProcess typePIDFunction nameSource location</tr>
<tr>
</tr>
</table>

> 2. Header Row: This singular row contains the column headings for the table.
> Headings are only shown for columns that are presented in the table, and tihs
> list my be varied by selecting various elemens of the Top Line described
> earlier. Clicking on any elemet of this row will cause the table to be sorted
> based on that row. A second click will cause a reverse sort.

<table>
<tr>
<td>990</td>
<td>249</td>
<td>0</td>
<td>28</td>
<td>7</td>
<td>363</td>
<td>Chrome_CacheThread</td>
<td>Chrome_IOThread</td>
<td>Browser</td>
<td>3036</td>
<td>disk_cache::InFlightIO::OnIOComplete</td>
<td><a href="http://chromesrc.appspot.com/?path=.%5Cdisk_cache%5Cin_flight_io.cc&line=62">in_flight_io.cc \[62\]</a></td>
</tr>
</table>

> 3. Data Rows: This section may contain 0 or more rows of data, each
> corresponding to a collection of runs of tasks. The final cell is clickable,
> and provides quick access to a recent copy of the source code, at the point of
> interest.

1194 rows (1184 hidden) Show none Show more Show all

> 4..Display Control Row: This singular final row can be used to select, a view
> of additional or fewer rows of the Data Row section. It contains buttons to
> request viewing of additional lines, or to reduce viewed lines. It also shows
> how many lines a table has. By default, only the first 30 rows of a single
> table are visible. When multiple tables are presented, then a smaller initial
> number of rows are present.

**Final Link: Reset Tracking data**

At the very bottom of the about:profile page is a link for Reseting Tracking
data. This link sulfaces a useful hack until snapshot functionality can provide
the maximum value between two snapshots. The link can be used to force all data
entries in all rows of the browser to zero. The next snapshot (or page reload)
will show all new data, an dwill, as a result, show the max value since the last
reset. The reset is done using various hackery, and minor miscounts are
possible. As a result, this is really a hack. Once the snapshot facility better
supports max, this link will be removed from the UI.

**FAQ**

**How is this implemented?**

If you're a Google employee, check out [this
techtalk](https://goto.google.com/chrome-profiler-internals-techtalk-video).
Otherwise [check out the
code](https://cs.chromium.org/search/?q=tracked_objects).

**I'd like to see just the tasks that appear between two distinct points in
time. How do I avoid seeing all the other data?**

On the top line, select the Snapshot link. When that is selected you will see
the data and time of each data collection effort. Instead of hitting reload, you
can click the snapshot button, and acquire another image of the profiler state
of the browser. If you click on any two lines, then you will see the changes
that have taken place between those two snapshots. When you click on only one
line, then you'll see the data from startup through that point in time.

note: Max is not currently supported in the delta between two points. That
feature will be supported RSN.

**How much does this profiling and analysis impact performance?**

There is very little measurable performance impact. If you'd like to disable the
tracking, use the command line switch --enable-tracking=0

The implementation attempts to gather all this information across all threads
with asymptotically zero locking, zero atomic operations, and a very small
amount of memory (that does not grow after all birth locations and associated
destruction threads have been observed at least once). The details of how this
achieved is provided in a long comment atop the file src/base/tracked_objects.h,
as well as a forth-coming video and associated slide presentation.

**Basic Debugging Assistance: How did \*this\* task get posted?**

When a breakpoint is hit in a debugger on a given thread, the stack often starts
with a message loop running a task, and there is very little information about
what part of the codebase truly caused that task to be run. When a newbie
developer is trying to follow the flow of events in Chromium, it is often
difficult to understand the connection between (for example) the IO Thread, the
File Thread, and the Main UI Thread. The infrastructure for task tracking
provides basic information to see through these posted task transitions.

Assuming you've reached a breakpoint of interest, but the top of the call stack
shows a message loop, and the calling of a Run() method, then you are perfectly
set up to see the source of any task. Most debuggers will traverse to the
following base class of interest seamlessly, but here is the more complete
description of where to look to find the answer (a specific file and line
number).

1.  Find the level of the stack that has the Run() method (typically in
            the message_loop.cc file), and look at the Task instance that is
            calling Run().
2.  The Task instance has a base class
            [TrackingInfo](https://code.google.com/p/chromium/codesearch#chromium/src/base/tracking_info.h&q=TrackingInfo&sq=package:chromium&type=cs).
3.  The TrackingInfo base class has an member birth_tally of type
            [Births](https://code.google.com/p/chromium/codesearch#chromium/src/base/tracked_objects.h&q=Births&sq=package:chromium&type=cs&l=234).
4.  That Births instance in turn has a base class BirthOnThread, which
            has a member location_ of type Location.
5.  Finally, the Location instance will contain the function_name_, the
            file_name_, and the line_number_ indicating where the task was
            posted (typically just after construction).

---

Please address comments and suggestions to Jim Roskind a.k.a., jar at
chromium.org.
