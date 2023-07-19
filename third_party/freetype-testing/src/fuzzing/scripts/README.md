[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Build Status](https://travis-ci.com/freetype/freetype2-testing.svg?branch=master)](https://travis-ci.com/freetype/freetype2-testing)

# FreeType

https://www.freetype.org

FreeType is a freely available software library to render fonts.

# Scripts

The scripts in this folder are designed to build (and prepare) the fuzz
targets in (and for) various scenarios.  At the moment, there are three main
stakeholders:

- **OSS-Fuzz** has two entry points.  The process is currently split into two
  parts to allow for some hacking in [OSS-Fuzz's
  `build.sh`](https://github.com/google/oss-fuzz/blob/master/projects/freetype2/build.sh).
    1. First, it calls [`build-fuzzers.sh`](build-fuzzers.sh).  All
       environment variables are set by OSS-Fuzz and in [OSS-Fuzz's
       `build.sh`](https://github.com/google/oss-fuzz/blob/master/projects/freetype2/build.sh).
    2. After that, OSS-Fuzz calls
       [`prepare-oss-fuzz.sh`](prepare-oss-fuzz.sh), which copies all files to
       their designated places and zips up the corpora.
              
- **Travis CI**  calls [`run-travis-ci.sh`](run-travis-ci.sh), which supports
  two different rows/lanes:
    1. [**Regression Suite**](travis-ci/regression-suite.sh):  build a
       standard version of the [test driver](/fuzzing/src/driver) and run all
       available regression tests.
    2. [**OSS-Fuzz Build**](travis-ci/oss-fuzz-build.sh):  build OSS-Fuzz's
       fuzzers through OSS-Fuzz's infrastructure tools to confirm that the
       build process works as expected.

- **Developers** usually want to use [`custom-build.sh`](custom-build.sh) or
  [`cleanup.sh`](cleanup.sh).  See [Debugging the Fuzz
  Targets](#debugging-the-fuzz-targets) for details.

# Debugging the Fuzz Targets

This section demonstrates a few aspects of working with this subproject as a
*developer* when debugging FreeType.  Debugging other submodules or the fuzz
targets themselves works similarly but has a few edge cases that have to be
taken care of.

## Building the Fuzz Targets

As stated above, debugging sessions usually start like this:

```
$ ./custom-build.sh
```

After choosing all settings, the script takes care of initialising/resetting
relevant build systems before it starts the build process.  The result of this
process is a test driver artefact in `/fuzzing/build/bin`.

Due to reasons regarding caching specific builds (and running different setups
in parallel), the name of the test driver artefact is based on chosen
settings.  To keep simple things simple, `/fuzzing/build/bin/driver` will
always link to the most recently built driver.

More details in regard to using the test driver can be obtained from simply
questioning its usage message:

```
$ ./driver --help
```

The same holds true for the build script:

```
$ ./custom-build.sh --help
```

One specific option of the build script shall be highlighted here:

```
$ ./custom-build.sh --rebuild
```

`--rebuild` only make sense when used **after a completely successful build
without that option**.  Essentially, what it does is to call `make` in every
used build system to trigger a rebuild of the test driver **without**
resetting anything.  That way, rebuilding the test driver is a matter of
seconds instead of it taking up to a few minutes.  Mind, however, that using
`--rebuild` after an *unsuccessful* initial build results in undefined
behaviour and, most certainly, does not accomplish meaningful things.

## Making Changes to FreeType

FreeType lives in `/external/freetype2` as a *read-only* copy of the
repository.  Every build script always tries to pull the latest version of
FreeType.  Hence, calling `./custom-build.sh` ensures that the test driver is
built with FreeType's upstream.

When debugging FreeType, it is advisable to make changes to
`/external/freetype2` as these changes will be detected and used by
`./custom-build.sh --rebuild`.  **BEWARE**: calling `./custom-build.sh`
(without `--rebuild`) resets all changes made to that submodule.  Thus, it is
good practice to save changes in the form of a patch before rebuilding the
test target:

```
$ cd /external/freetype2
$ git diff >~/my.patch
```

Once a bug is fixed, this patch can be applied to a writable copy of
FreeType's repository.

## Valgrind

[Valgrind](http://valgrind.org) can be very helpful when debugging FreeType
and it works out of the box:

```
$ valgrind ./driver --foo ~/foo
```

The best performance and the best results can be achieved when using
`valgrind` without sanitizers and also without `glog`.

## Glog

[Google's Logging Module](https://github.com/google/glog) can be activated
through `custom-build.sh`.  If activated, it will write all logging messages
to a temporary file on the disk and, additionally, print `error` logs to
`stderr`.  Glog will only print *all* logging messages (especially `info`) to
`stderr` if advised per environment variable:

```
$ GLOG_logtostderr=1 ./driver --foo ~/foo
```

Further settings can be found here: http://rpg.ifi.uzh.ch/docs/glog.html
