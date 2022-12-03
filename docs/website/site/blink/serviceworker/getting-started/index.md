---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/serviceworker
  - Service workers
page_name: getting-started
title: Getting Started
---

### For web developer documentation, see:

*   [Jake Archibald's
            "simple-serviceworker-tutorial"](https://github.com/jakearchibald/simple-serviceworker-tutorial)
*   ["Introduction to Service
            Worker"](http://www.html5rocks.com/en/tutorials/service-worker/introduction/)
            at html5rocks.com
*   ["The Offline
            Cookbook"](http://jakearchibald.com/2014/offline-cookbook/)
*   ["Is Service Worker Ready
            Yet?"](https://jakearchibald.github.io/isserviceworkerready/)

If you are a Chromium developer, read on!

Here's how to get a demo Service Worker up and running:

**Steps:**

1.  Open the demo at
            <https://googlechrome.github.io/samples/service-worker/basic/index.html>
            ([source](https://github.com/GoogleChrome/samples))
2.  Open the Inspector. Things are logged there later on.
3.  Try visiting Cleveland and get a 404. Go back.
4.  Click the 'Register' button, watch the Inspector console, then try
            visiting Cleveland again... ***oooh***.
5.  Go to chrome://inspect#service-workers and click on Inspect to open
            the Inspector and debug your Service Worker.

### Running it locally

*   Clone the [git repo](https://github.com/GoogleChrome/samples)
*   Navigate to the samples/service-worker/basic folder
*   Start a local webserver (see below)
*   Open http://localhost:8080/index.html (or whatever path and port you
            set up.)
*   Feel free to edit the JS and try some new things!

### How to run a local server

On a system with python:

$ python -m SimpleHTTPServer 8080

On Android use Chrome Developer Tools' [port
forwarding](https://developer.chrome.com/devtools/docs/remote-debugging#enable-reverse-port-forwarding).

### Development flow

In order to guarantee that the latest version of your Service Worker script is
being used, follow these instructions:

1.  Configure your local server to serve your Service Worker script as
            non-cacheable (cache-control: no-cache)
2.  After you made changes to your service worker script:
    1.  **close all but one** of the tabs pointing to your web
                application
    2.  hit **shift-reload** to bypass the service worker as to ensure
                that the remaining tab isn't under the control of a service
                worker
    3.  hit **reload** to let the newer version of the Service Worker
                control the page.

### Documentation

*   For an overview of the possibilities: Jake's [ServiceWorker is
            coming, look busy](https://www.youtube.com/watch?v=SmZ9XcTpMS4) talk
            (30min)
