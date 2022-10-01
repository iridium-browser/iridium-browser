---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: security-bug-lifecycle
title: Security Bug Lifecycle
---

The [canonical version of this document is now in the Chromium source
tree](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/security/sheriff.md).

The following document describes the process of working from an inbound security
bug report to releasing a fix on all affected branches. You can also refer to
the [Security Cheat Sheets](/system/errors/NodeNotFound) to provide a quick walk
through or as a reference to check your progress.

[TOC]

## Triage

The first step of triage is confirming that a bug does exist and it does
represent a vulnerability. Most reporters provide a version in the bug report,
but you’ll generally need to verify against the current stable, beta, and trunk
builds. The [Reporting Security
Bugs](/Home/chromium-security/reporting-security-bugs) page provides information
on what detail is required and how to get it, so you should use that as a guide
to fill in anything the report is missing. If you don’t have a particular build
on hand to perform the analysis against, you can leave a comment in the bug
requesting that another team member assist in verifying it. The list of
currently open security (modulo **Security-Severity-None**) bugs can be viewed
with [this
query](https://code.google.com/p/chromium/issues/list?can=2&q=Type%3DBug-Security+-Security_Severity%3DNone&sort=-secseverity&groupby=&colspec=ID+Pri+Area+Feature+Status+Summary+Modified+Owner+Mstone+OS+Secseverity+Reporter+Secimpacts).

### Security Features & Non-Vulnerabilities

There are a number of bugs we want to keep track of for security reasons, but
shouldn't track as vulnerabilities. This includes new security features and
improvements, or changes that simply have a security impact or warrant a closer
inspection from a security perspective. In this case the bug should *not* be
filed as *Type-Security*. Instead it should be filed as whatever type is
relevant (generally **Type-Bug**) and also have the **Feature-Security** label
set. This ensures the security team will still receive updates, but keeps it out
of the list of open vulnerabilities.

### Assigning Labels

**==First verify the bug, requirements to trigger, and affected platforms==**;
if you cannot verify the bug directly (e.g. nonavailability of the affected
platform) then assign or CC an owner who can. Once you’ve verified the bug, you
need to determine the appropriate labels. You should follow the severity
guidelines [Severity Guidelines for Security
Issues](/developers/severity-guidelines) to determine the rating for the
**Security-Severity-**\* label. Remember to also consider any mitigating factors
that might reduce the severity, such as unusual or excessive interaction, or the
feasibility of reliably triggering the condition in the wild (e.g. race
conditions are notoriously difficult).

Once you’ve determined the severity you can assign the milestone and priority
according to the following guidelines:

*   **Security-Severity-Critical**
    *   **Pri-0**
    *   Current stable milestone (or earliest milestone impacted)
*   **Security-Severity-High**
    *   **Pri-1**
    *   Current stable milestone (or earliest milestone impacted)
*   **Security-Severity-Medium**
    *   **Pri-2**
    *   Next stable milestone
*   **Security-Severity-Low**
    *   **Pri-3**
    *   Next stable milestone
*   **Security-Severity-None**
    *   No priority required
    *   No milestone required

Set the SecImpacts flags to identify the affected branches, so we know what
needs to be merged and have historical tracking data:

*   **SecImpacts-Stable** - Affects shipping branch and should be
            considered for merge.
*   **SecImpacts-Beta** - Affects beta branch and fix and should be
            considered for merge.
*   **SecImpacts-None** - Doesn't affect a shipping branch (no merge
            required).

The milestone targets may shift slightly to adjust for a release schedule. For
example, when the current stable is at the end of its cycle you will generally
bump all open bugs to target the next stable milestone.

Also, it's important to ensure that all security bugs are set to
**Type-Security** and any security bugs with a severity other than
**Security-Severity-None** include the **Restrict-View-SecurityTeam** flag.

The remaining triage follows the [standard
guidelines](/getting-involved/bug-triage). Key points to remember are the
following:

*   Update the status to **Untriaged**, **Available**, **Assigned**, or
            **Started** (as appropriate).
*   Apply the following labels as needed: **OS-**\*, **Feature-**\*,
            **Area-**\*, **Internals-**\*, **Crash**.
*   Any changes that will be merged to the beta branch must be labeled
            as **ReleaseBlock-Stable**.
*   Ensure the comment adequately explains any status changes
            *(severity, milestone, and priority assignment generally require
            explanatory text)*.

### Upstreaming WebKit Bugs

Any WebKit bugs should be upstreamed to
[bugs.webkit.org](http://bugs.webkit.org/) immediately after verification, basic
analysis to isolate the issue, and reducing any test cases to a manageable size.
When upstreaming WebKit security bugs, remember the following points:

*   File against the security component
*   Include a link back to the Chromium bug in your comment
*   Include a credit to the original reporter if appropriate
*   CC any Chromium Team members working on the bug downstream

#### Getting Access to WebKit Security Bugs

Any members of the Chromium Security Team involved in bug triage will need to
join the [WebKit Security Group](http://www.webkit.org/security/). If you're not
already on WebKit Security, please ask another Chromium Security Team member to
nominate you. After joining you'll need to request security bug access (this can
be done by replying to your confirmation email). Once bug access is granted,
you'll need to configure your account to get emailed on all inbound security
bugs. You do this by adding **webkit-security-unassigned@lists.webkit.org** to
the **User Watching** section on the [Email
Preferences](https://bugs.webkit.org/userprefs.cgi?tab=email) tab on WebKit's
bugzilla. (You'll probably want to create a Gmail filter on these messages as
well.)

**Landing WebKit Patches**

To contribute a patch to WebKit you can generally just follow the [instructions
on webkit.org](http://www.webkit.org/coding/contributing.html). However, one
additional step must be taken to land patches for security bugs, because the
WebKit Commit Bot and the WebKit Review Bot are not included in the WebKit
Security Group. This means your patch will not be placed in the commit queue
after receiving cq+ unless the bots are explicitly given access to the bug. To
do this, add these addresses to the cc: list of the WebKit bug:

*   webkit.review.bot@gmail.com
*   commit-queue@webkit.org

### Timeline

As a rule, you’ll want to ensure that the full initial triage is performed and
the bug is updated within 48 hours of the initial report. During business hours
the basic triage and bug update should occur within four hours at most.

### Erroneous Reports

Some reports are obviously not legitimate, or represent behavior that we do not
intend to change. In those cases you should mark the bug as **WontFix** or
**Invalid** (as appropriate) and provide a simple explanation to the reporter.
For non-security bugs erroneously reported via the security template, you should
remove all security labels, set the “Crash” label if appropriate, add in the
appropriate area/feature labels, and set the status to “Untriaged.”

## Shepherding

The shepherding process starts with finding an owner for a bug. If you have
access to security bugs, you can find a list of all open security bugs without
an owner
[here](http://code.google.com/p/chromium/issues/list?can=2&q=Type%3DBug-Security+-Security_SecSeverity%3DNone+-has%3Aowner&sort=-security_secseverity&colspec=ID+Pri+Area+Feature+Status+Summary+Modified+Owner+Mstone+OS+Secseverity&x=mstone&y=area&cells=tiles).
When analysing a bug for triage you’ll often find that you can work out a
solution yourself. If that’s the case and you have time to develop a patch, then
this is usually the most expedient solution to getting a fix out.

For bugs that you can’t fix yourself, you’ll need to find an owner. If you don’t
know who to assign a bug to, it’s often best to start by asking other members of
the security team if they’re familiar with the code. Then they can either take
the ownership of the bug, or point you to a developer who would make a good
owner.

If no one on the security team has a good suggestion, you’ll generally have to
use svn blame or some other means to track back one of the original developers
of the code. If you don’t have a checkout handy, you can always use the annotate
feature on <http://src.chromium.org/>.

### Following Up

Priorities and schedules vary between developers. So, when you assign a security
bug to an owner you should make sure you convey the timeline you’re expecting
for a fix. If you don’t see regular progress in the bug report, you may need to
manually follow up with the owner. Oftentimes, the bug just ended up being
harder to reproduce or fix than it originally appeared, so it’s a good idea to
offer help whenever there are unanticipated delays.

Keep in mind that on average we get high severity bugs fixed on the release
branch within *30 days* of the initial report, and almost always within *60
days* at the most. For critical severity bugs we have a hard deadline of 60 days
from initial report to a shipping fix.

### Handling a Fix

Once a fix is applied to trunk either you or the owner will change the bug
status to **Merge-Approved** and switch the **Restrict-View-SecurityTeam** label
to **Restrict-View-SecurityNotify**. At that point you’ll need to evaluate
whether the bug is a good candidate for a merge.

## Merging

Security fixes often need to be merged to the stable branch (this includes
patches to Chrome, WebKit, or other dependencies). Generally this is done by one
of the security team members. The first thing you need to do is evaluate whether
a fix should be considered for merge. Generally any high or critical severity
fixes should be seriously considered for merge. Medium and low-severity fixes
can be considered as well if they're particularly worrisome (e.g. bad
information leak), have significant non-security impact (e.g. top crasher), or
are just very low risk.

One of the key considerations in determining what to merge is assessing the risk
of breakage, which the bug’s owner should be able to convey. For risky fixes
you’ll often want to consider bumping it to a later milestone. When you've
decided to merge a fix, you should verify with a release manager that the merge
window is open for any affected branches. Normally a fix will need to be merged
to both the current stable and beta branches.

You should merge fixes via [drover](/developers/how-tos/drover), which automates
most of the process. If the [drover merge](/developers/how-tos/drover) does not
apply cleanly you may want to reconsider whether the fix will introduce an
unacceptable breakage risk. In the event that you can’t use drover to merge a
fix, you still have the option of manually merging from a branch checkout. In
general, most team members should have a checkout for the current stable, beta
and trunk branches. So, this can generally just follow the [normal patch
process](/developers/contributing-code), but using a branch repository.

Once the fix is successfully merged you will need to update the the bug with the
merge revision numbers, and change the status to **FixUnreleased** (or to
**Fixed** if the stable branch is not affected) and set the merge status to
**Merge-Completed** (if drover didn't do so). For beta merges, you should also
verify that the **ReleaseBlock-Stable** flag is present.

### Updating and Release Notes

After a patch has been landed (and potentially merged) you’ll need to update the
[security release
document](https://docs.google.com/a/google.com/Doc?docid=0AUtTDLyP_VtYY2dxczZrNHpfODhzajZzbWd6&hl=en),
which is used for tracking security bugs for release notes.

### Vulnerability Rewards Nomination

Generally, once a fix has been applied to trunk we will consider a bug for a
vulnerability reward. This will generally be done by the team member shepherding
the bug, or the owner if it’s a security team member. To remind yourself that
you may want to consider a specific bug you can add the **reward-topanel** label
in the tracker.

When nominating a bug for a reward, be sure to consider the overall quality, and
any additional value the reporter brought to the process. For reference, the
general guidelines are listed on the [Vulnerability Rewards
Program](/Home/chromium-security/vulnerability-rewards-program) page.
