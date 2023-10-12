---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: chromium-modularization
title: Chromium Modularization
---

[TOC]

## Background

Chromium is a big project, and it makes sense to modularize. Many sub-teams work
largely independently, such as the sandbox and V8 teams, and the WebKit code is
merged on regular schedules. It makes sense to modularize such that these teams
can work independently, yet still share common code.

For an overview of the modules that exist, please see [Chromium code
layout](/developers/how-tos/getting-around-the-chrome-source-code).

## Quick start

Say you need to make a change that adds or changes something in `base` for the
benefit of something in `chrome`.

First you make your change, get it reviewed and checked in. At this point,
Chromium is still pulling a specific, older revision of base without your
changes. It does this by pulling in a specific version of `webkit`, and then
specifying that it should use the version of base that the `webkit` module is
currently using (see the "From" syntax below). Nobody will see your change
because of this.

Then you update the `webkit` module to pull in your new version of `base`. First
you need to figure out which version your new version is. It's a good idea to
first do `svn update` in `base/` to make sure everything is up-to-date, then do
`svn info`. You'll see a `Revision` line that tells you the current revision
number of the current module. Put this revision number into the `webkit/DEPS`
file for the `base` module. Get this change reviewed and submitted.

At this point, `chrome` is still pulling a specific version of `webkit` that
pulls `base` without your change. Repeat the same procedure as before, but this
time, make `chrome` pull the latest version of `webkit`. Along with this deps
file changes should go your corresponding changes to Chromium. This allows you
to atomically pull in the most recent version and update Chromium to be
compatible with it.

Dependencies

Each project depends on a variety of modules. For example, the WebKit project
depends on the networking code, our shared base code, and some third-party
libraries. Chromium and V8 in turn both depend on WebKit. (In practice, Chromium
depends on everything since we are in the Chromium tree). Each project must list
the projects it depends on, and which version it wants to pull. We do not
support transitive dependencies, so each project must list all of the projects
it depends on.

A user's *client* specifies a set of target *solutions* to check out. (Yes,
please excuse the Visual Studio jargon.) In the simplest case, a user's client
only specifies a single solution, say the `chrome` solution. The root directory
of each solution has a text file named `DEPS` that defines the set of
dependencies for the project. Users can override these dependencies if desired.

The contents of the `DEPS` file is a python associative array, which looks
something like the following:

deps = {
"breakpad" : "http://google-breakpad.googlecode.com/svn/trunk@189",
"webkit" : "http://src.chromium.org/chrome/trunk/src/webkit@3395",
"v8" : "http://v8.googlecode.com/svn/trunk@77829",
...
}

The `DEPS` file can easily be used to express a dependency on a subversion tag
and other subversion servers:

deps = {
...
"breakpad" : "http://google-breakpad.googlecode.com/svn/tags/1.0.29.0",
...
}

While it is possible for a `DEPS` file to specify that the trunk of a dependency
be used, it is intended that a `DEPS` file instead specify a known good revision
number or tag. This ensures that Chromium developers, for example, are insulated
from activity on the trunk of a dependency. When the maintainer(s) of the
dependency decide to make Chromium use the new revision of the dependency, they
just need to contribute a change to the `chrome/DEPS` file.

### The "From" keyword

Given that dependencies are not computed recursively, it can be a pain to
maintain complex dependency trees manually, especially if modules have multiple
overlapping dependencies. To simplify such situations, `gclient` supports the
`From` keyword which can be used to express a dependency in terms of the `DEPS`
file of another module. For example, you could have:

deps = {
"breakpad" : "http://google-breakpad.googlecode.com/svn/trunk@189",
"webkit" : "http://src.chromium.org/chrome/trunk/src/webkit@3395",
"v8" : From("webkit"),
...
}

The use of the `From` keyword above indicates that the version of "v8" to be
pulled should be determined by inspecting `webkit/DEPS`.

## Test Data

Some modules like `webkit` have very large amounts of test data. For these
modules, a good convention is to define a separate module for the test data. In
the case of the `webkit` module, `webkit/DEPS` can define a dependency on the
webkit test data module, but `chrome/DEPS` can exclude this dependency. As a
result, Chromium developers are exempted from having to checkout the WebKit test
data by default, but this also does not inconvenience WebKit developers who
prefer to check out the test data. Also, by having the test data in a parallel
directory, the cost of manually updating just the webkit directory is minimized.

## Tooling

To facilitate working with a bunch of separately versioned modules, some tooling
is needed.

### Installation

Checkout the `depot_tools` package, which includes `gclient`, `gcl`, and `svn`:
`$ `git clone https://chromium.googlesource.com/chromium/tools/depot_tools

Add the `depot_tools` directory to your PATH.

### Basic Usage

To checkout Chromium the first time, follow [these
instructions](/developers/how-tos/get-the-code).

### Advanced Usage

The contents of a default `.gclient` file looks something like:

solutions = \[
{ "name" : "chrome",
"url" : "http://src.chromium.org/chrome/trunk/src/chrome",
"custom_deps" : {}
}
\]

An element of the `solutions` array (a *solution*) describes a repository
directory that will be checked out into your working copy. Each solution may
optionally define additional dependencies (via its `DEPS` file) to be checked
out alongside the solution's directory. A solution may also specify custom
dependencies (via the `custom_deps` property) that override or augment the
dependencies specified by the `DEPS` file.

Users can edit this file to add new solutions or alter dependencies for a
particular solution.

For example, a V8 developer may wish to checkout the V8 trunk alongside a stable
version of Chromium. So, they might setup a `.gclient` file like so:

solutions = \[
{ "name" : "chrome",
"url" : "http://src.chromium.org/chrome/trunk/src/chrome@5000",
"custom_deps" : {
"v8" : "http://v8.googlecode.com/svn/trunk"
}
}
\]

The above specifies that a particular revision of Chromium, r5000, should be
used instead of the Chromium trunk. It then specifies that the V8 trunk should
be used instead of the version of V8 specified by `chrome/DEPS`.

The V8 example is somewhat simple since V8 does not itself have other
dependencies. For modules like webkit, specifying custom dependencies for each
of WebKit's dependencies is tedious and could potentially get out of sync with
what other developers are using (as specified in `webkit/DEPS`). So, an
alternative approach to working with the trunk of another module is to set up a
second solution:

solutions = \[
{ "name" : "chrome",
"url" : "http://src.chromium.org/chrome/trunk/src/chrome@5000",
"custom_deps" : {
"skia" : None,
"webkit" : None,
"v8" : None
}
},
{ "name" : "webkit",
"url" : "http://src.chromium.org/chrome/trunk/src/webkit",
"custom_deps" : {}
}
\]

The above `.gclient` file specifies that the svn URLs given in `chrome/DEPS` for
the skia, webkit, and v8 modules should be ignored. Those modules are then
fetched according to `webkit/DEPS`. Both `chrome/DEPS` and `webkit/DEPS`, in
this example, specify other common dependencies such as `base`. The tool will
complain if the two solutions specify conflicting dependencies. A user must
explicitly ignore a dependency that conflicts.
