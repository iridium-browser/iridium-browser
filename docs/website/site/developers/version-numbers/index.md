---
breadcrumbs:
- - /developers
  - For Developers
page_name: version-numbers
title: Version Numbers
---

## Chromium

Chromium version numbers consist of 4 parts: MAJOR.MINOR.BUILD.PATCH.

*   MAJOR and MINOR **may** get updated with any significant Google
            Chrome release (Beta or Stable update). MAJOR **must** get updated
            for any backwards incompatible user data change (since this data
            survives updates).
*   BUILD **must** get updated whenever a release candidate is built
            from the current trunk (at least weekly for Dev channel release
            candidates). The BUILD number is an ever-increasing number
            representing a point in time of the Chromium trunk.
*   PATCH **must** get updated whenever a release candidate is built
            from the BUILD branch.

MAJOR and MINOR track updates to the Google Chrome stable channel. In this
sense, they reflect a scheduling or marketing decision rather than anything
about the code itself. These numbers are generally only significant for tracking
milestones. In the event that we get a significant release vehicle for Chromium
code **other** than Google Chrome, we can revisit the versioning scheme.

**The BUILD and PATCH numbers together are the canonical representation of what
code is in a given release.** The BUILD number is always increasing as the
source code trunk advances, so build 180 is always newer code than build 177.
The PATCH number is always increasing for a given BUILD. Developers and testers
generally refer to an instance of the product (Chromium or Google Chrome) as
BUILD.PATCH. It is the shortest unambiguous name for a build.

For example, the 154 branch was originally released as 0.3.154.9, but now stands
at 1.0.154.65. It's the same basic code with a lot of bug fixes applied. The
fact that it went from a Beta release to several 1.0 stable releases just
reflects the decision to call some version (1.0.154.36) 'out of Beta'.

## Chromium OS

Starting with the R16 release, we standardized on the following: \[Chrome
Version.\]&lt;TIP_BUILD&gt;.&lt;BRANCH_BUILD&gt;.&lt;BRANCH_BRANCH_BUILD&gt;

*   The Chromium version is implicit. On the R25 branch, Chromium OS
            will be using Chromium version 25.
*   TIP_BUILD will be updated every time the canaries run on the tip of
            tree (trunk/master in various VCS schemas). This is typically done
            every 6 hours. The number increases monotonically.
*   BRANCH_BUILD will be updated every time a new build is started on a
            branch. This happens whenever a commit is made to the branch.
*   BRANCH_BRANCH_BUILD is used when a branch of a branch is made. This
            is a bit uncommon, but has happened. It follows the same rules as
            BRANCH_BUILD.

An example Chromium OS version string:

*   Version 25.0.1364.126
    *   This is the Chromium version (see previous section for details)
*   Platform 3428.193.0 (Official Build) stable-channel stumpy
    *   This was branched from the tip of tree (ToT) when it was at
                3428.0.0. It is the 193rd build on that branch.

### Coordinating Versions

#### Chromium & Chromium OS

All releases of Chromium OS are tagged in the
[chromiumos/manifest-versions.git](https://chromium.googlesource.com/chromiumos/manifest-versions/+/HEAD/paladin/buildspecs/)
repository. You can look up a Chromium version by doing:

*   find the buildspec for the specific version you're interested in
    *   e.g. 5636.0.0-rc1.xml
*   look up the git sha1 for the chromiumos-overlay repo in it
    *   e.g. 4670dbea357617d1fc09db88829b71d8eb1f82e2
*   look at the chromeos-base/chromeos-chrome/ subdir of that repo at
            that revision
    *   e.g.
                [chromeos-base/chromeos-chrome](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/4670dbea357617d1fc09db88829b71d8eb1f82e2/chromeos-base/chromeos-chrome/)
                contains version 35.0.1891.2
    *   e.g. git ls-files 4670dbea357617d1fc09db88829b71d8eb1f82e2
                chromeos-base/chromeos-chrome/

#### Chrome & Chrome OS

You can use the online [CrOS-OmahaProxy](http://cros-omahaproxy.appspot.com/)
app engine to see what versions of Chrome shipped in which versions of Chrome
OS.

Alternatively, all releases of Chrome are tagged in the
[chromeos/manifest-versions.git](https://chrome-internal.googlesource.com/chromeos/manifest-versions/+/HEAD/buildspecs/)
repository. Follow the same steps as above w/Chromium.
