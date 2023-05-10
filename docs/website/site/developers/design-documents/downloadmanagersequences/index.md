---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: downloadmanagersequences
title: Download Manager Sequences
---

The following are typical paths taken during a file download.

The start sequence shows what happens after the headers have been received in
response to an HTTP request for a file.

[<img alt="image"
src="/developers/design-documents/downloadmanagersequences/Download_start.png">](/developers/design-documents/downloadmanagersequences/Download_start.png)

Start

This sequence shows what happens when the file has finished downloading:

[<img alt="image"
src="/developers/design-documents/downloadmanagersequences/Download_complete.png">](/developers/design-documents/downloadmanagersequences/Download_complete.png)

Complete

This sequence shows what happens when the user resumes a download that has been
paused:

[<img alt="image"
src="/developers/design-documents/downloadmanagersequences/Download_unpaused.png">](/developers/design-documents/downloadmanagersequences/Download_unpaused.png)

Unpause

This following sequences have not been checked in yet.

This sequence shows what should happen when an error occurs during a file
download.

Note that currently, the "Complete" sequence is executed in this case, so the
file appears to have downloaded properly but is in fact incomplete.

[<img alt="image"
src="/developers/design-documents/downloadmanagersequences/Download_interrupted.png">](/developers/design-documents/downloadmanagersequences/Download_interrupted.png)

Interrupted

This shows how a sequence would be restarted after an interruption (error)
occurs.

It basically sends a new request for the remainder of the data, and appends it
to the existing partial file.

[<img alt="image"
src="/developers/design-documents/downloadmanagersequences/Download_restart.png">](/developers/design-documents/downloadmanagersequences/Download_restart.png)

Restart
