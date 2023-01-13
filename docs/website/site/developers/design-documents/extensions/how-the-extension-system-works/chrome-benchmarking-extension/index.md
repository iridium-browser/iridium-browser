---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/how-the-extension-system-works
  - How the Extension System Works
page_name: chrome-benchmarking-extension
title: Benchmarking Extension
---

The Chromium Benchmarking Extension is a quick-and-dirty way to test page load
time performance within Chrome.

### Features

*   Can clear the cache between each page load
*   Can clear existing connections between each page load
*   Works with both SPDY and HTTP
*   Measures time-to-first-paint, overall page load time KB
            read/written, and several other metrics
*   Can compare performance between SPDY and HTTP

### Requirements

*   Chrome version 7 or later
*   Command-line flag "--enable-benchmarking" should be used to start
            chrome

### Screenshot

> ### [<img alt="image"
> src="/developers/design-documents/extensions/how-the-extension-system-works/chrome-benchmarking-extension/benchmark-sm2.png">](/developers/design-documents/extensions/how-the-extension-system-works/chrome-benchmarking-extension/benchmark-sm2.png)

> ### [(Click to
> enlarge)](/developers/design-documents/extensions/how-the-extension-system-works/chrome-benchmarking-extension/benchmark-lg.png)

### Instructions

> **Option 1: Use the benchmark from the chromium source code**

> The benchmark is part of the chromium source code. You'll find it here:

> &lt;path-to-chrome-tree&gt;\\src\\chrome\\common\\extensions\\docs\\examples\\extensions\\benchmark

> To run Chrome with the benchmark, use the following command line:
> chrome.exe --enable-benchmarking
> --load-extension=&lt;path-to-chrome-tree&gt;\\src\\chrome\\common\\extensions\\docs\\examples\\extensions\\benchmark
> **Option 2: Install from Chrome Extension Gallery**
> Install the extension from Chrome Extension Gallery following [this
> link](https://chrome.google.com/extensions/detail/channimfdomahekjcahlbpccbgaopjll).
> Then you'll need to restart the browser to use it.
> When you run chrome, use:
> chrome.exe --enable-benchmarking
> As of M34, the method that seems to work is to load the source code as an
> unpacked extension after enabling developer mode and use the flag
> --enable-net-benchmarking in addition to --enable-benchmarking

Options

> *   Iterations: How many times the test page should be loaded to
              collect performance date.
> *   Clear Results: Clear result table.
> *   Clear connectons: Reset the internal socket pool, oherwise chrome
              will reuse the socket connections if possible.
> *   Clear cache: Reset the cache, otherwise chrome could load the page
              from cache if possible.
> *   Enable Spdy: Use spdy to load a page. When enabled, if the web
              server does not support spdy, an error is reported and test will
              stop.
> *   URL to load: A comma separated list of urls to collect performance
              date. Note, the urls listed here should be the final url. E.g.: if
              url1 will be redirect to url2, url2 should be used. Otherwise, an
              error will be reported and test will stop. You can use "Load URLs
              From File" to load the comma separated url list from file if your
              test set is large.
