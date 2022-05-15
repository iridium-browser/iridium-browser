---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/build
  - Chromium OS Build
page_name: faq
title: Build FAQ
---

[TOC]

# cbuildbot

## Overview of cbuildbot

<https://www.chromium.org/chromium-os/build/using-remote-trybots>

## How to add metadata to cbuildbot

<https://sites.google.com/a/chromium.org/dev/cbuildbot-metadata-json-tutorial>

## How to run processes in parallel (chromite.lib.parallel)

##
<https://sites.google.com/a/chromium.org/dev/chromium-os/build/chromite-parallel>

## Why is the HWTest phase stage 'orange'?

The hardware test phase is 'orange' when it fails with a warning. These warnings
occur when a test passed with warnings or after retries. They can also occur
with the tests are aborted (see below).

## How do I test that my change does not break an incremental build?

To test whether your change breaks an incremental build, you will need to do two
builds in a row in the same sandbox. You can do this with local tryjobs like the
following:

```none
cros tryjob --local -g 1234 chell-incremental
cros tryjob --local -g 1234 chell-incremental
```

## In cbuildbot_config.py, what does important=False mean?

From the documentation in cbuildbot_config.py: If important=False then this bot
cannot cause the tree to turn red. Set this to False if you are setting up a new
bot. Once the bot is on the waterfall and is consistently green, mark the
builder as important=True.

# Google Storage

## What bucket can I use for testing?

If you are a Googler, you can use gs://chromeos-throw-away-bucket for testing.
Keep in mind that the contents can be read or deleted by anyone at Google.

We also have another similar bucket named gs://chromeos-shared-team-bucket .
Probably using gs://chromeos-throw-away-bucket makes more sense since it is
clear that the contents there can be deleted by anyone at Google.

If you are not a Googler, you should create your own bucket in Google Storage.

# Portage/Emerge

## How do I force emerge to use prebuilt binaries?

```none
emerge -g --usepkgonly
(Note: The --getbinpkg only flag for emerge does not actually work)
```

## How do I check the USE flags for a package I am emerging?

Emerge will tell you what USE flags are used when the **-pv** flags are provided
e.g.

```none
emerge-$BOARD chromeos-chrome -pv
```

# Development

FAQ items covering general development questions

## What should I read first/Where do I start?

<https://chromium.googlesource.com/chromiumos/docs/+/master/developer_guide.md>

## How can I make changes depend on each other with Cq-Depend?

<https://sites.google.com/a/chromium.org/dev/developers/tree-sheriffs/sheriff-details-chromium-os/commit-queue-overview#TOC-How-do-I-specify-the-dependencies-of-a-change->

## How do I create a new bucket in gsutil?

[gsutil
FAQ](https://chromium.googlesource.com/chromiumos/docs/+/master/gsutil.md#FAQ)

## How do I port a new board to Chromium OS?

<https://sites.google.com/a/chromium.org/dev/chromium-os/how-tos-and-troubleshooting/chromiumos-board-porting-guide>

## Making changes to the Chromium browser on ChromiumOS (AKA Simple Chrome)

[
https://chromium.googlesource.com/chromiumos/docs/+/master/simple_chrome_workflow.md](https://chromium.googlesource.com/chromiumos/docs/+/master/simple_chrome_workflow.md)

## How to install an image on a DUT via cros flash?

<http://www.chromium.org/chromium-os/build/cros-flash>