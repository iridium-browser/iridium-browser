---
breadcrumbs:
- - /issue-tracking
  - Issue Tracking
page_name: autotriage
title: 'Sheriffbot: Auto-triage Rules for Bugs'
---

[TOC]

Bug reporting, triage and fixing are essential parts of the software development
process. In large software projects such as Chromium, the bug tracking system is
the central hub for coordination that collects informal comments about bug
reports and development issues. Hundreds of bug reports are submitted every day,
and the triage process is often error prone, tedious, and time consuming. In
order to make triage more efficient, we automate certain tasks.

Googlers can view sheriffbot's source at
[go/sheriffbot-source](https://goto.google.com/sheriffbot-source). Please file
issues with Sheriffbot
[here](https://bugs.chromium.org/p/chromium/issues/entry?components=Tools%3EStability%3ESheriffbot).

### Unconfirmed bugs which are not updated for a year

*   Purpose: Bugs often slip through the cracks. If they've gone
             unconfirmed for over a year, it is unlikely that any progress
             will be made.
*   Action: Set the status to **Archived**.

### Available bugs which have no associated component and have not been modified for a year

*   Purpose: Clear out bug backlog by archiving bugs which are
            unlikely to receive attention and let reporters know that they
            should re-file if they still care about the issue.
*   Action: Add a comment and change the status to **Archived**.

### Available bugs with an associated component and are not modified for a year

*   Purpose: Recirculate stale bugs in case they were mistriaged
            initially.
*   Action: Add a comment, apply the tracking label
            **Hotlist-Recharge-Cold** and change the status to
            **Untriaged**. The **Hotlist-Recharge-Cold** label is applied
            for tracking purposes, and should not be removed after
            re-triaging the issue.

### Launch bug for experimental features - update and clean up

*   Launch bugs that are at least 3 milestones older than current
            stable:
    *   Purpose: In this case, we assume that the launch bug has
                    been forgotten and that the feature has already shipped.
    *   Action: Add a warning message asking for review. If no
                action is taken after 30 days, set the status to
                **Archived**.
*   Launch bugs with no milestone and are not modified in the last
            90 days:
    *   Purpose: Launch bugs should always be targeted for a
                specific milestone, so something has gone wrong if a bug is
                in this state.
    *   Action: Add a comment requesting that an appropriate
                milestone is applied to the issue.

### Needs-Feedback bugs, which received feedback.

*   Purpose: Developers often request feedback, but don't
            follow-up after it is received.
*   Action: If a reporter has provided feedback, remove the
            **Needs-Feedback** label, and add the feedback requester to the
            bug cc'list.

### Needs-Feedback bugs where feedback is not received for more than 30 days

*   Purpose: If required feedback is not received, it is unlikely
            that any progress can be made on the issue.
*   Action: Add a comment and set the status to **Archived**.

### P0 bug reminders

*   Purpose: P0 bugs are critical bugs which require immediate
            action.
*   Action: If the bug is open for more than 3 days, we leave a
            comment as a reminder. Note that a maximum of 2 nags will be
            applied to the issue.

### TE-Verified bugs which are not modified for 90 days *(This rule has been removed as of 08/04/16. Reference bug: 630626)*

*   Purpose: Developers often forget to close bugs out after they've
             been verified.
*   Action: Set the status to **Archived**.

### Medium+ severity security bug nags on issues not updated by their owners in the last 14 days

*   Purpose: Security vulnerabilities are serious threats to our
            users that require developer attention.
*   Action: Leave a comment asking for a status update. Note that a
            maximum of 2 nags will be applied to the issue.

### Merge-Approval clean up

*   Purpose: Developers often forget to remove **Merge-Approved-MX**
            label after merging change to specific branch.
*   Action: If the bug is not updated in specific milestone branch
            cycle (42 days), remove all label that starts with
            **Merge-Approved-MX** label and add a comment asking developers
            to re-request a merge if needed.

### Milestone punting

*   Purpose: Update issues with latest milestone after branch point.
*   Action: Replace milestone label with latest milestone for
            milestones that have passed their branch point. Certain issues,
            such as Launch bugs, Pri-0 issues, Release blockers, and issues
            where merges have been requested or approved are excluded.

### Open bugs with CL landed which are not modified for 6 months *(This rule has been disabled as of 07/22/16, pending furthur discussion. Reference bug: 629092)*

*   Purpose: Developers often forget to close the bugs after landing
            CLs.
*   Action:
    *   Send a reminder by adding **Hotlist-OpenBugWithCL** label asking Developers to either:
        *   Close bug if all neccessary CLs have landed and no furthur work is needed OR
        *   Remove **Hotlist-OpenBugWithCL** label if more changes are needed.
    *   If no action is taken in 30 days, set the status to **Archived**.

### Bugs with no stars, no owners and not modified for an year *(This rule has been disabled as of 08/12/16. Reference bug: 637278)*

*   Purpose: Issues with no owners, no stars are unlikely to receive
            any attention, so clear-out these issues and let reporters know
            that they should re-file if they still care about the issue.
*   Action: Add a comment and change the status to **Archived.**

### Set for re-triage, the issues with bouncing owners.

*   Purpose: Issues with owners who cannot be contacted (bouncing
            owners) is unlikely to get resolved by the owner, so clear out
            these issues by sending for re-triaging again.
*   Action: Remove the bounced owner, add comment, apply the
            tracking label **Hotlist-Recharge-BouncingOwner** and change the
            status to **Untriaged**.

### Cleanup any auto-generated issues with no owners and are inactive for more than 90 days.

*   Purpose: Auto-generated issues with no owners and more than 90
            days of inactivity is unlikely to be viewed and resolved, so
            clean up these issues by archiving them.
*   Action: Add comment and set the status to **Archived**.

### Reminder for release blocking issues with no milestone and OS labels.

*   Purpose: All release blocking bugs should have milestone and OS
            label associated with it in order to release the fixes promptly.
*   Action: Leave a comment asking to add milestone and OS
            labels and cc the release block label adder to the issue. Note
            that a maximum of 2 nags will be applied to the issue.

### Add label (Merge-TBD) to track any blocking issues where a merge may be required

*   Purpose: We need to ensure all blockers that are fixed
            post-branch have the relevant CLs merged back to the release
            branch; otherwise, regressions will escape to stable.
*   Action: Add the label "Merge-TBD" when issues marked as
                blocking (ReleaseBlock=Dev,Beta,Stable) targeting a milestone
                (via M-XX or Target-XX) that is past branch point (via
                [schedule](https://chromiumdash.appspot.com/schedule)) are
                marked as fixed.

### Increase priority of issues tagged with ReleaseBlock labels

*   Release Blockers will be escalated to P0 if they
    *   are tagged to block Beta or Stable releases within 10 days
        of the target milestone's promotion date.
    *   are tagged to block a Dev release for any milestone.
*   Release Blockers will be escalated to P1 if they
    *   are tagged to block Beta or Stable releases prior to 10 days
        of the target milestone's promotion date.
