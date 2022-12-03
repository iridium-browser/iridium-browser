---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: blink-triaging
title: Component:Blink bug labeling rotation
---

[Document with tips for bug
labeling](https://docs.google.com/document/d/1l9XehKEHAJu3-LnWDdXl8-t-8rz9dk8dy1bEI4zzUOU/edit)

## [Mapping of labels to owners and teams](https://docs.google.com/spreadsheets/d/19JEFMvsxD3eThyGiJRqAjcpx362LHUDdVzICAg7TYZA/edit#gid=0)

## Goal

Deliver Blink bugs in crbug.com to engineers who are responsible for the bug
area by adding Blink&gt;*Foo* components.

## Example Instructions

We don't have a common instruction yet. The below is an example, and you don't
need to follow it. Important points are:

*   Reduce the number of [Component=Blink
            bugs](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Component%3DBlink)
*   Newer bugs are important.

### Task 1: Handling Component=Blink issues (mandatory, daily)

1) Search for ["Component=Blink -Hotlist=CodeHealth -Needs=Feedback
-Needs=Reduction"](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Component%3DBlink+-Hotlist%3DCodeHealth+-Needs%3DFeedback+-Needs%3DReduction)

2) Read the issue description and comments and add Blink&gt;*Foo* component and
remove Blink component, if the area/ownership is clear. Otherwise:

*   If the issue has not enough information, ask for additional
            information, add Needs-Feedback label, and add your email address to
            Cc field.
    Add a Next-Action value set to 14 days from the current date.
    You're responsible for this bug. You should handle the bug until you
    identify Blink areas or feedback timeout.
    [This
    link](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3DBlink+-Hotlist%3DCodeHealth+Needs%3DFeedback+cc%3Ame&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)
    shows the list of bugs of this kind you are responsible for.
*   If the issue doesn't seem to be reproducible, but plausible, add
            Needs-TestConfirmation.
*   If the reproduction is too complex to understand the culprit area,
            add Needs-Reduction.
*   If you understand the culprit, but can't find an appropriate
            Blink&gt;*Foo* component (eg. by looking at similar bugs resolved in
            the not-too-distant past), email
            [crblink-rotation@](https://groups.google.com/a/chromium.org/forum/#!forum/crblink-rotation)
            (and/or add Hotlist-BlinkNoLabel, this is TBD). You should find an
            owner if the bug looks serious.

**Task 2: Handling Component=UI issues (mandatory, daily)**

1) Search for untriaged
[Component=UI](https://bugs.chromium.org/p/chromium/issues/list?q=Component%3DUI%20-Needs%3DFeedback%20-Type%3DFeature%2CTask&can=2)
bugs

2) Read the issue description and add comments or move to sub-components of UI
or other components (including Blink sub-components as appropriate). Set
priorities as needed.

### Task 3: Handling issues without Component: field (optional)

Do the same things as task 1 and task 2 for issues without Component: field. If
an issue isn't related to Blink, add appropriate non-Blink components such as
Component:`UI`, Component:`Internals`.

*   [Bugs created in the last 30 days without
            owners](https://bugs.chromium.org/p/chromium/issues/list?q=-has%3Acomponent%20-reporter%3Achromium.org%20-label%3Aautofiled%20-label%3Aperformance%20-hotlist%3DHistogramEraser%20%20-hotlist%3DMetrics-Eraser%20opened%3Etoday-30%20-has%3Aowner&can=2)
*   [Bugs that have owners but that haven't been modified in the last
            year](https://bugs.chromium.org/p/chromium/issues/list?q=-has%3Acomponent%20-reporter%3Achromium.org%20-label%3Aautofiled%20-label%3Aperformance%20-hotlist%3DHistogramEraser%20%20-hotlist%3DMetrics-Eraser%20modified%3Ctoday-365%20has%3Aowner%20%20-hotlist%3DExpiredHistograms&can=2)
*   [All bugs without
            owners](https://bugs.chromium.org/p/chromium/issues/list?q=-has%3Acomponent%20-reporter%3Achromium.org%20-label%3Aautofiled%20-label%3Aperformance%20-hotlist%3DHistogramEraser%20%20-hotlist%3DMetrics-Eraser%20-has%3Aowner&can=2)
*   [All
            bugs](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=-has%3Acomponent+-reporter%3Achromium.org+-label%3Aautofiled+-label%3Aperformance+-hotlist%3DHistogramEraser++-hotlist%3DMetrics-Eraser)

**Task 4: monitor stale P0/P1 security bugs (optional)**

*   Review all result from [this
            search](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Type%3DBug-Security+component%3Ablink+pri%3D0%2C1+modified%3Ctoday-30&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids).
*   Check that the bug has an owner, the owner is actively working on
            the issue, and is fixing. Re-assign, re-categorize or ping the bug
            as appropriate.

**Deprecated:**

### Reducing/Confirming Component=Blink bugs (mandatory, daily)

1) Search for "[Component=Blink
Needs=Reduction](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Component%3DBlink+Needs%3DReduction)",
choose one, and investigate it to identify Blink areas by reading HTML/CSS/JS
code and/or making a reduction.

2) Add Blink&gt;*Foo* components, remove Blink component and Needs-Reduction
when confirmed and updated the status accordingly, if needed.

## Contact

Public mailing list:
[crblink-rotation@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/crblink-rotation)
(<https://groups.google.com/a/chromium.org/forum/#!forum/crblink-rotation>)
