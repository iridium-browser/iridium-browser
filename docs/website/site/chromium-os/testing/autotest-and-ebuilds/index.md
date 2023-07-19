---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: autotest-and-ebuilds
title: Autotest and Ebuilds
---

[TOC]

## Useful documents

Autotest documentation on GitHub -
**<https://github.com/autotest/autotest/wiki/AutotestApi>**

This would be a good read if you want to familiarize yourself with the basic
Autotest concepts

Gentoo Portage ebuild/eclass Information -
<http://www.gentoo.org/proj/en/devrel/handbook/handbook.xml?part=2>

Getting to know the package build system we use.

ChromiumOS specific Portage FAQ -
<http://www.chromium.org/chromium-os/how-tos-and-troubleshooting/portage-build-faq>

Learning something about the way we use portage.

**## Autotest and ebuild workflow**

**To familiarize with autotest concepts, you should start with the upstream Autotest documentation at: <https://github.com/autotest/autotest/wiki/AutotestApi>**
**The rest of this document is going to use some terms and only explain them vaguely.**

**### Overview**

**At a high level, tests are organized in test cases, each test case being either server or client, with one main .py file named the same as the test case, and one or more control files. In order to be able to perform all tasks on a given test, autotest expects tests to be placed in a monolithic file structure of:**
**/client/tests/**
**/client/site_tests/**
**/server/tests/**
**/server/site_tests/**
**Each test directory has to have at least a control file, but typically also has a main job module (named the same as the test case). Furthermore, if it needs any additional files checked in, they are typically placed in a files/ directory, and separate projects that can be built with a Makefile inside the src/ directory.**
**Due to structural limitations in Chromium OS, it is not possible to store all test cases in this structure in a single large source repository as upstream autotest source would (placed at third_party/autotest/files/ in Chromium OS). In particular, the following has been required in the past:**
**- Having confidential (publicly inaccessible) tests or generally per-test ACLs for sharing only with a particular partner only.**
**- Storing test cases along with the project they wrap around, because the test requires binaries built as a by-product of the project’s own build system. (fe. chrome or tpm tests)**
**Furthermore, it has been desired to generally build everything that is not strongly ordered in parallel, significantly decreasing build times. That, however, requires proper dependency tree declaration and being able to specify which test cases require what dependencies, in addition to being able to process different “independent” parts of a single source repository in parallel.**
**This leads to the ebuild workflow, which generally allows compositing any
number of sources in any format into a single monolithic tree, whose contents
depend on build parameters.**

[<img alt="image"
src="/chromium-os/testing/autotest-user-doc/atest-diagram.png">](/chromium-os/testing/autotest-user-doc/atest-diagram.png)

This allows using standard autotest workflow without any change, however, unlike
what upstream does, the tests aren’t run directly from the source repository,
rather from a staging read-only install location. This leads to certain
differences in workflow:

    Source may live in an arbitrary location or can be generated on the fly.
    Anything that can be created as an ebuild (shell script) can be a test
    source. (cros-workon may be utilised, introducing a fairly standard Chromium
    OS project workflow)

*   The staging location (/build/${board}/usr/local/autotest/) may not
            be modified; if one wants to modify it, they have to find the source
            to it (using other tools, see FAQ).

    Propagating source changes requires an emerge step.

### Ebuild setup, autotest eclass

NOTE: This assumes some basic knowledge of how ebuilds in Chromium OS work.
Refer to for example
<http://www.chromium.org/chromium-os/how-tos-and-troubleshooting/portage-build-faq>
for some documentation.
An autotest ebuild is an ebuild that produces test cases and installs them into
the staging area. It has three general tasks:

*   Obtain the source - This is generally (but not necessarily) provided
            by ‘cros-workon’ eclass. It could also work with the more standard
            tarball SRC_URI pathway or generally any shell code executed in
            src_unpack().
*   Prepare test cases - This includes, but is not limited to
            preprocessing any source, copying source files or intermediate
            binaries into the expected locations, where they will be taken over
            by autotest code, specifically the setup() function of the
            appropriate test. Typically, this is not needed.
*   Call autotest to ‘build’ all sources and subsequently install them -
            This should be done exclusively by inheriting the autotest eclass,
            which bundles up all the necessary code to install into the
            intermediate location.

Autotest eclass is inherited by all autotest ebuilds, only requires a number of
variables specified and works by itself otherwise. Most variables describe the
locations and listings of work that needs to be done:

*   Location variables define the paths to directories containing the
            test files:

AUTOTEST_{CLIENT,SERVER}_{TESTS,SITE_TESTS}
AUTOTEST_{DEPS,PROFILERS,CONFIG}

These typically only need to be specified if they differ from the defaults
(which follow the upstream directory structure)

*   List variables (AUTOTEST_\*_LIST) define the list of deps,
            profilers, configs that should be handled by this ebuild.
*   IUSE test list specification TESTS=, is a USE_EXPANDed specification
            of tests managed by the given ebuild. By virtue of being an IUSE
            variable, all of the options are visible as USE flag toggles while
            building the ebuild, unlike with list variables which are a given
            and the ebuild has to be modified for those to change.

Each ebuild usually operates on a single source repository. That does not always
have to hold true, however, and in case of autotest, many ebuilds check out the
sources of the same source repository (*autotest.git*). Invariably, this means
that they have to be careful to not install the same files and split the sources
between themselves to avoid file install collisions.
If more than one autotest ebuild operates on the same source repository, they
have to use the above variables to define mutually exclusive slices in order to
not collide during installation. Generally, if we have a source repository with
client site_tests A and B, you can have either:

*   one ebuild with IUSE_TESTS=”+tests_A +tests_B”
*   two different ebuilds, one with IUSE_TESTS=”+tests_A”, the other
            with IUSE_TESTS=”+tests_B”

As soon as an overlap between ebuilds happens, either an outside mechanism has
to ensure the overlapping tests are never enabled at the same time, or file
collisions happen.
