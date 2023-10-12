---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: web-workers
title: Web Workers
---

Web Workers spec:
<http://www.whatwg.org/specs/web-apps/current-work/multipage/workers.html>

As of Nov 2015, the most up-to-date documentation is:
<https://docs.google.com/document/d/1i3IA3TG00rpQ7MKlpNFYUF6EfLcV01_Cv3IYG_DjF7M/edit>

Contents below are obsolete!

As of Nov 2014, the most up-to-date documentation was:
<https://docs.google.com/a/chromium.org/document/d/1NYMYf-_P_K2iPKlSGyv5ou_x0yL_4IRwt-DPOYqRb0s/edit#>

As of Oct 2013, Blink/Chromium's multi-process architecture Web Workers
(Dedicated and Shared Worker) was implemented as follows:

*   **Dedicated Worker**
    *   Runs in the same **renderer process** as its parent document,
                but on a different thread (see
                [Source/core/workers/WorkerThread.\*](https://code.google.com/p/chromium/codesearch#search/&q=file:WorkerThread.h&sq=package:chromium&type=cs)
                in blink for details). Note that it’s NOT the chromium’s worker
                process thread implemented in content/worker/worker_thread.\*.
    *   In Chromium the renderer process’s main thread is implemented by
                content/renderer/render_thread_impl.\*, while the Blink’s worker
                thread does NOT have corresponding message_loop/thread in
                chromium (yet). You can use
                [webkit_glue::WorkerTaskRunner](https://code.google.com/p/chromium/codesearch#chromium/src/webkit/child/worker_task_runner.h&l=19)
                (webkit/child/worker_task_runner.\*) to post tasks to the
                Blink’s worker threads in chromium.
    *   The embedder’s platform code in Chromium for Dedicated Worker is
                [/content/renderer/\*](http://src.chromium.org/viewvc/chrome/trunk/src/content/renderer/)
                (in addition to content/child/\*), but NOT content/worker/\*.
    *   Platform APIs are accessible in Worker contexts via
                WebKit::Platform::current(), and it’s implemented in
                [RendererWebKitPlatformSupport](https://code.google.com/p/chromium/codesearch#chromium/src/content/renderer/renderer_webkitplatformsupport_impl.h&l=47&q=RendererWebKitPlatformSupport&type=cs&sq=package:chromium)
                (content/renderer/renderer_webkitplatformsupport_impl.cc) in
                Chromium (as in the case for regular document contexts).
    *   Note that to call Platform APIs from Worker contexts it needs to
                be implemented in a thread-safe way in the embedder code (e.g.
                do not lazily create a shared object without lock), or the
                worker code should explicitly relay the call onto the renderer’s
                main thread in Blink (e.g. by
                [WTF::callOnMainThread](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/wtf/MainThread.h&l=48&q=callOnMainThread&type=cs&sq=package:chromium)).
*   **Shared Worker (the following was valid until April 2014)**
    *   Runs in its own separate process called **worker process** and
                is connected to multiple renderer processes via the browser
                process. Shared Worker also runs in its own worker thread.
    *   In Chromium the worker process’s main thread is implemented by
                content/worker/worker_thread.\* (note that this is NOT the
                worker thread you refer in Blink!!). As well as in Dedicated
                Workers the Blink’s worker thread does not have corresponding
                message_loop/thread in chromium.
    *   The embedder’s platform code in Chromium for Shared Worker is
                [/content/worker/\*](http://src.chromium.org/viewvc/chrome/trunk/src/content/worker/)
                (in addition to content/child/\*).
    *   Platform APIs are accessible in Worker contexts via
                WebKit::Platform::current(), and it’s implemented in
                [WorkerWebKitPlatformSupportImpl](https://code.google.com/p/chromium/codesearch#chromium/src/content/worker/worker_webkitplatformsupport_impl.h&q=WorkerWebKitPlatformSupport&sq=package:chromium&type=cs&l=30)
                (content/worker/worker_webkitplatformsupport_impl.cc) in
                Chromium. As well as for Dedicated Worker, to use a platform API
                in Worker contexts the API needs to be implemented in a
                thread-safe way in the embedder code, or Blink-side code should
                explicitly relay the platform API access onto the main thread by
                [WTF::callOnMainThread](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/wtf/MainThread.h&l=48&q=callOnMainThread&type=cs&sq=package:chromium).
