---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
- - /developers/testing/commit-queue
  - Chromium Commit Queue
page_name: integration-with-rietveld
title: 'Design: 3-way Integration with Rietveld and the Try Server'
---

[TOC]

## Objective

Drastically reduce the *perceived time* (wall-clock time) from asking the
infrastructure to commit a change list (CL) to the time it is committed. This
reduces the time a developer has to care about a particular CL, by getting it in
or refusing it as fast as possible.

## Background

The Chromium team uses the [Commit
Queue](/developers/testing/commit-queue/design), which in turn uses the [Try
Server](/system/errors/NodeNotFound), to perform automated testing of their
pending CL on multiple platforms in multiple configurations.
[Rietveld](http://code.google.com/p/rietveld/) is used for code reviews and the
Chromium team uses a [slightly forked
version](http://code.google.com/p/rietveld/source/browse/?name=chromium) to
better integrate it with the rest of the Chromium infrastructure.

Before the integration, the Commit Queue is awaken as soon as the CL author, or
one of his reviewer, checks the infamous *Commit checkbox* on Rietveld. The
Commit Queue polls updates from Rietveld at a rate of 0.1hz to get new CLs to
test.

This means that pre-commit verifications, which include code styling, OWNERS
check, a proper LGTM from at least one Chromium Committer, properly testing the
patches on the Try Server, only starts at the moment the checkbox is set. This
means a maximal time is spent between the moment the checkbox is checked and the
change is committed.

Also, before the integration, because of the way try jobs are triggered, the
Commit Queue cannot trust the try jobs triggered by anyone but itself. The
"previous way" of triggering try jobs is to commit a raw diff into subversion.
Let's just say that it is *suboptimal*.

## Overview

This project is about implementing techniques in the integration so the Commit
Queue can efficiently run presubmit checks and Try Jobs on the Try Server before
the CL author request the commit to be merged into the source tree;

The Commit Queue is also trusting the try jobs triggered by others than itself.
The new way involves the Try Server polling Rietveld directly to fetch the diff
*directly* from the code review. This is the reason the try jobs can be
"trusted".

Rietveld acts as a datastore to map a Verification steps to a Patchset.

## Infrastructure

The infrastructure consist of:

1.  Rietveld is running on AppEngine, using AppEngine's supplied GAIA
            account management.
    1.  It becomes the primary database for the try job results.
2.  The Commit Queue process, which is a stand alone process.
3.  The Try Server, which is an infrastructure itself.

## Detailed Design

### 1. Trusting try jobs

The first task is to make sure Try Jobs results on Rietveld can be trusted. This
is done by differentiating try jobs triggered directly from Rietveld from any
other try jobs. The polling code is in
[try_job_rietveld.py](http://src.chromium.org/viewvc/chrome/trunk/tools/build/scripts/master/try_job_rietveld.py?view=markup)
and sets the build property `try_job_key` that is then used to certify this try
jobs was indeed triggered from Rietveld in
[view_chromium.py](http://code.google.com/p/rietveld/source/browse/codereview/views_chromium.py?name=chromium).
The json packets are pushed via Buildbot's [HttpStatusPush
implementation](http://src.chromium.org/viewvc/chrome/trunk/tools/build/third_party/buildbot_8_4p1/buildbot/status/status_push.py?view=markup)
from the Try Server back into Rietveld's /status_listener URL as HTTP POST
requests.

### 2. Polling for pending CLs

Rietveld already has a mechanism to search for issues with specific criteria.
For example,

<https://codereview.chromium.org/search?closed=3&commit=3&modified_after=2012-09-18&format=json&limit=10>

While polling it really not optimal, it permits reusing the current API. The
`/search` interface supports specifying an opaque datastore `cursor` so only new
issues are returned. The cursor to use on the next request is returned inside
the json data.

### 3. Reacting to CL modifications

For each new CL modified, the CQ must decide if it reacts to the CL. To enable
preemptive CL verification, the following steps are used on each new CL;

1.  Determine if the CL author is a trusted author, e.g. a [Chromium
            Committer](/getting-involved/become-a-committer).
    1.  If it is and the user asked for review, initiate checks.
    2.  If not, wait for a LGTM from a trusted reviewer. Note that it's
                not waiting for the Commit checkbox to be checked.
2.  Keep track of the previous checks for the last patchset, discarding
            data as new patchsets are uploaded.
3.  Add a note on the issue if a check failure occurs.

Then for commit time;

1.  Upon Commit checkbox signal, only re-run the Secure Checks and
            commit.
    1.  Secure checks include things that must not change; the CL
                description must have stayed the same, the tree is open, etc.

## Project Information

This project is driven primary by rogerta@ and maruel@. It benefits immediately
from Try Server enhancement projects like the [Swarm integration
project](/system/errors/NodeNotFound) but it is still designed to work without
Try Server performance boost and work around its general sluggishness.

## Caveats

1.  Presubmit checks are not yet run on the Try Server but are run
            locally instead. This needs to be changed to accelerate the CQ and
            keep its integrity.
2.  This significantly increases the Try Server utilization. Without the
            [Swarm integration project](/system/errors/NodeNotFound), this would
            likely overload the Try Server.

## Latency

The whole purpose of this project is to reduce the latency of the pre-commit
checks. The latency will boils down to

1.  Poll rate.
2.  Pre-commit forced checks.

At that point, we are not aiming at optimizing the commit delay below 30
seconds, lower than that is a nice to have.

## Scalability

The Commit Queue is a single threaded process. As such, it is bound by its
slowest operations. This currently include the PRESUBMIT checks and
applying/commiting CLs. The goal is to make checks happen earlier and reuse the
results from previous checks whenever they are trusted, not to make the checks
faster.

## Redundancy and Reliability

There are multiple single points of failure;

1.  Rietveld on AppEngine. If AppEngine blows up, we're screwed.
2.  The Try Server, which is a single-threaded process. Restarting it,
            in case of hard failure, is only lossy for the jobs in progress at
            the time of shutdown. In case of catastrophic failure, all the
            previous try jobs would need to be re-run. This already happened in
            the past and can be handled relatively fast, within an hour or so.
3.  The Commit Queue process itself is also single-threaded and
            single-homed. It is designed to be mostly lossless when restarted or
            if an exception occurs. The Try Jobs database can be reconstructed
            from the information stored on Rietveld.

## Security Consideration

The security is using the following assumptions;

1.  Rietveld administrators are not rogue.
2.  Rietveld does not permit a CL author to fake its identity and the
            patchsets, once created, are immutable.
    1.  The commit queue ensures the CL description is not modified
                between the moment it is running its presubmit check and the
                moment it is committing.
3.  The Commit Queue is doing proper verification w.r.t. which is a
            trusted committer.
    1.  In the end, the Commit Queue process is the one deciding if a
                commit gets in or not and doing it.
4.  The committer are trusted. They already can commit anything they
            want. It is an non-goal to restrict what committers can commit.

## Testing Plan

The integration itself, being an integration project, is mostly tested directly
"on prod". While there are a few unit tests to check basic functionality, most
of the bugs are at integration points. This being a productivity tool for
developers, it is expected that they have some clue about what's going wrong.
