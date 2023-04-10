---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: dynamic-test-suites
title: Dynamic Test Suites
---

## Autotest Dynamic Test Suites

The next iteration of CrOS testing in Autotest is more dynamic and requires
fewer manual changes with respect to adding and disabling tests. The first pass
at this required suites to be manually defined by hardcoding a list of tests,
and added infrastructure to find a set of machines on which to run those tests.
The former required manual management, and the latter can already be done by
appropriately using autotest. Dynamic suites, in contrast, are defined by
looking over all existing control files at runtime, and rely on the autotest
infrastructure to shard the tests across all available, appropriate machines.
As far as someone scheduling a BVT run is concerned, sending an RPC to the
autotest frontend telling it to schedule ‘control.bvt’ for a certain build on a
certain board is sufficient; all other logic exists inside of the Autotest
frontend (AFE).

### Detailed Design

#### Specifying job dependencies

#### This is how a single test specifies the kind of machine on which it can be run. DEPENDENCIES is just a list of labels. A machine satisfying all of them must be found in order for the job to be successfully scheduled.

*   #### DEPENDENCIES=‘CDMA’, ‘POWER’, ‘GOBI3K’, ‘X86’

#### All current tests need to be re-evaluated and updated to encapsulate required dependencies. e.g. GSM, CDMA, POWER, GOBI3K, BLUETOOTH etc etc. For architecture-dependent tests, we can also include labels like ARM, TEGRA2, TEGRA3, X86, etc.

#### Board compatibility

We ultimately decided against specifically expressing which boards a test should
and should not run on. We couldn’t come up with a reason we’d need to avoid a
specific board or class of boards that could not be expressed with DEPENDENCIES.
Bluetooth tests don’t apply to Alex? Well, no Alex device will have BLUETOOTH.
ACPI tests don’t apply to ARM? Well, they’ll DEPEND on x86.
Instead, we will rely on dynamic_suite code in the AFE to distinguish between a
test that has unsatisfiable dependencies at the time of scheduling, and one that
can be scheduled, but does not run in the time allotted. In the latter case, the
job will be ABORTed. In the former case, we will fail the test with TEST_NA. In
the Alex case above, for example, all tests with a BLUETOOTH dep will be
TEST_NA. As a more complicated example, consider a GOBI3K test on Mario, and
assume we have only one Mario with that modem in it. If all is well, the test
can be scheduled in the time allowed and things are good. If the device is down,
the AFE still knows that it should have a machine that meets the test’s
DEPENDENCIES, but discovers that the device is not currently ready. That’s fine,
and it schedules the test. When the time allotted for the suite is up, the test
would be ABORTed and included as a failure in the test results.

#### Job reporting

For TPM’s, one can provide a view of test results per-build-per-board that shows
all jobs that could be scheduled, and the results of those tests. Specifically,
jobs that are TEST_NA can be ignored if the customers of the dashboard
so-desire.
For the lab team, we can show all jobs that we tried to schedule, so that we can
see which jobs are getting starved and detect new, unexpected starvation.

#### Defining suites

\*\*\* For more updated information, please see the [Dynamic Suites Code
Lab](/chromium-os/testing/test-code-labs/dynamic-suite-codelab).

We've added a custom variable to control files called SUITE. SUITE can contain a
comma-delineated list of arbitrary strings. Only suite names that have an
associated control file will actually be usable in any meaningful way. By
convention, we put all suite control files in a top-level directory called
'test_suites', and name the files appropriately. For example,
`test_suites/control.bvt` is the control file for the BVT suite.

**This does require parser changes, and does make including upstream tests in
our suites a bit complex. The former is not that difficult (and probably
upstreamable). We could likely address the latter by adding a level of
control-file-indirection, and having a site-specific file that uses SUITE and
references the upstream test.**

**We use the already-existing “EXPERIMENTAL” flag to mark tests as candidates for inclusion in a suite, and will run them along with the non-experimental tests -- but tag them separately so that results can be separated out in the final report.**
**We considered making all test suites a query of sorts over the labels defined
in control files (the BVT would be all SHORT, functional tests that are not
experimental, for example), which would avoid these issues, but decided that
this smacked too much of magical side effects for use in our primary test
suites. However, we will provide a library of code that teams can use to create
their own suite control files. For example, ‘kernel.soak’ could simply be a
one-liner that found and scheduled jobs for all SHORT, MEDIUM, and LONG
functional and stress tests in the “kernel” category.**

**### The code**

**#### Scheduling a suite for a given board and image**

**We defined a new autotest RPC for this, with the following interface:**

```none
def create_suite_job(suite_name, board, build, pool, check_hosts=True,
```

```none
                     num=None, file_bugs=False, timeout=24, timeout_mins=None,
```

```none
                     priority=priorities.Priority.DEFAULT,
```

```none
                     suite_args=None, wait_for_results=True):
```

```none
    """
```

```none
    Create a job to run a test suite on the given device with the given image.
```

```none
```

```none
    When the timeout specified in the control file is reached, the
```

```none
    job is guaranteed to have completed and results will be available.
```

```none
```

```none
    @param suite_name: the test suite to run, e.g. 'bvt'.
```

```none
    @param board: the kind of device to run the tests on.
```

```none
    @param build: unique name by which to refer to the image from now on.
```

```none
    @param pool: Specify the pool of machines to use for scheduling
```

```none
            purposes.
```

```none
    @param check_hosts: require appropriate live hosts to exist in the lab.
```

```none
    @param num: Specify the number of machines to schedule across.
```

```none
    @param file_bugs: File a bug on each test failure in this suite.
```

```none
    @param timeout: The max lifetime of this suite, in hours.
```

```none
    @param timeout_mins: The max lifetime of this suite, in minutes. Takes
```

```none
                         priority over timeout.
```

```none
    @param priority: Integer denoting priority. Higher is more important.
```

```none
    @param suite_args: Optional arguments which will be parsed by the suite
```

```none
                       control file. Used by control.test_that_wrapper to
```

```none
                       determine which tests to run.
```

```none
    @param wait_for_results: Set to False to run the suite job without waiting
```

```none
                             for test jobs to finish. Default is True.
```

```none
```

```none
    @raises ControlFileNotFound: if a unique suite control file doesn't exist.
```

```none
    @raises NoControlFileList: if we can't list the control files at all.
```

```none
    @raises StageBuildFailure: if the dev server throws 500 while staging build.
```

```none
    @raises ControlFileEmpty: if the control file exists on the server, but
```

```none
                              can't be read.
```

```none
```

```none
    @return: the job ID of the suite; -1 on error.
```

```none
    """
```

```none
```

*   **Change priority:** The arg priority is an integer denoting
            priority, used by our scheduler to decide how to prioritize jobs.
            You can find all valid priority options in
            autotest_lib.client.common_lib.priorities. Currently, the priority
            ladder looks like - Weekly &lt; Daily &lt;PostBuild &lt; Default
            &lt; Build &lt; PFQ &lt; CQ.
*   **Wait or not wait for results:** Use the arg
            wait_for_results=True/False to tell the suite job whether you want
            it to wait for all test jobs to finish or not. Not waiting for the
            results can reduce the load burden to our rpc server, so it is
            recommended whenever it is possible.

#### Running the suite

**create_suite_job() starts by telling the dev server to fetch the image to be tested, and its associated autotest bundle, from the URL provided. Clients of autotest will no longer need to be aware of the dev server. The details of this may change going forward, but all this work will be handled by this RPC.**
**Once the image and autotest bundle are staged on dev server, it goes through all the control files associated with the build being tested, and schedule the proper jobs. At that point, we let the autotest scheduler take responsibility for all the async scheduling/dependency scheduling. It is important that all jobs are scheduled via rpc calls with a name that associates them all together so that when we want to look at results we can aggregate them all in a query. This will allow callers (like the buildbot-autotest connection) to poll for the status of all jobs kicked off by the BVT suite control file, and eventually find and gather up results for reporting purposes**
