---
breadcrumbs:
- - /for-testers
  - For Testers
- - /for-testers/bug-reporting-guidelines
  - Bug Life Cycle and Reporting Guidelines
page_name: reporting-crash-bug
title: Reporting a Crash Bug
---

If you are planning to report a crash bug, please make sure you include enough
information for debugging purpose.

*   Make sure you have the latest Chrome version (on Windows go to
            Chrome Options-&gt;About Google Chrome)
*   Make sure you have [crash reporting
            enabled](https://support.google.com/chrome/answer/96817).
*   Try to get exact reproducible steps for the crash, if possible.
*   Try to also provide more debugging info by either: .
    *   **Reporting the crash directly**: If possible, and you are using
                Google Chrome, enable crash reporting in Google Chrome's
                settings: go to chrome://chrome/settings, click "Show advanced
                settings..." and in the Privacy section select select
                "Automatically send usage statistics and crash reports to
                Google", then see the instructions below for getting a crash ID.
                (Note that if this setting was not already enabled, you'll need
                to reproduce the crash first.)
    *   **Collecting crash data**: Otherwise, see the instructions for
                collecting crash data.

### Getting a Crash ID

If crash reporting is enabled, you should provide the crash ID in your bug
report, or your client ID if the crash ID isn't available. **Go to
`chrome://crashes` and see if your crash is listed**. If so, copy and paste the
crash ID into your bug report.

Otherwise, copy and paste the line containing "client_id" in your `Local State`
file from your profile into the bug report. The location of the profile depends
on your platform:

*   #### Windows:
    *   #### Vista and higher: `%LOCALAPPDATA%\Google\Chrome\User Data`
    *   #### `XP: %USERPROFILE%\Local Settings\Application
                Data\Google\Chrome\User Data` on Windows XP
*   #### Linux: `~/.config/google-chrome`
*   #### Mac: `~/Library/Application Support/Google/Chrome`

### Collecting Crash Data

If crash reporting is not enabled, or no crash ID is available, you can collect
raw crash data.

#### Windows

*   Windows can be configured to log crash reports to
            %localappdata%\\crashdumps, by using *regedit* to create the key
            (registry folder)
            "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\Windows Error
            Reporting\\LocalDumps". For details see this [Microsoft
            article](https://msdn.microsoft.com/en-us/library/windows/desktop/bb787181(v=vs.85).aspx).
            You can see the recorded crash dumps by pasting
            %localappdata%\\crashdumps into Windows Explorer or the search area
            in the start menu. Look for a chrome.exe.123.dmp file from the time
            of your crash, and if there is one, attach it to your bug.
*   You can capture a minidump file using windbg.exe:
    *   If you do not already have `windbg.exe`, download it from
                Microsoft
                [website](http://www.microsoft.com/whdc/DevTools/Debugging/default.mspx)
    *   Run WinDbg with this command - `windbg.exe -o <full path to
                chrome.exe>`
    *   Keep hitting F5 until you hit an exception (Access violation or
                some other error) or until Chrome window comes up
    *   If Chrome Window opens successfully, try to reproduce the crash.
                When crash happens you should see an exception reported in
                WinDbg (Access violation or some other error)
    *   After the exception has been reported save the dump file by
                typing this exact command in WinDbg command: `.dump
                C:\chrome.dmp` (do not forget '`.`' at the beginning)
    *   Once the dump file has been successfully saved, compress this
                dump file, upload it somewhere and post the link to the file
                along with the bug description.

#### Linux

*   If you are using Chromium follow
            [these](https://wiki.ubuntu.com/Chromium/Debugging) instructions on
            how to get additional details about the Chromium crash on Ubuntu.

#### Mac

*   System crash reports are logged to `~/Library/Logs/CrashReporter`,
            so check there for a crash file from the time of your crash, and if
            there is one, attach it to your bug.

**Android**

*   System crash reports are logged to `/data/data/$PACKAGE/cache/Crash\
            Reports/`, where `$PACKAGE` depends on which app (chrome, content
            test shell, etc..), as defined
            [here](https://cs.chromium.org/chromium/src/third_party/catapult/devil/devil/android/constants/chrome.py?l=11).
*   Sometimes the crash will also be printed to the "logcat" and/or as a
            tombstone file. More info
            [here](/developers/how-tos/debugging-on-android#TOC-Symbolizing-Crashstacks-Tombstones)
            on how to obtain and symbolize it.
*   Alternatively, use "`adb shell bugreport`" to collect system-wide
            information.
