---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/build
  - Chromium OS Build
page_name: tour-of-the-chromiumos-buildbot
title: Waterfall Tour
---

A ChromiumOS Waterfall hosts and lists groups of ChromiumOS builds.

ChromiumOS uses [LUCI](https://chromium.googlesource.com/infra/luci/luci-go/) to
run continuous builds, integration (commit queue) and tests. It uses chromite to
centralize/abstract the process of building for various configurations.

There are a number of builder groups that each present a waterfall view of their
status, but with chromiumos' use of ebuilds and manifests, many of the LUCI's
paradigms are not appropriate.
There is a column of changes, but these represent changes present in the
repositories, but does not represent which ones are included in the product by
the manifest references and the ebuilds used.

[TOC]

## Basics

The LUCI scheduler watches the clock, git repositories and previous builds,
telling builders to start building and testing new revisions. LUCI's Milo
service serves the "waterfall" page that shows all the results. For the full and
incremental builders every time a new revision is discovered, the client
triggers builds. Each of these builders generally has a single bot, so it is
usually busy when this happens. When the bot becomes idle the most recent
trigger is chosen to represent all automatic builds to date and it is started.
The rest of the pending builds are presumed to be covered enough, and are
removed. The build itself for chromeos is a sequence of three steps:

1.  Update the builder recipes (Basically, the piece the Chromium
            infrastructure people maintain to run the builders)
2.  Update the chromite tools (Basically, the piece the ChromeOS build
            people maintain to drive a ChromiumOS build)
3.  Run the cbuildbot command with a configuration name to do the real
            work.

The third step involves many stages, and it is parsed and presented as
individual steps by the recipe engine, but it is all coming from the one
command. This command can be run on local checkouts, by developers, in the same
ways as the bots do in order to predict and reproduce failures, tests, etc. This
page is about the mechanics that get it run, not what it does once it is run.

While each of these steps is executing Milo is collecting the output, and
presents the state on the web interface. There are other people and processes
that also monitor the state the client exports, or the side effects of the build
on git or google storage, to start further builds, send emails, etc.

## Builders

Let's take a look at the [waterfall
page](http://build.chromium.org/p/chromiumos/waterfall). For now, skip over the
header (green, red, yellow, etc.) and the box at the top with lots of links and
horizontal stripes (we'll come back to them later), and look down at the row of
grey boxes with "changes" at the left. Those boxes show one column per builder.
In ChromeOS we tend to name these by board/config name, and role. Each of these
builders is associated with one or more bots, which are machines willing to run
such builds. Each builder belongs to exactly one client (waterfall) but in some
waterfalls they can be willing to serve more than one builder. The title in this
row will link to a page that lists the recent builds completed for this
configuration, and the builders associated with it. This is a useful view to see
what recent runs have looked like, especially for comparing or contrasting.

## Builds

Once a build request gets assigned to a builder it begins a "Build".
Chromite/cbuildbot generally starts with clean up and an open sync to the state
of the tree (tip of branch in the branch case). Chromite/cbuildbot will tell you
what it is building in the sync stage, or provide references as links in the
build steps. Each build will have a number, a bot, and will run until completion
or fatal error. The waterfall view will present the exposed steps in reverse
chronological order in the column it belongs in. There is a list of times
(generally in Google Coordinated Time) in the leftmost column to record when
observation are made, and the boxes change colors to represent the current state
as they progress. When a bot is finished it may disconnect and locally clean
itself up (e.g. by rebooting) before asking for its next slug of work.

## Current Activity

In the row just above the titles for the grid there is a summary for each
builder of what is currently going on, and what is upcoming (active builds,
pending builds, delay to next time based decision). Beware that the ETA in this
box is computed using a very simple predictive model that assume each build is
roughly the same as the ones before. This goes very wrong when changing between
builds that work, and builds which break a little ways in. Beware of these
predictions in anything other than routine situations.

## Last Build

The top row of the grid part is a summary of how the previous build finished. We
often view this as the state of the builder. In a rapidly cycling build system
it is a close representative of what the state of the tree is for the type of
build represented. For a slow-cycling system, or build requests that don't
follow a smooth tree ordering this is less true and relevant. For ChromeOS
consider the type of builder before thinking about what the state (or color) of
the builder means.

## The Announcement

When present, we tend to use variations on a theme for the announcement box on
top of the waterfall (and some other) pages. We use the announcement to
abbreviate and present other interesting state information. The top bar and it's
color represents the
[state](http://www.chromium.org/developers/tree-sheriffs/sheriff-details-chromium-os#TOC-How-do-I-read-the-waterfall-)
of the tree, including the current
[message](http://chromiumos-status.appspot.com/). Under that there are 2 panes.

*   On the right there are usefully grouped lines of boxes that repeat
            the last build (see above) status. The label on the left of each row
            indicates what it is, and should link to a view of the waterfall
            with just those builders for more detail.
*   On the left there are useful links, lists of current rotations
            (roles taken by different people each day or two), and links to
            different views of the build data.

## Different types of ChromeOS builders

Almost all of these come from the workings of chromite, but are very useful to
have an idea about to understand the big picture:

*   Full - a build that cleans out the chroot, and builds from stable
            ebuilds at the tip of tree
*   Canary - an official build with a new saved version
*   Incremental - a build that updates the repos, and builds from stable
            ebuilds at the tip of tree
*   Try - a build that updates the repos, applies a patch, updates
            ebuilds and tries to build
*   [ChromeOS ASAN information](/system/errors/NodeNotFound)ASAN - see
*   SDK - try new toolsets and update the prepackaged information for
            the chromeos SDK
*   Toolchain - try newer compilers
*   (chrome) - try building and testing chromeos with tip of tree chrome
            version
*   branch - pull a branch of the source code and try building there
            (factory, firmware, releases)
*   LKGM - distribute a successful and stable manifest to other
            interested parties

## When the columns aren't right

Currently the set of columns and the allocations of bots to back them is managed
by the Chrome Infrastructure team, please file an
[Infrastructure](https://goto.google.com/cros-infra-bug)
bug OS-Chrome to start the process of correcting it. Please
describe the type of build, and the cbuildbot configuration it should be
running.
