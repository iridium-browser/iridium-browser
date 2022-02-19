[![License: GPL
v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Build Status](https://travis-ci.com/freetype/freetype2-testing.svg?branch=master)](https://travis-ci.com/freetype/freetype2-testing)

# FreeType

https://www.freetype.org

FreeType is a freely available software library to render fonts.

# Targets

Targets can be virtually of any shape or size.  They do not have to inherit
from a specific interface as long as they are capable of working with an input
blob of `const uint8_t*` whose length is measured in `size_t` (e.g. see
[`base.h`](/fuzzing/src/targets/base.h#L64)).

## Design Choice

All targets get shipped in a single static libray which is linked against by
the [driver](fuzzing/src/driver) and the [fuzzers](/fuzzing/src/fuzzers).
This method results in a significant size overhead in the fuzz targets,
compared to only linking dedicated objects.  However, two main reasons lead to
this decision:

1. Assembling the targets [in a single
   place](/fuzzing/src/targets/CMakeLists.txt) keeps the build process
   maintainable since there are many dependencies between the
   [targets](/fuzzing/src/targets), their [iterators](/fuzzing/src/iterators),
   and their [visitors](/fuzzing/src/visitors).

2. As of August 2018, [OSS-Fuzz](https://github.com/google/oss-fuzz) does not
   support any kind of dynamic linking.

## Adding a New Target

Several steps have to be completed to make a new target available throughout
all parts of the fuzzing subproject:

- Basics:
    - [Build](/fuzzing/src/targets) the new target and export it as part of
      the [target library](/fuzzing/src/targets/CMakeLists.txt).
    - Prepare a suitable (new and unique) [fuzz corpus](/fuzzing/corpora).
    - Register the new target in the root
      [CMakeLists.txt](/fuzzing/CMakeLists.txt) (`add_fuzz_target()`) to
      automatically generate a fuzzer and regression tests from it.

- Add the new target to the [test driver](/fuzzing/src/driver/driver.cpp).

- Prepare the new target for OSS-Fuzz:
    - Provide [settings](/fuzzing/settings/oss-fuzz) if necessary.
    - Tell OSS-Fuzz's build process to
      [prepare and use](/fuzzing/scripts/prepare-oss-fuzz.sh) the new target.

- Test the new target:
    - Build the new target with
      [custom-script.sh](/fuzzing/scripts/custom-build.sh), enable the logger,
      and confirm that all API functions are called as expected.
    - Make sure [run-travis-ci.sh](/fuzzing/scripts/run-travis-ci.sh) runs
      successfully and executes the new corpus.
    - Ensure that the changes do not break FreeType's [OSS-Fuzz
      build](https://github.com/google/oss-fuzz/blob/master/docs/new_project_guide.md#testing-locally)
      and, in addition, that the new fuzzer fuzzes (let it reach at least
      10,000 runs locally).
