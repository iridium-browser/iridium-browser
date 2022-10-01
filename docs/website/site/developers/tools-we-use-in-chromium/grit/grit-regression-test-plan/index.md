---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/tools-we-use-in-chromium
  - Tools we use in Chromium
- - /developers/tools-we-use-in-chromium/grit
  - GRIT
page_name: grit-regression-test-plan
title: GRIT Regression Test Plan
---

Multiple projects use GRIT. This document details the regression testing plan
employed by the GRIT project to ensure compatibility with all of the projects
using it.

The plan is fairly straight forward:

1.  Each change must pass grit.py unit with no errors before being
            checked in. Ideally, the command should be tested on both Linux and
            Windows, since some behavior may break on case-insensitive platforms
            and other behavior may break on case-sensitive platforms, or may
            break due to different path separator characters.
2.  Projects using GRIT should proactively contribute unit tests and
            regression tests that will be run as part of [grit.py unit
            ](https://code.google.com/p/grit-i18n/wiki/RegressionTestPlan)to
            ensure any functionality they require does not get modified by
            accident.
3.  When a project reports that a new version of GRIT has broken
            functionality for that project, the GRIT team and folks on the
            project should work together to get a regression test added.

Individual projects, of course, may put in place extensive tests in their own
repository that further help to ensure that a new revision of GRIT does not
break them. For example, when the revision of GRIT used is changed in the
[Chromium project](http://www.chromium.org/), the change would normally pass
through that project's [try
servers](http://www.chromium.org/developers/testing/try-server-usage) or [commit
queue](/developers/testing/commit-queue) which would help catch any new behavior
of GRIT breaking the project's test suite.
