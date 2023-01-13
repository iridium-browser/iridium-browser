---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: strict-origin-isolation-trial
title: Strict Origin Isolation Trial
---

tl;dr This page describes a desktop Canary-only field trial to study the effect
of isolating pages by origin (as opposed to the current [Site
Isolation](/Home/chromium-security/site-isolation) approach using sites) on
Chrome's performance. The trial will be one week in duration, starting around
May 23rd, 2019. Some pages, such as those that rely on setting document.domain
to perform cross-origin scripting, may encounter issues during this trial.

Tracking Issue: <https://crbug.com/902399> “Evaluate feasibility of widespread
origin isolation”

## Overview

The Strict Origin Isolation Trial is a short-duration (one week) field trial
designed to gather preliminary data about the performance impact of changing the
granularity of isolation from site (protocol and eTLD+1) to origin (protocol,
host, and port).

Strict Origin Isolation would improve security by ensuring different origins do
not share a process with each other, but it poses a risk of increased resource
usage. This study will allow us to study the expected impact on process count,
memory usage, and other performance metrics if all origins were isolated,
including potential performance benefits from increased parallelization.

However, this trial also poses a functional risk to web pages that script
same-site but cross-origin frames, which is possible if the pages modify their
document.domain values via JavaScript. Such cross-origin scripting will be
disrupted if the documents are in different processes, resulting in JavaScript
errors visible in the DevTools console and potentially broken page features.

<table>
<tr>

<td>Sample cross-origin scripting error:</td>

<td>On a page a.example.com with a subframe b.example.com, where both execute document.domain = ‘example.com’, and b.example.com attempts something like top.document.body.innertext, the following error occurs:</td>

</tr>
<tr>

<td>VM118:1 Uncaught DOMException: Blocked a frame with origin "https://b.example.com" from accessing a cross-origin frame.</td>

<td> at &lt;anonymous&gt;:1:5</td>

</tr>
</table>

Because of this risk, we will limit this trial to a single week on only the
Canary channel, to avoid disruptions to users of the Dev, Beta, and Stable
channels. 50% of Canary channel users will be opted in to the Strict Origin
Isolation mode, while the remaining 50% will act as a control group using Site
Isolation. The trial will only affect Chrome versions 76.0.3791.0 and higher.

## Trial Contact Information

During the trial, if you need further information, please reach out to:

    [wjmaclean@chromium.org](mailto:wjmaclean@chromium.org)

    [site-isolation-dev@chromium.org](mailto:site-isolation-dev@chromium.org)

## Disabling the Trial

If you encounter issues during the trial, you can opt-out by running Chrome with
--disable-features=StrictOriginIsolation, or changing the
chrome://flags/#strict-origin-isolation flag from Default to Disabled.

## Reporting Bugs

If you experience incorrect behaviour during the trial, first check the
variations list in chrome://version to see if the variation for the trial
(0x4c825337 or 1283609399), is present. If so, and if the issue goes away when
disabling the trial (see above), please file a bug by clicking on [this
link](https://bugs.chromium.org/p/chromium/issues/entry?template=Defect+report+from+user&components=Internals%3ESandbox%3ESiteIsolation&blocking=902399&cc=wjmaclean@chromium.org&summary=Issue+during+Strict+Origin+Isolation+Trial:).

## Recommended Developer Actions

To avoid impact from the trial (and otherwise improve security), web developers
can avoid modifying document.domain to script cross-origin frames.

## Summary

Determining the feasibility of increasing Chromium’s isolation granularity from
sites to origins is important for improving security performance for our users.
This trial is a first step to better understand the implications, before
considering next steps.
