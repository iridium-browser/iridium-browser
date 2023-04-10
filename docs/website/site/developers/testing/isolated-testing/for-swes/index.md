---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
- - /developers/testing/isolated-testing
  - Isolated Testing
page_name: for-swes
title: Isolated Testing for SWEs
---

[TOC]

## Overview

**Also See:**
[//docs/workflow/debugging-with-swarming.md](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/workflow/debugging-with-swarming.md)

This page explains how to convert a googletest executable into a fully isolated
test. This is done by describing *runtime dependencies* for a test.

For information about the infrastructure itself and the roadmap, see [Isolated
Testing Infrastructure](/developers/testing/isolated-testing/infrastructure).

Note: If you're a Googler who needs to perform a manual test on a platform for
which you cannot readily build (e.g. Windows Developer with no development
environment on your Mac book), follow [these instructions to get the build from
a try run](https://goto.google.com/runtrybuiltbits).

### What's "Isolate"?

#### **Goal: describe the exact list of files needed to run a executable.**

The isolate project is all the file formats, tools and server code to make this
work **fast** and efficiently. Every test executable list the data files it
depends on, which allows the testers bots to only download the files they need
instead of the whole Chromium source tree.

They are described in [BUILD.gn files](/developers/gn-build-configuration).

### What's "Swarming"?

**Goal: distribute tasks fast and efficiently in an heterogeneous fleet of
bots.**

The swarming project is all the tools, server and bot code to run a step (like a
unit test) on a remote bot and get results back. It has native google-test
sharding capability.

### What are the advantages?

By reducing the amount of data that we need to transfer to the tester machines,
it becomes much easier to increase the number of tester bots that we have using
Swarming (part of [LUCI](https://github.com/luci/)).

## Adding a new test

*   Make sure the test exist in a BUILD.gn file.
*   Add missing items to the "data" section until the test runs locally
            isolated via mb.py run.
*   Add the test to the corresponding json file in //testing/buildbot/.
*   All the required data files, executables started by the test
            executable\* required by the test to succeed are listed as a
            dependency.
    *   For example, `browser_tests.exe` needs `chrome.exe`, which needs
                `resources.pak`, etc.

Expectations of the tests:

*   Must **not** do directory walking or otherwise try to guess what
            should be tested.
*   Must **not** edit any input file.
*   Must **not** write at all in the current directory. It must only use
            the temporary directory for temporary files.

## HowTos

### Download binaries from a try job

*   Visit the try job build, e.g. from <https://ci.chromium.org/>.
*   Search for \[trigger\] my_test and click Swarming task "shard #0"
            link
*   Scroll down a bit, follow the instructions in the **Reproducing this
            Task Locally** section

### Run a test isolated locally

```none
echo gn > out/Release/mb_type  # Must be done once to avoid mb.py from performing a clobber
tools/mb/mb.py run //out/Release base_unittests  # Builds, generates .isolate (via gn desc runtime_deps), and runs (via "isolate.py run")
```

### Run a test built locally on Swarming

#### 1. Eligibility and login

Right now, only users Chromium team members can use the infrastructure directly.
Note that the whole [Swarming infrastructure is open
source](https://github.com/luci/luci-py) so if any other company would help to
recreate the same infrastructure internally, send us a note at
[infra-dev@chromium.org](https://groups.google.com/a/chromium.org/g/infra-dev)

By login first, you have access tokens so that the following commands do not
have to constantly prompt for your identity. You only need to do this once:

```none
luci-auth login
```

If you are running through a text only session on a remote machine, append
argument `--auth-no-local-webserver`

#### 2. One step build & run

```none
python3 tools/mb/mb.py run --swarmed out/Release \
base_unittests \
 -- --gtest_filter=<test filter>
```

This will perform the steps below, picking defaults based on your local
setup. If it fails, you can re-run the last command run by mb.py to see
the cause of the failure. You may need to login again.

If you want to pick different defaults, e.g., run tests on Windows-11 from
a Windows 10 machine, you need to use --no-default-dimensions and explicitly
specify some dimensions, e.g.,

none
```
python3 tools/mb/mb.py run \
  --swarmed out/Release \
  -d os Windows-11 \
  -d pool chromium.tests \
  --no-default-dimensions base_unittests \
  -- --gtest_filter=<test filter>
```

Or, you can run the individual steps below manually.

#### 3. Build & Generate .isolate file

The isolate file describes what are the files that needs to be mapped on the
Swarming bot. It is generated via GN "data" and "data_deps" statements, and is
processed by mb.py:

```none
ninja -C out/Release base_unittests.exe
echo gn > out/Release/mb_type  # Must be done once to avoid mb.py from performing a clobber
python3 tools\mb\mb.py isolate //out/Release base_unittests # Creates out/Release/base_unittests.isolate
```

#### 4. Compute .isolated file and upload it

The isolated file contains the SHA-1 of each input files. It is archives along
all the inputs to the Isolate server. Since the isolate server is a
content-addressed cache, only the ones missing from the cache are uploaded.
Depending on your connection speed and the size of the executable, it may take
up to a minute:

```none
tools\luci-go\isolate archive \
  -i out\Release\base_unittests.isolate \
  -cas-instance chromium-swarm
```
This will output a digest string that you use in step 5.

#### 5. Trigger the task

That's where a task is requested, specifying the isolated (tree of SHA-1 of
files to map in):

```none
tools\luci-go\swarming trigger \
  -digest <digest from step 4> \
  -server chromium-swarm.appspot.com \
  -d os=Windows-10-19042 \
  -d pool=chromium.tests \
  -d cpu=x86-64 -- out\Release\base_unittests.exe
```

Wait for results. OS currently available:

*   Windows-7-SP1 (64 bits)
*   Windows-8.1-SP0
*   Windows-2008ServerR2-SP1 (64 bits)
*   Windows-10-15063 (64 bits)
*   Ubuntu-14.04 and Ubuntu-16.04 (64 bits)
*   Mac-10.13 to Mac-13

For other available --dimension values, look at Swarming bots e.g.:
<https://chromium-swarm.appspot.com/botlist>.

That's it. Feel free to contact the team at
[infra-dev@chromium.org](https://groups.google.com/a/chromium.org/g/infra-dev)
for any chromium open source specific questions.

**Notes:**

*   `-d pool Chrome` is needed!
*   -d cpu x86-64 is useful to prevent your 64 bit build from being run
            on 32-bit Windows trybots.

### Additional Notes

*   Running an executable on a swarming bot is documented at in the
            [user
            guide](https://chromium.googlesource.com/infra/luci/luci-py.git/+/HEAD/appengine/swarming/doc/User-Guide.md).
*   Monitor the running tasks by visiting
            [chromium-swarm.appspot.com](https://chromium-swarm.appspot.com).

## FAQ

### I run a task on Swarming and it hangs there

It is possible that all the bots are currently fully utilized. In this case, the
task is in PENDING state.

### It seems tedious to list each test data file individually, can I just list src/ ?

In theory yes, in practice please don't and keep the list to the strict minimum.
The goal is not to run the tests more slowly and having the bots download 20 gb
of data. Reasons includes:

1.  Isolate Server is optimized for &lt; 50000 files scenario. There's a
            2ms/file cost per cache hit. So for example, layout tests are
            currently out of the use case since there's &gt; 80000 files.
2.  It's always possible to go coarser but it's much harder to get back
            stricter.

### Where can I find the .isolated file?

The .isolated files are generated when you build the isolation specific version
of a target, e.g. out/Debug or out/Release. The isolation target name is just
the normal target name with a _run added to the end.

### I have an error, is it related to .isolate files?

If you have a test that passes locally and fails on some trybots, then it could
be related.

This error can be seen when a browser test js file is not found:

TypeError: Cannot read property 'testCaseBodies' of undefined

### Where should I file bugs?

Swarming specific bugs can be filed on at <http://crbug.com> in [component
Infra&gt;LUCI&gt;TaskDistribution](https://bugs.chromium.org/p/chromium/issues/list?q=component%3AInfra%3ELUCI%3ETaskDistribution).
