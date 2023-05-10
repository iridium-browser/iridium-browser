---
breadcrumbs:
- - /for-testers
  - For Testers
page_name: enable-logging
title: How to enable logging
---

To enable logging, launch Chrome with these command line flags:

`--enable-logging=stderr --v=1 # Output will be printed to standard error (eg.
printed in the console) and to the debugger`

--enable-logging=stderr --v=1 &gt; log.txt 2&gt;&1 # Capture stderr and stdout
to a log file

*   This will turn on full logging support (INFO, WARNING, ERROR, and
            VERBOSE0 for &gt;=M9).
*   --enable-logging=stderr enables both logging to stderr and using
            OutputDebugStringA on Windows. Note that the sandbox prevents either
            form of this logging from renderers being visible unless you attach
            a debugger to the renderer to see the OutputDebugStringA output.
*   See newer instructions for Linux [at
            docs/linux/debugging.md](https://chromium.googlesource.com/chromium/src/+/lkgr/docs/linux/debugging.md#logging)
*   If you are debugging renderer startup on Windows, we recommend you
            enable histogram logging for additional debugging by using the extra
            command line --vmodule=metrics=2
*   Verbose logging shows up with their own VERBOSEn label.
    *   `--vmodule` enables verbose logging on a per module basis.
                Details in
                [base/logging.h](https://chromium.googlesource.com/chromium/src/+/HEAD/base/logging.h).
                Add --v=-3 at the end to suppress all other logging.
*   Any page load (even the new tab page) will print messages tagged
            with VERBOSE1; for example:
*   \[28304:28320:265508881314:VERBOSE1:chrome/browser/renderer_host/resource_dispatcher_host.cc(1098)\]
            OnResponseStarted: chrome://newtab/
*   The output will be saved to the file `chrome_debug.log` in [Chrome's
            user data directory](/user-experience/user-data-directory)
    *   Release builds: The parent directory of Default/.
    *   Debug builds: The binary build folder (e.g. out\\Debug).
    *   Chrome OS
        *   Open file:///var/log/messages
        *   `/var/log/chrome` at the login screen.
        *   Files within the `log` subdirectory under the logged-in
                    user's encrypted home directory, which resides under
                    `/home/chronos`.
*   Logs are overwritten each time you restart chrome.
*   To enable logging from the render processes on Windows you need to
            output the log to a file specified by absolute path. e.g.
            `--log-file=c:\src\log.txt` as well also specifying the
            `--enable-logging` command line.

*   To see WTF_LOG, use `--blink-platform-log-channels`

Note that:

*   If the environment variable CHROME_LOG_FILE is set, Chrome will
            write its debug log to its specified location. Example: Setting
            CHROME_LOG_FILE to "chrome_debug.log" will cause the log file to be
            written to the Chrome process's current working directory while
            setting it to "D:\\chrome_debug.log" will write the log to the root
            of your computer's D: drive.
*   To override the log file path in a test harness that runs Chrome,
            use this pattern:

    ```none
    #include "chrome/common/env_vars.h"
    ...
    // Set the log file path in the environment for the test browser.
    std::wstring log_file_path = ...;
    SetEnvironmentVariable(env_vars::kLogFileName, log_file_path.c_str());
    ```

### How do I specify the command line flags?

See [command line flags](/developers/how-tos/run-chromium-with-flags) page.

### What personal information does the log file contain?

Before attaching your chrome_debug.log to a bug report, be aware that it can
contain some personal information, such as URLs opened during that session of
chrome.

Since the debug log is a human-readable text file, you can open it up with a
text editor (notepad, vim, etc..) and review the information it contains, and
erase anything you don't want the bug investigators to see.

The boilerplate values enclosed by brackets on each line are in the format:

\[process_id:thread_id:ticks_in_microseconds:log_level:file_name(line_number)\]

### Sawbuck

Alternatively to the above, you can use the Sawbuck utility (for Windows) to
view, filter and search the logs in realtime, in a handy-dandy GUI.

[<img alt="image"
src="https://sawbuck.googlecode.com/files/Sawbuck_screenshot.png">](https://sawbuck.googlecode.com/files/Sawbuck_screenshot.png)

First download and install the latest version of
[Sawbuck](https://github.com/google/sawbuck/releases/latest), launch it, then
select "Configure Providers.." form the "Log" menu.

This will bring up a dialog that looks something like this:

[<img alt="image" src="/for-testers/enable-logging/configure%20providers.png">
](/for-testers/enable-logging/configure%20providers.png)

Set the log level for Chrome, Chrome Frame, and/or the Setup program to whatever
suits you, and click "OK". You can revisit this dialog at any time to increase
or decrease the log verbosity.

Now select "Capture" from the "Log" menu, and you should start seeing Chrome's
log messages.

Note that Sawbuck has a feature that allows you to view the call trace for each
log message, which can come in handy when you're trying to home in on a
particular problem.

**Note for 64-bit Chrome:** Reporting of callstacks, source file, and line info
does not currently work when originating from 64-bit Chrome, and log messages
will be garbage by default (<https://crbug.com/456884>). Change the "Enable
Mask" for the Chrome and Chrome Setup providers so that "Text Only" is the only
option selected to have non-garbaled log messages.
