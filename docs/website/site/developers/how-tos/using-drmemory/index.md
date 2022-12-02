---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: using-drmemory
title: Using Dr. Memory
---

[TOC]

### Note: Dr.Memory bot has retired and the documentation here is obsolete (added on Jul. 27, 2017)

### Dr. Memory is a new open-source tool for finding memory errors - similar to Memcheck (Valgrind) but with Windows support, including detection of Windows handle leaks and GDI usage errors.

Dr. Memory is running on the MFYI waterfall [MFYI
waterfall](http://build.chromium.org/p/chromium.memory.fyi/waterfall?builder=Chromium+Windows+Builder+(DrMemory)&builder=Windows+Unit+(DrMemory)&builder=Windows+Browser+(DrMemory)+(1)&builder=Windows+Browser+(DrMemory)+(2)&reload=none)
where it finds bugs in Chromium on Windows.

If you want to try Dr. Memory on your machine, please do the following:

*   First of all, you need Windows. Win7 x64 is preferred. DrMemory
            supports Linux, but the support is not as robust.

*   [Build](/developers/how-tos/build-instructions-windows) Chromium
            tests on Windows with the following GN configuration (you can save
            some time by building only the small tests like googleurl, ipc,
            base, net to start with). Build with the following setting in order
            to disable inlining and FPO for better callstacks:

    enable_iterator_debugging = false

    is_component_build = true
    is_debug = false

*   The Dr. Memory binaries are downloaded from Google Storage
            automatically (on Windows) into the third_party/drmemory directory.
*   Now you can run chromium tests like this:
    tools\\valgrind\\chrome_tests.bat -t *&lt;test&gt;* --tool drmemory_light #
    only search for unaddressable accesses and uses-after-free
    or
    tools\\valgrind\\chrome_tests.bat -t *&lt;test&gt;* --tool drmemory_full #
    search for uninits and leaks etc as well
    Alternatively, if you prefer Msysgit, you can also use
    tools/valgrind/chrome_tests.sh with the same arguments. Just be aware that
    the bots use the .bat script.
*   You can use --build-dir to select between Debug and Release, for
            example: tools\\valgrind\\chrome_tests.bat -t base_unittests --tool
            drmemory_light --build-dir=out\\Release
*   You can use --gtest_filter to run an individual test or a subset of
            tests, for examle:
    tools\\valgrind\\chrome_tests.bat -t base_unittests --tool drmemory_light
    --gtest_filter=A\*
*   You can use --drmemory_ops to add additional Dr. Memory options, for
            example: tools\\valgrind\\chrome_tests.bat -t base_unittests --tool
            drmemory_light --drmemory_ops "-pause_at_error"

*   Extra Dr. Memory specific options can be passed via option
            --drmemory_ops, for example: tools\\valgrind\\chrome_tests.bat
            --drmemory_ops "-fuzz" for [fuzzing the test with Dr.
            Memory](/developers/testing/dr-fuzz).
*   It will print you some error reports at the end.

If for some reason you cannot reproduce a bot failure on your Windows machine,
look for the exact command that started the tests and run that. For example,
here is one variation of the test command that catches some memory bugs that the
above commands may not catch:

src\\tools\\valgrind\\chrome_tests.bat --brave-new-test-launcher
--test-launcher-bot-mode --test &lt;test&gt; --tool drmemory_full --target
Release --build-dir src\\out

### OK, I've found a real bug

Great!

Please file a crbug and add the
[Stability-Memory-DrMemory](http://code.google.com/p/chromium/issues/list?q=Stability-Memory-DrMemory)
label.

### drmemory_light vs drmemory_full

Light: searches for unaddressable accesses like OOB or use-after-frees, handle
leaks, and graphical library usage errors, incurring a moderate execution
slowdown.

Full: additionally searches for uninitialized reads but adds a large slowdown.

More on the "full" mode:

If you're familiar with the Valgrind guts you probably know that tools like
Valgrind and Dr. Memory need to intercept and handle all system calls to know
precisely what data is uninitialized, what is read and so on.

In contrast to the open-source Linux kernel with just a hundred system calls or
so, on Windows we have an open-ended problem of handling undocumented system
calls.
We're trying hard to do that but we haven't finished yet. We have some
heuristics that work pretty well to ignore errors stemming from system libraries
making system calls as opposed to Chromium code, but if you do see an error
report that you believe is a false positive, please create a short reproducer
and file it into our [issue
tracker](http://code.google.com/p/drmemory/issues/list?can=2&q=Bug%3DFalsePositive).

### Suppressing error reports from the bots

**TODO(rnk): When the DrMemory bots have moved to the chromium.memory.fyi
waterfall, this information will be moved into the [memory
sheriff](/system/errors/NodeNotFound) page.**

**timurrrr: most of this info sounds DrMemory-specific, do we really want to put
it onto the "common" page?**

The DrMemory suppression file is at
tools/valgrind/drmemory/suppressions\[_full\].txt.
Generally speaking, suppressions in these files are maintained in exactly the
same way as those for all of the other memory tools we use for Chromium, except
that DrMemory uses a slightly different suppression format.

When the bot generates a report, follow the link to the bot's stdio and search
for "hash=#" to find the report quickly. You should see something similar to:

UNADDRESSABLE ACCESS: reading 0x00000000-0x00000004 4 byte(s) # 0
TestingProfile::FinishInit \[chrome\\test\\base\\testing_profile.cc:211\] # 1
TestingProfile::TestingProfile \[chrome\\test\\base\\testing_profile.cc:164\] #
2 BrowserAboutHandlerTest_WillHandleBrowserAboutURL_Test::TestBody
\[chrome\\browser\\browser_about_handler_unittest.cc:110\] # 3
testing::Test::Run \[testing\\gtest\\src\\gtest.cc:2162\]Note: @0:00:08.656 in
thread 1900 Note: instruction: mov (%ebx) -&gt; %esi Suppression (error
hash=#30459DB8320B1FA3#): { UNADDRESSABLE ACCESS
name=&lt;insert_a_suppression_name_here&gt; \*!TestingProfile::FinishInit
\*!TestingProfile::TestingProfile
\*!BrowserAboutHandlerTest_WillHandleBrowserAboutURL_Test::TestBody
\*!testing::Test::Run }

As a starting point, you can copy the suppression (the portion between the curly
brackets **not including the brackets**) into the suppressions.txt file and then
widen it so it covers any similar reports on other bots.
The curly brackets are **not** part of the suppression as in all the other
memory tools! DrMemory suppressions are separated by blank lines instead, so
make sure your suppression does not run into an existing suppression.

Each frame from the stack trace is of the form
"&lt;module&gt;!&lt;function&gt;". For functions from Chrome, we generally link
the code statically into many different binaries, so we always use a wildcard
for the module.
For third party code, it is generally helpful to identify which DLL the frame
came from, especially since we often do not have debug info for windows system
libraries.

As in the other tool suppression formats, '\*' is a single frame variable-width
wildcard, '?' is a single-character wildcard, and '...' will match any number of
frames.

DrMemory has support for a handful of other pattern matching mechanisms.
In particular, if you paste the instruction from the report on a line starting
with "instruction=", you can match the exact instruction producing the report.
The instruction= qualifier is mostly useful when suppressing bit-level false
positive uninitialized reads in third party libraries where we don't have an
accurate callstack and want to make a tight suppression.

Once you have your suppression, you can test that it suppresses the existing
reports using tools/valgrind/waterfall.sh as you would with the other memory
tools.

### Running Chromium under Dr. Memory on a given set of URLs

First, put the list of URLs into tools\\valgrind\\reliability\\url_list.txt, one
URL per line without the http:// prefix. You can also put in the local URLs like
file://c:\\mypath\\repro.htm

Then, run

tools\\valgrind\\chrome_tests.bat -t reliability --tool drmemory
--tool_flags="-no_check_uninitialized -no_count_leaks"

If you're searching for some stability/security bugs, you may consider running
Chromium in the single process mode to increase the probability of the report by
applying this patch:

Index: tools/valgrind/chrome_tests.py
===================================================================
--- tools/valgrind/chrome_tests.py ([revision
96002](http://code.google.com/p/chromium/source/detail?r=96002))
+++ tools/valgrind/chrome_tests.py (working copy)
@@ -271,7 +271,8 @@
# Valgrind timeouts are in seconds.
UI_VALGRIND_ARGS = \["--timeout=7200", "--trace_children", "--indirect"\]
# UI test timeouts are in milliseconds.
- UI_TEST_ARGS = \["--ui-test-action-timeout=120000",
+ UI_TEST_ARGS = \["--single-process=yes",
+ "--ui-test-action-timeout=120000",
"--ui-test-action-max-timeout=280000"\]
def TestAutomatedUI(self):

### Running your custom command line under Dr. Memory

Sometimes you may need to run your own program or your own custom command line
under Dr. Memory. For example, if you want to prefix your own "chrome.exe
--my-flags" command with Dr. Memory, do the following:
`tools\valgrind\drmemory.bat chrome.exe -- --my-flags # don't forget to increase
the timeouts`
If the command line runs Chrome, you may consider increasing the timeouts by
appending

--ui-test-action-timeout=120000 --ui-test-action-max-timeout=280000
--ui-test-terminate-timeout=120000

to the commandline.

Random notes for the curious ones

You can find the Dr. Memory suppression file for Chrome at
tools\\valgrind\\drmemory\\suppressions\[_full\].txt

The report files are parsed and re-formatted to be more familiar for Valgrind
users (see tools\\valgrind\\drmemory_analyze.py).

Excluded tests are listed in tools\\valgrind\\gtest_exclude - see [Using
Valgrind](/system/errors/NodeNotFound) more for information.

Some debugging tips while using Dr. Memory can be found
[here](https://github.com/DynamoRIO/drmemory/wiki/Debugging).

For those still wanting to use Valgrind, be aware Valgrind requires Chrome be
built using the system memory allocator. To do this, add use_allocator = "none"
to your gn args.

### Feedback?

Drop drmemory-team@ a message
