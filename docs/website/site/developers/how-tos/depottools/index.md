---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: depottools
title: Using depot_tools
---

[TOC]

View the [**updated depot_tools documentation**
here.](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools.html)
These same docs are also available as man pages.
Not all the information on this page has been migrated to the man pages yet, so
this resource will stay around for a while, but where there are discrepancies,
the man pages should be considered authoritative.

The [depot_tools
tutorial](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html)
walks through a few key scenarios like managing branches.

## Introduction

Chromium uses a package of scripts, the depot_tools, to manage interaction with
the Chromium source code repository and the Chromium development process. It
contains the following utilities:

*   gclient: Meta-checkout tool managing both subversion and git
            checkouts. It is similar to [repo
            tool](https://gerrit.googlesource.com/git-repo) except that it works
            on Linux, OS X, and Windows and supports both svn and git. On the
            other hand, gclient doesn't integrate any code review functionality.
*   **gcl**: [Rietveld](https://github.com/rietveld-codereview/rietveld)
            code review tool for *subversion*. The gcl tool runs [presubmit
            scripts](/developers/how-tos/depottools/presubmit-scripts).
*   **git-cl**:
            [Rietveld](https://github.com/rietveld-codereview/rietveld) code
            review tool for *git*. The git-cl tool runs [presubmit
            scripts](/developers/how-tos/depottools/presubmit-scripts).
*   **svn** \[Windows only\]: [subversion
            client](https://subversion.apache.org/) for Chromium development.
            (Executable Subversion binaries are included in the depot_tools on
            Windows systems as a convenience, so that working with Chromium
            source code does not require a separate Subversion download.)
*   **drover**: Quickly revert svn commits.
*   **cpplint.py**: Checks for C++ style compliance.
*   **pylint**: Checks for Python style compliance.
*   **presubmit_support.py**: Runs PRESUBMIT.py presubmit checks.
*   **repo**: The repo tool.
*   **wtf**: Displays the active git branches in a chromium os checkout.
*   **weekly**: Displays the log of checkins for a particular developer
            since a particular date for git checkouts.
*   **git-gs**: Wrapper for git grep with relevant source types.
*   **zsh-goodies**: Completion for zsh users.

It is highly encouraged to look around and open the files in a text editor as
this page can quickly become outdated.

Please keep ~~this page~~ the man pages updated!

## Installing

See the
[depot_tools_tutorial](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up)
for set-up instructions.

## Help!

These tools don't have man pages but have integrated help! Try all of these
commands! If the doc is not adequate, send patches to fix them.

*   gclient help
    *   It works for subcommands too like: gclient help sync
*   git-cl help
    *   It works for subcommands too like: git-cl help patch

Otherwise, there are
[many](http://www.microsoft.com/resources/documentation/windows/xp/all/proddocs/en-us/app_notepad.mspx?mfr=true)
[great](https://www.vim.org/) [text](https://www.gnu.org/software/emacs/)
[editors](https://notepad-plus-plus.org/) that can help you out to read what the
tools are actually doing.

## gclient

`gclient` is a python script to manage a workspace of modular dependencies that
are each checked out independently from different subversion or git
repositories. Features are:

*   Dependencies can be specified on a per-OS basis.
*   Dependencies can be specified relative to their parent dependency.
*   Variables can be used to abstract concepts.
*   Hooks can be specified to be run after a checkout.
*   .gclient and DEPS are python scripts, you can hack in easily or add
            additional configuration data.

### .gclient file

It's the primary file. It is, in fact, a python script. It specifies the
following variables:

*   **solutions**: an array of dictionaries specifying the projects that
            will be fetched.
*   **hooks**: additional hooks to be run when this meta checkout is
            synced.
*   **target_os**: an optional array of (target) operating systems to
            fetch OS-specific dependencies for.
*   **cache_dir**: Primarily for bots, multiple working sets use a
            single git cache. See [gclient.py
            --cache-dir](https://code.google.com/p/chromium/codesearch#chromium/tools/depot_tools/gclient.py&sq=package:chromium&type=cs&q=%22parser.add_option('--cache-dir'%22&l=1865).

Additional variables are ignored.

Each project described in the solutions array can contain an optional DEPS file
that will be processed. The .gclient file is generated with `gclient config
<url>` or by hand. Each solutions entry is a dictionary that can contain the
following variables:

*   **name**: really, the path of the checkout.
*   **url**: the remote repository to fetch/clone.
*   **custom_deps**: (optional) override the dependencies specified in
            the deps and deps_os variables in child DEPS files. Useful when you
            want to fetch a writable checkout instead of the default read-only
            checkout of a dependency, e.g. you work on native_client from a
            chromium checkout.
*   **custom_vars**: (optional) override the variables defined in vars
            in child DEPS files. Example: override the WebKit version used for a
            chromium checkout.
*   **safesync_url**: (optional) url to fetch to retrieve the revision
            to sync this checkout to.

### DEPS file

A DEPS file specifies dependencies of a project. It is in fact a python script.
It specifies the following variables:

*   **deps**: a dictionary of child dependencies to fetch.
*   **deps_os**: a dictionary of OSes for OS-specific dependencies, each
            containing a dictionary of child dependencies to fetch.
*   **vars**: a dictionary of variables to define. Mainly useful to
            easily override a batch of revisions at once.
*   **hooks**: hooks to run after a sync.
*   **use_relative_paths**: relative paths should specify the checkout
            relative to this directory instead of the root gclient checkout.

Additional variables are ignored. Special keywords are:

*   **File()**: used for dependencies, specify to expect to checkout a
            file instead of a directory.
*   **From()**: used to fetch a dependency definition from another DEPS
            file, for chaining.
*   **Var()**: replace this string with a variable defined in vars or
            overridden.

### Pinned deps

Each dependency checkout URL can (and usually does) contain a revision number or
git hash, which means you're going to check out and build from that specific
revision of the module in question. We call that **pinned deps**. The advantage
is that you can build from a known working revision, even if it comes from a
completely different SCM repository or going back in time. The drawback is you
have to update the revision number(s) constantly, what we call **deps rolls**.

### DEPS examples

Chromium's
[src/DEPS](https://chromium.googlesource.com/chromium/src/+/HEAD/DEPS) is a
fairly complex example that will show all the possibilities of a DEPS file.

## Sending patches

[Contributing code](/developers/contributing-code) is done the same way as in
other Chromium repositories.

## Disabling auto update

The `gclient` and `git-cl` scripts are actually wrapper scripts that will, by
default, always update the `depot_tools` to the latest versions of the tools
checked in at
<https://chromium.googlesource.com/chromium/tools/depot_tools.git>. If for some
reason you wish to disable this auto-update behavior, either:

*   Set the environment variable `DEPOT_TOOLS_UPDATE=0`.
*   Remove `depot_tools/.git`. This may be appropriate if you choose to
            install depot_tools in a common location for use by multiple users
            (for example, `/usr/local/bin` on a Linux system).

Note: If you aren't using either of these helper scripts (e.g. you're developing
Chromium OS), then you will need to manually update depot_tools yourself from
time to time with a simple: git pull

### Caveat

Chromium engineers expect the auto-updating behavior of depot_tools, checkout or
presubmit breakage may ensue.
