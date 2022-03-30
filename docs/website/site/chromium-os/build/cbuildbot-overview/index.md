---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/build
  - Chromium OS Build
page_name: cbuildbot-overview
title: Anatomy of a cbuildbot build
---

[TOC]

## Introduction

This page describes the basic steps involved in a cbuildbot build. It is
intended for people who are interested in learning more about how cbuildbot
works.

If you're just looking to use cbuildbot, please see our other documentation:

*   [Remote trybot](/chromium-os/build/using-remote-trybots)
*   [Local
            trybot](http://www.chromium.org/chromium-os/build/local-trybot-documentation)

<table>
<tr>

<td>## Basic Stages</td>

<td> <table></td>
<td> <tr></td>
<td><td>Report</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#Report">stdio</a></td></td>
<td><td> \[<a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#google-storage">Artifacts\[x86-generic\]: x86-generic-full/R36-5814.0.0-b13319</a>\]</td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>UploadTestArtifacts</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#UploadTestArtifacts">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>Archive</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#Archive">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>CPEExport</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#CPEExport">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>DebugSymbols</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#DebugSymbols">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>UploadPrebuilts</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#UploadPrebuilts">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>UnitTest</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#UnitTest">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>VMTest (attempt 1)</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#VMTest%20%28attempt%201%29">stdio</a></td></td>
<td><td> \[<a href="https://storage.cloud.google.com/chromeos-image-archive/x86-generic-full/R36-5814.0.0-b13319/vm%5ftest%5fresults%5f1">vm_test_results_1</a>\]</td></td>
<td><td> \[<a href="https://storage.cloud.google.com/chromeos-image-archive/x86-generic-full/R36-5814.0.0-b13319/vm%5ftest%5fresults%5f1.tgz">vm_test_results_1.tgz</a>\]</td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>ChromeSDK</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#ChromeSDK">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>BuildImage</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#BuildImage">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>BuildPackages</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#BuildPackages">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>SyncChrome</td> </td>
<td><td>tag 36.0.1968.0</td></td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#SyncChrome">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>SetupBoard</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#SetupBoard">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>Uprev</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#Uprev">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>InitSDK</td> </td>
<td><td>2014.05.01.051520</td></td>
<td><td>91</td></td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#InitSDK">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>ReportBuildStart</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#ReportBuildStart">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>Sync</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#Sync">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>CleanUp</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#CleanUp">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>cbuildbot</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#cbuildbot%2flogs%2fpreamble">preamble</a></td></td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#cbuildbot">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>Clear and Clone chromite</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#Clear%20and%20Clone%20chromite">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>Sync buildbot files</td> </td>
<td><td> <a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/cbuildbot-overview.1422040020867#update%5fscripts">stdio</a></td></td>
<td> </tr></td>
<td> <tr></td>
<td><td>x86-generic full</td> </td>
<td><td><a href="http://sites.google.com/a/chromium.org/dev/chromium-os/build/builders/x86-generic%20full/builds/13319">Build 13319</a></td></td>
<td><td>scheduler</td></td>
<td> </tr></td>
<td> </table></td>

<td>A cbuildbot run is composed of multiple stages, some of which run in parallel. The basic stages are shown in the diagram on the right (which should be read from the bottom to the top), and are are described and broken down into three categories (Build, Test, Archive) below. The exact stages depends on the type build config used, and may include newly added stages that are not described here.</td>

*   <td>Internal/House Keeping</td>
    *   <td>update scripts: Run by buildbot to bootstrap itself.</td>
    *   <td>Clear and Clone chromite: Run by buildbot to get a clean
                chromite repo (for cbuildbot).</td>
    *   <td>cbuildbot: Start running cbuildbot itself (which runs all
                other stages below).</td>
    *   <td>CleanUp: Tidy up the current Chromium OS checkout (cache
                files/previous builds/etc...).</td>
*   <td>Build</td>
    *   <td>InitSDK: Create or update the chroot</td>
    *   <td>Uprev: Update ebuilds to reflect changes that have been
                submitted by developers.</td>
    *   <td>SetupBoard: Create the board root (e.g. /build/lumpy), if
                needed.</td>
    *   <td>BuildPackages: Build all packages.</td>
    *   <td>BuildImage: Build all images. Also, if needed, create and
                upload update payloads, so that the HWTest stage can start as
                soon as possible </td>
*   <td>Tests</td>
    *   <td>AUTest: Run autoupdate tests on real hardware.</td>
    *   <td>ChromeSDK: Test that it is possible to compile Chrome using
                the Simple Chrome workflow.</td>
    *   <td>HWTest: Run a test suite on real hardware.</td>
    *   <td>SignerTest: Verify that the image passes various security
                checks and can be signed.</td>
    *   <td>VMTest: Run smoke tests inside a virtual machine. Also
                verifies gmerge in a VM.</td>
    *   <td>UnitTest: Run unit tests locally. Only works with x86 or
                amd64 targets.</td>
*   <td>Archive</td>
    *   <td>Archive: Build, archive, and upload various artifacts.</td>
    *   <td>DevInstallerPrebuilts (optional): Upload binary versions of
                packages so that users in developer mode can download them using
                the dev installer.</td>
    *   <td>UploadPrebuilts (optional): Upload prebuilt binaries to
                Google Storage to speed up developer builds.</td>
    *   <td>UploadTestArtifacts: Upload artifacts for testing.</td>
    *   <td>DebugSymbols: Generate & upload breakpad symbols for crash
                handling at runtime.</td>
    *   <td>CPEExport: Export <a href="http://cpe.mitre.org/">CPE</a>
                information for security analysis/tracking.</td>

</tr>
</table>

## Simplified Build Flow Diagram

The following diagram is a simplified view of a typical build:

<img alt="image"
src="https://lh5.googleusercontent.com/7PKaC1rNMv8lIPqjYsW4hQ-2a8xtem2_d4Xy3srOOGicXh9hOuTfxnghRU4zc4Cag9VwumMW0wqOfP82litgOCPXeVKmdHpPmnQdahWii4i8zeqZ6jnURSCY"
height=368px; width=608px;>

In the test/archive phase of cbuildbot, every stage we launch runs in parallel.
In the above example, there are 5 stages running in parallel (HWTest, UnitTest,
UploadPrebuilts, VMTest, Archive).

With the exception of the build stages, each step uploads its own artifacts.

## Build Overview

The following steps are run in order.

### InitSDK: Create or update the chroot

If the chroot does not exist, or we find the chroot is broken in any way (e.g.
we can’t enter the chroot with cros_sdk), we delete the chroot and replace it.
Otherwise, this step is basically a no-op.

### Uprev: Tell Portage about new changes

In this stage, we iterate through all of the stable cros-workon ebuilds and
check whether the git hash inside the ebuild is up to date. If not, we update
it. Similarly, we also check whether the developer has made changes to the
unstable ebuild.

### SetupBoard: Create the board root

Besides running the setup_board script, this step also sets up a shared user
password, if applicable.

### BuildPackages: Build all packages

This stage just runs the build_packages script. If PGO data is needed, then this
step will wait for the PGO data to be ready first.

### BuildImage: Build all images

The build image phase is called “BuildImage”, but it does more than build an
image -- it also builds and uploads autotest tarballs and payloads, as shown in
the diagram below.

<img alt="image"
src="https://lh4.googleusercontent.com/bFfBcLIYrsBaiggqU-51RFFdV3siCR8fUU9iV4idvo9qWFphco-w9VrYltJrmANob4wjLK0d4WNAoC8PYOQ3VStSKNY_LxKjwGadlxZfZ9RiAfutBGJrjWiF"
height=270px; width=362px;>

## Test Overview

All of the below tests are optional and run in parallel. Every builder will run
a subset of the tests below. (Some builders even run no tests.)

### AUTest: Run autoupdate tests on real hardware

Before launching autoupdate tests, this stage builds and uploads a tarball of
the necessary autoupdate artifacts. The actual launching of the test is
implemented using the logic from the HWTest stage.

### ChromeSDK: Verify that Chrome compiles using the Simple Chrome workflow

On canaries, we actually build Chrome twice -- once inside the chroot, for use
in the build, and once outside the chroot, in the ChromeSDK stage, to help
verify that the simple Chrome workflow continues to work.

### HWTest: Run a test suite on real hardware

This stage launches tests with a timeout. If the timeout is exceeded, we report
a warning or an error, depending on configuration.

### SignerTest: Run various security checks on the image

This stage just runs the security check script on the image.

### VMTest: Run smoke tests inside a virtual machine

Besides running smoke tests on the image, this stage also verifies that gmerge
can merge packages into a VM instance. Once the test results are built, they are
uploaded to Google Storage, along with symbolized crash reports.

### UnitTest: Run unit tests locally

This stage runs unit tests in parallel. If quick_unit is False, this is
accomplished by looking for ebuilds which have the “src_test” clause and merging
them with FEATURES=test. If quick_unit is True, we only run tests on ebuilds
that have been uprevved in the Uprev stage.

## Archive Overview

<img alt="image"
src="https://lh6.googleusercontent.com/QAEXNOc5XudQU0UJt2sfYPsd6ZWsBTaf97fRfefF-d0Z22u1KwWlhT8cTuv7LsJsU0nf9k_JVVyGI6_PYAFOjw-etihBKPH8W9uTvncVotQqweKyAgAbwr4A"
height=712px; width=574px;>

## Release Process

<img alt="image"
src="https://lh4.googleusercontent.com/VmWgu1dJR8ALlV3aR4iWjbRHW99u5vemd3eovZpHIa-ZyKKwQUIJi3nBYDajjGULVVT88RX5Xfjx_7xaHijnCQSKkuknr8AVheXJSSdWTt-HCVb8sDe_WUUmvA"
height=655px; width=700px;>

## Buildbot files

An edited list of the files under the buildbot subdirectory:

*   builderstage.py: Module containing the base class for the stages
            that a builder runs. All of the stages inherit from this base class,
            so logic that is common to all stages is placed here.
*   cbuildbot_commands.py: Module containing the various individual
            commands a builder can run. These functions basically just wrap
            RunCommand -- they provide a friendlier interface so that callers
            don’t need to build up the command line themselves. These commands
            are mostly called by cbuildbot_stages.py.
*   cbuildbot_config.py: Buildbot configuration for cbuildbot. Tells us
            what buildbots to run.
*   cbuildbot.py: Placeholder file to invoke bin/cbuildbot. See
            scripts/cbuildbot.py for the source code of cbuildbot.
*   cbuildbot_results.py: Classes for collecting results of build stages
            as they run. Each build stage will record its success/failure status
            in this singleton object by calling Results.Record(...). This data
            is later used to build a report, and to determine whether the build
            was successful.
*   cbuildbot_stages.py: Module containing the various stages that a
            builder runs. All of the stages discussed above are stored in this
            file.
*   constants.py: This module contains constants used by cbuildbot and
            related code.
*   lkgm_manager.py: This module implements the LKGMManager, which is
            used in both the commit queue and preflight queues. It helps the
            client track what the agents are doing, and helps the agents
            communicate their status back to the client. The client sends its
            manifest to the agents by committing it to git, and the agents
            communicate their status back to the client by uploading it to
            Google Storage.
*   manifest_version.py: This module implements the BuildSpecsManager,
            which is used by canaries. It is simpler than LKGMManager because
            there is no need for bidirectional client/agents communication --
            the workers just need to know the manifest to use and they are good
            to go.
*   portage_utilities.py: Routines and classes for working with Portage
            overlays and ebuilds. This library is used in several places,
            including during the Uprev stage.
*   remote_try.py: Class for launching remote tryjobs.
*   repository.py: Routines and classes for working with git
            repositories. This is useful, for example, for syncing the source
            code.
*   trybot_patch_pool.py: Classes for managing patches passed into a
            remote trybot run, and filtering them as appropriate.
*   validation_pool.py: This module implements the core logic of the
            commit queue.