---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
- - /developers/testing/chromium-build-infrastructure
  - Chromium build infrastructure
page_name: repro-a-build
title: How to repro a build from a waterfall.
---

Reproducing a builder's build can generally be done by following the
instructions located under the 'run_recipe' link on the build page itself.

In particular, you need to [install
depot_tools](/developers/how-tos/install-depot-tools), and then get a build
checkout:

```none
$ cd <empty dir>
$ # One of:
$ fetch infra           # external user
$ fetch infra_internal  # internal user
$ cd build
```

And then navigate to a build that you want to repo. Look for the build step
which looks like:

    ==setup_build== setup_build
    running recipe: "&lt;RECIPE_NAME&gt;" ( 0 secs )

    *   ==stdio==
    *   ==run_recipe==

Then click the 'run_recipe' link and you'll get some instructions on how to
repro the build.
