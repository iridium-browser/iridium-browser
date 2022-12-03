# Open Screen Library

The Open Screen Library implements the Open Screen Protocol, Multicast DNS and
DNS-SD, and the Chromecast protocols (discovery, application control, and media
streaming).

The library consists of feature modules that share a [common platform
API](platform/README.md) that must be implemented and linked by the embedding
application.

The major feature modules in the library can be used independently and have
their own documentation:

  * [Cast protocols](cast/README.md) (aka `libcast`)
  * [Open Screen Protocol](osp/README.md)
  * [Multicast DNS and DNS-SD](discovery/README.md)

# Getting the code

## Installing depot_tools

Library dependencies are managed using `gclient`, from the
[depot_tools](https://www.chromium.org/developers/how-tos/depottools) repo.

To get gclient, run the following command in your terminal:
```bash
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
```

Then add the `depot_tools` folder to your `PATH` environment variable.

Note that openscreen does not use other features of `depot_tools` like `repo` or
`drover`.  However, some `git-cl` functions *do* work, like `git cl try`,
`git cl format`, `git cl lint`, and `git cl upload.`

## Checking out code

From the parent directory of where you want the openscreen checkout (e.g.,
`~/my_project_dir`), configure `gclient` and check out openscreen with the
following commands:

```bash
    cd ~/my_project_dir
    gclient config https://chromium.googlesource.com/openscreen
    gclient sync
```

The first `gclient` command will create a default .gclient file in
`~/my_project_dir` that describes how to pull down the `openscreen` repository.
The second command creates an `openscreen/` subdirectory, downloads the source
code, all third-party dependencies, and the toolchain needed to build things;
and at their appropriate revisions.

## Syncing your local checkout

To update your local checkout from the openscreen reference repository, just run

```bash
   cd ~/my_project_dir/openscreen
   git pull
   gclient sync
```

This will rebase any local commits on the remote top-of-tree, and update any
dependencies that have changed.

# Build setup

The following are the main tools are required for development/builds.

- Installed by gclient automatically
  - Build file generator: `gn` (installed into `buildtools/`)
  - Code formatter: `clang-format` (installed into `buildtools/`)
  - Builder: `ninja`
  - Compiler/Linker: `clang`
- Installed by you
  - JSON validator: `yajsv`
  - `libstdc++`
  - `gcc`
  - XCode

## yajsv installation

1. Install `go` from [https://golang.org](https://golang.org) or your Linux package manager.
2. `go install github.com/neilpa/yajsv@latest`

## libstdc++ (Linux only)

Ensure that libstdc++ 8 is installed, as clang depends on the system
instance of it. On Debian flavors, you can run:

```bash
   sudo apt-get install libstdc++-8-dev libstdc++6-8-dbg
```

## XCode (Mac only)

On Mac OS X, the build will use the clang provided by
[XCode](https://apps.apple.com/us/app/xcode/id497799835?mt=12). You can install
the XCode command-line tools only or the full version of XCode.

```bash
xcode-select --install
```

TODO(https://issuetracker.google.com/202964797): Switch to use Chromium clang for Mac builds.

##  gcc (optional, Linux only)

Setting the `gn` argument `is_gcc=true` on Linux enables building using gcc
instead.

```bash
  mkdir out/debug-gcc
  gn gen out/debug-gcc --args="is_gcc=true"
```

Note that g++ version 7 or newer must be installed.  On Debian flavors you can
run:

```bash
  sudo apt-get install gcc-7
```

## Debug build

Setting the `gn` argument `is_debug=true` enables debug build.

```bash
  gn gen out/debug --args="is_debug=true"
```

## gn configuration

Running `gn args` opens an editor that allows to create a list of arguments
passed to every invocation of `gn gen`.  `gn args --list` will list all of the
possible arguments you can set.

```bash
  gn args out/debug
```

# Building targets

We use the Open Screen Protocol demo application as an example, however, the
instructions are essentially the same for all executable targets.

``` bash
  mkdir out/debug
  gn gen out/debug             # Creates the build directory and necessary ninja files
  ninja -C out/debug osp_demo  # Builds the executable with ninja
  ./out/debug/osp_demo         # Runs the executable
```

The `-C` argument to `ninja` works just like it does for GNU Make: it specifies
the working directory for the build.  So the same could be done as follows:

``` bash
  ./gn gen out/debug
  cd out/debug
  ninja osp_demo
  ./osp_demo
```

After editing a file, only `ninja` needs to be rerun, not `gn`.  If you have
edited a `BUILD.gn` file, `ninja` will re-run `gn` for you.

We recommend using `autoninja` instead of `ninja`, which takes the same
command-line arguments but automatically parallelizes the build for your system,
depending on number of processor cores, amount of RAM, etc.

Also, while specifying build targets is possible while using ninja, typically
for development it is sufficient to just build everything, especially since the
Open Screen repository is still quite small. That makes the invocation to the
build system simplify to:

```bash
  autoninja -C out/debug
```

For details on running `osp_demo`, see its [README.md](osp/demo/README.md).

## Building all targets

Running `ninja -C out/debug gn_all` will build all non-test targets in the
repository.

`gn ls --type=executable out/debug` will list all of the executable targets
that can be built.

If you want to customize the build further, you can run `gn args out/debug` to
pull up an editor for build flags. `gn args --list out/debug` prints all of
the build flags available.

## Building and running unit tests

```bash
  ninja -C out/debug openscreen_unittests
  ./out/debug/openscreen_unittests
```

# Contributing changes

Open Screen library code should follow the [Open Screen Library Style
Guide](docs/style_guide.md).

This library uses [Chromium Gerrit](https://chromium-review.googlesource.com/) for
patch management and code review (for better or worse).  You will need to register
for an account at `chromium-review.googlesource.com` to upload patches for review.

The following sections contain some tips about dealing with Gerrit for code
reviews, specifically when pushing patches for review, getting patches reviewed,
and committing patches.

# Uploading a patch for review

The `git cl` tool handles details of interacting with Gerrit (the Chromium code
review tool) and is recommended for pushing patches for review.  Once you have
committed changes locally, simply run:

```bash
  git cl format
  git cl upload
```

The first command will will auto-format the code changes using
`clang-format`. Then, the second command runs the `PRESUBMIT.py` script to check
style and, if it passes, a newcode review will be posted on
`chromium-review.googlesource.com`.

If you make additional commits to your local branch, then running `git cl
upload` again in the same branch will merge those commits into the ongoing
review as a new patchset.

It's simplest to create a local git branch for each patch you want reviewed
separately.  `git cl` keeps track of review status separately for each local
branch.

## Addressing merge conflicts

If conflicting commits have been landed in the repository for a patch in review,
Gerrit will flag the patch as having a merge conflict.  In that case, use the
instructions above to rebase your commits on top-of-tree and upload a new
patchset with the merge conflicts resolved.

## Tryjobs

Clicking the `CQ DRY RUN` button (also, confusingly, labeled `COMMIT QUEUE +1`)
will run the current patchset through all LUCI builders and report the results.
It is always a good idea get a green tryjob on a patch before sending it for
review to avoid extra back-and-forth.

You can also run `git cl try` from the commandline to submit a tryjob.

## Code reviews

Send your patch to one or more committers in the
[COMMITTERS](https://chromium.googlesource.com/openscreen/+/refs/heads/master/COMMITTERS)
file for code review.  All patches must receive at least one LGTM by a committer
before it can be submitted.

## Submitting patches

After your patch has received one or more LGTM commit it by clicking the
`SUBMIT` button (or, confusingly, `COMMIT QUEUE +2`) in Gerrit.  This will run
your patch through the builders again before committing to the main openscreen
repository.

# Additional resources

* [Continuous builders](docs/continuous_build.md)
* [Building and running fuzz tests](docs/fuzzing.md)
* [Running on a Raspberry PI](docs/raspberry_pi.md)
* [Unit test code coverage](docs/code_coverage.md)
