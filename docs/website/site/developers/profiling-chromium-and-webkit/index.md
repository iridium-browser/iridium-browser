---
breadcrumbs:
- - /developers
  - For Developers
page_name: profiling-chromium-and-webkit
title: Profiling Chromium and Blink
---

There are a few ways to profile Chromium and Blink. Here are some of the tools
that work well for diagnosing performance problems.

See also the [Deep memory
profiler](http://www.chromium.org/developers/deep-memory-profiler).

## Built-In Tools

For JavaScript issues, the built in profiler works very well. To use this open
up the Chrome Dev Tools (right click, Inspect Element) and select the 'Profiles'
tab.

For a broader understanding of Chromium speed and bottlenecks, as well as
understanding how posted-task and threads interact in aggregate, there is a
cross-platform, task-level profiler built in. Profiler results can be seen in
about:profiler (or equivalently chrome://profiler) For more details, visit
(<http://www.chromium.org/developers/threaded-task-tracking>).

See chrome://tracing for timelines showing TRACE_EVENT activity across all the
different threads; originally used for GPU performance, and will probably
require you to add TRACE_EVENT calls to the features you're interested in
outside of compositing & rendering (this was named about://gpu through M14).

## C++

For native C++ code the tools depend on the OS.

Note that basic printf debugging and using a general debugger (such as gdb) may
be sufficient for some purposes. However, more specialized tools are available.

## Linux, ChromeOS, Android

See <https://chromium.googlesource.com/chromium/src/+/HEAD/docs/profiling.md>

### OS X

DTrace and the pre-packaged "CPU Sampler" tool in Xcode work well. Shark or the
command-line sample work also, though they both will spend an exceedingly long
time processing symbols if you are running Leopard (10.5). Anecdotally this is
much faster in Snow Leopard (10.6)

### Windows

Windows Performance Toolkit (WPT, aka xperf, ETW, or WPA) is a free profiler
from Microsoft that can profile CPU consumption, CPU idle time, file I/O, disk
I/O, and more. To use WPT you need Windows 7 or higher. Getting started
instructions for recording a trace using UIforETW can be found on [this blog
post](https://randomascii.wordpress.com/2015/09/01/xperf-basics-recording-a-trace-the-ultimate-easy-way/).
Trace analysis can then be done (be sure to configure [Chrome's symbol
server](/developers/how-tos/debugging-on-windows)) or the trace can be shared on
Google drive by analysis by somebody else. If you will be doing the analysis of
the traces that you record then you may want to check the "Chrome developer" box
in the settings dialog. This will tell UIforETW to print a summary of your
Chrome process tree. You can also check some of the chrome tracing categories
(maybe input, toplevel, latency, blink.user_timing,
disable-by-default-toplevel.flow) so that Chrome will emit those chrome:tracing
events into the ETW event stream. WPT is the recommended Windows profiler for
any Chrome performance problems that aren't handled by the built-in tools. If
you are profiling officially published Chrome builds (Canary, stable, etc.) then
you need to add Chrome's symbol server to WPA using the Trace-&gt; Configure
Symbol Paths dialog. If you are profiling a local build then you can either add
your local build directory to that dialog or set use_full_pdb_paths = true in
your gn args so that the full path to the PDB is put in the EXE and DLL files.

Visual Studio also includes an [instrumenting
profiler](https://msdn.microsoft.com/en-us/library/dd255369.aspx). "vsinstr
/help" and "vsperfcmd /help" have details for additional options. Currently this
requires a non-debug non-component build of Chromium (debug or component builds
omit the /profile linker flag).

AMD Code Analyst is a free profiler that can run inside Visual Studio. It
captures frequency counts for functions in every process on the computer. It can
optionally capture call-stack information, %CPU, and memory usage statistics;
even with the Frame Pointer Omission optimization turned off
(build\\internal\\release_defaults.gypi; under 'VCCLCompilerTool' set
'OmitFramePointers':'false'?), the call stack capture can have lots of bad
information, but at least the most-frequent-caller seems accurate in practice.

Intel's VTune 9.1 does work in the Sampling mode (using the hardware performance
counters), but call graphs are unavailable in Windows 7/64. Note also that
drilling down into the results for chrome.dll is extremely slow (on the order of
many minutes) and may appear hung. It does work (I suggest coffee or foosball).
VTune has been essentially supplanted by Intel® VTune™ Amplifier XE, which is an
entirely new code base and interface, AFAIK.

Very Sleepy (<http://www.codersnotes.com/sleepy>) is a light-weight standalone
profiler that seems to works pretty well for casual use and offers a decent set
of features.

## Android

See [Profiling Content Shell on
Android](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/profiling_content_shell_on_android.md)
for detailed Android-specific instructions, and [Telemetry
profiling](/developers/telemetry/profiling) for general instructions about using
Telemetry to profile (rather than using `adb_profile_chrome`).

GPU profiling

Both nVidia PerfHUD and Microsoft PIX are freely available. They may not run
without making minor changes to how the graphics contexts are set up; check with
the chrome-gpu team for current details.

The OpenGL Profiler for OSX allows real-time inspection of the top GL
performance bottlenecks, as well as call traces. In order to use it with
Chrome/Mac, you must pass --disable-gpu-sandbox on the command line. Some people
have had more luck attaching it to the GPU process after-the-fact than launching
Chrome from within the Profiler; YMMV.

GPUView is a Windows tool that utilizes ETW (Event Tracing for Windows) for
visualizing low-level GPU, driver and kernel interactions in a time-based
viewer. It's available as part of the Microsoft Windows Performance Toolkit, in
%ProgramFiles%\\Windows Kits\\10\\Windows Performance Toolkit\\GPUView. There's
a README.TXT in there with basic instructions, or see
<http://graphics.stanford.edu/~mdfisher/GPUView.html>. Traces for loading into
GPUView can be recorded with wprui or UIforETW (see the Windows section).

## Googlers Only

Take a look at go/chrome-performance-how for an overview of internal performance
tools.
