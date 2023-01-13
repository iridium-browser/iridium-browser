---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/tools-we-use-in-chromium
  - Tools we use in Chromium
- - /developers/tools-we-use-in-chromium/grit
  - GRIT
page_name: how-to-contribute-to-grit
title: How to contribute to GRIT
---

Please feel free to contribute patches to GRIT. If you have a new project using
GRIT, please let us know by sending a message to the developer group
(<http://groups.google.com/group/grit-developer>), and join the user group
(<http://groups.google.com/group/grit-users>). Also, note the [regression test
plan](/developers/tools-we-use-in-chromium/grit/grit-regression-test-plan) that
describes how we ensure GRIT stays compatible with your project.

Those who send the occasional patch should ask one of the project committers
(one of {benrg, brettw, flackr, gfeher, joaodasilva, joi, mal, markusheintz,
mnissler, newt, pastarmovj, sergeyu, thakis, thestig, tony}@chromium.org) to
land their patch for them after review. Please read the following section on
preparing patches for review.

Preparing Patches

You should read the [user's
guide](/developers/tools-we-use-in-chromium/grit/grit-users-guide) and the
[design
overview](/developers/tools-we-use-in-chromium/grit/grit-design-overview) for
background on the intended usage and design of GRIT. For more details or for
help with how to design your patch, ask one of the project committers.

We use the Chromium depot_tools to upload patches to the Rietveld review tool.
Please [install the tools](/developers/how-tos/install-depot-tools) and then use
git cl upload to upload a patch before you send it for review.

Before sending patches for review, make sure grit.py unit runs without warnings.

For large patches, it is advisable to discuss with committers on the project
first, either by mailing a specific committer you think knows the code you are
changing, or by mailing the developer list. It is also advisable to test large
patches with Chromium before sending it for review, as it is the largest known
open-source project using GRIT, so it exercises many features. To do this:

*   Follow the [Chromium instructions for setting up a
            checkout](/developers/how-tos), and make sure you are able to fetch
            and build the Chromium code.
*   Go to the local GRIT checkout at src/tools/grit and move to a new
            branch (e.g. git co -b my_grit_wip_branch)
*   Get the latest version of the repo by running git pull master
*   You can now add your changes, build Chromium, check that the build
            succeeds, check that gclient runhooks succeeds, etc.
