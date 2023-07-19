---
breadcrumbs:
- - /Home
  - Chromium
page_name: loading
title: Loading
---

## Information about Loading efforts in Chromium

Last updated May 2021

## **North Star**

Loading on the web is user centric, sustainable, fast and delightful.

*   **user centric:** the experience is described through key user
            moments or needs, and assessed against a perceptual model of
            performance.
*   **sustainable:** a sensible contract between Web developers, UA and
            users about how a web application loads, with usage of memory /
            power / data that is proportional to value.
*   **fast and delightful:**
    *   Meet the [Core Web Vitals thresholds](https://web.dev/defining-core-web-vitals-thresholds/)

These are the goals we should strive for.

Some aspects might be extremely challenging but making progress toward these is
what should drive our work.

## How do I find out what's happening?

*   Communications: loading-dev@chromium.org is our public discussion
            group for all things related to Loading in Chrome.
*   crbug: [blink](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3ABlink%3ELoader%2CBlink%3ENetwork%2CBlink%3EServiceWorker%2CBlink%3EWorker&sort=pri+-component&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids)

## I have a reproducible bad Loading user experience, what do I do?

*   ## As a user:
    *   File a [Speed bug](https://bugs.chromium.org/p/chromium/issues/entry?template=Speed%20Bug)
    *   Include the "Loading" keyword in the subject if the issue fits within the scope of the North Star.
    *   If you are not sure, don't include the "Loading" keyword, triage will make sure it shows up in the right bucket.
*   ## As a chromium developer
    *   Same steps but try to [record a trace](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs)
                (select every trace categories on the left side).

## I'm a dev and interested in helping on Loading. How do I get started?

Reach out via loading-dev@ and tell us more about you:

*   share your particular interest and expertise
*   tell us how familiar you are with chromium development
*   point to CLs if not a lot of developers are familiar with your work

## Contacts

Your friendly PM: kenjibaheux

Blink TLs: kouhei, yhirano (blink&gt;network)

"Here here!": reach out to kenjibaheux@ if you want your name to be added here.

## Other resources

*   [OOR-CORS: Ouf of Renderer CORS](/Home/loading/oor-cors)
*   <https://chromium.googlesource.com/chromium/src/+/main/net/docs/life-of-a-url-request.md>
*   <https://docs.google.com/presentation/d/1ku7pkh09h6sQ6epudsVvHehRzvanAU7ckzfiVvMoljo/edit?usp=sharing>
*   <https://chromium.googlesource.com/chromium/src/+/main/third_party/blink/renderer/platform/loader/README.md>
*   <https://docs.google.com/presentation/d/1r9KHuYbNlgqQ6UABAMiWz0ONTpSTnMaDJ8UeYZGWjls/edit>
