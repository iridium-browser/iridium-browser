---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: diagnostic-mode
title: Diagnostic Mode
---

Chromium (and Google Chrome) comes with a set of built-in diagnostic tests that
any user can run. These tests can help determine the root cause of the following
problems.

*   Chrome fails to start
*   Chrome crashes quickly after started
*   Chrome starts but any tab created crashes or fails to load
*   Chrome is extremely slow

**Work in Progress** : Diagnostic mode is currently only available in the
tip-of-tree version of Chromium for Windows and in the developer channel version
of Google Chrome for Windows. Furthermore, only a small set of diagnostic tests
currently run (more are planned in the future).

Diagnostic mode is an environment that runs a set of tests before the browser
starts. These tests vary in sophistication, from trivial checks that critical
files exist, to more complex (and time consuming) tests that test the integrity
of the key databases. Each test can pass, fail or simply be skipped if the test
is not applicable. The output should be used as as a starting point for further,
more specific, investigation and troubleshooting.

### Privacy

Diagnostic mode does not automatically send any information to Google. It is up
to the user to copy the text output from running diagnostic mode that they
consider appropriate and helpful for others to see. Furthermore, care has been
taken to expose little to no personal information in the diagnostic output so
that users can feel comfortable sharing it with internet help groups.

### How to run (windows only)

1.  Make sure no instances of chrome.exe are running. Check in the
            Windows Task Manager for any zombie chrome.exe processes and end
            them if necessary.
2.  Open a command line session (cmd.exe)
3.  Change directory (cd) to the directory of chrome.exe, for example,in
            Vista, regular chrome install is in C:\\Users\\&lt;user
            name&gt;\\AppData\\Local\\Google\\Chrome\\Application
4.  Type chrome.exe --diagnostics \[enter\]

A second console window will open and the tests will run automatically, and you
should see something like this:

[<img alt="image"
src="/administrators/diagnostic-mode/diagmode_wiki.png">](/administrators/diagnostic-mode/diagmode_wiki.png)

This shows a run where all tests passed. If a test fails it will be marked with
a **\[FAIL\]** entry.
