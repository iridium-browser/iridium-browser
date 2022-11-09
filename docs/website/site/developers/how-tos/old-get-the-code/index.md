---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: old-get-the-code
title: The old instructions for getting the code
---

## *This page is obsolete. Please see [Get the Code: Checkout, Build, & Run Chromium](/developers/how-tos/get-the-code) instead.*

## Prerequisites

Chromium supports building on Windows, Mac and Linux host systems. Linux is
required for building Android, and a Mac is required for building iOS.

### Platform-specific requirements

This page documents common checkout and build instructions. There are
platform-specific pages with additional information and requirements:

*   [Linux](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/linux_build_instructions.md)
*   [Windows](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/windows_build_instructions.md)
*   [Mac](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/mac_build_instructions.md)
*   [ChromeOS](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/chromeos_build_instructions.md)
*   [iOS](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/ios_build_instructions.md)
*   [Cast](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/cast_build_instructions.md)
*   [Android](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/android_build_instructions.md)

### Set up your environment

Check out and install the [depot_tools
package](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up).
This contains the custom tools necessary to check out and build.

Create a chromium directory for the checkout and change to it (you can call this
whatever you like and put it wherever you like, as long as the full path has no
spaces):

```none
mkdir chromium
cd chromium
```

## Check out the source code

Use the "fetch" tool that came with depot_tools:

```none
fetch chromium    # [fetch --no-history chromium]
cd src    # All other commands are executed from the src/ directory.
```

Use `--no-history` if you don't need repo history and want a faster checkout.
Expect a checkout to take at least 30 minutes on fast connections, and many
hours on slower connections.

### Post-sync hooks

Some platform-specific pages (linked above) may have extra instructions. In
particular, on Ubuntu Linux run:

```none
./build/install-build-deps.sh
```

**Optional:** [install API keys](/developers/how-tos/api-keys) which allow your
build to use certain Google services. This isn't necessary for most development
and testing purposes.

Run hooks to fetch everything needed for your build setup.

```none
gclient runhooks
```

### Update the checkout

To sync to newer versions of the code (not necessary the first time), run the
following in your src/ directory:

```none
git rebase-update
gclient sync
```

The first command updates the primary Chromium source repository and rebases
your local development branches on top of tip-of-tree. The second command
updates all of the dependencies specified in the DEPS file. See also "More help
managing your checkout" below.

## Setting up the build

GN is our meta-build system. It reads build configuration from `BUILD.gn` files
and writes Ninja files to your build directory. To create a GN build directory:

```none
gn gen out/Default
```

*   You only have to do this once, it will regenerate automatically when
            you build if the build files changed.
*   You can replace `out/Default` with another name inside the `out`
            directory.
*   To specify build parameters for GN builds, including release
            settings, see [GN build
            configuration](/developers/gn-build-configuration). The default will
            be a debug component build matching the current host operating
            system and CPU.
*   For more info on GN, run `gn help` on the command line or read the
            [quick start
            guide](https://chromium.googlesource.com/chromium/src/+/HEAD/tools/gn/docs/quick_start.md).

## Building

Build Chromium (the "chrome" target) with Ninja using the command:

```none
ninja -C out/Default chrome
```

List all GN targets by running `gn ls out/Default` from the command line. To
compile one, pass to Ninja the GN label with no preceding "//" (so for
`//ui/display:display_unittests` use `ninja -C out/Default
ui/display:display_unittests`).

## Running

You can run chrome with (substituting "Default" with your build directory):

*   Linux/ChromeOS: `out/Default/chrome`
*   Windows: `out\Default\chrome.exe`
*   Mac: `out/Default/Chromium.app/Contents/MacOS/Chromium`
*   [Android](/developers/how-tos/android-build-instructions)
*   [iOS](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/ios_build_instructions.md)

### Running tests

Run the test targets listed above in the same manner. You can specify only a
certain set of tests to run using `--gtest_filter`, e.g.

```none
out/Default/unit_tests --gtest_filter="PushClientTest.*"
```

You can find out more about GoogleTest on the [GoogleTest wiki
page](https://github.com/google/googletest)

## Quick start to submit a patch

See [contributing code](/developers/contributing-code) for a more in-depth
guide.

<pre><code>git checkout -b my_patch
<i>...write, compile, test...</i>
git commit -a
git cl upload
</code></pre>

The `git cl upload` command will upload your code review to
[codereview.chromium.org](https://codereview.chromium.org/) for review.

*   Add reviewers and submit your code for review by clicking on
            "Publish+Mail Comments" (you can leave the mail message empty).
*   If you have try bot access, you can click "CQ dry run" which will
            compile and run the tests.
*   Once your patch has been reviewed and marked LGTM ("Looks Good To
            Me") by an authorized reviewer, click the "Commit" checkbox below
            the patch to submit to the commit queue.

### More help managing your checkout

*   [Depot tools
            manpage](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools.html)
            and
            [tutorial](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html).
*   [Working with release
            branches](/developers/how-tos/get-the-code/working-with-release-branches).
*   [Working with nested
            repositories](/developers/how-tos/get-the-code/working-with-nested-repos).
*   [Commit or revert changes manually](/system/errors/NodeNotFound).
*   [git-drover](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/git-drover.html)
            for merging changes to release branches.

## Getting help

*   Ensure your checkout has been properly updated (`git
            rebase-update`).
*   Check you're on a stable, unmodified branch from master (`git
            map-branches`).
*   Check you have no uncommitted changes (`git status`).
*   Join the `#chromium` IRC channel on `irc.freenode.net` (see the [IRC
            page](/developers/irc) for more).
*   Join the [chromium-dev Google
            Group](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-dev),
            and other [technical discussion
            groups](/developers/technical-discussion-groups). **These are not
            support channels for Chrome itself but forums for developers.**
*   If you think there is an infrastructure problem that affects many
            developers, file [a new bug with the label
            'Infra'](https://bugs.chromium.org/p/chromium/issues/entry?template=Build%20Infrastructure).
            It will be looked at by our infrastructure team.
*   If you work at Google, check out the [Googler-specific Chrome
            documentation](http://wiki/Main/ChromeBuildInstructions).
