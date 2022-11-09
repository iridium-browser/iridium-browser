---
breadcrumbs:
- - /issue-tracking
  - Issue Tracking
page_name: release-block-guidelines
title: Release Block Guidelines
---

**See also [this more recent document](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/process/release_blockers.md).**

**What are the intents of ReleaseBlock?**

ReleaseBlock issues are immediately gating the release of the branch (Dev, Beta,
Stable). These issues require the highest priority of attention from the
engineers assigned (since they are directly on the critical path). They are
meant to be used as a tool for program managers to focus engineers to get a
Chrome release launched with high standard of quality (e.g. absent critical
regressions, stability, build issues, etc...).

**What aren’t the intents?**

ReleaseBlock issues **are not** meant to be a scoping or prioritization tool (we
have specific labels and normal processes for each of those, respectively
mstone- and pri-). If an issue isn’t getting sufficient attention, it should be
escalated to a TPM and/or the priority adjusted accordingly. Making arbitrary
issues ReleaseBlockers, when mstone and proper prioritization would otherwise
suffice, makes it very hard to see the true critical path of our release cycle.

**Best Practices:**

*   Nominate blockers liberally, it’s better to catch someone’s
            attention than to ignore an issue.
    *   That said, expect TPMs to say no sometimes.
*   Ask yourself if you would really block a release for this issue, if
            the answer if no, the next best option is to make sure the priority
            is set correctly, the issue is assigned, and an mstone label is set.
*   All ReleaseBlock bugs should have owners
*   If you have a request for an out of band (i.e. non-releaseblock)
            merge, add a Merge-Requested flag and the TPM responsible for the
            branch will take a look.

**Signals of Poor Practices:**

*   Assigned a P2/P3 issue as a ReleaseBlocker
    *   ***Explanation:***
        *   P2/P3 are generally considered nice to have merges
*   Setting a ReleaseBlock label 6 weeks in advance of a branch point.
    *   ***Explanation:***
        *   It suggests that normal processes for prioritization/
                    scoping aren’t being followed. In cases where issues are not
                    imminently blocking a release (i.e. 3-6 weeks in advance),
                    such practices dilute the intent of a releaseblock, which
                    literally means, drop what you are doing and work on nothing
                    else. Such calls may be overly disruptive to engineers, and
                    may not be the correct priority call if the issue in
                    question does not need immediate attention.
*   Setting a ReleaseBlock issue w/ out an owner or marking it as
            Available
    *   ***Explanation:***
        *   If an issue is really critically urgent it should be
                    assigned right away, we should not wait around for normal
                    triage to determine an owner (i.e. we should not be sitting
                    on these issues).
