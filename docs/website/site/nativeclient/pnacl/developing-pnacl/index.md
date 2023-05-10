---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/pnacl
  - PNaCl
page_name: developing-pnacl
title: Developing PNaCl
---

[TOC]

**This page is intended for developers making changes to the PNaCl tools. If you
just want to build PNaCl-based applications, see the [Native Client developer
documentation](https://developers.google.com/native-client/dev/overview). If you
just want to build the toolchain, e.g. to package PNaCl or Chromium with NaCl
support, see the shorter document on
[building](/nativeclient/pnacl/building-pnacl-components-for-distribution-packagers).**

## Prerequisites

In addition to what is needed to \[build nacl\], several packages may need to be
installed on your system. For Ubuntu systems, e.g.:

```none
sudo apt-get install git cmake texinfo flex bison ccache g++ g++-multilib gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
```

```none
test -e /usr/include/asm || sudo ln -s /usr/include/asm-generic /usr/include/asm
```

## Get the Native Client Source

To build the PNaCl toolchain, you will need to check out the entire native
client project [with
git](http://www.chromium.org/nativeclient/how-tos/how-to-use-git-svn-with-native-client).
After that:

```none
cd $NACL_DIR/native_client
```

Once you have the native client project checked out, you can [check out the
PNaCl
sources](/nativeclient/pnacl/developing-pnacl#TOC-TL-DR-for-checking-out-PNaCl-sources-building-and-testing).

## Prebuilt binaries

Pre-built binaries for PNaCl are downloaded from the DEPS file by gclient, and
are located in one of these directories, depending upon your architecture and
operating system.

```none
toolchain/linux_x86/pnacl_newlib_raw
toolchain/mac_x86/pnacl_newlib_raw
toolchain/win_x86/pnacl_newlib_raw
```

If you want to use these binaries outside of Scons, you must build and install
the SDK libraries. This installs the libraries such as libnacl from the NaCl
repo that go into the SDK and are needed to link programs without any special
flags. (Note that this does not install the Pepper libraries, as those come from
the Chromium repository):

```none
pnacl/build.sh sdk
```

## Sanity Check: Compiling a test program with the prebuilt toolchain

At this point, you probably want to test if the pre-built toolchain works. Let's
start by creating a "hello world" program:

```none
#include <stdio.h>
int main(int argc, const char *argv[]) {
  printf("Hello World!\n");
  return 0;
}
```

Add the PNaCl toolchain to your path for convenience, if desired:

```none
PATH=${PATH}:toolchain/linux_x86/pnacl_newlib/bin    # Adjust the name for your system  
```

Now compile hello.c:

```none
pnacl-clang hello.c -o hello.nonfinal.pexe
pnacl-finalize hello.nonfinal.pexe -o hello.pexe
```

(NOTE: If you get an error about missing crt1.o or -lnacl, you must run the SDK
install step. See the installation instructions in the previous section.)

You have now produced a "pexe", a PNaCl bitcode application. To run this pexe,
it must be first translated to native code. Let's translate it to X86-32, X86-64
and ARM.

```none
pnacl-translate -arch x86-32 hello.pexe -o hello_x86_32.nexe
pnacl-translate -arch x86-64 hello.pexe -o hello_x86_64.nexe
pnacl-translate -arch arm    hello.pexe -o hello_arm.nexe
```

Now you have 3 native executables, one for each architecture. To run them inside
of `sel_ldr`, use the helper script, `native_client/run.py`:

```none
./run.py hello_x86_32.nexe
./run.py hello_x86_64.nexe
./run.py hello_arm.nexe     # By default it will look for qemu to run the ARM nexe.
```

## Testing a PNaCl toolchain with SCons

To use the PNaCl toolchain with the native client scons tests, just specify
"bitcode=1" on the command-line.

Some sample command lines:

```none
./scons platform=arm bitcode=1                   # build everything
./scons platform=arm bitcode=1  smoke_tests      # run smoke_tests
./scons platform=x86-32 bitcode=1                # build everything
./scons platform=x86-32 bitcode=1  smoke_tests   # run smoke_tests
./scons platform=x86-64 bitcode=1                # build everything
./scons platform=x86-64 bitcode=1  smoke_tests   # run smoke_tests
./scons -j32 -k --verbose ...                    # parallelize, don't abort on first error, verbose output
./scons platform=... bitcode=1 use_sz=1 ...      # run using pnacl-sz instead of pnacl-llc
```

The scons tests are more conveniently wrapped in a testing script:

```none
pnacl/test.sh test-x86-32      # Runs the smoke tests as above, plus nonpexe_tests
pnacl/test.sh test-x86-64
pnacl/test.sh test-arm
pnacl/test.sh test-x86-32-sbtc # Runs the same tests, but with the sandboxed translator
pnacl/test.sh test-x86-64-sbtc
pnacl/test.sh test-arm-sbtc
```

## Other Compiler Tests

### LLVM regression tests

These tests can be run directly by running 'make check' in the build directory
(pnacl/build/llvm_x86_64). There are currently a few known failures which are
not marked as XFAIL in the usual LLVM way, so instead the regression tests can
also be run in the following way, which will ignore the known failures. (This
only works after you have built the PNaCl LLVM and have the build directory
available).

```none
pnacl/scripts/llvm-test.py --llvm-regression
```

The LLVM regression tests run quickly and should be run before every LLVM
commit, along with the scons tests.

The following tests take longer and are generally not run by the developers on
every commit, but they should be run for large changes, and are run on the
toolchain testing bots on every commit.

### GCC Torture tests

To run the gcc torture test suite against PNaCl, do the following:

```none
tools/toolchain_tester/torture_test.py pnacl x86-64 --concurrency=12
```

Other options are documented in torture_test.py and
tools/toolchain_tester/toolchain_tester.py

### LLVM Test Suite

To run the LLVM test suite against PNaCl, do

```none
pnacl/scripts/llvm-test.py --testsuite-all --arch x86-64
```

This is equivalent to

```none
pnacl/scripts/llvm-test.py --testsuite-prereq --arch x86-64
pnacl/scripts/llvm-test.py --testsuite-clean
pnacl/scripts/llvm-test.py --testsuite-configure
pnacl/scripts/llvm-test.py --testsuite-run --arch x86-64
pnacl/scripts/llvm-test.py --testsuite-report --arch x86-64
```

The test suite needs to be cleaned and re-configured each time you switch
architectures, but the results files persist after cleaning, so testsuite-report
will give the result from the last run for that architecture. Because there is
no good way to filter the set of tests that are actually run and there are a lot
of known failures (e.g. tests that have not been ported to use newlib), this
test suite takes a long time to run.

You need to set LD_LIBRARY_PATH before running the test suite, otherwise the
native build of each test, whose output the PNaCl build is validated against,
will fail to run. It's also helpful to set PNACL_CONCURRENCY.

```none
LD_LIBRARY_PATH=$NACL_DIR/native_client/toolchain/linux_x86/pnacl_newlib/lib PNACL_CONCURRENCY=12 pnacl/scripts/llvm-test.py --testsuite-all --arch x86-64
```

### Spec2k

If you have access to the spec2k benchmark suite you can use the harness in
tests/spec2k/ to run some more extensive tests.

If SPEC_DIR is the directory containing the benchmark code/data, run:

```none
pnacl/test.sh test-spec <SPEC_DIR> SetupPnaclX8664Opt
```

test.sh is a convenience script and clobbers old results, etc. The actual steps
are:

```none
pushd tests/spec2k
./run_all.sh CleanBenchmarks
./run_all.sh PopulateFromSpecHarness <SPEC_DIR>
./run_all.sh TimedBuildAndRunBenchmarks SETUP
popd
```

where SETUP is for example "SetupPnaclArmOpt". See "run_all.sh" for a full list
of setups.

### Subzero standalone tests

These tests depend on having built PNaCl's branch of LLVM from source (uses some
of the headers, etc.). See "checking out PNaCl sources, building and testing"
below.

In general look at the Makefile.standalone file for various targets and options.
E.g., you can compile with DEBUG=1 while debugging, so that variables aren't
optimized-out and functions aren't so inlined, etc.

```none
cd toolchain_build/src/subzero
make -j32 -f Makefile.standalone check
```

## Toolchain Development

### TL;DR for checking out PNaCl sources, building and testing

If you are using Chromium's build of clang to build PNaCl (the default) and you
have never done so, you need to install it.

From the native_client/ directory:

```none
../tools/clang/scripts/update.py
```

From now on, when you run 'gclient sync' clang will also be updated if needed.

Now you can build the PNaCl toolchain:

```none
toolchain_build/toolchain_build_pnacl.py --verbose --sync --clobber --install toolchain/linux_x86/pnacl_newlib_raw
rm -rf toolchain/linux_x86/pnacl_newlib
ln -s pnacl_newlib_raw toolchain/linux_x86/pnacl_newlib
# syncs the sources and builds the developer toolchain
# Add the --build-sbtc flag to also build the sandboxed translator
```

If everything built correctly, run the scons tests as described above.

### Important Scripts

Most tasks related to the toolchains are focused a few scripts.

*   the driver scripts in the directory: `pnacl/driver/`
*   the toolchain builder and helper script:
            `toolchain_build/toolchain_build_pnacl.py`
*   the old toolchain build script, still used for the sandboxed
            translator: pnacl/build.sh

#### toolchain_build/toolchain_build_pnacl.py

This script checks out and builds the complete PNaCl toolchain. Run with the -h
flag for a full list of flags and build targets. Here are the most important
flags:

```none
optional arguments:
--build-sbtc          Build the sandboxed translators
...
Usage: toolchain_build_pnacl.py [options] [targets...]
...
Options:
  -h, --help            show this help message and exit
  -v, --verbose         Produce more output.
  -c, --clobber         Clobber working directories before building.
  -y, --sync            Run source target commands [i.e. sync the targets' sources before building them]
  -i, --ignore-dependencies
                        Ignore target dependencies and build only the
                        specified target.
  --install=INSTALL     After building, copy contents of build packages to the
                        specified directory
```

#### Sandboxed translator build

The build.sh script is now only used to build the sandboxed translator. It has 2
relevant arguments:

*   **translator-all**: builds the sandboxed translator nexes (not
            included in 'build-all' or 'all')
*   **clean**: cleans the build files

However, it is called from toolchain_build_pnacl.py for the bots, and it expects
to use the toolchain built by the translator_compiler target of
toolchain_build_pnacl.py. Use the --build-sbtc flag of toolchain_build_pnacl.py
and build everything (or just the sandboxed_translators target and its
dependencies) for that.

#### pnacl/test.sh

> This script runs toolchain tests. It deletes old testing artifacts before
> running. The main invocation is test-all.

#### pnacl/driver/\*

> This directory contains the compiler drivers, which are in Python. If you
> change them, run e.g. "toolchain_build/toolchain_build_pnacl.py --install
> toolchain/linux_x86/pnacl_newlib driver_i686_linux" to install them.

Git Repositories

Most of the code required to build the PNaCl toolchain lives in git repositories
hosted on chromium.googlesource.com. The build.sh script will automatically
check out and update these repositories. The checked-out code is placed in
toolchain_build/src.

Relevant Directories

<table>
<tr>
<td> toolchain_build/src</td>
<td> holds all the git checkouts managed by toolchain_build_pnacl.py</td>
</tr>
<tr>
<td> toolchain/${os}_x86/pnacl_newlib</td>
<td> root of PNaCl toolchain installed by package_version.py</td>
</tr>
<tr>
<td> toolchain_build/out </td>
<td> holds build directories for the pnacl toolchain</td>
</tr>
<tr>
<td> toolchain_build/out/toolchain_build.log </td>
<td> holds the logs of the builds</td>
</tr>
</table>

## Process for making changes to the PNaCl toolchain

If you've made changes to PNaCl toolchain, either to the driver or git
repositories, you can rebuild PNaCl by doing:

```none
toolchain_build/toolchain_build_pnacl.py -v --install toolchain/linux_x86/pnacl_newlib_raw     # Incrementally build and reinstall
pnacl/test.sh test-all      # Run the scons tests
```

## How to send out reviews for git repos

To send out reviews for the git repos, you must have [depot
tools](/developers/how-tos/install-depot-tools). Code reviews are sent using the
git cl script in depot_tools.

**If you are going to commit**, you will need to have a credential in your
`~/.netrc` file:

1.  Go to <https://chromium.googlesource.com/new-password>
2.  Login with your @chromium.org account.
3.  Follow the instructions in the "Staying Authenticated" section.

The first time you set up code review you'll need to let `git cl` do some setup:

**```none
cd toolchain_build/src/llvm
git cl config
```**

Just press Enter at the prompts.

To do local development, create a new branch:

**```none
git checkout -b mybranch origin/master
```**

**The last argument to git checkout (the start point) is important because if
your local branch does not track the remote master branch, then `git cl land`
will not work)**

Edit your code, and commit as many times as you want:

**```none
git commit -a -m “comment describing changes”
```**

**Then use git-cl to upload to the code review site:**

**```none
git cl upload
```**

You'll need to authenticate to codereview.chromium.org also. Most likely this
will be your ldap email. This will upload the review, and give you an issue
number, e.g. http://codereview.chromium.org/1557002. Go to the issue webpage,
and edit the change summary, add reviewers, then hit send mail. Command line
options from gcl such as reviewers ("-r") and send mail ("--send_mail") are also
available with git cl.

Git cl associates each branch with a review issue ID. So you if you `git cl
upload` again from the same branch, it will upload a new patch set to the same
issue.

When you get the LGTM, you can push the changes.

```none
git cl land
```

If there have been changes on the master branch since you forked your local
branch, git cl may complain, in which case you should rebase it against the new
HEAD before you push:

```none
git fetch origin
git rebase origin/master
```

#### Update stable revisions in the pnacl COMPONENT_REVISIONS, test, review and submit

At this point the official **toolchain builder** is still not incorporating your
changes as it builds off of specific commit IDs in the external repo.

To fix this modify **native_client/pnacl/COMPONENT_REVISIONS** to point at your
new commit ID which look like

```none
llvm=efa7b3d5c85f0242787eb091dadedc2727215df4
```

You can then make a standard native_client change list and test the CL with
trybots (CLs affecting pnacl/ will end up selecting pnacl trybots). Upload,
trybot, get an LGTM, then commit. You can also test locally by cleaning the
toolchain directory, rebuilding, and testing as described earlier.

Or, you can automatically update the file using a script: pnacl/deps_update.py

### **Change the native client pnacl_newlib.json file, test, review and submit**

**Finally, to allow users to download the new revision built by the toolchain
builder, look in the** toolchain_revisions/pnacl_newlib.json file. The current
revision is in a line near the end:

```none
"revision": 13392,
```

Update this to an svn revision number that is new enough to include your
COMPONENT_REVISIONS change by running:

```none
build/package_version/package_version.py setrevision --revision-package pnacl_newlib --revision 12345
```

Now create a CL and test it with some try jobs (e.g., `git cl try my_cl_name`).
NOTE: You will need to wait for a toolchain to be built ([see the
waterfall](http://build.chromium.org/p/client.nacl.toolchain/waterfall)) before
this change actually works (downloads a toolchain tarball). Also see
build/update_pnacl_tool_revisions.py to automate this process.

### Misc Notes

Building Validators

In the case that you want to run the native client validators on your nexes,
build the validators like so:

```none
./scons targetplatform=arm sdl=none arm-ncval-core
./scons platform=x86-32 sdl=none ncval
./scons platform=x86-64 sdl=none ncval
```

The validators will be placed in scons-out/{host}-{platform}/staging/.
