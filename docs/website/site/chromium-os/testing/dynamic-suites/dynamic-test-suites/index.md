---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/dynamic-suites
  - Dynamic Suites
page_name: dynamic-test-suites
title: Running Dynamic Test Suites
---

This document describes how to write, update, run, disable, and enumerate tests
that are in a Test Suite.

[TOC]

A test suite is a collection of tests, client and server side, that are run on
across a particular build to validate a larger system.

If you are interested in how dynamic suites works and how it interacts with the
CrOS Infrastructure please check out it's [generously documented py
module](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/server/cros/dynamic_suite/dynamic_suite.py).

Test Suites build on the Autotest [control file
specification](https://github.com/autotest/autotest/wiki/ControlRequirements).

# How to Create a new Test Suite

In order to create a valid suite the following two requirements need to be met:

*   A Suite control file (e.g.
            [test_suites/control.bvt](http://git.chromium.org/gitweb/?p=chromiumos/third_party/autotest.git;a=blob;f=test_suites/control.bvt;hb=HEAD))
*   At least one test with the SUITE='your_suite' tag in it.

We have a mechanism that allows suites of tests to be defined dynamically by a
control file at runtime.
`[test_suites/control.bvt](http://git.chromium.org/gitweb/?p=chromiumos/third_party/autotest.git;a=blob;f=test_suites/control.bvt;hb=HEAD)`
in the autotest repo is a simple example of such a suite, and
[login_LoginSuccess](http://git.chromium.org/gitweb/?p=chromiumos/third_party/autotest.git;a=blob;f=test_suites/control.bvt;hb=HEAD)
is one test that's included in this suite. Looking at the control file for
login_LoginSuccess, note the line `SUITE='bvt'`. This is used in `control.bvt`
like this:

```none
suite_tag = 'bvt'
bvt = dynamic_suite.Suite.create_from_name(suite_tag, build, pool=pool)
```

The infrastructure finds login_LoginSuccess by matching the contents of the
SUITE field in its control file against the suite tag passed into
`dynamic_suite.Suite.create_from_name()`

`If one test should be in multiple suites the SUITE= control file variable can
be a comma delimited list of suites.`

```none
SUITE='bvt, Kernel_Regression, Nightly'
```

# How to Modify Suites

This section will go over adding, removing, and marking tests as experimental.

Some suites have restrictions when it comes to adding tests to them. To read
about the requirements of a particular test suite refer to its suite control
under autotest/files/test_suite/control.suite_name.

For example:
[control.bvt](http://git.chromium.org/gitweb/?p=chromiumos/third_party/autotest.git;a=blob;f=test_suites/control.bvt)

## How to Add tests to a Suite

Add the SUITE="suite_name" to the control file of the test you want to be run in
the suite. If this test is new add it as an experimental test first, see below.

## How to Disable Tests in Test Suite?

To disable a test *remove* the Suite Name from the SUITE= Tag, if it is the only
suite in the SUITE variable remove the entire variable.

You may consider marking a test as experimental to avoid closing of the tree but
continue running tests.

## Add new experimental tests?

Some times you may want to add tests that are experimental to start gathering
some data but you do not want the tree to be closed. To do this use the
EXPERIMENTAL control file variable.

```none
AUTHOR = "Autotest Team"
NAME = "Sleeptest"
TIME = "SHORT"
TEST_CATEGORY = "Functional"
TEST_CLASS = "General"
TEST_TYPE = "client"
SUITE = "bvt"
EXPERIMENTAL = "True"
```

An example of a test marked experimental.

# Suite Options

These are options that you can use to tweak the behavior in your suite control
file.

### check_hosts

Boolean, if the hosts required are not currently in good health fail the suite.
You may want to let the suite sit around for a while and wait for the hosts to
be repaired. Default: True

### skip_reimage

Whether or not you want the suite to re-image machines before running the tests.
Default: False

### add_experimental

If you only want the suite to include non-experimental tests. Default: True

### file_bugs

Whether or not bugs should be filed when tests fail (See below for a more
detailed example)

### suite_dependencies

If you want to apply a dependency across the whole suite (See below for a more
detailed example)

### max_runtime_mins

How long each job should be allowed to run in a suite. Default is 24 hours. (See
below for an example)

## Add a suite-level dependency?

In addition to DEPENDENCIES specified on a per-test level, a set of suite
dependencies can be specified at the suite level. These dependencies will be
appended to the dependency list of all tests when they are run through that
suite. To add suite-level dependencies, pass in your desired dependencies to
dynamic_suite.reimage_and_run with the suite_dependencies argument. For example:

```none
dynamic_suite.reimage_and_run(
    build=build, board=board, name='network3g_tmobile', job=job, pool=pool,
    check_hosts=check_hosts, add_experimental=True, num=num,
    file_bugs=file_bugs, skip_reimage=dynamic_suite.skip_reimage(globals()),
    suite_dependencies='carrier:tmobile')
```

Multiple suite-level dependencies can be given as a comma separated string.

## How do I enable automatic bug filing for my suite?

If your suite is already being run via suite_scheduler, you only need to edit
the suite control file.

*   Drop an optional _BUG_TEMPLATE dictionary into your suite control
            file:

```none
_BUG_TEMPLATE = {
    'labels': ['OS-Chrome','Type-Bug', 'your-label'],
    'owner': '',	
    'status': None,	
    'summary': None,	
    'title': None,	
    'ccs': ['a@chromium.org','b@chromium.org', 'c@chromium.org']	
}
```

*   Set the file_bugs option in your suite control file, and add a
            bug_template argument to reimage_and_run.

```none
dynamic_suite.reimage_and_run(
    build=build, board=board, name='perf_v2', job=job, pool=pool,
    check_hosts=check_hosts, add_experimental=True, num=num,
    file_bugs=True, skip_reimage=dynamic_suite.skip_reimage(globals()),
    bug_template=_BUG_TEMPLATE)
```

## How do I run jobs in a suite for longer than 24 hours?

In your suite control file pass the max_runtime_mins option:

An example to allow tests to run for 6 hours.

```none
dynamic_suite.reimage_and_run(
    build=build, board=board, name='bvt', job=job, pool=pool,
    check_hosts=check_hosts, add_experimental=True, num=num,
    file_bugs=file_bugs, skip_reimage=dynamic_suite.skip_reimage(globals()),
    max_runtime_mins=60*6)
```

# How to enumerate what tests will run for a given Suite?

There is a tool under the autotest repo to enumerate tests that will run in a
suite.

From within the chroot the following command can be executed:

<pre><code>
~/trunk/src/third_party/autotest/files/site_utils/suite_enumerator.py -a ~/trunk/src/third_party/autotest/files <b>bvt</b>
</code></pre>

-a option is to specify what Autotest directory to look in.

The argument **bvt** is the SUITE you are intersted in.

# How to enumerate what suites are available?

```none
~/trunk/src/third_party/autotest/files/site_utils/suite_enumerator.py -a ~/trunk/src/third_party/autotest/files -l 
```

# How to run suites with run_remote_tests?

```none
./run_remote_tests suite:bvt
```

Note that if your suite contains multiple control files that are associated with
the same autotest, you may get an error that says something like this:

TestError: &lt;AUTOTEST_NAME&gt; already exists; multiple tests cannot run with
the same subdirectory

To fix this, add a "tag" parameter to the job.run_test() in the associated
control files (e.g., job.run_test('foo', tag='some_tag')) to distinguish them
and make the results get written into uniquely-named result directories.

# Suite Scheduler (Running suites out of band)

Suite scheduler is the piece of software that is responsible for kicking off
suites out of band based on a set of events as opposed to in a waterfall. If you
are interested in running suites in a waterfall refer to the HW_Test section
below. Below is a run down on how this works and what needs to happen to add new
suites to this configuration.

## How are suites handled on branches?

Suites are scheduled based off of build artifacts, the autotest tarball
generated at build time is used to enumerate what control files belong to what
suites and what version of those particular tests should be run. If updated a
test on TOT it will need to be back ported to every branch that needs those
updates just like any other change.

## How do I schedule my suite using suite_scheduler?

If your suite is a part of **bvt, kaen_bvt, or smoke** it will automatically run
and no further action must be taken.

An out of band process runs on the Autotest server to schedule suite runs based
on a number of events. The code lives under
***autotest/files/site_utils/suite_scheduler*** and is configured via a Python
config file located under
[autotest/files/suite_scheduler.ini](http://git.chromium.org/gitweb/?p=chromiumos/third_party/autotest.git;a=blob;f=suite_scheduler.ini)

To add your suite send a code review with a new entry as outlined below.

### Example Config Entry

```none
[WeeklyKernelRegressions]
run_on: weekly
suite: kernel_weekly_regression
branch_specs: >=R21
pool: suites
```

There are 5 parts to a configuration entry.

1.  A **\[unique name\]** as the toplevel marker for the config
            \[WeeklyKernelRegression\]
2.  What events the suite should **run_on**
3.  What **suite** the config entry is for.
4.  What **branch_specs** to apply to the config entry.
5.  Lastly what **pool** should be used.

The top level unique name marker is part of the Python config format and must be
**unique** and it should reflect what suite is being configured.

#### Supported *run_on* events

*   **nightly** - At trigger time the latest image available on the
            devserver is used to schedule jobs against.
*   **new_build** - When a new build is marked as PASS in
            manifest-versions (Happens via the build process) a suite will be
            kicked off with this build.
*   **weekly** - Once a week on sunday the latest image available will
            be used to schedule jobs against.

Pending implementation: new_chrome [crbug.com/212678](https://crbug.com/212678)

\*If there are other events you would like to have available please [file a
bug](http://goto.google.com/chromeos-lab-bug).

#### Options for *suite* line

The **suite line** can only be used to describe *one* individual suite. The name
used pertains to the suite added under
**autotest/files/test_suites/control.*suite_name***. The above example would
invoke the control.kernel_weekly_regression suite file.

#### Valid *branch_specs* options

There are three different branch specs that can be used to ensure your suite
runs on a particular branch:

*   **factory** - Any factory branch that is producing images will cause
            this event to fire.
*   **firmware** - Any firmware branch that is producing images will
            cause this event to fire.
*   **&gt;=R21** - Any **release branch** that is greater than or equal
            to R21.
    *   The &gt;= is required and is used to avoid trying to run tests
                on branches that do not have the proper control files.

#### What *pool* should I select?

A pool is a label on a machine like pool:bvt. We are using pools extensively
currently as our dependency support is not quite there yet. Unless you are doing
something out of the ordinary with extra hardware required to be attached to the
DUT you should always use *pool: suites* which is a general pool for suite runs.

If you are doing things that require extra hardware chances are there is already
a precedent for this in the suite_scheduler.ini file and you should reference
those. When in doubt ask chromeos-lab-infrastructure@google.com.

# Running suites in BuildBot (HW_Test Stage)

Suites can also be run as a step in the buildbot waterfall. This assumes you are
running your builds via cbuildbot, if you are not please [file a
bug](http://goto.google.com/chromeos-lab-bug) with your need and the Lab team
can look at how to best direct you.

**\*Note** when running the HW_Test step you are blocking your waterfall on
actual hardware tests. If you do not want your builders to block on this
consider using Suite Scheduler as described above.

**\*Note if your build client is not on the master2 vlan you will need to [file
a bug](http://goto.google.com/chromeos-lab-bug) with the Chrome OS lab team so a
proxy connection can be set up.**

Adding a HW_Test step to your cbuildbot config is as easy as editing
cbuildbot_config.py in the chromite repo and adding a hw_test entry like the
following:

Example running the **bvt suite** for a lumpy build:

<pre><code>
internal_paladin.add_config('lumpy-paladin',
  boards=['lumpy'],
  paladin_builder_name='lumpy paladin',
  <b>upload_hw_test_artifacts=True,</b>
  <b>hw_tests=['bvt'],</b>
)
</code></pre>

You need to specify **upload_hw_test_artifacts** so that Autotest has access to
the image and the build artifacts via Google Storage and specify what suite(s)
you want to run via a list called **hw_tests**.

## Format of job names for running suites in Autotest

For suite jobs:

```none
$build-test_suites/control.$suite_name
```

i.e.

**x86-alex-release/R20-2264.0.0-test_suites/control.kernel.per_build.benchmarks**

For sub-jobs run from the suite:

```none
$build/$suite_name/$test_name
```

i.e.

**x86-alex-release/R20-2264.0.0/kernel.per_build.benchmarks/Hackbench**

If a particular test is set with EXPERIMENTAL = True it is prefaced with
experimental_test_name.

i.e.

**x86-alex-release/R20-2264.0.0/kernel.per_build.benchmarks/experimental_platform_AesThroughput**

## \*How to force the scheduling of an event suite via suite_scheduler.py

\*This is not a typical use case for users.

Suite scheduler lives under
*autotest/files/site_utils/suite_scheduler/suite_scheduler.py*

An example of forcing all nightly suites for stumpy to be kicked off.

```none
site_utils/suite_scheduler/suite_scheduler.py -i  stumpy-release/R21-2270.0.0 -e nightly
```

## \*How to schedule an individual suite?

If you need to run an individual suite for a particular build but not all of the
suites that would be kicked off by a particular event you can use the same
command the buildbots use, run_suite.py.

```none
site_utils/run_suite.py -b parrot -p pool:parrot_2gb -s kernel_weekly_regression -i parrot-release/R23-1913.156.1 -u 1
```

# Where can I see results from suite runs?

Other than the BVT suite no other suite results are displayed on the buildbot
waterfall. To see your results you need to refer to the Autotest dashboard at
<http://cautotest.corp.google.com/results/dashboard>.

From there:

*   Select a board/release of interest by clicking on the cells with BVT
            test results (e.g. "39/39").
    *   i.e.
                <http://cautotest.corp.google.com/results/dashboard/alex/x86-alex-r21/bvt.html>
*   Then at the top you will see categories that pertain to that
            board/milestone click on one of those to see your results
    *   i.e.
                <http://cautotest.corp.google.com/results/dashboard/alex/x86-alex-r21/security.html>

# How do I get notified via email if suite runs fail?

Refer to: <http://cautotest.corp.google.com/results/dashboard/emails.html>
