---
breadcrumbs:
- - /for-testers
  - For Testers
page_name: bug-reporting-guidelines
title: Bug Life Cycle and Reporting Guidelines
---

[TOC]

## Important links

### Chromium (the web browser)

*   Report bugs at <https://crbug.com/wizard>
*   Specifically:
    *   [Bug Reporting Guidelines for the Mac & Linux
                builds](/for-testers/bug-reporting-guidlines-for-the-mac-linux-builds)
                (with links to known issues pages)
    *   [Instructions for reporting
                crashes](/for-testers/bug-reporting-guidelines/reporting-crash-bug).
*   View existing bugs at
            <https://bugs.chromium.org/p/chromium/issues/list>

### Chromium OS (the operating system)

*   Report bugs at
            <https://code.google.com/p/chromium/issues/entry?template=Defect%20on%20Chrome%20OS>
*   View existing bugs at [Chromium OS
            issues](https://code.google.com/p/chromium/issues/list?can=2&q=os%3Dchrome)

You need a [Google Account](https://www.google.com/accounts/NewAccount)
associated with your email address in order to use the bug system.

## Bug reporting guidelines

*   If you're a web developer, see [How to file a good
            bug](https://developers.google.com/web/feedback/file-a-bug)
*   Make sure the bug is verified with the latest Chromium (or Chrome
            canary) build.
*   If it's one of the following bug types, please provide some further
            information:
    *   **Web site compatibility problem:** Please provide a URL to
                replicate the issue.
    *   **Hanging tab: See [Reporting hanging tab
                bugs](/for-testers/bug-reporting-guidelines/hanging-tabs).**
    *   **Crash: See [Reporting crash
                bugs](/for-testers/bug-reporting-guidelines/reporting-crash-bug).**
*   Provide a high-level problem description.
*   Mention detailed steps to replicate the issue.
*   Include the expected behavior.
*   Verify the bug in other browsers and provide the information.
*   Include screenshots, if they might help.
*   If a bug can be [reduced to a simplified
            test](/system/errors/NodeNotFound), then create a simplified test
            and attach it to the bug.
*   Additional [Bug Reporting Guidlines for the Mac & Linux
            builds](/for-testers/bug-reporting-guidlines-for-the-mac-linux-builds).
*   Additional Guidelines for [Reporting Security
            Bugs](/Home/chromium-security/reporting-security-bugs).

## Release block guidelines

*   [Release Block Guidelines](/issue-tracking/release-block-guidelines)

## Triage guidelines

*   [Triage Best
            Practices](/for-testers/bug-reporting-guidelines/triage-best-practices)

## Labels

Labels are used to help the engineering team categorize and prioritize the bug
reports that are coming in. Each report can (and should) have multiple labels.

For details on labels used by the Chromium project, see [Chromium Bug
Labels](/for-testers/bug-reporting-guidelines/chromium-bug-labels).

## Status

### Open bugs

<table>
<tr>
Status value 	 Description </tr>
<tr>
<td>Unconfirmed</td>
<td>The default for public bugs. Waiting for someone to validate, reproduce, or otherwise confirm that this is a bug. </td>
</tr>
<tr>
<td>Untriaged</td>
<td>A confirmed bug that has not been reviewed for priority or assignment. This is the default for project members' new bugs. </td>
</tr>
<tr>
<td>Available</td>
<td>Confirmed and triaged, but not assigned. Feel free to take these bugs! </td>
</tr>
<tr>
<td>Assigned</td>
<td>In someone's work queue. </td>
</tr>
<tr>
<td>Started</td>
<td>Actively being worked on. </td>
</tr>
</table>

### Closed bugs

<table>
<tr>
Status value 		 Description 	</tr>
<tr>
<td>Fixed</td>
<td>Fixed.</td>
</tr>
<tr>
<td>Verified</td>
<td>The fix has been verified by test or by the original reporter.</td>
</tr>
<tr>
<td>Duplicate</td>

<td>This issue has been reported in another bug, or shares the same root cause as another bug. When Duplicate is selected, a field will appear for the ID of the other bug --- be sure to fill this in.</td>

<td>Mark the bug with less information/discussion in it as the Duplicate.</td>

</tr>
<tr>
<td>WontFix</td>
<td>Covers all the reasons we chose to close the bug without taking action (can't repro, working as intended, obsolete).</td>
</tr>
<tr>
<td>ExternalDependency</td>
<td>Bugs that turn out to be in another project's code and that we've filed with that other project. Useful for tracking known issues that manifest themselves in our product, but that need to be fixed elsewhere (such as WebKit and V8 issues).</td>
</tr>
<tr>
<td>FixUnreleased</td>
<td>A special state for security hotfixes to mark bugs that are fixed, but not yet delivered to users. Bugs with this status will be visible only to project members and the original reporter.</td>
</tr>
<tr>
<td>Invalid</td>
<td>Illegible, spam, etc.</td>
</tr>
</table>

## Bug life cycle

*   When a bug is first logged, it is given **Unconfirmed** status.
*   The status is changed from unconfirmed to Untriaged once it has been
            verified as a Chromium bug.
*   Once a bug has been picked up by a developer, it is marked as
            Assigned.
*   A status of **Started** means a fix is being worked on.
*   A status of **Fixed** means that the bug has been fixed, and
            **Verified** means that the fix has been tested and confirmed.
            Please note that it will take some time for the "fix" to make it
            into the various channels (canary, beta, release) - pay attention to
            the milestone attached to the bug, and compare it to
            chrome://version.

## Deciding where to submit your bug

Usually, Chromium-related bugs should be filed under one of the following
projects:

*   [chromium](http://code.google.com/p/chromium/issues/entry)
*   [blink](http://crbug.com/) and add component "Blink"

## Helping with bug triage

Read <http://www.chromium.org/getting-involved/bug-triage> if you're interested
in helping with bug triage.

Infrastructure and build tools

If you find an issue with our infrastructure or build tools, please file the
ticket using the Build Infrastructure template:

*   <https://code.google.com/p/chromium/issues/entry?template=Build%20Infrastructure>
