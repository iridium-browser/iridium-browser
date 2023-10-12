---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: suite_scheduler-1
title: Suite Scheduler Design Doc
---

**Autotest Test Scheduler V2**

**We’ve recently added [dynamically-defined test suites](/chromium-os/testing/dynamic-test-suites) for Chrome OS, but can currently only run them synchronously from our build workers. That works great for a BVT suite, but is less than ideal for longer-running suites, suites whose results are not needed immediately, and suites that need to be run on a particular build on-demand.**

**# Requirements**

**The scheduler must be able to:**

*   **Run tests on a new full build.**
*   **Run tests only if chrome has changed on a new build.**
*   **Skip a suite if it has already run for a particular platform/build
            combo.**
*   **Suites do not need to be configured to run per-platform,
            per-milestone**
    *   **The scheduler will enumerate supported platforms by listing
                labels from the AFE that start with “board:”**
*   **Run a suite at a nightly/weekly interval.**
    *   **These runs should use the latest image available for the
                platform that is being run against.**
*   **Run one-shot for a particular build, scheduling all configured
            jobs for that event/build**
    *   **./test_scheduler.py -e &lt;list,of,events&gt; -i build**

**The scheduler should never wait for any of these jobs, but instead schedule them with the AFE and then return to waiting for other events to occur.**

**# Detailed Design**

**There are two conceptual classes of tasks in the scheduler: timed-event tasks and build-event tasks.**
**On each event, the scheduler will wake up and, for each known platform, ask the dev server for the name of the most-recent staged build. It will then run through all the tasks configured to run on this event and query the AFE to see if a job already exists for this (platform, build, suite) tuple. We’ll facilitate this by having a convention for naming jobs, e.g. stumpy-release/R19-1998.0.0-a1-b1135-test_suites/control.bvt for a nightly-triggered regression test of the stumpy build ‘R19-1998.0.0-a1-b1135’. If an appropriate job does not exist, the scheduler will fire-and-forget one.**
**This de-duping logic allows us to avoid a lot of complexity; we can simply throw tasks on the to-be-scheduled queue in the scheduler, and trust that they’ll get tossed out by this check if they would duplicate work.**
**As the suite definition files themselves currently handle sharding, we don’t need to take it into account here.**

**## Branches**

**We support a couple symbolic branch names:**

*   **factory**
*   **firmware**

**We believe that there’s only one of these that matters, per platform, at any given time.**
**We also support release branch cutoffs. If a Task can’t be run on branches earlier than R18, say, we allow you to specify ‘branch: &gt;=R18’ in your task config.**

**## Timed-event tasks**

**We’ll start with just ‘nightly’ and ‘weekly’. The time/day of these events can be configured at startup.**
**When a timer fires, the scheduler will enumerate the supported platforms by querying the AFE. Then, for each platform and branch, we can glob through Google Storage to find the latest build (this requires that we put factory/firmware builds in appropriately-named buckets in GS). Now, we can go through all triggered tasks and schedule their associated suites. This may include some already-run suites, but they will be thrown out by our de-duping logic.**

**## Build-event tasks**

**Initially, we’ll support two events: ‘new-build’, and ‘new-chrome’. The scheduler will poll git, looking for updates to the manifest-versions repo(s) and the chrome ebuilds.**
**Every time a new build completes, a manifest is published to the manifest-versions git repo (internal or external, depending on the build). By looking at the path that file is checked in at, we can determine what target it was built for, and what the name of the build is. By looking in the file a bit, we can determine what branch the build was on. With all that info, we can look through the configured tasks, and figure out which ones to schedule.**
**When we see that a Chrome rev has occurred, we can register each ‘new-chrome’-triggered task to run on the next new build for all supported platforms.**

**## Configuration file format**

**We will use python config files.**
**\[Regression\]**
**suite=regression**
**run_on=new-build**
**branch=&gt;=R18**
**\[NightlyPower\]**
**suite=power**
**run_on=nightly**
**pool=remote-power**
**branch=&gt;=R15, firmware**
**\[FAFT\]**
**suite=faft**
**run_on=new-build**
**branch=firmware**

**# Alternatives considered**

**## Buildbot**

**We considered using buildbot in a couple of different ways, but ultimately decided against it. The largest issue here is that the builders all live in the golo, and the lab does (and will) not. We currently cope with this for the sake of HWTest because we need to return results to the build infrastructure. We prefer to avoid adding further dependencies on the ability to talk from golo &lt;-&gt; corp unless it is strictly necessary to achieve desired functionality. It’s certainly not necessary to add this complexity from the point of view of correctly scheduling tests.**
**One proposed approach could have us using build workers that trigger when the real builders finish to handle event-triggered suites, and one that kicks off at the same time every day to run timer-based suites. Issues are these:**

1.  **We’d still need an extra tool to force the scheduling of suites
            for back-filling purposes.**
2.  **Configuration becomes more manual and is spread over the code
            base**
    1.  **When a new build appears, have to set up a bot to start
                watching it**
    2.  **When a branch build dies, have to change config to repurpose
                the bot**
    3.  **Config dictating which suites to run on which triggers lives
                separately from the code that decides which builds to watch and
                trigger on.**

**Another approach would have us setting up a worker for every suite/trigger/build combo. This has similar maintenance headaches as the above.**

**## Using Gerrit triggers in lieu of git triggers**

**This also would require communicating from two disconnected networks, so we
also decided against it.**
