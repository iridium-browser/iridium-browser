---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: git-server-side-information
title: Git server-side information
---

[TOC]

This guide has been migrated to markdown:
<https://chromium.googlesource.com/chromiumos/docs/+/HEAD/source_layout.md#FAQ>

Please update any stale links you might have come across.

---

## How do I add my project to the CQ?

1.  Add new git repository to
            chromiumos/infra/config/chromeos_repos.star (in the checkout). For
            example, this would be a CL like
            [this](https://chrome-internal-review.googlesource.com/c/chromeos/infra/config/+/1367369/).
2.  After modifying chromeos_repos.star, run ./regenerate_config.sh in
            the base of infra/config.
3.  Email the CL to the CI oncaller.
