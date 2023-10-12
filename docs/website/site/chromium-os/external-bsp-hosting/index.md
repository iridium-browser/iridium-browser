---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: external-bsp-hosting
title: External BSP Hosting
---

[TOC]

## Introduction

When any BSP developer wants to build their code with Chromium OS and have it
easily shared with other developers, they currently typically have to commit
their BSPs to either the public or private Chromium OS repositories and update
the respective repo manifest files if necessary. In hosting their ebuilds in the
Chromium OS repositories, however, they get tied into the [Chromium OS Commit
Queue](/developers/tree-sheriffs/sheriff-details-chromium-os/commit-queue-overview)
and code review process, which is not always ideal for them.

### Local Manifests

In order to pull in code hosted on an external git repository, developers can
make use of local manifest files. Local manifest files allow more remotes and
repositories to be specified for download in additional xml files without direct
dependencies being specified in the main Chromium OS manifest files. Any number
of local manifest files may be supplied, allowing developers to name and
distribute their local manifest files for each project.

After following the instructions for [getting the
source](/chromium-os/developer-guide#TOC-Get-the-Source), create a
“local_manifests” folder inside the .repo folder. Assuming like in the setup
instructions that your source code is in ${HOME}/chromiumos, run the following
commands:

> `cd ${HOME}/chromiumos/.repo`

> `mkdir local_manifests`

Then, create or download any additional XML manifest files into the created
folder and run \`repo sync\` to pull down the new code.

### Manifest Format

Local manifest files can have any name as long as they are of the .xml
extension. Their format is the same as normal manifest files, and can be viewed
[here](https://gerrit.googlesource.com/git-repo/+/HEAD/docs/manifest-format.txt).
A simple example manifest file for pulling
<http://github.com/chromiumos/foo.git> on branch “branchname” into
src/private-overlays/overlay-foo-private:

File: ${HOME}/chromiumos/.repo/local_manifests/foo.xml

> `<?xml version="1.0" encoding="UTF-8"?>`

> `<manifest>`

> <remote name="github" fetch="https://github.com/" />`

> <project path="src/private-overlays/overlay-foo-private" name="chromiumos/foo"
> remote="github" revision="branchname"/>`

> `</manifest>`

## BSP Overlays

This section is meant as a quick-start guide for the basics of setting up a BSP
within an overlay, but doesn't fully cover creating the overlay itself. The
basics of setting up an overlay can be found in the [Board Porting
guide](/chromium-os/how-tos-and-troubleshooting/chromiumos-board-porting-guide).
Even with board and BSP code hosted in an external repository, the board name
still has to be added to the cros-board.eclass file. Also, for brevity,
copyright headers have been left out of these sample templates, but should be
added in all ebuild files in whatever format is correct for your organization.

make.conf

> This file contains several [key
> settings](/chromium-os/how-tos-and-troubleshooting/chromiumos-board-porting-guide#TOC-make.conf:-Global-Build-Settings)
> for kernel configurations, such as the architecture, config, and tune. The key
> fields are:

> *   CHROMEOS_KERNEL_CONFIG - the kernel configuration file to use. The
              path is relative to the kernel source checkout. Setting this
              overrides any use of CHROMEOS_KERNEL_SPLITCONFIG.
> *   CHROMEOS_KERNEL_SPLITCONFIG - the kernel splitconfig to use.
              Splitconfigs can be used instead of a full config. For details on
              splitconfigs, see
              [here](/chromium-os/how-tos-and-troubleshooting/kernel-configuration).
> *   MARCH_TUNE - The common set of compiler flags used to optimize for
              your processor.
> *   CFLAGS, CXXFLAGS - The compiler flags to use when building the
              code.
> *   USE - use flags to set. When using splitconfigs, for example,
              providing the ‘nowerror’ flag disables treating warnings as
              errors.

> For an example MIPS board, the make.conf would look something like:

> > `CHROMEOS_KERNEL_CONFIG="arch/mips/configs/ci20_defconfig"`

> > `CHROMEOS_KERNEL_ARCH="mips"`

> > `# We assume that we are running a little endian 32bit MIPS CPU. If you
> > have`

> > `# a MIPS64 CPU, then you really should be using the n32 overlay instead.`

> > `MARCH_TUNE="-march=mips32"`

> > `CFLAGS="${CFLAGS} ${MARCH_TUNE}"`

> > `CXXFLAGS="${CXXFLAGS} ${MARCH_TUNE}"`

> > `# Kernel doesn't yet build cleanly.`

> > `USE="${USE} nowerror"`

### Kernel

The following files are required for customizing the kernel source, build, or
install.

virtual/linux-sources/linux-sources-2.ebuild

> `EAPI="6"`
> `DESCRIPTION="Chrome OS Kernel virtual package"`
> `HOMEPAGE="http://src.chromium.org"`
> `LICENSE="GPL-2"`
> `SLOT="0"`
> `KEYWORDS="mips"`
> `IUSE="-kernel_sources"`
> `RDEPEND="`
> sys-kernel/<kernel_name>[kernel_sources=]`
> `"`

    Replace the KEYWORDS value with the correct platform

    Replace the LICENSE value with your license

    Replace “&lt;kernel_name&gt;” in the RDEPEND field with the name of your
    kernel package, such as “ci20-kernel”

sys-kernel/&lt;kernel_name&gt;/&lt;kernel_name&gt;-&lt;version&gt;.ebuild

sys-kernel/&lt;kernel_name&gt;/&lt;kernel_name&gt;-&lt;version&gt;-r1.ebuild

> `EAPI="6"`
> `CROS_WORKON_REPO="<git_repo_path>"`
> `CROS_WORKON_PROJECT="<git_repo_name>"`
> `CROS_WORKON_BLACKLIST="1"`
> `CROS_WORKON_COMMIT="<git_commit_hash>"`
> `# This must be inherited *after* EGIT/CROS_WORKON variables defined`
> `inherit git-2 cros-kernel cros-workon`
> `DESCRIPTION="Chrome OS Kernel-<kernel_name>"`
> `KEYWORDS="mips"`
> `DEPEND="!sys-kernel/chromeos-kernel-next`
> !sys-kernel/chromeos-kernel`
> `"`
> `RDEPEND="${DEPEND}"`

    Replace “&lt;kernel_name&gt;” with the name of your kernel package, as
    specified in the RDEPEND field of the linux-sources-2.ebuild file

    Replace the KEYWORDS value with the correct platform

    The ‘&lt;kernel_name&gt;-&lt;version&gt;-r1.ebuild’ file is just a symbolic
    link to the ‘&lt;kernel_name&gt;-&lt;version&gt;.ebuild’ file. Whenever
    changes are made to the ebuild file, the symlink should be
    revbumped/renamed, for instance incrementing ‘-r1’ to ‘-r2’.

    &lt;version&gt; should be the version number, for instance “3.0.8”

    &lt;git_repo_path&gt; is the git repository path, such as
    “git://github.com/MIPS”

    &lt;git_repo_name&gt; is the git repository name, such as “CI20_linux”. This
    field is prepended with &lt;git_repo_path&gt; when referring to a repository
    location.

    &lt;git_commit_hash&gt; is the hash of the git commit to use. For instance,
    on the CI20_linux repo, “2e5af7d6650ce6435f2894bdd7ee7f0333950942” would
    refer to the head of the 3.0.8 branch
    (<https://github.com/MIPS/CI20_linux/tree/ci20-v3.0.8>)

    By default, this builds the Makefile’s “vmlinuz.bin” target for mips, uImage
    for arm, and uses the default target for everything else

    For customizing the build process, the following functions can be added to
    the ebuild:

        src_prepare() { }

            [Preparation](https://devmanual.gentoo.org/ebuild-writing/functions/src_prepare/index.html)
            of the source, such as applying patches

        src_configure() { }

            [Configuring](https://devmanual.gentoo.org/ebuild-writing/functions/src_configure/index.html)
            the source

        src_compile() { }

            [Compiling](https://devmanual.gentoo.org/ebuild-writing/functions/src_compile/index.html)
            the source, i.e. calling make or emake.

        src_test() { }

            [Run](https://devmanual.gentoo.org/ebuild-writing/functions/src_test/index.html)
            pre-install test scripts

        src_install() { }

            [Install](https://devmanual.gentoo.org/ebuild-writing/functions/src_install/index.html)
            the package

### Firmware

The following files are required for customizing the firmware to install.

virtual/chromeos-firmware/chromeos-firmware-2.ebuild

> `EAPI="6"`
> `DESCRIPTION="Chrome OS Firmware virtual package"`
> `HOMEPAGE="`[`http://src.chromium.org`](http://src.chromium.org/)`"`
> `LICENSE="BSD"`
> `SLOT="0"`
> `KEYWORDS="arm"`
> `IUSE=""`
> `RDEPEND="chromeos-base/chromeos-firmware-<name>"`

    Replace the LICENSE value with your correct license

    Replace the KEYWORDS field with your board’s architecture (arm, mips, ppc,
    x86, etc)

    Replace &lt;name&gt; with the name of your board

chromeos-base/chromeos-firmware-&lt;name&gt;/chromeos-firmware-&lt;name&gt;-&lt;version&gt;.ebuild

chromeos-base/chromeos-firmware-&lt;name&gt;/chromeos-firmware-&lt;name&gt;-&lt;version&gt;-r1.ebuild

> `EAPI="6"`
> `CROS_WORKON_REPO="<git_repo_path>"`
> `CROS_WORKON_PROJECT="<git_repo_name>"`
> `CROS_WORKON_BLACKLIST="1"`
> `CROS_WORKON_COMMIT="<git_commit_hash>"`
> `inherit git-2 cros-workon`
> `DESCRIPTION="Chrome OS Firmware"`
> `KEYWORDS="mips"`

    Replace the KEYWORDS value with the correct platform

    The ‘chromeos-firmware-&lt;name&gt;-&lt;version&gt;-r1.ebuild’ file is just
    a symbolic link to the
    ‘chromeos-firmware-&lt;name&gt;-&lt;version&gt;.ebuild’ file. Whenever
    changes are made to the ebuild file, the symlink should be
    revbumped/renamed, for instance incrementing ‘-r1’ to ‘-r2’.

    &lt;version&gt; should be the version number, for instance “2015.03.05” for
    the revision date for repos that do not have branched versions

    &lt;git_repo_path&gt; is the git repository path, such as
    “git://github.com/raspberrypi”

    &lt;git_repo_name&gt; is the git repository name, such as “firmware”. This
    field is prepended with &lt;git_repo_path&gt; when referring to a repository
    location.

    &lt;git_commit_hash&gt; is the hash of the git commit to use. For instance,
    on the raspberrypi/firmware repo, “cdcb50646bc035e9f3e74b99d764023c4b98d7d5”
    would refer to the current head of the master branch
    (https://github.com/raspberrypi/firmware/tree/master)

    By default, this builds and installs the Makefile’s default target

    For customizing the build process, the following functions can be added to
    the ebuild:

        src_prepare() { }

            [Preparation](https://devmanual.gentoo.org/ebuild-writing/functions/src_prepare/index.html)
            of the source, such as applying patches

        src_configure() { }

            [Configuring](https://devmanual.gentoo.org/ebuild-writing/functions/src_configure/index.html)
            the source

        src_compile() { }

            [Compiling](https://devmanual.gentoo.org/ebuild-writing/functions/src_compile/index.html)
            the source, i.e. calling make or emake.

        src_test() { }

            [Run](https://devmanual.gentoo.org/ebuild-writing/functions/src_test/index.html)
            pre-install test scripts

        src_install() { }

            [Install](https://devmanual.gentoo.org/ebuild-writing/functions/src_install/index.html)
            the package

### Board Specific Packages

The following files are required for adding any specific drivers and tools to a
board.

virtual/chromeos-bsp/chromeos-bsp-2.ebuild

> `EAPI="6"`
> `DESCRIPTION="Chrome OS BSP virtual package"`
> `HOMEPAGE="`[`http://src.chromium.org`](http://src.chromium.org/)`"`
> `LICENSE="BSD"`
> `SLOT="0"`
> `KEYWORDS="arm"`
> `RDEPEND="chromeos-base/chromeos-bsp-<name>"`

    Replace the LICENSE value with your correct license

    Replace the KEYWORDS field with your board’s architecture (arm, mips, ppc,
    x86, etc)

    Replace &lt;name&gt; with the name of your board

*chromeos-base/chromeos-bsp-&lt;name&gt;/chromeos-bsp-&lt;name&gt;-0.0.1.ebuild*

*chromeos-base/chromeos-bsp-&lt;name&gt;/chromeos-bsp-&lt;name&gt;-0.0.1-r1.ebuild*

> `EAPI="6"`
> `DESCRIPTION="<name> bsp (meta package to pull in driver/tool dependencies)"`
> `LICENSE="BSD-Google"`
> `SLOT="0"`
> `KEYWORDS="arm"`
> `IUSE=""`
> `DEPEND=""`
> `RDEPEND=""`

    Replace the LICENSE value with your correct license

    Replace the KEYWORDS field with your board’s architecture (arm, mips, ppc,
    x86, etc)

    Replace &lt;name&gt; with the name of your board

    The ‘chromeos-bsp-&lt;name&gt;-0.0.1-r1.ebuild’ file is just a symbolic link
    to the ‘chromeos-bsp-&lt;name&gt;-0.0.1.ebuild’ file. Whenever changes are
    made to the ebuild file, the symlink should be revbumped/renamed, for
    instance incrementing ‘-r1’ to ‘-r2’.

    Adding any packages that currently have ebuilds in the source tree should be
    added to the DEPEND and RDEPEND fields. The DEPEND field specifies
    build-time dependencies that will not be installed into the build image. The
    RDEPEND field specifies run-time dependencies that will be installed into
    the build image.

    Other source can be built and installed by adding
    [src_prepare](https://devmanual.gentoo.org/ebuild-writing/functions/src_prepare/index.html)(),
    [src_configure](https://devmanual.gentoo.org/ebuild-writing/functions/src_configure/index.html)(),
    [src_compile](https://devmanual.gentoo.org/ebuild-writing/functions/src_compile/index.html)(),
    [src_test](https://devmanual.gentoo.org/ebuild-writing/functions/src_test/index.html)(),
    and
    [src_install](https://devmanual.gentoo.org/ebuild-writing/functions/src_install/index.html)()
    functions to do the respective work.
