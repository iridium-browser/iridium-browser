---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: instrumented-libraries-for-dynamic-tools
title: Instrumented libraries for dynamic tools
---

Instrumented libraries are a part of Chromium's development infrastructure. They
are intended to complement sanitizer tools
([AddressSanitizer](/developers/testing/addresssanitizer),
[MemorySanitizer](/developers/testing/memorysanitizer),
[ThreadSanitizer](/developers/testing/threadsanitizer-tsan-v2)).

Only Ubuntu Trusty x86_64 is supported at this time.

## Overview

Sanitizer tools rely on compile-time instrumentation. However, Chromium code may
call into system-installed third-party shared libraries, which were not built
with the appropriate instrumentation. This is a problem:

*   bugs in third-party libraries, which may affect Chromium, go
            undetected,
*   certain Chromium bugs may go undetected (e.g. passing an invalid
            buffer to third-party code),
*   MemorySanitizer generates lots of bogus reports, which makes it
            unusable. This happens because MSan doesn't recognize any memory
            initialization which happens in uninstrumented code.

To avoid this issue, we've made it possible to make Chromium use
sanitizer-instrumented versions of third-party DSOs. By setting a GYP flag, you
can either have them built from source during Chromium build, or download
pre-built binaries from Google Storage. The list contains ~50 third-party
packages, which should cover most of the DSO dependencies of Chrome and tests
(enough at least to run MSan without bogus reports).

## Using pre-built binaries

Follow the MemorySanitizer [instructions](/developers/testing/memorysanitizer).

Note that we don't provide pre-built binaries for every configuration. At this
point in time only MSan is supported, with `msan_track_origins` either 0 or 2.

## Building from source

First you need to install build dependencies:

```none
sudo third_party/instrumented_libraries/scripts/install-build-deps.sh
```

Additionally, if you have gccgo installed, you probably want to remove it with:

```none
sudo apt-get remove --purge gccgo-4.9
```

With this package installed, running clang++ gives the error cannot find
-lstdc++.

To build instrumented libraries from source, add
`use_locally_built_instrumented_libraries=true` to `args.gn`. This will add ~50
extra steps to the build. Each step runs a script which does the following:

*   checks out a specific package with `apt-get source`,
*   maybe applies a Chromium-specific patch,
*   builds the package using `./configure` + `make`,
*   installs the shared libraries into
            `$OUTPUT_DIR/instrumented_libraries/<sanitizer_name>/lib/`,
*   copies the source archives to
            `$OUTPUT_DIR/instrumented_libraries/sources/`.

GOMA is supported (just add `use_goma=1`).

## Adding new packages

You'll need to ping earthdok@ or glider@ to do this. The information below is
for reference.

To add a new package, you need to do the following:

*   get OSS compliance approval,
*   add a new target to `third_party/instrumented_libraries/BUILD.gn`,
*   add the package to
            `third_party/instrumented_libraries/scripts/install-build-deps.sh`,
*   make sure it builds and works on Trusty (i.e. where applicable),
*   update the pre-built binaries.

Usually you want to use the same configure flags that `debian/rules` uses.

To rebuild the binaries, run:

```none
third_party/instrumented_libraries/scripts/build_and_package.py all
```

The entire process will take several hours. For that reason, it is recommended
to use --parallel to build all configs concurrently, and -j96 (or whatever value
you prefer) to build multiple packages concurrently.

It's a good idea to not do this on Goobuntu. We have a couple GCE instances
configured for this. You can also build in an Ubuntu VM.

After uploading the archives to GCS as the script instructs, you'll get several
`.sha1` files. You should commit those under
`third_party/instrumented_libraries/binaries/`.
