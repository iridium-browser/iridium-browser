---
breadcrumbs:
- - /developers
  - For Developers
page_name: shutdown
title: Shutdown
---

[TOC]

## Profile Destruction

See [shutdown.md](https://chromium.googlesource.com/chromium/src.git/+/HEAD/docs/shutdown.md#Step-0_Profile-destruction)

## Browser Process Shutdown

`BrowserProcess` is also refcounted, much like `Profile`. `ScopedKeepAlive`
inhibits teardown, and the refcount is managed by `KeepAliveRegistry`.

### Stopping the UI Message Loop

*   A UI event is received that leads to `~ScopedKeepAlive` (closing the window,
            closing the last tab, keyboard shortcut for app termination, etc).
*   If the refcount in `KeepAliveRegistry` drops to zero, then we begin
            application termination.
    *   Note that macOS keeps the application alive even without browser
                windows by adding an extra `ScopedKeepAlive` in `AppController`.
*   If the refcount reaches zero, post a task to the message loop to exit
*   Notification `content::NOTIFICATION_APP_TERMINATING` is broadcast
*   The UI message loop eventually stops running, we exit out of
            `RunUIMessageLoop()`. Note that all other main browser threads are
            still running their message loops. So even though the main (UI)
            thread outlives all other joined browser threads, its `MessageLoop`
            terminates first.

### BrowserProcessImpl deletion

*   After exiting the UI message loop, the shutdown sequence is started
            in BrowserMainParts::RunMainMessageLoopParts, which calls
            PostMainMessageLoopRun.
*   In ChromeBrowserMainParts::PostMainMessageLoopRun:
    *   The ProcessSingleton is released.
    *   MetricsService (which records UMA metrics) is stopped, which:
        *   Creates a log (for future upload) of all remaining metrics.
        *   Records a successful shutdown.
        *   The persistent metrics file, on disk, remains in case other
                    metrics are updated after this point. Any such values will
                    be sent during the next run sometime after Browser startup.
    *   We persist Local State to disk.
    *   We delete g_browser_process, which:
        *   Deletes the ProfileManager (which deletes all the profiles
                    and persists their state, such as Preferences)
        *   Joins the watchdog, IO, CACHE, PROCESS_LAUNCHER threads, in
                    that order
        *   Shuts down the DownloadFileManager and SaveFileManager
        *   Joins the FILE thread
        *   Deletes the ResourceDispatcherHost which joins the WEBKIT
                    thread
        *   Joins the DB thread

### ContentMain() exit

*   The AtExitManager goes out of scope and destroys all
            Singletons/LazyInstances

## Renderer Process Shutdown

### Browser process triggers shutdown via:

*   Closing a tab (explain the sequence of events from UI thread objects
            to the IO thread termination of a renderer process)
