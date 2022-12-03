---
breadcrumbs:
- - - - Fixing Flaky Tests
- - - Testing and infrastructure
- -   /developers
  -   For Developers
page_name: fixing-flaky-unit-tests
title: Fixing Flaky Unit Tests
---

[TOC]

## Overview

Chrome has dozens of unit-test suites. They generally have the name
`<component>`_unittests, e.g., 'views_unittests', and the catch-all 'unit_tests'.
Unit tests run in batches of 10 tests per process. If a test fails, it is
retried in its own process. If the retry succeeds, the original failure is
called a hidden flake. This type of flake tends to be fairly reproducible if
you run the same set of tests in a single process as the failing trybot did. To
determine what that set of tests is, do the following:

* Click on the Test Results tab of the failing build page.
* Click on the failing test.
* Open the task page.
* Search for the failing test in the raw output.

Up to 9 of the previous tests listed in the output run in the same process as
the failing test. Since unit tests run in batches of 10, each batch starts with
a test number ending in 1, and ends with a test number ending in 0, e.g., tests
numbered 3541 through 3550 are run in the same process.

To run the tests locally, use --test-launcher-filter-file=`<file with names
of up to 10 tests>`, or gtest_filter=`<test1>:<test2>:<test3>`... If you get a
reproducible case, run the test suite under the debugger, and debug the failure.

## Causes of failures, and typical fixes

### Global state interactions between tests
If one test leaves some global state in an unexpected,
non-default state, subsequent tests in the same batch/process can fail. Working
backwards from the failure to the nearest global variable used tends to uncover
the root issue. Comparing a failed run to a successful run of the test in
question can also point to the root cause, e.g., using a debugger or logging
to find where the execution path diverges between success and failure.

If you’ve found the global variable left in an unexpected state, the next step
is to find the test that’s leaving it in an unexpected state. The simplest
approach is to remove earlier tests from the gtest_filter and see when the flaky
test stops failing. Another technique is to add a DCHECK in the OnTestEnd
function in the test executable that checks if the global variable is in the
default state (see
[How to add OnTestEnd checks](#add-ontestend-checks-to-find-tests-which-violate-constraints)
). Running all the tests in the
test executable with that DCHECK will uncover any other tests that are leaving
the global state in a non-default state, e.g.,
[2948066](https://crrev.com/c/2948066). You may want to commit the DCHECK change
to prevent this from happening in the future.

If you know the culprit is in the same test class, you can put the DCHECK in
the TearDown() method for the test class.

There are several different ways to fix this type of issue. If possible, use a
method that prevents this type of flake from recurring.

#### Scoped state setters, aka RAII
Declare an object whose constructor sets the global state to the desired state,
and whose destructor restores it to the default. If there is a test-only method
that sets the desired state, making it private will force tests to use the
scoped state setter, and prevent this type of flake from recurring.

Example CLs: [2831260](https://crrev.com/c/2831260),
[2679227](https://crrev.com/c/2679227), [2918613](https://crrev.com/c/2918613),
[2919893](https://crrev.com/c/2919893)

#### Force state to default in OnTestStart
If the global state is used by a lot of tests, and is critical to them working
correctly, resetting the state in the executable’s OnTestStart method is
reasonable. For example, we do this to force the app locale to en-US, because so
many tests expect the locale to be en-US. In some cases, non-test code changes
global state, which means the test infrastructure or tests themselves need to
reset the global state.

Example CL: [2678132](https://crrev.com/c/2678132).

#### Have the test or test class set the required state
Sometimes it’s not feasible to keep the global state from mutating, usually
because non-test code can change some global state without the test necessarily
knowing about it. In cases like this, it’s simplest for the test to set or reset
the global state to the expected value at the start of the test. The less
frequently used the global state is, the more likely it is that you should reset
the global state in the start of tests that use it, as opposed to forcing the
state to its default in OnTestStart.

Example CLs : [2668507](https://crrev.com/c/2668507),
[2668674](https://crrev.com/c/2668674), [2693168](https://crrev.com/c/2693168)

### Leaking global observers
If a test registers a global observer or listener and forgets to unregister it,
several bad things can happen. One is that a deleted observer may be called by
a subsequent test, which will usually crash, but may corrupt memory. Adding
DCHECKs that global observers are cleaned up in OnTestEnd can sometimes work,
but some Chrome code leaves global observers registered by design. Adding
logging when the number of global observers increases between OnTestStart and
OnTestEnd can help pinpoint truly leaky tests.

Example CLs: [1653905](https://crrev.com/c/1653905),
 [2618221](https://crrev.com/c/2618221)

### Data races
If the test is only flaking on Thread Sanitizer (Tsan) trybots, most likely
Tsan is detecting a data race in the test. The most common cause of this error
used to be initializing a ScopedFeatureList after the test has started up a
thread that checks if a feature is enabled, or destroying a ScopedFeatureList
while a thread that checks if a feature is enabled is still running.
[3400019](https://crrev.com/c/3400019) fixed the general case of this but
corner cases have occurred since then. Tests can avoid the issue completely by
initializing the ScopedFeatureList in the test class's constructor, which means
that tests that use the same features will be in the same class. If the test has
its own task environment, it needs to be destroyed before the ScopedFeatureList,
so the ScopedFeatureList should be declared before the task environment in the
test class.

Example CL: [2773418](https://crrev.com/c/2773418)

A similar, but much less common issue can occur in tests that call
SetupCommandLine, e.g., [2775863](https://crrev.com/c/2775863)

Race conditions can also happen when a thread is started before one of the
objects it references is created, or joined after one of these objects is
destroyed. These may be fixed by changing the order in which objects are
declared, in order to enforce creation and destruction order.

Sometimes destruction of an object can race with another thread trying to
access the object. While not ideal, making sure the BrowserTaskEnvironment runs
all outstanding tasks before destroying the object can work around this kind of
issue, e.g., [2907652](https://crrev.com/c/2907652).

### Sequence Checker errors
Typically, these are caused by multiple threads unexpectedly accessing the same
object. If the error comes when a ref-counted object is destroyed on a different
thread than it was created on, a simple fix is to make the object inherit from
RefCountedDeleteOnSequence, e.g., [2785850](https://crrev.com/c/2785850).
Sequence Checker can also DCHECK if a global async callback is leaked by one
test and a subsequent test triggers the callback. This was ameliorated by
[2718665](https://crrev.com/c/2718665).

### Service dependency issues
The dependency manager is global, and is not cleared between tests running in
the same process. This can lead to differences in the dependency graph between
one test and the next, which can cause objects to get initialized in an
unexpected order. If a test fails because a service is null, or service
initialization fails unexpectedly, it may be related to changes made to the
dependency graph by a previous test. Often it's best to hand this kind of
problem off to a test owner. One common cause of this is use of AtExitManager.
When unit tests use AtExitManager, all singletons are
reset. This removes them from the dependency graph, which gets the
dependency graph into a broken state for the next test. Removing the use of
AtExitManger should fix this, e.g., [3293747](https://crrev.com/c/3293747).
Or, tests can ensure that the state they need exists, e.g.,
[2937452](https://crrev.com/c/2937452).

### Global maps not cleared between tests
Some code keeps a global map between an object and some other state, and does
not clear it between tests. E.g., one test can cause a Profile* to get added to
the map, and a subsequent test in the same process can create a Profile object
with the same address, causing state to leak between tests.

Example CLs: [2970728](https://crrev.com/c/2970728),
[2904474](https://crrev.com/c/2904474)

### Contention for global system resources
Generally, unit tests run in multiple shards on a machine, so several tests can
be running at the same time on a machine. Also, other tests suites can be
running on the same machine. Tests can contend for things like the
file system and the clipboard. For example, if two tests run in separate
processes, but both modify the same file, flakiness can ensue. This can be fixed
by overriding Chrome’s path to the file, using ScopedFilePath. For clipboard
issues, tests should use ui::TestClipboard instead of the system clipboard.

Example CLs: [2986782](https://crrev.com/c/2986782), [1625942](https://crrev.com/c/1625942)

[Please feel free to add other types of issues and sample CLs here]

### Focus issues
Unit tests that need to have focus can be flaky if other tests running on the
bot open windows and take focus. Typically, when a unit test sets focus, as long
as it does things synchronously, it doesn't have to worry about losing focus.
However, on Windows, the key event handler does a ::PeekMessage, which can cause
a loss of focus. So if the test generates key events, that can trigger loss of
focus. See [bug 1330440](https://crbug.com/1330440). One option is to make
the test an interactive ui test.

## Techniques for reproducing flakes

### Run with --single-process-tests in debugger
Run the whole suite (or a subset) in the debugger. If a test crashes, you will
be dropped into the debugger and you can investigate what caused the crash.

### Build tests with gn args is_debug=true, is_component_build=true.

Debug builds initialize allocated and freed memory to known states, which makes
it easier to determine if an object is used after it’s deleted, or is
uninitialized. E.g., with the Windows allocator, used in component builds,
0xcdcdcdcdcd means the memory is uninitialized, and 0xdddddddd means it is
freed. If a flake doesn’t reproduce, build with gn args from the “Lookup GN
Args” step on the failing trybot.

### Run with --test-launcher-filter-file=<file> where file contains a list of tests

### Run tests starting with the first letter of the flaky test

E.g., --single-process-tests --gtest_filter=S*.*. This is just a crude way of
sharding a test suite, so you can shake out global state bugs without having to
run the whole suite.

### Run with--gtest_shuffle
Repeatedly run with --gtest_shuffle, and then, use --gtest_random_seed to
reproduce the failures found with a particular --gtest_shuffle run.

### Increase the batch size with --test-launcher-batch-limit
Increase the number of tests run in each process via
--test-launcher-batch-limit=`<desired value>`. This will increase the
likelihood of global state interactions between tests. Fixing those will help
prevent future flakiness.

### Use --gtest_flagfile

Run with --gtest_flagfile=`<file>` where file contains
 --gtest_filter=`<test1>:<test2>:<test3>...`
This allows you to completely control the tests that run, and their order.

### Add OnTestEnd checks to find tests which violate constraints
Each unit test executable has a small .cc module with a main() function that
runs the tests. It is typically called run_all_unittests.cc. In order to add
OnTestEnd checks, the test suite needs to subclass TestSuite if it does not
already do so, add a listener to the test suite, and override the OnTestEnd
method in its listener.
[base/test/run_all_unittests.cc](https://source.chromium.org/chromium/chromium/src/+/main:/base/test/run_all_unittests.cc)
has a relatively simple example of this. Also see
[chrome_unit_test_suite.cc](https://source.chromium.org/chromium/chromium/src/+/main:chrome/test/base/chrome_unit_test_suite.cc).

### Parameterize flaky test with a repeat count and run on trybots
Create a CL that runs the test many times, and adds any useful additional logging, e.g.:

```none
 TEST_F(ExtensionServiceTest, InstallTheme)
```

becomes:

```none
class ExtensionServiceFlakyTest : public ExtensionServiceTest,
                                  public testing::WithParamInterface<int> {};

#if defined(MEMORY_SANITIZER) || defined(THREAD_SANITIZER) || \
    defined(ADDRESS_SANITIZER)
constexpr int kInstantiationCount = 500;
#elif !defined(NDEBUG)
constexpr int kInstantiationCount = 1000;
#else
constexpr int kInstantiationCount = 3000;
#endif

INSTANTIATE_TEST_SUITE_P(AAAA,
                         ExtensionServiceFlakyTest,
                         testing::Range(0, kInstantiationCount));

TEST_P(ExtensionServiceFlakyTest, InstallTheme) {
```

Upload CL and run on trybots.

### Add VLOGs and enable them only in relevant tests

VLOGging can be enabled and disabled per file on the fly with `logging::ScopedVmoduleSwitches`.
This means you can add more detailed logging in your code and only enable it inside
your tests (or just the flaky test) without causing log-spam for everyone else.

See [here](https://crrev.com/c/3648806/34/content/browser/back_forward_cache_browsertest.cc)
for an example that enables the vlogging from [`back_forward_cache_impl.cc`](https://source.chromium.org/chromium/chromium/src/+/main:content/browser/renderer_host/back_forward_cache_impl.cc;drc=86a29a03b4b3b39964885d101e0db3e0c935e532)
only in `BackForwardCacheBrowserTest`. This means that failures on bots for all those tests
will include logs from that file.

Since your test may be the only time these VLOGs are used, be especially careful not to include
any side-effects in the logging that could cause a difference between a test run and
production.

### Windows App Verifier (AppVerif.exe)
This app can catch a lot of different kinds of errors, including Windows handle
issues and AddressSanitizer (ASan)-like things like memory use after free (as
long as PartitionAlloc is disabled, which it is in component builds). To run it
on a unit test, run AppVerif.exe, pick File | Add Application, and browse to the
unit test you wish to test. Then, run that unit test under the debugger, with
--single-process-tests. If App Verifier detects an error, it will drop you into
the debugger.

### gtest-parallel
gtest-parallel can be useful for reproducing race conditions, resource
contention, and simulating trybot behavior under heavy load.

It can be found on GitHub [here](https://github.com/google/gtest-parallel).
Example Windows command:
```
python "c:\src\utils\gtest-parallel-master\gtest_parallel.py" out\Default\unit_tests.exe --gtest_filter=ExtensionServiceSyncTest.ProcessSyncDataUninstall --repeat=500 --workers=80 > par_run.txt
```
The higher the number of workers, the more likely test timeouts are, so if
there are a lot of test timeouts, reduce the number of workers.
