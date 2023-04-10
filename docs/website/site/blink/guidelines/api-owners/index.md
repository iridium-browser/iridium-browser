---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/guidelines
  - Guidelines and Policies
page_name: api-owners
title: Blink API owners
---

Summary

The Blink API owners oversee, enforce and evolve the [Blink Intent
process](/blink/launching-features). Their most public role is to approve
experimentation and shipping of new or changed
[web-exposed](/blink/guidelines/web-exposed)
[APIs](https://chromestatus.com/features) to Chromium-based browsers.

You can find a list of the current API owners
[here](https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/API_OWNERS).

## What is the Blink Intent process?

The [Blink Intent process](/blink/launching-features) - proceeding from
Prototype to Ship - is how web-exposed features ship in Chromium.

## What are the Blink API owners?

The Blink API owners are the stewards of the [core values of
Blink](/blink/guidelines/values) and those [values in
practice](/blink/guidelines/web-platform-changes-guidelines). They achieve this
by enforcing proper use of the Intent process, which is designed to protect and
enhance those values.

The Blink Intent process itself is also overseen and evolved by the Blink API
owners.

## What do the Blink API owners actually do?

It can be summarized as:

*   LGTMs from at least three Blink API owners are necessary to ship or
            change a web platform feature in Chromium
*   One LGTM is necessary to start an experiment or a deprecation period
*   The primary task of Blink API owners is to decide whether to give
            those LGTMs

They also give reasons, if needed, why the LGTM was provided or not provided.
The reasons are needed in cases where it may not otherwise be clear why the
decision was made. LGTMs are provided via email to
[blink-dev@chromium.org](https://groups.google.com/a/chromium.org/g/blink-dev).
Public discussion about issues of interest to the API owners occurs on
[blink-api-owners-discuss@chromium.org](https://groups.google.com/a/chromium.org/g/blink-api-owners-discuss).
Weekly meetings process the [backlog of intents needing
decisions](https://docs.google.com/spreadsheets/d/1pvXEMD5pRioognaqEzglS-4ZBSQ_YmzL8Fiz7yt4Bb4/edit#gid=0&fvid=1112765777).
Any significant changes to the Intent process also need the approval of the API
owners.

In practice, the Blink API owners may also help developers through tricky parts
of the Intent process.

## How do the Blink API owners arrive at their decisions?

The Blink API owners review the Intent process was faithfully followed, and most
importantly that nothing about experimenting with or shipping these features is
in conflict with any of the core values of Blink.

The API owners will ask any questions about the evidence provided on the
blink-dev thread for the Intent, so that everyone can see those questions.

More details

[Procedures governing the API owners](/blink/guidelines/api-owners/procedures)

[Requirements for becoming an API
owner](/blink/guidelines/api-owners/requirements)
