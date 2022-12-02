---
breadcrumbs:
- - /devtools
  - Google Chrome Developer Tools
page_name: capturing-a-timeline-trace
title: Capturing a Timeline Trace
---

### Prereqs if recording from Android:

*   Use an up-to-date [Chrome Dev Channel on
            android](https://play.google.com/store/apps/details?id=com.chrome.dev&hl=en).
*   [Connect to the device with remote
            debugging](https://developers.google.com/web/tools/chrome-devtools/debug/remote-debugging/remote-debugging)

## Steps:

1.  Inspect the tab w/ devtools, open Timeline panel.
2.  The only checkbox checked should be JS Profile.
3.  Hit record.
4.  Do whatever action is slow. (reload, tapping an icon, scrolling).
5.  End the recording
6.  If that looks like it captured the right stuff, right-click and save
            it.
    *   [<img alt="image"
                src="/devtools/capturing-a-timeline-trace/save%20timeline.png">](/devtools/capturing-a-timeline-trace/save%20timeline.png)
7.  Zip the file if you can. It'll compress a 40MB file to about 4MB.
8.  Now you can share it: attaching it to a bug, emailing it, or sharing
            via dropbox/etc.
