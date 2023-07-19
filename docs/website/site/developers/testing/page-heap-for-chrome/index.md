---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: page-heap-for-chrome
title: Page Heap for Chromium
---

Page Heap is a Windows mode to help identify memory errors, including those in
third-party or OS supplied code. Application Verifier is a Windows mode that
enables Page Heap and can also detect additional programming errors.

When using Application Verifier on official builds of Chrome you need to add
`--disable-features=RendererCodeIntegrity` to avoid sandbox crashes in renderer
processes. See [crbug.com/1004989](https://crbug.com/1004989) for details.
Application Verifier can be used on non-official developer builds, but you
should probably close stable Chrome to avoid having it crash. See also [this
page](/developers/how-tos/debugging-on-windows) for information on Application
Verifier.

### Enabling Page Heap

1. One way of enabling page heap is using [Application
Verifier](https://randomascii.wordpress.com/2011/12/07/increased-reliability-through-more-crashes/)
(comes with the Windows Platform SDK, use X64 or WOW versions), which enables
full page heap and also enables other checks. If you run Application Verifier
you can enable the checks for chrome by using File-&gt; Add Application to add
chrome.exe. Then open the Basics section and uncheck *Leak* and *SRWLock*, to
avoid known failures that do not indicate real problems (see crbug.com/807500
for details). You may also need to disable *Handles* and *Locks* depending on
your graphics driver.

For some builds (debug builds for sure) it is necessary to disable Basics-&gt;
TLS due to aggressive TLS index range-checking done by v8.

After adjusting these settings be sure to hit Save. Note that all chrome.exe
process launches will be affected so you may want to shut down stable Chrome
while testing to avoid confusing problems. You should make sure that Chrome has
fully shut down.

Page heap should always be used with a 64-bit version of Chrome to avoid address
space exhaustion - every allocation takes a minimum of 4 KB of memory and at
least 8 KB of address space.

2. Another way to turn on page heap is using gflags, which is included in
Windows Debugging Tools.It comes with the Debuggers package in the Windows SDK.
If you're a Googler and use the depot_tools toolchain, there's a version in
"depot_tools\\win_toolchain\\vs_files\\&lt;hash&gt;\\win_sdk\\Debuggers\\x64".

Or add the SDK's Windows Debugging Tools to your path: "c:\\Program Files
(x86)\\Windows Kits\\10\\Debuggers\\x64"

3. Enable full page heap for a particular executable with this command:

gflags.exe /p /enable chrome.exe /full

This method doesn't seem to work anymore - it just says "No application has page
heap enabled." and does nothing, so run gflags.exe and let the UI come up. Or
use Application Verifier.

If chrome gets too slow with full page heap turned on, you can enable it with
normal page heap:

gflags.exe /p /enable chrome.exe

Tip: since you need to run this as administrator, it might be easiest if you
right-click on your console program and select Run as administrator so that all
operations in that shell are already privileged.

See Background section for more information on page heap and gflags.

### Disabling Page Heap

To disable page heap when you're done, run:

gflags.exe /p /disable chrome.exe

### Background

1. Page heap is Window built-in support for heap verification. There are two
modes:

- Full-Page heap places a non-accessible page at the end of the allocation.
Full-page heap has high memory requirements. Its advantage is that a process
will access violate (AV) exactly at the point of illegal memory operation.

- Normal page heap checks fill patterns when the block gets freed. Normal page
heap can be used for testing large-scale process without the high memory
consumption overhead of full-page heap. However, normal page heap delays
detection until the blocks are freed - thus failures are more difficult to
debug.

When an application foo.exe is launched, Windows looks up in
"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution
Options\\foo.exe" for page heap and other settings of the executable and acts
accordingly.

2. To turn on page heap for an executable, one just needs to modify the settings
in registry. Gflags is a utility downloadable from Microsoft to edit settings
under "Image File Execution Options".

### Troubleshooting

*   Are you sure you're using the 32-bit version of gflags? If you use
            the 64-bit version, you won't get any error messages, but nothing
            will happen.
*   Are you sure you're using a 64-bit build of Chrome? The 32-bit
            version will run out of address space and spuriously fail with full
            page heap.
*   Are you debugging browser_tests? You might want to enable page heap
            for both browser_tests.exe and chrome.exe, or try running
            browser_tests with the --single-process flag.
