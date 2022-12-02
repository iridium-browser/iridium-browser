---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/packages
  - packages
- - /chromium-os/packages/crash-reporting
  - Crash Reporting (Chrome OS System)
page_name: stack-traces-from-chrome-os-crash-dumps
title: Stack Traces From Chrome OS Crash Dumps
---

If you need to get a stack from a BVT failure that has already been uploaded to
Google Cloud Storage, there's a much easier way to do this. See the [Chrome OS
sheriff](/developers/tree-sheriffs/sheriff-details-chromium-os) documentation
under "How to find test results/crash reports". Look in a directory like
desktopui_MediaAudioFeedback/sysinfo/iteration.1/var/spool/crash for the
chrome.dmp.txt file. It should have a stack.

If you run a test on a device using run_remote_tests and the test crashes (for
example, with a Chrome sig 11) the test script will copy a minidump file back to
your workstation. However, it will not automatically generate a stack trace for
you. Here's how to get one. (Directions are based on the [Linux and Windows
instructions](/developers/decoding-crash-dumps).)

Run your test:

**./run_remote_tests.sh --remote=172.22.70.164 desktopui_MediaAudioFeedback
--args 'v=1'**

Output for a crash looks like this:

Crashes detected during testing:

---------------------------------------------------------------------

chrome sig 11

desktopui_MediaAudioFeedback/desktopui_MediaAudioFeedback

---------------------------------------------------------------------

Look for a line with a path to the .dmp file (it didn't really generate a stack
trace for you):

10:49:03 INFO | Generated stack trace for dump
/tmp/run_remote_tests.wNYt/desktopui_MediaAudioFeedback/desktopui_MediaAudioFeedback/sysinfo/iteration.1/var/spool/crash/chrome.20120813.104825.5708.dmp

There's a core file in that directory, but you can't use it directly as the
chrome binary is stripped. We can, however, generate symbols from the
chrome.debug file and use those with minidump_stackwalk to get a stack trace.

First, copy the dmp file so you don't need to type the long path over and over:

**cp
/tmp/run_remote_tests.wNYt/desktopui_MediaAudioFeedback/desktopui_MediaAudioFeedback/sysinfo/iteration.1/var/spool/crash/chrome.20120813.104825.5708.dmp
~/foo.dmp**

Make a temporary directory like /tmp/my_symbols. Force minidump_stackwalk to
generate errors about paths not found by pointing it at that directory:

**mkdir /tmp/my_symbols**

**minidump_stackwalk ~/foo.dmp /tmp/my_symbols 2&gt;&1 | grep "No symbol file" |
grep "chrome"**

This will tell you where you need to put the debugging symbols. It will be a
path with a hash in it, like /tmp/my_symbols/chrome/&lt;hash&gt;:

2012-08-13 11:31:05: simple_symbol_supplier.cc:193: INFO: No symbol file at
/tmp/my_symbols/chrome/6CC67691D7F2ECBF5E95D2BEF7F11C5A0/chrome.sym

Now we need to generate the symbols and put them at that location. Create the
directory using the path above:

**mkdir -p /tmp/my_symbols/chrome/6CC67691D7F2ECBF5E95D2BEF7F11C5A0**

Run dump_syms with the chrome binary and directory of the chrome.debug symbol
file. Substitute your own board for "lumpy". This step can take 1-2 minutes.

**dump_syms /build/lumpy/opt/google/chrome/chrome
/build/lumpy/usr/lib/debug/opt/google/chrome &gt; /tmp/chrome.sym**

Move the symbol file into the path above:

**mv /tmp/chrome.sym /tmp/my_symbols/chrome/6CC67691D7F2ECBF5E95D2BEF7F11C5A0/**

Finally, you can run minidump_stackwalk to get a stack trace. It will look
automatically for chrome/&lt;hash&gt; under the /tmp/my_symbols directory:

**minidump_stackwalk ~/foo.dmp /tmp/my_symbols**

If you need symbols for things like shared libraries you can repeat the above
process grepping the path-not-found errors for things like "libglsl.so" instead
of "chrome". See the [Linux instructions](/developers/decoding-crash-dumps) for
details.
