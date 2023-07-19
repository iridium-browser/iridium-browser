---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: creating-local-mirrors
title: Creating Local Mirrors
---

[TOC]

## Background

While git works great for keeping things up-to-date and for speeding up network
transfers (when you need to pull in new commits), the sheer size of the
ChromiumOS repository is still a hurdle for many groups. Your company might not
have the greatest connection to the machine where the repositories are hosted,
or perhaps even your region or country have bottlenecks our of your control. In
these cases, you probably don't want all of the engineers in your company doing
the same multi-gigabyte repository mirroring on their various machines as this
can prohibitively slow to development. Especially if something goes wrong and
they want to create a fresh checkout!

Have no fear though, it is easy to create your own local mirror of the Chromium
OS repository, keep it up-to-date, and have all local people pull from that. And
for those people who want to contribute back, it's easy to do that too!

## Overview

You will need one machine that has a public network connection (for fetching
updates from the Chromium OS repository), as well as running a git service (so
your local people can then mirror from your system). While a dedicated machine
is obviously recommended (as the machine will be seeing a lot of network and
disk traffic), an existing system will suffice.

You will not need root as all of the operations can be run as a normal user.

## Helper Script

Rather than having to maintain the mirror manually, we've written a script that
should automate things for you. To fetch it:

```none
wget 'https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/scripts/cros_mirror?format=text' -O - | base64 -d > cros_mirror
chmod a+rx cros_mirror
```

## Server Setup

You will need two bits of information:

*   Where to store all the code (many gigabytes)
*   How to serve the data to local users

Let's assume you're going to store the code in /home/cros/mirror/ and run a
local git repo from a machine known as "local-cros-mirror". Pick any other
settings you like.

## Initial Sync

The first transfer of source code will take a while (depending on the speed of
your connection to our servers) as it will be fetching many gigabytes of data.

```none
./cros_mirror -r /home/cros/mirror -u git://local-cros-mirror
```

Once that finishes, you're ready to start serving up data!

## Keeping up-to-date

You can put this command into a cronjob to keep the tree up-to-date:

```none
./cros_mirror -r /home/cros/mirror
```

It should automatically load the previous settings, and be race free from other
users pulling from it simultaneously.

## Example Server Usage

With these example settings, you could launch a git instance (as any user who
has read access to the mirror dir, so doesn't require root) like so:

```none
git daemon --base-path=/home/cros/mirror/ --export-all
```

And then your users could pull from it like so:

```none
repo init -u git://local-cros-mirror/chromiumos/manifest.git --repo-url=git://local-cros-mirror/external/repo.git
```

## Alternative Client Usage (git insteadOf)

The helper script works by rewriting the manifest on the fly to point to the
local system. Clients can support this locally without needing to use the
rewritten manifest by using the insteadOf config.

```none
git config "url.git://local-cros-mirror.insteadOf" "https://git.chromium.org/git"
```
