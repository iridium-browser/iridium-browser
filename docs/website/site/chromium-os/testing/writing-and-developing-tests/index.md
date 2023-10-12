---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: writing-and-developing-tests
title: Writing and Developing Tests
---

[TOC]

For understanding and writing the actual python code for autotest, please refer
to the [Writing Tests
FAQ](/chromium-os/testing/writing-and-developing-tests/writing-tests-faq).

For understanding test deps, refer to the [Test Deps
FAQ](/chromium-os/testing/writing-and-developing-tests/test-deps-faq).

For understanding how to test your changes, refer to the [Testing Your Changes
FAQ](/chromium-os/testing/writing-and-developing-tests/testing-your-changes-faq).

### Writing a test

Currently, all code should be placed in a standard layout inside the
autotest.git repository, unless otherwise is necessary for technical reasons.
Regardless, the following text assumes that code is placed in generally any
repository.
For a test to be fully functional in Chromium OS, it has to be associated with
an ebuild. It is generally possible to run tests without an ebuild, either using
run_remote_tests --build or calling autoserv directly with the correct options,
but discouraged, as the same will not function with other parts of the system.

### Making a new test work with ebuilds

The choice of ebuild depends on the location of its sources. Structuring tests
into more smaller ebuilds (as opposed to one ebuild per source repository)
serves two purposes:
- Categorisation - Grouping similar tests together, possibly with deps they use
exclusively
- Parallelisation - Multiple independent ebuilds can build entirely in parallel
- Dependency tracking - Larger bundles of tests depend on more system packages
without proper resolution which dependency belongs to which test. This also
increases paralellism.
Current ebuild structure is largely a result of breaking off the biggest
blockers for parallelism, ie. tests depending on chrome or similar packages, and
as such, using any of the current ebuilds should be sufficient. (see FAQ for
listing of ebuilds)
After choosing the proper ebuild to add your test into, the test (in the form
“+tests_&lt;testname&gt;”) needs to be added to IUSE_TESTS list that all
autotest ebuilds have. Failing to do so will simply make ebuilds ignore your
tests entirely. As with all USE flags, prepending it with + means the test will
be enabled by default, and should be the default, unless you want to keep the
test experimental for your own use, or turn the USE flag on explicitly by other
means, eg. in a config for a particular board only.
Should a new ebuild be started, it should be added to chromeos-base/autotest-all
package, which is a meta-ebuild depending on all autotest ebuild packages that
can be built. autotest-all is used by the build system to automatically build
all tests that we have and therefore keep them from randomly breaking.

### Deps

Autotest uses deps to provide a de-facto dependencies into the ecosystem. A dep
is a directory in ‘client/deps’ with a structure similar to a test case without
a control file. A test case that depends on a dep will invoke the dep’s setup()
function in its own setup() function and will be able to access the files
provided by the dep. Note that autotest deps have nothing to do with system
dependencies.
As the calls to a dep are internal autotest code, it is not possible to
automatically detect these and make them an inter-package dependencies on the
ebuild level. For that reason, deps should either be provided by the same ebuild
that builds test that consume them, or ebuild dependencies need to be declared
manually between the dep ebuild and the test ebuild that uses it. An
autotest-deponly eclass exists to provide solution for ebuilds that build only
deps and no tests. A number of deponly ebuilds already exist.
Common deps are:
- chrome_test - Intending to use any of the test binaries produced by chrome.
- pyauto_dep - Using pyauto for your code.

### Working on a test

As mentioned, while working on a test, it is necessary to always propagate any
source changes using the emerge step. This can be either a manual emerge
(instant) or `cros build-packages` (longer, rebuilds all dependencies as well).
For cros-workon ebuilds, this means first starting to work on all ebuilds that
are affected. ([Which ebuilds are
these?](/chromium-os/testing/building-and-running-tests#TOC-Q5:-I-m-working-on-some-test-sources-how-do-I-know-which-ebuilds-to-cros_workon-start-in-order-to-properly-propagate-)):

```none
$ cros_workon --board=${board} start <list of ebuilds>
```

As described, all autotest ebuilds have a selectable list of tests, most of
which can be disabled and only the one test that is being worked on can be built
selectively, saving time.

```none
$ TESTS=testname emerge-${board} <list of ebuilds>
$ run_remote_tests.sh --remote=<ip> testname
```

Note: If tests have interdependencies not handled via deps, those cannot be
automatically detected, and you have to include all tests that are being
depended on. An example of that would be an old-style test suite, which is
really just a control file calling other tests selectively via hardcoded names.
In order to make that work, you have to determine the list of tests accordingly,
before you run the suite.

### Anatomy of test.test

This section describes the pieces of test.test that are available to use,
describing what each does and when it could be used.

#### Attributes

job backreference to the job this test instance is part of

outputdir eg. results/&lt;job&gt;/&lt;testname.tag&gt;

resultsdir eg. results/&lt;job&gt;/&lt;testname.tag&gt;/results

profdir eg. results/&lt;job&gt;/&lt;testname.tag&gt;/profiling

debugdir eg. results/&lt;job&gt;/&lt;testname.tag&gt;/debug

bindir eg. tests/&lt;test&gt;

src eg. tests/&lt;test&gt;/src

tmpdir eg. tmp/&lt;tempname&gt;_&lt;testname.tag&gt;

#### initialize()

Everytime a test runs this function runs. For example, if you have
iterations=100 set this will run 100 times. Use this when you need to do
something like ensure a package is there.

#### setup()

setup() runs the first time the test is set up. This is often used for compiling
source code one should not need to recompile the source for the test for every
iteration or install the same

#### run_once()

This is called by job.run_test N times, where N is controlled by the interations
parameter to run_test (Defaulting to one). it also gets called an additional
time with profilers turn on, if any profilers are enabled.

#### postprocess_iteration()

This processes any results generated by the test iteration and writes them out
into a keyval. It's generally not called for the profiling iteration, as that
may have difference performance implications.

#### warmup()

### Test naming conventions

(TODO) We don’t have any. These need to be defined and documented here.
