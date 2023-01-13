---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: verify-prebuilts-using-content-hashing
title: 'Design proposal: Verify prebuilts using content hashing'
---

# Verify prebuilts using content hashing

Proposal status: Implemented and in production

Author: David James &lt;davidjames@&gt;

## Summary

I'd like to propose a small change to the way we verify prebuilts. This change
is intended to ensure that developers do not need to build packages from source
if they have not modified them locally. This should shave significant time off
developer builds (e.g., up to 20 minutes, if the developer syncs at a busy
time).

## What needs to be changed?

Right now, we only use prebuilts for builds if the SHA1 of the commit that we
tested exactly matches the SHA1 in the remote repository. This is needed for
correctness, as explained below in the “Why is prebuilt verification needed at
all?” section.

Unfortunately, in practice, the SHA1 of the commit we tested often differs from
the SHA1 in the remote repository. This happens because when the commit queue
submits a change, Gerrit adds in new metadata to the commit message, causing the
prebuilts to become invalid.

Currently, the commit queue accounts for this problem by just rebuilding any
packages with incorrect SHA1s on the next run. This doesn’t help, though, if
developers sync during the day, when many changes are being pushed by the commit
queue. In that case, this problem causes developer builds to slow down
significantly, as they need to rebuild many packages from source.

## Proposed solution

We should use the SHA1 of the contents of the source tree to validate the
prebuilts. Unlike the commit hash, this SHA1 is unaffected by the history of the
repository, or by commit messages. This ensures that users always have correct
prebuilts, without causing unnecessary rebuilds.

### Implementation

1.  First, add the SHA1 of the contents of the source tree into the
            ebuild as `CROS_WORKON_TREE`. This can be calculated instantly using
            the following command:
    `git log -1 --format=%T`
2.  Next, add `CROS_WORKON_TREE_$CROS_WORKON_TREE` to `IUSE` in
            `cros-workon.eclass`.
3.  Finally, remove `CROS_WORKON_COMMIT_$CROS_WORKON_COMMIT` from `IUSE`
            from `cros-workon.eclass`.

## Why is prebuilt verification needed at all?

Besides the above proposal, another option for speeding up builds would be to
eliminate verification of SHA1s in ebuilds entirely. This would speed up builds,
but would open up developers to race conditions where they might have incorrect
builds.

Example: If one developer sends a change through the commit queue, and another
developer bypasses the commit queue at the same time, the ebuild will be marked
with the latest SHA1 (including both change), but the prebuilt may only include
the first change. This results in developers (and bots that use binaries) never
picking up the ‘chump’ change.

There are also other race conditions. Example:

1.  A chump change is pushed to power_manager.
2.  The incremental builder kicks off.
3.  Another chump change is pushed to power_manager.
4.  The commit queue kicks off and marks both changes as stable.
5.  The incremental builder kicks off again, and doesn’t notice any
            changes, because its local prebuilt has the same version number as
            the one that was pushed by the commit queue.

Based on the above problems, I think that we should not eliminate prebuilt
verification of SHA1s, and instead use the proposed solution above.
