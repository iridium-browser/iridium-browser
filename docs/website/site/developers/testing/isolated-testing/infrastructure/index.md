---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
- - /developers/testing/isolated-testing
  - Isolated Testing
page_name: infrastructure
title: Isolated Testing Infrastructure
---

[TOC]

## This page is obsolete, need to be migrated to markdown (https://crbug.com/1246778)

## Objective

"*Build and test at scale*". The goal is to drastically reduce whole test cycle
time by scaling building and test sharding across multiple bots seamlessly. It
does so by integrating Swarming within the Try Server and the Continuous
Integration system.

This page is about the Chromium specific Swarming infrastructure. For the
general Swarming Design, see [its
documentation](https://chromium.googlesource.com/infra/luci/luci-py/+/HEAD/appengine/swarming/doc/Design.md).

## Background

*   The Chromium waterfall used to use completely manual test sharding.
            A "builder" compiles and creates a .zip of the build output. Then
            "testers" download the zip, checkout the sources, unpack the zip
            inside the source checkout and run a few tests.
*   While we can continue throwing more faster hardware at the problem,
            the fundamental issue remains; as tests gets larger and slower, the
            end-to-end test latency will continue to increase, slowing down
            developer productivity.
*   This is a natural extension of the Chromium Try Server (initiated
            and written by maruel@ in 2008) that scaled up through the years and
            the Commit Queue (initiated and written by maruel@ in 2011).
*   Before the [Try Server](/system/errors/NodeNotFound), team members
            were not testing on other platforms than the one they were
            developing on, causing constant breakage. This helped getting at 50
            commits/day.
*   Before the [Commit Queue](/developers/testing/commit-queue/design),
            the overhead of manually triggered proper tests on all the important
            configuration was becoming increasingly cumbersome. This could be
            automated and was done. This helped sustain 200 commits/day.

But these are not sufficient to scale the team velocity at hundreds of commits
per day. Big design flaws remain in the way the team is working. In particular,
to scale the Chromium team productivity, significant changes in the
infrastructure need to happen. In particular, the latency of the testing across
platforms need to be drastically reduced. That requires getting the test result
in O(1) time, independent of:

1.  Number of platforms to test on.
2.  Number of test executables.
3.  Number of test cases.
4.  Duration of each test cases, especially in the worse case.
5.  Size of the data required to run the test.
6.  Size of the checkout.

To achieve this, sharding a test must be a constant cost. This is what the
Swarming integration is about.

## Overview

Using Swarming works around [Buildbot](http://buildbot.net/)'s limitations and
permits sharding automatically and in an unlimited way. For example, it permits
sharding the test cases on a large smoke test across multiple bots to reduce the
latency of running it. Buildbot on the other hand requires manual configuration
to shard the tests and is not very efficient at large scale.

By reusing the Isolated testing effort, we're going to be able to shard
efficiently the Swarming bots. By integrating Swarming infrastructure inside
[Buildbot](http://buildbot.net/), we worked around the manual sharding that
buildbot requires.

To recapitulate the Isolated design doc, `isolateserver.py` is used to archive
all the run time dependencies of a unit tests on the "builder" to Isolate
Server. Since the content store is content-addressed by the SHA-1 of the
content, only new contents are archived. Then only the SHA-1 of the manifest
describing the whole dependency is sent to the Swarming bots, with an index of
the shards that it needs to run. That is, **40 bytes for the hash plus 2
integers** is all that is required to know what OS is needed and what files are
needed to run a shard of test cases along `run_isolated.py`.

## How the infrastructure works

For each buildbot worker using Swarming:

1.  Checks out sources.
2.  Compile
3.  Runs 'isolate tests'. This archives the builds on
            <https://isolateserver.appspot.com>.
4.  Triggers Swarming tasks.
5.  Runs anything that needs to run locally.
6.  Collects Swarming tasks results.

The Commit Queue uses Swarming indirectly via the Try Server.

So there is really 2 layers of control involved. The first being **the LUCI
scheduler** which controls the overall "build", which includes syncing the
sources, compiling, requesting the test to be run on Swarming and asking it to
report success or failure. The second layer is the **Swarming server** itself
which "micro-distribute" test shards. Each test shard is actually a subset of
the test cases for a single unit test executable. All the unit tests are run
concurrently. So for example for a Try Job that requests `base_unittests`,
`net_unittests`, `unit_tests` and `browser_tests` to be run, they are all run
simultaneously on different Swarming bot, and slow tests, like browser_tests,
are further sharded across multiple bots, all simultaneously.

## Diagrams

How the Try Server is using Swarming:

#### Chromium Try Server Swarming Infrastructure

How using Swarming directly looks like

#### Using Swarming directly

## Project information

*   This project is an integral part of the Chromium Continuous
            Integration infrastructure and the Chromium Try Server.
*   While this project will greatly improve the Chromium Commit Queue
            performance, it has no direct relationship and the performance
            improvement, while we're aiming for it, is totally a side-effect of
            the reduced Try Server testing latency.
*   Code: <https://chromium.googlesource.com/infra/luci/luci-py.git>.

### Appengine Servers

*   [chromium-swarm.appspot.com](https://chromium-swarm.appspot.com):
            manages task execution
*   [isolateserver.appspot.com](https://isolateserver.appspot.com/):
            build output cache

### Canary Setup

*   [chromium-swarm-dev.appspot.com](https://chromium-swarm-dev.appspot.com)
*   [isolateserver-dev.appspot.com](https://isolateserver-dev.appspot.com)

## Latency

This project is primarily aimed at reducing the overall latency from "ask for
green light signal for a CL" to getting the signal. The CL can be "not committed
yet" or "just committed", the former being the Try Server, the later the
Continuous Integration servers. The latency is reduced by enabling a higher of
parallel shard execution and removing the constant costs of syncing the sources
and zipping the test executables, both which are extremely slow, in the orders
of minutes.
Other latencies includes;

1.  Time to archive the dependencies to the Isolate Server.
2.  Time to trigger a Swarming run.
3.  Time for the workers to react to a Swarming run request.
4.  Time for the workers to fetch the dependencies, map them in a
            temporary directory.
5.  Time for the workers to cleanup the temporary directory and report
            back stdout/stderr to the Swarming master.
6.  Time for the Swaming master to react and return the information to
            the Swarming client running on the buildbot worker.

## Scalability

All servers run on AppEngine. It scales just fine.

## Redundancy and Reliability

There are multiple single points of failures

1.  The Isolate Server which is hosted on AppEngine.
2.  The Swarming master, which is also hosted on AppEngine.

The swarming bots are intrinsically redundant. The Isolate Server data store
isn't redundant or reliable, it can be rebuilt from sources if needed. If it
fails, it will block the infrastructure.

## Security Consideration

Since the whole infrastructure is visible from the internet, like this design
doc, proper DACL need to be used. Both the Swarming and the Isolate servers
require valid Google accounts. The credential verification is completely managed
by
[auth_service](https://github.com/luci/luci-py/tree/master/appengine/auth_service).

## Testing Plan

All the code (Swarming master, Isolate Server and swarming_client code) are
tested in canary before being rolled out to prod. See the Canary Setup above.

## FAQ

### Why not a faulty file system like FUSE?

Faulty file systems are inherently slow: every time a file is missing, the whole
process hangs, the FUSE adapter downloads the file synchronously, then the
process resume. Multiply 8000x; that's what browser_tests lists. With a
pre-loaded content-addressed file-system, all the files can be cached safely
locally and be downloaded simultaneously. The saving and speed improvement is
enormous.
