---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/trace-event-profiling-tool
  - The Trace Event Profiling Tool (about:tracing)
page_name: recording-tracing-runs
title: Recording Tracing Runs
---

When diagnosing performance problems it can be valuable to see what Chrome is
doing "under the hood." One way to get a more detailed view into what's going on
is to use the about:tracing tool.

See [Trace Event Profiling Tool
(about:tracing)](/developers/how-tos/trace-event-profiling-tool) for more
information on what this tool does.

[TOC]

## Capture a trace on Chrome desktop

Sometimes Chromium developers will ask for about:tracing results in a bug report
to help us understand what's going on.
**Here's an easy step-by-step guide: [ Submitting an Awesome Performance
Bug](/developers/how-tos/submitting-a-performance-bug)**

The TL;DR:

1.  Open a new tab and type "**about:tracing**" in the Omnibox.
2.  Press "**Record**". Choose the "**Manually select settings**"
            preset, click the **All** button above the left column and press
            "**Record**".
3.  Switch back to the tab with the content you're debugging and do
            whatever it is that's slower than it should be (or behaving
            incorrectly). A few seconds of the bad activity is usually
            sufficient.
4.  Switch back to the tracing tab and press "**Stop**", then "**Save**"
5.  (Optional) If this is for a bug report, upload the output file as an
            attachment to bug. If the file is bigger than 10MB, zip it first
            (the files compress well, and files bigger than 10MB can be
            truncated during upload).

**IMPORTANT:** *Before attaching a trace to a bug keep in mind that it will
contain the URL and Title of all open tabs, URLs of subresources used by each
tab, extension IDs, hardware identifying details, and other similar information
that you may not want to make public.*

### *Capturing chrome desktop startup*

> Assuming $CHROME is set to the command you need on your OS to start chrome,
> the following will record the first 7 seconds of Chrome's lifetime into
> /tmp/foo.json

> > > `$CHROME --trace-startup --trace-startup-file=/tmp/foo.json
> > > --trace-startup-duration=7`

> On Windows you can modify the properties for your Chrome shortcut to add this
> after where it says chrome.exe (don't forget to modify it back when you are
> done):

> > > `--trace-startup --trace-startup-file=%temp%\foo.json
> > > --trace-startup-duration=7`

## Capture a trace from Chrome on Android (on device)

First you need to enable Chrome's developer options. To do this, open **Settings
&gt; About Chrome**. Then tap **"Application version"** 7 times in a row:

[<img alt="image"
src="/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/s.png"
height=400
width=194>](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/s.png)

After this you can access developer settings from **Settings &gt; Developer
options**. From this menu, select **Tracing**:

[<img alt="image"
src="/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/s2.png"
height=400
width=194>](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/s2.png)

Select your categories and tap **"Record trace"** to start recording:

[<img alt="image"
src="/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/s3.png"
height=400
width=194>](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/s3.png)

To stop the recording, pull down on the notification shade and tap **"Stop
recording"**. After this, you can access the trace with the **"Share trace"**
button:

[<img alt="image"
src="/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/s4.png"
height=400
width=194>](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/s4.png)

## Capture a trace from Chrome on Android (with DevTools)

> **Tip:** this also works with tracing the system Android WebView.

> 1. If you're debugging a WebView app: edit the app to call the **static**
> method `WebView.setWebContentsDebuggingEnabled(true)` ([API
> docs](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs))

> 2. Connect Chrome DevTools on your desktop to the Android device. Follow
> [these instructions for
> Chrome](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs)
> or [these instructions for
> WebView](https://developer.chrome.com/docs/devtools/remote-debugging/webviews/)
> 3. Go to `chrome://inspect?tracing` on desktop chrome. Find the app to be
> traced, and click on the trace link beside the title.

> [<img alt="image"
> src="/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/Screen%20Shot%202015-06-04%20at%2012.46.58%20PM.png"
> height=277
> width=400>](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/Screen%20Shot%202015-06-04%20at%2012.46.58%20PM.png)

> 3. Click on "Record" at top left.

> [<img alt="image"
> src="/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/Screen%20Shot%202015-06-04%20at%2012.47.34%20PM.png">](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/Screen%20Shot%202015-06-04%20at%2012.47.34%20PM.png)

> 4. Select trace categories. Hit Record, then Stop.
> [<img alt="image"
> src="/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/Screenshot%20from%202014-10-14%2010-19-11.png"
> height=313
> width=400>](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/Screenshot%20from%202014-10-14%2010-19-11.png)

## Capture a trace from Chrome on Android (on the command line)

> You'll need:

> *   Your device running Chrome on Android
> *   USB debugging enabled and the device attached.
> *   [ADB](http://developer.android.com/guide/developing/tools/adb.html)
> *   Linux or Mac desktop. Sorry Windows friends!

> You can use \`adb_profile_chrome\` from the build/android/ directory in
> Chromium's source checkout.

> Follow these steps to record a trace:

> #### **1. Launch the browser**

> Run Chrome normally.
> Developers, however, may want to launch from the command line:
> > `$ build/android/adb_run_content_shell URL `# run content shell

> > `$ clank/bin/adb_run_chrome URL # run clank (Googlers only) `

> #### **2. Optionally, make a screen recording (optional)**

> ` $ build/android/screenshot.py --video `

> ` # or `

> ` $ adb shell screenrecord /sdcard/recording.mp4 && adb pull
> /sdcard/recording.mp4`

> #### **3. Record a trace**

> #### Look at the [usage documentation for adb_trace and profile_chrome](https://github.com/johnmccutchan/adb_trace/blob/master/README.md#basics).

> > [<img alt="image" src="/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/Screen%20Shot%202015-06-04%20at%201.37.00%20PM.png" height=215 width=320>](https://github.com/johnmccutchan/adb_trace/blob/master/README.md#basics)

> For example:

> ` $ build/android/adb_profile_chrome --browser build --time 5 --view`

> Note: build here refers to the version of Chrome you are tracing. Other valid
> values are stable, beta (see --help for a full list).

### Tracing options for Android

> You can also use additional command line flags to capture more data. For
> example:

> **1. Systrace:** --systrace gfx,view,input,freq,sched

> > [ <img alt="image"
> > src="/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/tracing-systrace.png"
> > height=109
> > width=400>](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/tracing-systrace.png)

> Captures data about system-level events. Generally useful categories are

> > > sched = Kernel scheduling decisions

> > > freq = CPU core frequency scaling

> > > gfx = SurfaceFlinger events, Vsync

> > > view = Android Framework events

> > > input = Framework input event processing

> adb shell atrace --list_categories shows the rest.

> **2. IPC messages:** --trace-flow

> > [ <img alt="image"
> > src="/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/tracing-flow.png"
> > height=143
> > width=400>](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/tracing-flow.png)

> Adds arrows showing IPC message flows, making it easier to see message
> traffic.

> 3. More trace categories, e.g.:
> --categories=disabled-by-default-cc.debug.scheduler,\*

> > [ <img alt="image"
> > src="/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/tracing-scheduler.png"
> > height=106
> > width=400>](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/tracing-scheduler.png)

> Adding more trace categories makes it possible to see events that are normally
> disabled. This particular example gives information about why the compositor
> scheduler decided to run a given action. To see the full list of categories,
> use --categories=list.

> **4. Frame viewer data:** --trace-frame-viewer

> > [ <img alt="image"
> > src="/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/tracing-frame-viewer.png"
> > height=233
> > width=400>](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/tracing-frame-viewer.png)

> Captures rich layer data from the compositor. This is useful for understanding
> how the DOM affects the compositor's layer tree structure. When viewing traces
> like this, make sure to launch Chrome (on the desktop) with the
> --enable-skia-benchmarking command line flag so that all features are
> available.

> **5. Perf profiling data:** --perf
> [Perf](https://perf.wiki.kernel.org/index.php/Main_Page) is a sampling
> profiler which can help you find performance intensive parts of the source
> code. The --perf switch runs perf and automatically combines the data with all
> other enabled event sources.

> > [ <img alt="image"
> > src="/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/sunburst.png"
> > height=400
> > width=396>](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs/sunburst.png)

> You can also view the raw recorded perf data using the command line which is
> printed out by adb_profile_chrome.

## Making a Good Recording

*   Keep the recording to a max of 10 seconds
*   Try to keep to 1 activity per recording. If you see two slowdowns,
            do one recording each. Or leave a second long pause between each.
*   Pause and leave the computer completely idle for 2 seconds before
            and after demonstrating the slowdown. This helps make the slow part
            obvious in the recording.

### Step by step:

1.  Start with only the tab you’re investigating and about:tracing open
            in a fresh browser session (see below for other methods of figuring
            out which tab is which if you need multiple).
2.  Set up the tab for investigation to right before the point where the
            problem will occur (e.g. right before an animation is going to be
            triggered, or right before part of the page that scrolls slowly)
3.  Start a tracing run in the about:tracing tab
4.  Switch to the tab under investigation
5.  Pause for a couple seconds to record empty space on the timeline
            (makes finding it later easier)
6.  Perform the action to trigger the bad performance behavior (e.g.
            start the animation or scroll the page). Keeping the recorded
            activity under 10 seconds is a good idea.
7.  Pause again
8.  Switch back to the about:tracing tab and stop the recording.

## Tracing in Telemetry

For more repeatable trace recordings you may want to consider using
[Telemetry](/developers/telemetry) to automate the process. In particular the
--profiler=trace, --profiler=v8 and --profiler=android-systrace flags will save
a trace for each tested page.

## Trace Report File Formats

After recording one can save a trace report by pressing the Save button. This
will save the trace in JSON format.

It is also possible to convert it to HTML (which is easier to link to) using
[Catapult](https://github.com/catapult-project/catapult)'s
[trace2html](https://github.com/catapult-project/catapult/blob/master/tracing/bin/trace2html)
script.

## Slide deck of Android Tracing tricks

This information is already detailed above.
