# GN Frequently Asked Questions

[TOC]

## Where is the GN documentation?

GN has extensive built-in help, so you can run `gn help`, but you can also see
all of the help on [the reference page](reference.md). See also the [quick
start](quick_start.md) guide and the [language and operation
details](language.md).

## Can I generate XCode or Visual Studio projects?

You can generate skeleton (or wrapper) projects for Xcode, Visual Studio,
QTCreator, and Eclipse that will list the files and targets in the build, but
use Ninja to do the actual build. You cannot generate "real" projects that look
and compile like native ones.

Run `gn help gen` for more details.

## How do I do cross-compiles?

GN has robust support for doing cross compiles and building things for
multiple architectures in a single build.

See [GNCrossCompiles](cross_compiles.md) for more info.

## Can I control what targets are built by default?

Yes! If you create a group target called "default" in the top-level (root) build
file, i.e., "//:default", GN will tell Ninja to build that by default, rather
than building everything.

## Are there any public presentations on GN?

[There's at least one](https://docs.google.com/presentation/d/15Zwb53JcncHfEwHpnG_PoIbbzQ3GQi_cpujYwbpcbZo/edit?usp=sharing), from 2015. There
haven't been big changes since then apart from moving it to a standalone
repo, so it should still be relevant.

## What is the order of flags and values given to the compiler?

The final values of compiler flags, linker flags, defines, and include
directories are collected from various sources by GN. The ordering is defined
as:

1. Those set directly on the current target (not in a config).
2. Those set on the `configs` on the target in order that the configs appear in the list.
3. Those set on the `all_dependent_configs` on the target in order that the configs appear in the list.
4. Those set on the `public_configs` on the target in order that those configs appear in the list.
5. `all_dependent_configs` pulled from dependencies, in the order of the `deps` list. This is done recursively. If a config appears more than once, only the first occurrence will be used.
6. `public_configs` pulled from dependencies, in the order of the `deps` list. If a dependency is public, they will be applied recursively.

If you need a specific relative ordering of values you may need to put those
flags in a config and prepend or append that config in a way that produces the
desired result.

## How can a target affect those that depend on it?

The main way that information flows up the dependency graph is via
`public_configs`. This allows a target to add preprocessor defines, compiler
flags, and linker flags to targets that depend on it. A typical example is a
library that requires `include_dirs` and `defines` to be set in a certain way
for its headers to work. It would put its values in a config:

```
config("icu_config") {
  include_dirs = [ "//third_party/icu" ]
  defines = [ "U_USING_ICU_NAMESPACE=0" ]
}
```

The library would then reference that as a `public_config` which will apply it
to any target that directly depends on the `icu` target:

```
shared_library("icu") {
  sources = [ ... ]
  deps = [ ... ]

  public_configs = [ ":icu_config" ]  # Label of config defined above.
}
```

A `public_config` applies only to direct dependencies of the target. If a target
wants to "republish" the `public_configs` from its dependencies, it would list
those dependencies in its `public_deps`. In this example, a "browser" library
might use ICU headers in its own headers, so anything that depends on it also
needs to get the ICU configuration:

```
shared_library("browser") {
  ...

  # Anything that depends on this "browser" library will also get ICU's settings.
  public_deps = [ "//third_party/icu" ]
}
```

Another way apply settings up the dependency graph is with
`all_dependent_configs` which works like `public_configs` except that it is
applied to all dependent targets regardless of `deps`/`public_deps`. Use of this
feature is discouraged because it is easy to accumulate lots of unnecessary
settings in a large project. Ideally all targets can define which information
their dependencies need and can control this explicitly with `public_deps`.

The last way that information can be collected across the dependency graph is
with the metadata feature. This allows data (see `gn help metadata`) to be
collected from targets to be written to disk (see `gn help generated_file`) for
the build to use. Computed metadata values are written after all BUILD.gn files
are processed and are not available to the GN script.

Sometimes people want to write conditional GN code based on values of a
dependency. This is not possible: GN has no defined order for loading BUILD.gn
files (this allows pararellism) so GN may not have even loaded the file
containing a dependency when you might want information about it. The only way
information flows around the dependency graph is if GN itself is propagating
that data after the targets are defined.

## How can a target affect its dependencies?

Sometimes you might have a dependency graph **A 🠲 Z** or a longer chain **A 🠲 B
🠲 C 🠲 Z** and want to control some aspect of **Z** when used from **A**. This is
not possible in GN: information only flows up the dependency chain.

Every label in GN is compiled once per _toolchain_. This means that every target
that depends on **B** gets the same version of **B**. If you need different
variants of **B** there are only two options:

1. Explicitly define two similar but differently named targets encoding the
variations you need. This can be done without specifying everything twice using
a template or by writing things like the sources to a variable and using that
variable in each version of the target.

2. Use different toolchains. This is commonly used to encode "host" versus
"target" differences or to compile parts of a project with something like ASAN.
It is possible to use toolchains to encode any variation you might desire but
this can be difficult to manage and might be impossible or discoraged depending
on how your project is set up (Chrome and Fuchsia use toolchains for specific
purposes only).

## How can I recursively copy a directory as a build step?

Sometimes people want to write a build action that expresses copying all files
(possibly recursively, possily not) from a source directory without specifying
all files in that directory in a BUILD file. This is not possible to express:
correct builds must list all inputs. Most approaches people try to work around
this break in some way for incremental builds, either the build step is run
every time (the build is always "dirty"), file modifications will be missed, or
file additions will be missed.

One thing people try is to write an action that declares an input directory and
an output directory and have it copy all files from the source to the
destination. But incremental builds are likely going to be incorrect if you do
this. Ninja determines if an output is in need of rebuilding by comparing the
last modified date of the source to the last modified date of the destination.
Since almost no filesystems propagate the last modified date of files to their
directory, modifications to files in the source will not trigger an incremental
rebuild.

Beware when testing this: most filesystems update the last modified date of the
parent directory (but not recursive parents) when adding to or removing a file
from that directory so this will appear to work in many cases. But no modern
production filesystems propagate modification times of the contents of the files
to any directories because it would be very slow. The result will be that
modifications to the source files will not be reflected in the output when doing
incremental builds.

Another thing people try is to write all of the source files to a "depfile" (see
`gn help depfile`) and to write a single stamp file that tracks the modified
date of the output. This approach also may appear to work but is subtly wrong:
the additions of new files to the source directory will not trigger the build
step and that addition will not be reflected in an incremental build.

## How can I reference all files in a directory (glob)?

Sometimes people want to automatically refer to all files in a directory,
typically something like `"*.cc"` for the sources. This is called a "glob."

Globs are not supported in GN. In order for Ninja to know when to re-run
GN, it would need to check the directory modification times of any
directories being globbed. Directory modification times that reflect
additions and removals of files are not as reliably implemented across
platforms and filesystems as file modification times (for example, it is
not supported on Windows FAT32 drives).

Even if directory modification times work properly on your build systems,
GN's philosophy prefers a very explicit build specification style that
is contrary to globs.

## Why does "gn check" complain about conditionally included headers?

The "gn check" feature (see `gn help check`) validates that the source code's
use of header files follows the requirements set up in the build. It can be a
very useful tool for ensuring build correctness.

GN scans the source code for `#include` directives and checks that the included
files are allowed given the specification of the build. But it is relatively
simplistic and does not understand the preprocessor. This means that some
headers that are correctly included for a different build variant might be
flagged by GN. To disable checking of an include, append a "nogncheck"
annotation to the include line:

```
#if defined(OS_ANDROID)
#include "src/android/foo/bar.h"  // nogncheck
#endif
```

Correctly handling these cases requires a full preprocessor implementation
because many preprocessor conditions depend on values set by other headers.
Implementing this would require new code and complexity to define the toolchain
and run the preprocessor, and also means that a full build be done before doing
the check (since some headers will be generated at build-time). So far, the
complexity and disadvantages have outweighed the advantages of a perfectly
correct "gn check" implementation.

## Why does "gn check" miss my header?

The "gn check" feature (see previous question) only checks for headers that have
been declared in the current toolchain. So if your header never appears in a
`sources` or `public` list, any file will be able to include it without "gn
check" failing. As a result, targets should always list all headers they contain
even though listing them does not affect the build.

Sometimes a feature request is made to flag unknown headers so that people will
know they should be added to the build. But the silent omission of headers
outside of the current toolchain is an important feature that limits the
necessity of "nogncheck" annotations (see previous question).

In a large project like Chrome, many platform-specific headers will only be
defined in that platform's build (for example, Android-specific headers would
only be listed in the build when compiling for Android). Because the checking
doesn't understand the preprocessor, checking unknown files would flag uses of
these headers even if they were properly guarded by platform conditionals. By
ignoring headers outside of the current toolchain, the "nogncheck" annotations
can be omitted for most platform-specific files.
