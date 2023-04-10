---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/platform-predictability
  - Web Platform Predictability
page_name: objectives
title: Objectives
---

At Google we define and track progress against our goals using
["OKRs"](https://www.gv.com/lib/how-google-sets-goals-objectives-and-key-results-okrs)
(Objectives and Key Results). Here are the OKRs for the web platform
predictability infrastructure project. Note that these are **intentionally
aggressive** and so we will be happy if we deliver 60%-70% of them. Platform
predictability also includes broader efforts by other web platform teams (eg.
improving interop of the features they own), but those are owned by the
individual teams (eg. [input-dev](/teams/input-dev/input-objectives)) and so not
included here.

## 2016 Q3

    ### Improve interop testing

    Invest in a high quality test suite for the web platform as a whole.

        #### Imports happening daily via simplified import process.

        *   Add a builder which tries to update, and aborts if an
                    automatic update couldn't be completed.
        *   Add a script to trigger try jobs to discover which new tests
                    are failing across different platforms.
        *   Add a mapping of directories to auto-update preferences, and
                    when updating, edit W3CImportExpectations/TestExpectations
                    and file bugs according to this mapping.

        #### Easier contribution to web-platform-tests

        Implement a way to contribute to WPT repository with Chromium tools. Our
        tools assume that some configuration files are checked in to the target
        repository, and we need to put such files to a separated repository for
        WPT.
        *   Agreement on changes in our tools with chrome-infra
        *   Implement
        *   Land the first CL using the process to both of Chromium repo
                    and WPT repo
        *   Multiple Blink engineers landed CLs with the process

        #### Bugs filed for all automated web-platform-tests that fail on Chrome but pass on two other engines

        Use this to increase our understanding of the value of WPT

        #### Eliminate csswg-test build step

        Test harness runs unbuilt tests

        #### Eliminate csswg-test pull-request backlog

        32 as of 6/20

        #### W3C PointerEvent Tests: Complete automation through script injection

        shared with input-dev.

    ### Improve understanding of web compat

    Tools for better data for decision making that impacts compat, and process
    improvements that reduces the risk/cost of regressions.

        #### Googlers can use bisect-builds.py to bisect regressions on Linux Chrome down to an individual commit position

        Internal only for now due to only official Chrome builds being archived.
        Obviously doesn't include positions which fail to build, etc.

        #### Land scripts and publish instructions for using cluster telemetry for UseCounter analysis

        #### Fix fast shutdown and PageVisits issues in UseCounters in Chrome 54

        <https://crbug.com/597963>, <https://crbug.com/236262>

        #### Web platform TLs regularly use a view of regression rates per component

        #### Extend Cluster telemetry to add new anlyasis page and support runs on 100k web pages

        Goal is developers can intuitively and easily schedule analysis runs
        using Cluster Telemetry's new analysis page. Specific AIs: Make CT using
        swarming. No waiting in queue. Add 200+ VMs to CT pool. Add support for
        100k pages. <http://skbug.com/5465>, <http://skbug.com/5316>,
        <http://skbug.com/5245>

        #### Remove (or downgrade) deprecation warnings not on imminent path for removal

        Goal is developers can easily tell what things are highly likely to
        impact them and when <https://crbug.com/568218>

        #### Concrete plan for deprecation API

        <https://crbug.com/564071>

        #### Establish regular triage of Hotlist-PredictabilityInfra bugs

        Combined with hotlist-interop triage?

    ### Improve web platform feature interoperability

    Partner with feature teams to invest in improving interop in specific
    high-return areas (as an exception to the general rule that teams own their
    own interop).

        #### Ship the unprefixed Fullscreen API

        #### Ship the fullscreen top-layer rewrite

        Shared with layout-dev

        #### Concrete standards proposal and tests for event loop behavior

        <https://github.com/atotic/event-loop>
        *   Land interop tests for promise resolution behavior, file
                    bugs for failures in all browsers
        *   Specify that some events fire just before rAF
        *   Engage other vendors in discussion about which events should
                    be defined to be coupled to rAF, and to what extent their
                    order should be defined
        *   Write interop tests for rAF-coupled events

        #### At least bi-weekly triage of Hotlist-Interop issues

        Triage process here:
        <https://docs.google.com/document/d/1fDE3cFjQta0i7zx7JY1Icx0NNRS20HACz4uKb3Wo3hI/edit?pref=2&pli=1>

        #### STRETCH: Concrete shipping plan for unprefixed fullscreen in WebKit, EdgeHTML and Gecko

    ### Improve communication with web developers

        #### Own chromestatus.com and drive its improvement

        *   Take ownership of codebase, understand its structure
        *   Chromestatus is instrumented and we understand where people
                    are going on the site
        *   Meet with teams to discuss pain points and areas for
                    improvement for Chromestatus
        *   Best practices and guidance for using Chromestatus are
                    documented and visible \*on Chromestatus\*
        *   Meet with Mozilla, Microsoft, Opera to discuss merging all
                    status trackers

        #### Create and distribute a GCS / Endor survey to inform developer survey

        #### Reach consensus within Google on our developer feedback strategy

        go/webdevfeedback

        #### Create and distribute a comprehensive developer survey

        *   Buy in from all major parties on survey including primary
                    learnings we want
        *   First draft of survey completed
        *   Final draft of survey agreed upon by all major parties
        *   Survey distributed and gathering feedback

    ### Reduce footguns in the web platform

    Make it easier for web developers to reason about behavior of their
    applications, especially when they're composed of multiple 3rd-party
    components.
    *   **Initial implementation of Feature Policy (behind a flag)**
        *   Parse response header to construct policy
        *   Remove features based on policy
                    (document.{write,cookie,domain}, SyncXHR, WebRTC)
        *   Propagate removal to embedded frames
    *   **V1 Feature policy spec feature-complete**
        *   converge on v1 set of must-have features
        *   figure out all the spec plumbing and interop bits
        *   draft explainer, engage other browser vendors + developers
    *   **Integrate permission delegation with feature policy**
        *   Enforce API permissions based on parsed feature policy
