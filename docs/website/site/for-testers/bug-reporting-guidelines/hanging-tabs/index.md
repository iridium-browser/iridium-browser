---
breadcrumbs:
- - /for-testers
  - For Testers
- - /for-testers/bug-reporting-guidelines
  - Bug Life Cycle and Reporting Guidelines
page_name: hanging-tabs
title: Reporting a Hang Bug
---

To investigate problems when Chrome is frozen/locked-up/unresponsive, we
generally need a dump of the affected processes.

Exactly how to do this depends on your operating system:

## Windows

1.  [Download Process Explorer from
            Microsoft](http://technet.microsoft.com/en-us/sysinternals/bb896653).
            (This tool is like a more advanced version of the Task Manager; we
            will use it to create minidumps).
2.  Extract and launch Process Explorer
3.  Right click on the hung Chrome process, and choose "Create Dump"
            --&gt; "Create Minidump..." (If you aren't sure which one is
            misbehaving, repeat this step for each Chrome process)
4.  Attach the minidump file(s) to your bug report

Please note that minidump files can contain some information about your
computer, including the path to where Chrome is installed.

## Mac

1.  Verify you've turned crash reporting on. Go to the page
            chrome://settings/advanced using the address bar. Then verify that
            **Automatically send usage statistics and crash reports to Google**
            is checked.
2.  Open the Task Manager under the **Wrench menu &gt; Tools &gt; Task
            Manager**.
3.  If the columns at the top of the Task Manager window do not include
            "Process ID", then right-click on the table header and check the
            "Process ID" box.
4.  Note the process ID of the hung tab.
5.  Kill the process in a way that causes it to print a crash dump:

*   Open a terminal window and `kill -ABRT *<ProcessID>*` where
            `*<ProcessID>*` is the one you noted above. Copy and paste the crash
            ID dumped to the console into your bug:
    *   You can additionally find the crash dump in
                ~/Library/Application Support/Google/Chrome/Crash Reports/

## Linux

1.  Depending on your sandboxing configuration and Chrome version, you
            may need to pass "--allow-sandbox-debugging" when starting Chrome
            (see [this bug](http://crbug.com/169369)).
2.  Verify you've turned crash reporting on. Go to the page
            chrome://settings/advanced using the address bar. Then verify that
            **Automatically send usage statistics and crash reports to Google**
            is checked.
3.  Open the Task Manager under the **Wrench menu &gt; Tools &gt; Task
            Manager**.
4.  If the columns at the top of the Task Manager window do not include
            "Process ID", then right-click on the window and check the "Process
            ID" box.
5.  Note the process ID of the hung tab.
6.  Kill the process in a way that causes it to print a crash dump:

*   Open a terminal window and `kill -SEGV *<ProcessID>*` where
            `*<ProcessID>*` is the one you noted above. Copy and paste the crash
            ID dumped to the console into your bug:
    *   You can find the crash dump in your `~/.xsession-errors` file
                (assuming you didn't launch Chromium from a terminal yourself)
