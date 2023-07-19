---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/test-code-labs
  - Test Code Labs
page_name: dynamic-suite-codelab
title: Creating and deploying Chromium OS Dynamic Test Suites
---

[TOC]

[go/dynamic_suite_codelab](http://goto.google.com/dynamic_suite_codelab)

## References and Further Reading

1.  [Autotest Best
            Practices](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/docs/best-practices.md)
2.  [Codelab: Writing a server side
            test](/chromium-os/testing/test-code-labs/server-side-test)
3.  [Codelab: Writing a client side test](/system/errors/NodeNotFound)
4.  [Design Doc: Dynamic Test
            Suites](/chromium-os/testing/dynamic-test-suites)
5.  [Documentation: Working with Test
            Suites](/chromium-os/testing/test-suites)
6.  [Test Dependencies in Dynamic
            Suites](/chromium-os/testing/test-dependencies-in-dynamic-suites)

## Overview

The Chrome OS version of Autotest introduces a new type of suite, known as a
dynamic suite. Dynamic suites allow for the jobs in a suite to be sharded over a
pool of DUTs, and the dynamic suite infrastructure takes care of all of the
device imaging and test scheduling/sharding details.

Different tests in a suite may require different specific features of a DUT (for
instance a certain type of cellular modem, or an attached [servo
board](/chromium-os/servo)). These requirements can be specified as test
`DEPENDENCIES`, so that the test in question will only be scheduled on DUTs that
have the required labels. In addition, `DEPENDENCIES` can be specified at the
suite level, causing all tests invoked through the suite to inherit any
additional suite level `DEPENDENCIES`.

### Objectives

In this codelab, we will:

*   Create a new dynamic suite.
*   Add 2 existing and 2 new tests to the suite.
*   Enumerate the tests in the suite.
*   Add some test level and suite-level dependencies.
*   Run the suite in the lab.
*   Learn how to schedule the suite to be run regularly.

### Prerequisites

*   This codelab assumes a full working checkout of the Chromium OS
            repo. To get started with a Chromium OS checkout, see [How to Build
            Chrome OS
            "Internally"](https://sites.google.com/a/google.com/chromeos/resources/engineering/how-to-build).
*   This codelab assumes a basic familiarity with use of `git` to stage
            files and commit changes.

### Non Prerequisites

*   This codelab does NOT require a local instance of the Autotest
            server, or even the ability to build ChromiumOS locally (we will
            delegate the building to remote buildbots, and running our suite to
            the Test Lab server).
*   This codelab does NOT assume any familiarity with how to use `repo`
            or `cbuildbot`.

## Creating The Test

### Create a work branch

This codelab will involve touching or changing code in two git repositories
within the ChromiumOS repo. Our suite will be named **peaches**, so we will
start by creating a repo branch named peaches, associated with the two git repos
we will be modifying:

```none
user@host:~/chromiumos$ repo start peaches src/third_party/autotest/files src/third_party/chromiumos-overlay
```

If this succeeds, then you should be able to see your newly created branch.

```none
user@host:~/chromiumos$ repo branch
*  peaches                   | in:
                                   src/third_party/autotest/files
                                   src/third_party/chromiumos-overlay
```

### Create a dynamic suite

Test suites are defined by Autotest control files (Made up of Python with some
meta variables), similar to the control files used to define tests themselves.
The suite control files live in the Chromium OS source tree it the
`src/third_party/autotest/files/test_suites` directory. Poke around and take a
look at some of them to see their basic structure.

Once you are satisfied, create a new file in this directory named
`control.peaches`, with the contents given below. **Caution: copy-pasting from
Google Docs has been known to convert consecutive whitespace characters into
unicode characters, which will break your control file. Using CTRL-C + CTRL-V is
safer than using middle-click pasting on Linux.**

<pre><code># Copyright 2013 The ChromiumOS Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
AUTHOR = "Chrome OS Team"
<b>NAME = "peaches"</b>
PURPOSE = "A simple example suite."
CRITERIA = "All tests with SUITE=peaches must pass."
TIME = "SHORT"
TEST_CATEGORY = "General"
TEST_CLASS = "suite"
TEST_TYPE = "Server"
DOC = """
This is an example of a dynamic test suite.
@param build: The name of the image to test.
          Ex: x86-mario-release/R17-1412.33.0-a1-b29
@param board: The board to test on. Ex: x86-mario
@param pool: The pool of machines to utilize for scheduling. If pool=None
             board is used.
@param check_hosts: require appropriate live hosts to exist in the lab.
@param SKIP_IMAGE: (optional) If present and True, don't re-image devices.
@param file_bugs: If True your suite will file bugs on failures.
@param max_run_time: Amount of time each test shoud run in minutes.
"""
import common
from autotest_lib.server.cros.dynamic_suite import dynamic_suite
dynamic_suite.reimage_and_run(
    build=build, board=board, name=NAME, job=job, pool=pool,
    check_hosts=check_hosts, add_experimental=True, num=num,
    file_bugs=file_bugs, skip_reimage=dynamic_suite.skip_reimage(globals()))
</code></pre>

The suite control file’s `TEST_TYPE` is `Server`. This indicates simply that the
suite control file is meant to run server side. This restriction does not apply
to tests contained in the suite, the suite can contain both Client and Server
side tests regardless of this line in the suite control file.

### Create a new test control file

Tests can declare themselves to be part of any number of suites. This is done by
listing the suite in the test control file’s SUITE variable. To put a test into
multiple suites, simply use a comma separated list. In this codelab, we will add
two existing control files and two new control files to our suite. Let's start
with two new dummy tests control files. Create the file
`src/third_party/autotest/files/client/site_tests/peaches_DummyPass/control`
with the contents below. **Caution: copy-pasting from Google Docs has been known
to convert consecutive whitespace characters into unicode characters, which will
break your control file. Using CTRL-C + CTRL-V is safer than using middle-click
pasting on Linux.**

<pre><code># Copyright 2013 The ChromiumOS Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
AUTHOR = "Chrome OS Team"
NAME = "peaches_DummyPass"
PURPOSE = "Dummy test that passes immediately."
SUITE = "peaches"
TIME = "SHORT"
TEST_CATEGORY = "General"
<b>TEST_CLASS = "peaches"</b>
TEST_TYPE = "client"
DOC = """
Example test for peaches suite.
"""
job.run_test('dummy_Pass')
</code></pre>

### Create a new test control file with DEPENDENCIES

In the same directory, create another file, `control.bluetooth` with the same
contents, but with the test `NAME` changed to `peaches_DummyPass_BT` and with a
line added near the other declarations at the top specifying:

```none
. . .
DEPENDENCIES = "bluetooth"
. . .
```

This label tells the dynamic suite scheduler that this job may only run on DUTs
with the bluetooth label.

### Add two simple existing tests to the suite

Let's also add some existing tests to the peaches suite. For instance, edit the
`SUITE` lines of
`src/third_party/autotest/files/client/site_tests/login_LoginSuccess/control`
and `.../login_BadAuthentication/control` files to include `peaches`. If you
need to add a test to multiple suites to accomplish this, you can use a comma
separated list of the form `SUITE = "suite1, suite2, suite3"`

### Enumerate the suite’s tests

To verify that we have added these 4 tests to our suite, we can use the
`suite_enumerator` utility, as follows:

```none
user@host:~/chromiumos/src/third_party/autotest/files$ site_utils/suite_enumerator.py peaches -a .
./client/site_tests/login_BadAuthentication/control
./client/site_tests/peaches_DummyPass/control
./client/site_tests/login_LoginSuccess/control
./client/site_tests/peaches_DummyPass/control.bluetooth
```

### Include new test files in autotest-tests ebuild

Earlier, we added two new test control files for client-side tests. In order for
the new test files to be available to the DUT at test time, they must be
included in the appropriate overlay ebuild file. This procedure is explained in
more detail in a separate codelab -- writing a client side test (not yet
published).

Open the file
`src/third_party/chromiumos-overlay/chromeos-base/autotest-tests/autotest-tests-9999.ebuild`.
Near the bottom of the file, at the bottom of the long list of `IUSE_TESTS`
entries, add the following:

```none
. . .
      +tests_peaches_DummyPass
      +tests_peaches_DummyPass_BT
. . .
```

### Commit and upload changes

We need to commit our changes to two separate git repositories. The changes that
we made to the ebuild are required in order for the suite to run properly on a
DUT, so we need to make the Autotest repo changes depend on the ebuild changes.

First, from the chromiumos-overlay directory, create a commit with `git commit
-a`. Write a commit message that suits your fancy.

Find the Change-ID for the commit you just created, using `git show --stat`.
This will be some string similar in form to
I515b9c4775f518b7b000f964a00df9845ed0c6f6, in the commit message for the commit
you just created. You should also see in the output of that command that you
have changed 1 file.

Change directory to the Autotest repository, and create another commit, but this
time including in your commit message a line `CQ-DEPEND=CL:*****`, pasting in
the Change-ID of the first commit. This tells the build system that in order to
apply our patch to the Autotest repository, it must first apply our patch to the
chromiumos-overlay repository. You should see that 5 files have changed. If not,
you may have forgotten to add your new control files to the git repo! Run `git
add .` and `git commit -a --amend` to fix that.

Now, upload both your changes to gerrit with `repo upload . -d` from each
directory, or run `repo upload --br=<branch> -d` and uncomment the directories
to upload. The `-d` flag here marks our upload as a draft, so no prying eyes
will see our dirty hacking.

Once repo upload has finished its work, you will see links to your two new
changes on chromium-review.googlesource.com.

## Build your changes into a new image using cbuildbot

Determine the Change-ID for your Autotest changes. Then, submit your patch to be
built remotely:

```none
user@host:~/chromiumos/chromite/bin$ ./cbuildbot --remote -g ***** lumpy-release-tryjob
```

where you have pasted in the Change-ID of the changes to the Autotest
repository.

The output of this command should give you a buildbot link where you can follow
your build progress. The build will take about 6 hours.

### Run your new suite

Once the building step in the previous section has concluded, you should receive
an email to this effect from **cros.tryserver@chromium.org**. Follow the link in
this email to your build results page, then drill down to the "Report stdio"
link, and pull out the build number (which will be a string similar to
"**trybot-lumpy-release/R26-3556.0.0-b683**").

Now, run your suite with the command

```none
user@host:~/chromiumos/src/third_party/autotest/files/site_utils$ ./run_suite.py -s peaches -b lumpy -i ***** -p try-bot
```

where \*\*\*\*\* is the build number you just extracted.

Point your browser at <http://cautotest/>, and you should soon see your suite
job appear in the job list. After the suite job has started to run, it will
spawn sub-jobs for all the individual tests. Note that different tests may end
up running on different DUTs.

## Add suite-level dependencies

One of the tests we created in the codelab, peaches_DummyPass_BT, made use of a
DEPENDENCY to require that the test could only run on DUTs with the bluetooth
label. In addition to specifying dependencies at the test level, they can also
be specified at the suite level. When a suite with suite level dependencies is
run, all the jobs kicked off by the suite will have any suite dependencies added
in addition to the test level dependencies.

To add a suite level dependency, edit the control file for the suite. Add a
named argument to the call to dynamic_suite.reimage_and_run, of the form
suite_dependencies=’servo’, for example. Now, when the suite is run, all jobs
will inherit an additional dependency on servo. The string can contain multiple
dependencies as a comma separated list.

Suite-level dependencies can be useful when you want to run several closely
related suites consisting of the same tests, but with slightly different
dependencies. For example, if you want to run a suite focused on network3g
connectivity, but separately on devices configured for different cellular
carriers. See for instance <https://chromium-review.googlesource.com/41260/>

## Get your suite into the regular rotation with suite_scheduler

Suites can be scheduled to run in the test lab automatically, either triggered
by build events or at regular timed intervals. To add your suite to the
schedule, edit suite_scheduler.ini in the root directory of the Autotest repo.
Following in the footsteps of the other suites already in the file, it should be
easy to add your suite.

To add peaches as a suite that runs nightly, add the following to
suite_scheduler.ini

```none
. . .
[PeachesDaily]
run_on: nightly
suite: peaches
branch_specs: >=R21
pool: suites
num: 2
. . .
```

The fields above specify when suite runs should be triggered, which suite should
be run, which branches should trigger the suite to run, which machine pool the
suite should be assigned to, and the number of DUTs that the suite should
attempt to use. For more information on what pool to select refere to [What pool
should I
select](/chromium-os/testing/test-suites#TOC-What-pool-should-I-select-).

If you have added a new suite to suite_scheduler.ini, one for which a suite
control file did not exist before, you need to pay attention to the branch_specs
attribute. Suite control files are picked up from the build artifacts (unlike
other server-side control files). You can either backport your new suite control
file to older maintained branches, or avoid scheduling this suite against those
branches by using branch_specs to set a cutoff.

There are some subtleties in the num parameter, with respect to test
dependencies. You must ensure that the num parameter is greater than or equal to
the number of unique dependency sets over all the jobs in your suite. So, for
instance, if you have a suite (like peaches) that has some jobs with no
dependencies, and some jobs with 1 dependency (bluetooth), you must make use num
&gt;= 2, otherwise the suite will fail immediately on running. There’s a handy
sanity check script to make sure you’re satisfied this:

```none
./site_utils/suite_scheduler/suite_scheduler.py --sanity
```

This sanity check will also run as a pre-submit hook, so even if you forget to
run it yourself, you will be warned on repo upload that you have not fulfilled
the num criteria.
