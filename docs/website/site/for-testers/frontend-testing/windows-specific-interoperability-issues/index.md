---
breadcrumbs:
- - /for-testers
  - For Testers
- - /for-testers/frontend-testing
  - Frontend testing
page_name: windows-specific-interoperability-issues
title: Windows-specific interoperability issues
---

When users face crash on start or other odd behavior, one helpful thing to do is
to gather the list of dlls currently loaded in the chrome.exe processes whenever
possible. You can execute the script in
[list_chrome.zip](/for-testers/frontend-testing/windows-specific-interoperability-issues/list_chrome.zip)
which will generate a file named results.txt. The user can then send the content
of this file by email for further analysis.
