---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/build
  - Chromium OS Build
page_name: builder-overview
title: Builder Overview
---

[TOC]

All Chrome/Chromium OS waterfalls are accessible internally from
[uberchromegw.corp.google.com](http://uberchromegw.corp.google.com).

On the waterfall, each column represents a builder. Each builder has a waterfall
name (e.g. "pineview canary") runs an associated cbuildbot config (e.g.
`pineview-release-group`). The waterfall name and the config name are often
different (except on the [try
waterfall](https://uberchromegw.corp.google.com/i/chromiumos.tryserver/waterfall));
however, they are often easily associable.

All cbuildbot configs are defined in `chromite/cbuildbot/cbuildbot_config.py`,
and the generated JSON file containing all configs is viewable at
`chromite/cbuildbot/config_dump.json`.

This document explains the builder categories on the Top of Tree (ToT)
waterfalls.

### CQ

*1. What's the purpose of the builders?*

Commit Queue (CQ) picks up, builds, tests, and submits and/or rejects Chrome OS
CLs. The overview of CQ written for developers is available
[here](/system/errors/NodeNotFound).

*2. What are the related builders?*

[master-paladin](https://uberchromegw.corp.google.com/i/chromeos/waterfall?builder=master-paladin)
and all builders with their waterfall name ending with ***paladin***.

*3. How do the builders work?*

CQ adopts a master-worker model to verify the CLs on multiple platforms
simultaneously. The
[master-paladin](https://uberchromegw.corp.google.com/i/chromeos/waterfall?builder=master-paladin)
builder queries Gerrit for [eligible CLs](/system/errors/NodeNotFound),
cherry-picks them, and creates a manifest containing the commit hash of each
repository and the Gerrit CLs to apply on top of them. The newly created
manifest then triggers the worker builders to build and test the CLs on a set of
representative boards. CQ master periodically checks the statuses of the workers
until all the workers complete or the preset timeout hits, whichever occurs
first. After collecting all the build results, CQ master examines them and
submits/rejects CLs where applicable. CQ master also sends out alert emails when
necessary.

Note that CQ workers use Chrome binaries/prebuilts published by Chrome PFQ; they
do not build Chrome from scratch.

*4. How to triage failures?*

It is normal that CQ fails due to bad CLs in the run. If CQ fails several times
consecutively, it means that there could be a bad CL stuck in the CQ, a bug in
the ToT, or there could be infrastructure issues. Tree sheriff should handle the
initial triage. In the case where infrastructure failure is suspected, build
deputy and/or Lab sheriff is the right person to contact.

### Pre-CQ

*1. What's the purpose of the builders?*

Due to correctness and performance issues, CQ batches multiple CLs in a run.
This lowers isolation of CLs, i.e., good CLs will be affected (not be submitted
or even falsely rejected) by the bad CLs in the same run. To alleviate this
effect, every CL has to be vetted by Pre-CQ to be qualified for CQ pickup.
Pre-CQ builds and tests an isolated set of CLs from a developer. Unlike CQ,
Pre-CQ has lower test coverage due to resource/time constraints.

*2. What are the related builders?*

[pre-cq-launcher](https://uberchromegw.corp.google.com/i/chromeos/waterfall?builder=pre-cq-launcher)
and the builders on the [try
waterfall](https://uberchromegw.corp.google.com/i/chromiumos.tryserver/waterfall)
with name/config ending with ***-pre-cq**.*

*3. How do the builders work?*

Pre-CQ uses a loose master-worker model, where the master build is not tied to a
specific worker build.
[pre-cq-launcher](https://uberchromegw.corp.google.com/i/chromeos/waterfall?builder=pre-cq-launcher)
(i.e., the master) constantly examines the the statuses of Gerrit CLs and
launches remote trybots to verify each set of the CLs. If all trybots passed and
the CL has all required approvals, Pre-CQ launcher considered the CL verified.

*4. How to triage failures?*

Pre-CQ launcher should never fail for reasons other than infrastructure issues.
Build deputy should handle the failure triage.

### Canaries

*1. What's the purpose of the builders?*

Canary builders build, tests, and generate release artifacts for all boards
several times a day on a preset schedule.

*2. What are the related builders?*

[master-release](https://uberchromegw.corp.google.com/i/chromeos/waterfall?builder=master-release)
and all builders with waterfall name ending with ***canary***.

*3. How do the builders work?*

Canaries adopts a master-worker model.
[master-release](https://uberchromegw.corp.google.com/i/chromeos/waterfall?builder=master-release)
creates and publishes a manifest from the Top of Tree. The workers are then
triggered by the new manifest to start a new build. During which time, the
canary master periodically checks the statuses of the workers until all the
workers complete or the preset timeout hits, whichever occurs first. Finally,
canary master builder throttles the tree and/or sends out alert emails if there
are any worker failures.

*4. How to triage failures?*

For infrastructure issues, please contact build deputy or lab sheriff.
Non-infrastructure issues indicates bug in ToT; tree sheriff should triage the
failure and revert if necessary.

### Chrome PFQ

*1. What's the purpose of the builders?*

Chrome PFQ builds and tests the latest Chrome. On a successful run, it uprevs
the `chromeos-chrome` ebuild and publishes the new Chrome prebuilts. It is
essentially a CQ dedicated for Chrome.

*2. What are the related builders?*

[master-chromium-pfq](https://uberchromegw.corp.google.com/i/chromeos/waterfall?builder=master-chromium-pfq)
and all builders with waterfall name ending with ***chrome/chromium PFQ***.

*3. How do the builders work?*

Chrome PFQ adopts a master-worker model.
[master-chromium-pfq](https://uberchromegw.corp.google.com/i/chromeos/waterfall?builder=master-chromium-pfq)
creates a new manifest containing the Chrome version to build. The new manifest
triggers the worker builds and the master wait until all workers complete or the
timeout hits, whichever occurs first. If all workers passed, the Chrome PFQ
master uprevs the Chrome ebuild.

*4. How to triage failures?*

Tree sheriff should work with Chrome Gardener to resolve any Chrome issues.
Build deputy and/or lab sheriff should be roped in on infrastructure issues.

### Continuous

*1. What's the purpose of the builders?*

Continuous builders continuously builds and test the ToT code.

*2. What are the related builders?*

All continuous builders are standalone builders.

*3. How do the builders work?*

On failure, continuous builders throttle the tree.

*4. How to triage failures?*

For infrastructure issues, please contact build deputy or lab sheriff.
Non-infrastructure issues indicates bug in ToT; tree sheriff should triage the
failure and revert changes if necessary.

### ASAN

ASan builders build with [Address
Sanitizer](http://www.chromium.org/developers/testing/addresssanitizer), leading
to bigger binaries. ASan builders are [continuous
builders](http://www.chromium.org/chromium-os/build/builder-overview#TOC-Continuous).

### ChromiumOS SDK

ChromiumOS SDK builder builds the SDK and all the cross-compilers; it is a
[continuous
builder](http://www.chromium.org/chromium-os/build/builder-overview#TOC-Continuous).