---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/launching-features
  - Launching Features
page_name: old-process
title: old-process
---

<table>
<tr>

<td>## \[This documentation is now obsolete! Please refer <a href="/blink/launching-features">here</a> for the current process\]</td>

<td>## The process to launch a new feature</td>

<td><b>Note:</b> You can combine the intent-to-implement email with an
intent-to-experiment or with an intent-to-ship. You must still complete steps 2
- 4.</td>

1.  <td>Email blink-dev using the <a
            href="https://docs.google.com/document/d/1vlTlsQKThwaX0-lj_iZbVTzyqY7LioqERU8DK3u3XjI/edit#bookmark=kix.p5nalpch13qw">“Intent
            to Implement” template</a>. </td>
    *   <td>Formal approval isn't necessary to proceed with
                implementation</td>
    *   <td>Code cannot be checked into trunk without an LGTM in code
                review</td>
    *   <td>Opposition on the "Implement" thread will likely resurface
                when you try to ship, so try to resolve issues early</td>
2.  <td>Create an entry on <a
            href="http://chromestatus.com/">chromestatus</a>.</td>
    *   <td>You'll need to use an @chromium.org account for
                chromestatus.com. If you don't have one, please fill out this
                form.</td>
    *   <td>If you have trouble with chromestatus, please open an issue
                on GitHub.</td>
3.  <td>File an OWP launch tracking bug</td>
    *   <td>Some launches may require a formal Chrome review. If your
                feature has privacy, legal, or UI impact, please email
                web-platform-pms@google.com to set a review in motion.</td>
4.  <td>Most changes should start a <a
            href="https://w3ctag.github.io/workmode/#specification-reviews">TAG
            review</a>. If you're not sure, file one. And link to the bug
            tracker for that to make it easy.</td>
5.  <td>Implement your change as a Runtime-Enabled Feature.</td>
6.  <td>Your feature should have interop tests, preferably
            web-platform-tests. If Chrome is the first one implementing a spec,
            the requirements for web-platform-tests coverage + quality will be
            fairly high for shipping.</td>
7.  <td>Optionally, run an <a href="/blink/origin-trials">origin
            trial</a> for your feature to collect developer feedback/data, as
            input to the standardization process.</td>
    *   <td>If you answer “no” to all of the following questions, an
                origin trial is unnecessary (see caveat in \[1\]).</td>
        *   <td>Is there disagreement about how well this API satisfies
                    its intended use case?</td>
        *   <td>Are you unsure about what API shape will be the most
                    ergonomic in real world scenarios?</td>
        *   <td>Is it hard to quantify performance gains without testing
                    on real world sites?</td>
        *   <td>Is there a reason that this API needs to be deployed to
                    real users, rather than behind a flag, for data to be
                    meaningful?</td>
    *   <td>If you decide to run a trial, or are unsure, please first
                consult with the Origin Trials core team. </td>
        *   <td>Email <a
                    href="mailto:experimentation-dev@chromium.org">experimentation-dev@chromium.org</a>.
                    Google employees can alternatively schedule a meeting
                    directly with <a
                    href="mailto:origin-trials-core@google.com">origin-trials-core@google.com</a>.</td>
    *   <td>If you've decided to run an origin trial, do the following
                steps. If not then skip to step 8 of the launch process.</td>
        *   <td>Follow the instructions on <a
                    href="/blink/origin-trials/running-an-origin-trial">how to
                    run an origin trial</a>. Google employees should see <a
                    href="http://goto.google.com/running-an-origin-trial">go/running-an-origin-trial</a>.</td>
        *   <td>Email blink-dev using the <a
                    href="https://docs.google.com/document/d/1vlTlsQKThwaX0-lj_iZbVTzyqY7LioqERU8DK3u3XjI/edit#bookmark=id.pygli2e122ic">“Intent
                    to Experiment” template</a>. Wait for LGTM from at least 1
                    API Owner (again see caveat in \[1\]).</td>
        *   <td>At the start of every subsequent release, post an update
                    on usage of the feature on the intent to experiment thread
                    in blink-dev.</td>
        *   <td>There is an automatic and mandatory 1 week period when
                    your feature is disabled at the end of the origin trial,
                    before it potentially graduates to Stable. Tokens will
                    expire 1 week before the earliest stable channel launch
                    date, but note that a stable launch takes many days to
                    deploy to users. This exists to encourage feature authors to
                    make breaking changes, if appropriate, before the feature
                    lands in stable, and to make clear to clients of the origin
                    trial feature that in all circumstances the feature will be
                    disabled (hopefully only briefly) at some point.</td>
8.  <td>When your feature is ready to ship, email blink-dev using the <a
            href="https://docs.google.com/document/d/1vlTlsQKThwaX0-lj_iZbVTzyqY7LioqERU8DK3u3XjI/edit#bookmark=id.w8j30a6lypz0">“Intent
            to Ship” template</a>. </td>
    *   <td>Respond to any feedback or questions raised in the
                thread</td>
    *   <td>You need at least 3 LGTMs from API owners to launch.</td>
    *   <td>If you have resolved all feedback and are blocked on API
                owner LGTMs, add blink-api-owners-discuss@chromium.org
                requesting final approval.</td>
9.  <td>Enable by default.</td>

<td>\[1\] Origin trials should be run for a specific reason. These questions are
guidance to identifying that reason. However, there is still debate about the
right reasons, so the guidance may change. You can join the conversation in <a
href="https://docs.google.com/document/d/1oSlxRwsc8vTUGDGAPU6CaJ8dXRdvCdxvZJGxDp9IC3M/edit#heading=h.eeiog2sf7oht">this
doc</a>.</td>

</tr>
</table>
