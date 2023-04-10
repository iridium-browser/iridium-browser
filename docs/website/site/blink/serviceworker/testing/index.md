---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/serviceworker
  - Service workers
page_name: testing
title: Service Worker Testing
---

[TOC]

Service Worker has multiple kinds of tests: Content Browser Tests, WebKit Unit
Tests, Telemetry Performance Tests, and Blink Layout Tests.

## Performance Tests

### [Issue 372759](https://code.google.com/p/chromium/issues/detail?id=372759) is tracking the development of Service Worker performance tests.

How to run the performance tests:

1.  Build and install the content_shell_apk target per the [Android
            build
            instructions](https://code.google.com/p/chromium/wiki/AndroidBuildInstructions#Build_Content_shell).
2.  Build the forwarder2 target.
3.  tools/perf/run_benchmark --browser=android-content-shell
            \[--device=xxxx\] service_worker.service_worker

How to update the performance tests:

1.  Check out the [performance test
            sources](https://github.com/coonsta/Service-Worker-Performance). Do
            development, pull request, etc.
2.  Run a local server in that directory, for example: twistd -n web
            --path . --port 8091 (you must use that port number, it appears in
            the test Python scripts.)
3.  Build Linux Release Chromium

[The update_wpr tool](https://source.chromium.org/chromium/chromium/src/+/main:tools/perf/recording_benchmarks.md)
automates the following steps for you:

4.  tools/perf/record_wpr --browser=release
            --extra-browser-args='--enable-experimental-web-platform-features'
            service_worker_page_set
    Note: If you change the output directory of build (like 'out_desktop'), you
    can use '--browser=exact --browser-executable=path/to/your/chrome' instead
    of '--browser=release'.
5.  Briefly sanity check the
            tools/perf/page_sets/data/service_worker_nnn.wpr file to see that it
            doesn't contain requests that should have been handled by a Service
            Worker (look for GET and browse through the URLs.)
6.  Add the new SHA1 hash file (use git status to find it) and upload
            for review, commit as usual. Mention the path and commit hash from
            step 1.

## Web Tests Style

If your test needs to interact with the Service Worker as a client (many do),
open an iframe with a URL controlled by the Service Worker.

Prefix Service Worker scopes with "scope/" when appropriate. This helps prevent
unintentionally registering a Service Worker that controls resources in the test
directory.

Prefix each test by unregistering the Service Worker. If a previous test run
failed or was interrupted, it may have left a Service Worker registration in
place. Unregistering the existing Service Worker first, if any, improves the
reliability of the test.

Clean up resources when the test is done: Unregister the test's Service Worker
at the end of the test. Remove any iframes.
