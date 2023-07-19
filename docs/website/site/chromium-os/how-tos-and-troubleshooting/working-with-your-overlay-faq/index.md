---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: working-with-your-overlay-faq
title: Working with your Overlay FAQ
---

## Basics / Workflow

### What is a board? What is an overlay? What is an ebuild? What is a bsp?

Basic [terms and
concepts](https://docs.google.com/document/d/16PWfmUkv5ZRoeaJeggHqF8cNwgSPLeL2I_D7UKubV4w/pub)

### How do I push changes that I made to my app to my developer device?

Use [cros deploy](/chromium-os/build/cros-deploy)

## Packages

### My app depends and uses a third party library. How do I tell the build system to do that?

First, check if the third party library is already available as a Gentoo package
under
[src/third_party/portage-stable](https://chromium.googlesource.com/chromiumos/overlays/portage-stable/+/HEAD/)
or
[src/third_party/chromiumos-overlay](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/).

If yes, then just add the package name to the DEPEND (if build-time dependency)
and/or RDEPEND (if run-time dependency) of your app's ebuild.

### How do I add additional packages to my target &lt;board&gt;?

Add it as an RDEPEND to
chromeos-base/chromeos-bsp/chromeos-bsp-&lt;board&gt;.ebuild in your overlay.

Example
[BSP](https://chromium.googlesource.com/chromiumos/overlays/board-overlays/+/3c9d9f7a1e35861586cf39218d796e1f29869ad8/overlay-gizmo/chromeos-base/chromeos-bsp-gizmo/chromeos-bsp-gizmo-0.0.1.ebuild)
that pulls in peerd and bash on the target.

## Startup / Boot

### How do I make my program/service automatically start on boot?

CrOS uses the [upstart](http://upstart.ubuntu.com/cookbook/) system to manage
boot and service startup. Simply add an upstart job for your service and install
it into /etc/init

Example upstart script:
[link](https://chromium.googlesource.com/chromiumos/overlays/board-overlays/+/3c9d9f7a1e35861586cf39218d796e1f29869ad8/overlay-variant-panther-embedded/chromeos-base/chromeos-bsp-panther-embedded/files/load-network-drivers.conf)

### Should my upstart job depend upon boot-services or system-services?

system-services, unless your job in the critical boot path (e.g. network driver
setup), in which case boot-services is appropriate.

The following additional resources go into more detail:

[More information](/chromium-os/boot-milestones)

[Boot system design](/chromium-os/chromiumos-design-docs/userland-boot)
