---
breadcrumbs:
- - /developers
  - For Developers
page_name: change-logs
title: Viewing change logs for Chromium and Blink
---

## The way to generate change logs for Chromium is via gitiles. The format of the URLs is as follows:

https://chromium.googlesource.com/chromium/src/+log/oldhash..newhash?pretty=full

"pretty" can be left off. Options are "full" and "fuller".

*   [Example changelog without pretty
            option](https://chromium.googlesource.com/chromium/src/+log/a436b4f19b34bc9ae667530d7cf38916b8237172..ca100d5970b0d0b9a3af96d180f5ea2862227a48)
*   [Example changelog with
            pretty=full](https://chromium.googlesource.com/chromium/src/+log/a436b4f19b34bc9ae667530d7cf38916b8237172..ca100d5970b0d0b9a3af96d180f5ea2862227a48?pretty=full)
*   [Example changelog with
            pretty=fuller](https://chromium.googlesource.com/chromium/src/+log/a436b4f19b34bc9ae667530d7cf38916b8237172..ca100d5970b0d0b9a3af96d180f5ea2862227a48?pretty=fuller)

JSON format queries:

*   add format=JSON to any query (note the ALL CAPS). E.g.:
            <https://chromium.googlesource.com/chromium/src/+log/main?pretty=full&format=JSON>
*   The "next" key inside JSON provides a cursor for the next page. Use
            it with s=&lt;next_cursor&gt;, e.g.
            <https://chromium.googlesource.com/chromium/src/+log/main?pretty=full&s=baa385c24c57df62f64a5f1fecee739dc2dcc3ba>
    *   The last page will have no "next" key. Also any page except for
                the first will have a "previous" cursor to page backwards.
*   Page size: e.g. n=1000:
            <https://chromium.googlesource.com/chromium/src/+log/main?pretty=full&format=JSON&n=1000>

Note, that JSON format has a ")\]}'" line at the top, to prevent cross-site
scripting. When parsing, assert that the first line has ")\]}'", strip it, and
parse the rest of JSON normally.
