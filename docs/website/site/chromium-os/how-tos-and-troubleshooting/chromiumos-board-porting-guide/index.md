---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: chromiumos-board-porting-guide
title: Chromium OS Board Porting Guide
---

[TOC]

## Introduction

You've got a wonderful new platform that you'd like to run Chromium OS on, but
have no idea how to get there from here? Fret not my fellow traveler, for this
document shall be your guide. We promise it'll be a wondrous and enlightening
journey.

An overlay is the set of configuration files that define how the build system
will build a Chromium OS disk image. Configurations will be different depending
on the board and architecture type, and what files a developer wants installed
on a given disk image. Every command in the build process takes a required
\`--board\` parameter, which developers should pass the overlay name to.

On your first run through this document for creating your first platform, you
can skip the sections labeled *(advanced)*. This will let you focus on the
minimum bits to get running, and then you can revisit once you're more
comfortable with things.

### Private Boards *(advanced)*

This guide covers doing a public build (one that will be released to the world).
There is a [separate
guide](/chromium-os/how-tos-and-troubleshooting/chromiumos-board-porting-guide/private-boards)
(meant as an add-on to this one) for creating a private board. Setting up a
private board is useful for when you want to keep your project details a secret
before launch, or if your board contains proprietary drivers or firmware that
can't be released publicly.

## Naming

Possible board names follow a simple specification defined below. Note that this
should generally be followed for any new project, as such this encompasses
device and model names also, in general any new names that go into the Chromium
OS code base.

*   must have no upper case letters
    *   normalization to lower case keeps things simple
*   must start with a letter \[a-z\]
*   may contain as many letters \[a-z\] or numbers \[0-9\] as you like,
            but shorter is preferable, people have to type it often
*   should contain no more than one dash
    *   technically there is no limit, but the more dashes you add, the
                uglier it gets
*   must not contain any other character
    *   e.g. characters such as, but not limited to, + or % or @ or _ or
                / or . or : or ... are invalid
*   must avoid words that are used with generic (board independent)
            configurations
    *   e.g. "release" or "canary" or "firmware" or "factory" or "pfq"
                or "paladin" or "incremental" or "asan" or ...
    *   see
                [cbuildbot_config.py](http://git.chromium.org/gitweb/?p=chromiumos/chromite.git;a=blob;f=buildbot/cbuildbot_config.py)
                in the chromite repo for more examples (look at the CONFIG
                constants at the top)
        *   you can also run `` `cbuildbot -l -a` `` to get a feel for
                    the "group" names
*   should not be generic terms
    *   e.g. "board" or "boat" or "car" or "airplane" or "computer" or
                ...
*   should not be a common name
    *   e.g. "alex" or "bob" or "caroline" are examples, as there are
                real people involved with the projects with these names it makes
                for awkward conversations about the board
*   should be a noun, not an adjective or adverb
*   should be a playful name
    *   avoid terms with negative connotations
*   should not be very common in the code base to make for easy grepping
*   should be clever
*   must not be phonetically similar to an existing board
    *   it is not worth the hassle when talking or writing to try and
                figure out what people mean
    *   it might make it harder for non-native speakers to differentiate
    *   e.g. with "red" and "read", or "pour" and "pored", or "accept"
                and "except", or "flaunt" and "flout"
*   should be pronounceable in conversation
    *   no one likes to talk about the XK123A-Z03 board -- that's just
                boring
*   must not pick a name that Google intends to use with a Chrome OS
            board
    *   don't worry, it's unlikely
*   should not pick a name which would be easily confused with another
            project
    *   e.g. using a name from an Android project (e.g. mako), or a sub
                component of the OS (e.g. ash) is not advisable

Settled on a name? Hopefully it's nifty.

## Bare Framework

Let's start by laying the ground work for the board. We won't worry about the
fine details (like custom set of packages or flags or ...) at this point. We
just want a board that the build system will recognize and be able to produce a
generic set of artifacts. Once we have that, we'll start customizing things.

### Architecture

Be aware that we assume your board falls under one of the main (currently
supported) architectures:

*   amd64 (x86_64) -- 64-bit Intel/AMD processors
*   arm -- 32-bit ARM processors
*   arm64 -- 64-bit ARM processors
*   x86 (i386/i486/i586/i686) -- 32-bit Intel/AMD/etc... processors

If you're using a different architecture, please see the [Chromium OS
Architecture Porting
Guide](/chromium-os/how-tos-and-troubleshooting/chromiumos-architecture-porting-guide)
first.

### src/overlays/overlay-$BOARD/

This is the main location for per-board customization. Let's start off with a
simple overlay:

```none
overlay-$BOARD/
|-- metadata/
|   `-- layout.conf          # Gentoo repo syntatic sugar
|-- profiles/
|   `-- base/
|       |-- make.defaults    # Board customizations (USE/LINUX_FIRMWARE/etc...)
|       `-- parent           # Parent profile for this board (common arch/OS settings)
`-- toolchain.conf           # Toolchains needed to compile this board
```

Most of these files are one or two lines. Let's go through them one at a time.

#### toolchain.conf

For standard architectures, you only need one line here. Pick the one that
matches the architecture for your board.

<table>
<tr>
<td> <b>architecture</b></td>
<td> <b>tuple</b></td>
</tr>
<tr>
<td> amd64 (64-bit)</td>
<td> x86_64-cros-linux-gnu</td>
</tr>
<tr>
<td> arm (armv7)</td>
<td> armv7a-cros-linux-gnueabihf</td>
</tr>
<tr>
<td> arm64 (armv8/aarch64)</td>
<td> aarch64-cros-linux-gnu</td>
</tr>
<tr>
<td> x86 (32-bit)</td>
<td> i686-pc-linux-gnu</td>
</tr>
</table>

Example file for an x86_64 board:

```none
$ cat toolchain.conf
x86_64-cros-linux-gnu
```

Note that if you do need more than one toolchain, you can list as many as you
like (one per line). But the first entry must be the default one for your board.

#### metadata/layout.conf

Don't worry about the content of this file. Simply copy & paste what you see
below, and then update the `repo-name` field to match your $BOARD.

<pre><code>$ cat metadata/layout.conf
masters = portage-stable chromiumos eclass-overlay
profile-formats = portage-2
repo-name = <b>lumpy</b>
thin-manifests = true
use-manifests = strict
</code></pre>

#### profiles/base/

This is the "base" profile (aka the default) for your board. It allows you to
customize a wide array of settings.

This is where the majority of board customization happens -- USE customizations
across all packages in `make.defaults`, or USE and KEYWORDS on a per-package
basis (via the `package.use` and `package.keywords` files respectively). See the
portage(5) man page for all the gory details.

By using the profile, stacking behavior across other profiles is much easier to
reason about (when talking about chipsets, baseboards, projects, etc...).

#### profiles/base/make.defaults

This file can be used to customize global settings for your board. In this first
go, we'll use a standard form. New content later on should be appended to this.

```none
$ cat profiles/base/make.defaults
# Initial value just for style purposes.
USE=""
LINUX_FIRMWARE=""
# Custom optimization settings for C/C++ code.
BOARD_COMPILER_FLAGS=""
```

#### profiles/base/parent

This will vary based on your architecture.

<table>
<tr>
<td><b>architecture</b></td>
<td><b>parent profile</b></td>
</tr>
<tr>
<td> amd64</td>
<td>chromiumos:default/linux/*amd64*/10.0/chromeos</td>
</tr>
<tr>
<td> arm</td>
<td>chromiumos:default/linux/*arm*/10.0/chromeos</td>
</tr>
<tr>
<td> arm64</td>
<td>chromiumos:default/linux/*arm64*/10.0/chromeos</td>
</tr>
<tr>
<td> x86</td>
<td>chromiumos:default/linux/*x86*/10.0/chromeos</td>
</tr>
</table>

Example file for an x86_64 board:

```none
$ cat profiles/base/parent
chromiumos:default/linux/amd64/10.0/chromeos
```

#### profiles/&lt;sub-profile&gt;/ (*advanced)*

Sometimes you will want to take an existing board and try out some tweaks on it.
Perhaps you want to use a different kernel, or change one or two USE flags, or
use a different compiler settings (like more debugging or technology like ASAN).

To create a sub-profile, simply make a new directory under the profiles/ and
name it whatever you like. Many Google based systems have one called
`kernel-next` and we use this to easily test the new development kernel tree.

You should then create a `parent` file in there like so:

```none
$ cat profiles/kernel-next/parent
../base
```

This says the sub-profile will start off using the existing base profile (so you
don't have to duplicate all the settings you've already put into that
directory). From here, you can add any files like you would any other profile
directory.

Now to select this new profile, you use the `--profile` option when running
`setup_board`. You can either blow away the existing build dir (if one exists),
or tell the system to simply rewrite the configs to point to the new profile.
You'll have to make the decision using your knowledge of the profile (whether it
means a lot of changes or just swapping of one or two packages).

```none
# Recreate from scratch.
$ ./setup_board --profile=kernel-next --board=lumpy --force
# Or just rewrite the configs to use the new profile.
$ ./setup_board --profile=kernel-next --board=lumpy --regen_configs
```

### scripts/ (advanced)

This folder contains scripts used during `cros build-image` to tweak the
packages that are allowed on the final image. The scripts of interest here are

*   build_kernel_image.sh -- Sourced by the main build_kernel_image.sh
            script for board specific modifications.
*   board_specific_setup.sh -- Sourced by the main build_image.sh script
            for board specific modifications.
*   disk_layout.json -- Used by boards that need a different disk
            partition than the default one.

## Testing Initial Build

Since your board should be all set up and ready to go, let's test it.

```bash
$ cros build-packages --board=$BOARD
```

Those should both have worked and produced a generic build for that architecture
(using your $BOARD name). Let's set about customizing the build now.

Note: as you make changes below to your overlay, re-running
`cros build-packages` again will rebuild packages that have changed based on
your `USE` flags.  But other changes (like compiler settings or kernel configs)
will not trigger automatic rebuilds.  Only new package builds will use the new
settings.  Once you're happy with all your settings though, you can run
`cros build-packages` with the `--cleanbuild` flag, which will build everything
from scratch with the latest settings.

## make.defaults: Global Build Settings

This file contains a few key variables you'll be interested in. Since each of
these can be a large topic all by themselves, this is just an overview.

You can set variables like you would in a shell script (`VAR="value"`), as well
as expand them (`FOO="${VAR} foo"`). But do not try to use shell commands like
`if [...]; then` or sub-shells like `$(...)` or `` `...` `` as things will fail.

<table>
<tr>
<td><b>Setting</b></td>
<td><b>Meaning</b></td>
</tr>
<tr>
<td> CHROMEOS_KERNEL_SPLITCONFIG</td>
<td> The kernel defconfig to start with</td>
</tr>
<tr>
<td> CHROMEOS_KERNEL_ARCH</td>
<td> The kernel $ARCH to use (normally you do not need to set this)</td>
</tr>
<tr>
<td> USE</td>
<td> Global control of features (e.g. alsa or ldap or opengl or sse or ...)</td>
</tr>
<tr>
<td> INPUT_DEVICES</td>
<td> List of drivers used for input (e.g. keyboard or mouse or evdev or ...)</td>
</tr>
<tr>
<td> VIDEO_CARDS</td>
<td> List of drivers used for video output (e.g. intel or armsoc or nouveau or ...)</td>
</tr>
<tr>
<td> BOARD_COMPILER_FLAGS</td>
<td> The common set of compiler flags used to optimize for your processor (e.g. -mtune/-march)</td>
</tr>
<tr>
<td> CFLAGS</td>
<td> Compiler flags used to optimize when building C code</td>
</tr>
<tr>
<td> CXXFLAGS</td>
<td> Compiler flags used to optimize when building C++ code</td>
</tr>
<tr>
<td> LDFLAGS</td>
<td> Linker flags used to optimize when linking objects (normally you do not need to set this)</td>
</tr>
</table>

### Linux Kernel Settings

#### CHROMEOS_KERNEL_SPLITCONFIG

The kernel is automatically compiled & installed by the build system. In order
to configure it, you have to start with a defconfig file as the base (it can
later be refined by various USE flags). This value allows you to control exactly
that.

You can specify the relative path (to the root of the kernel tree) to your
defconfig. This is useful if you are using a custom kernel tree rather than the
official Chromium OS Linux kernel tree.

```none
$ grep CHROMEOS_KERNEL_CONFIG profiles/base/make.defaults
CHROMEOS_KERNEL_CONFIG="arch/arm/configs/bcmrpi_defconfig"
```

Or you can specify a Chromium OS config base. We have one for each major
platform/SoC that ships in an official Chrome OS device, and we have
architecture generic configs. You can find the full list by browsing the
[chromeos/config/ directory in the kernel
tree](https://chromium.googlesource.com/chromiumos/third_party/kernel/+/HEAD/chromeos/config/).
Unlike a defconfig, splitconfigs are much smaller fragments which start with a
common base and then enable/disable a few options relative to that.

```none
$ grep CHROMEOS_KERNEL_SPLITCONFIG profiles/base/make.defaults
CHROMEOS_KERNEL_SPLITCONFIG="chromeos-pinetrail-i386"
```

#### CHROMEOS_KERNEL_ARCH

The kernel build system normally detects what architecture you're using based on
your overall profile. For example, if you have an amd64 board overlay setup, the
build knows it should use ARCH=x86_64.

However, there arises edge cases where you want to run a kernel architecture
that is different from the userland. For example, say you wanted to run a 64-bit
x86_64 kernel, but you wanted to use a 32-bit i386 userland. If your profile is
normally setup for an x86 system, you can set this variable to x86_64 to get a
64-bit kernel.

```none
$ grep CHROMEOS_KERNEL_ARCH profiles/base/make.defaults
CHROMEOS_KERNEL_ARCH="x86_64"
```

### Global USE Feature Selection

One of the strengths of the Gentoo distribution is that you can easily control
general feature availability in your builds by changing your USE settings. For
example, if you don't want audio, you can disable alsa & oss. Or if you don't
want [LDAP](http://en.wikipedia.org/wiki/LDAP), you can disable that. All of the
ebuild files (the package scripts used to build/install code) have logic to
check each setting that is optional so you don't have to.

The downside, as you can imagine, is that with thousands and thousands of
packages, the number of possible USE flags is vast. There are also some optional
settings which only make sense with one or two packages (global vs local USE
flags). So weeding through which USE flags exactly matter can be a bit of a
monstrous task.

### Userland Device Driver Selection

An extension to the USE flag system are so called USE-expanded variables. This
helps reduce the global USE flag noise a bit by having specially named variables
with specific meaning. In this case, we'll discuss device inputs
(keyboards/mouse/etc...) and video outputs.

The exact driver availability (and naming convention) depends on what graphic
system you intend to use. The X project tends to have the widest selection but
is a larger overall install size, while DirectFB tends to have smaller selection
of drivers but be much smaller. We'll focus on X here as that is the main system
that Chromium supports.

#### INPUT_DEVICES

TBD

#### VIDEO_DEVICES

TBD

## Global Compiler Settings

### Compiler Settings

Everyone likes optimizations. Faster is better, right? Here's the nuts and bolts
of it.

While picking out flags to use, keep in mind that Chromium OS uses
[LLVM/Clang](https://clang.llvm.org/) for its compiler suite. It also uses the
[gold linker](http://en.wikipedia.org/wiki/Gold_(linker)). So see the respective
documentation.

#### BOARD_COMPILER_FLAGS

You should put compiler optimizations that target your specific cpu into this
variable. Commonly, this means:

*   -march=&lt;arch&gt; -- see the [GCC
            manual](http://gcc.gnu.org/onlinedocs/gcc/Submodel-Options.html) for
            complete lists of possible values here
    *   This selects the minimum **required** processor/architecture
                that the code will run on
    *   *armv7-a* is common for ARMv7 parts
    *   *core2* is common for Intel Core2 parts
    *   *corei7* is common for Intel Core i7 parts
*   -mtune=&lt;arch&gt; -- see the [GCC
            manual](http://gcc.gnu.org/onlinedocs/gcc/Submodel-Options.html) for
            complete lists of possible values here
    *   This selects the processor that the code will be optimized best
                for without requiring it
    *   If you don't specify this, then the -march setting will be used
    *   *cortex-a8* or *cortex-a9* or *cortex-a15* are common for ARM
                parts
*   -mfpu=&lt;fpu&gt; (ARM only)
    *   Typical values are neon and vfpv3-d16
    *   If you have an ARMv7, pick between these two values
    *   If you don't have an ARMv7, then you should already know the
                answer to this question :)
*   -mfloat-abi=hard (ARM only)
    *   Keep in mind that Chromium OS assumes you are using the hard
                float ABI. While it is certainly possible to get things working
                with a soft float ABI, you shouldn't waste your time. Join us in
                the future and migrate away from the old & slow soft float ABI
                (this also includes the softfp ABI -- it's just as bad).
*   -mmmx / -msse / -msse2 / etc...
    *   There are a variety of machine-specific optimization flags that
                start with -m that you might want to try out

#### CFLAGS

If you have flags that you want to use only when compiling C code, use this
variable. Otherwise, things will default to `-O2 -g -pipe`.

```none
$ grep CFLAGS profiles/base/make.defaults
CFLAGS="-march=foo"
```

#### CXXFLAGS

If you have flags that you want to use only when compiling C++ code, use this
variable. Otherwise, things will default to `-O2 -g -pipe`.

```none
$ grep CXXFLAGS profiles/base/make.defaults
CXXFLAGS="-march=foo"
```

#### LDFLAGS

If you have flags that you want to send directly to the linker for all links,
use this variable.

These should take the form as given to the compiler driver. i.e. use
`-Wl,-z,relro` rather than `-z relro`.

You can find common settings in the [GNU Linker
manual](http://sourceware.org/binutils/docs/ld/Options.html).

## Buildbot Configs

Presumably you'd like to be able to easily (and cleanly) produce artifacts for
your board. In our [chromite
repo](https://chromium.googlesource.com/chromiumos/chromite), we have a tool
called
[cbuildbot](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/scripts/cbuildbot.py)
that takes care of all that for us. Normally it's run by
[buildbot](http://buildbot.net/) on a
[waterfall](http://build.chromium.org/p/chromiumos/waterfall) to produce all the
goodness, but there's no requirement that buildbot be the tool you use. You can
even [run it locally](/chromium-os/build/local-trybot-documentation).

### chromite/buildbot/cbuildbot_config.py

At any rate, you'll need to update this [master
file](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/cbuildbot/cbuildbot_config.py)
to add your new board configs. There are a few classes of configs to be familiar
with:

*   $BOARD-paladin: used by the [Chromium OS
            CQ](/system/errors/NodeNotFound) -- not needed if you aren't an
            official Chrome OS device
*   $BOARD-full: a public build of the build
*   $BOARD-release: a private build of the board
*   $BOARD-firmware: build just the firmware (bootloader/etc...) for the
            board
*   $BOARD-factory: build the factory install shim -- used to create an
            image for programming devices in the factory/RMA/etc...

The file should be largely self-documenting.

## Google Storage Integration

Sometimes you'll want to host large files or prebuilt binary packages for your
repo, but you don't want to give out access to everyone in the world. You can
great some boto config files, and the build scripts will automatically use them
when they're found.

### googlestorage_account.boto

Create this file in the root of the overlay. This file stores the access
credential (in cleartext) for the role account.

### googlestorage_acl.\*

These are the ACL files used by gsutil to apply access for people. Used whenever
you want to upload files and grant access to people who have the boto file.

The TXT format is documented in the [gsutil
FAQ](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/gsutil.md#installing-gsutil).
The JSON format is not yet used. Support for the XML format has been dropped.
