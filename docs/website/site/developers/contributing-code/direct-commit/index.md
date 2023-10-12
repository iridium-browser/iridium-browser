---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/contributing-code
  - Contributing Code
page_name: direct-commit
title: Directly committing code
---

[TOC]

When [contributing code](/developers/contributing-code) to Chromium, the last
step in the life of a change list is committing it, after which it's closed.

The preferred way of committing changes is via the [commit
queue](/developers/testing/commit-queue).

However, in certain circumstances it is acceptable or necessary to directly
commit your change, bypassing the commit queue. This is discouraged however, due
to not running the tests (hence higher risk of breakage), and requires more
supervision on the part of the committer.

Updated instructions for bypassing the commit queue are here:

<https://chromium.googlesource.com/chromium/src/+/HEAD/docs/infra/cq.md>
