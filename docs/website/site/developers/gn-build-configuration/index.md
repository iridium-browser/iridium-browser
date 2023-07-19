---
breadcrumbs:
- - /developers
  - For Developers
page_name: gn-build-configuration
title: GN build configuration
---

This page provides some common build setups for the GN build. It assumes you
already [got a Chromium checkout](/developers/how-tos/get-the-code).

See also

*   Run "gn help" from the command line.
*   [All GN Docs](https://gn.googlesource.com/gn/+/HEAD/docs/)
*   [GN Quick Start
            Guide](https://gn.googlesource.com/gn/+/HEAD/docs/quick_start.md)
*   [GN
            Reference](https://gn.googlesource.com/gn/+/HEAD/docs/reference.md)
            (a dump of everything from "gn help" in one web page).

## Understanding GN build flags

Recall that in GN, you pick your own build directory. These should generally go
in a subdirectory of src/out. You set build arguments on a build directory by
typing:

```
$ gn args out/mybuild
```

This will bring up an editor. The format of the stuff in the args file is just
GN code, so set variables according to normal GN syntax (true/false for
booleans, double-quotes for string values, # for comments). When you close the
editor, a build will be made in that directory. To change the arguments for an
existing build, just re-run this command on the existing directory.

You can get a list of all available build arguments for a given build directory,
with documentation, by typing

```
$ gn args out/mybuild --list
```

To get documentation for a single flag (in this example, is_component_build):

```
$ gn args out/mybuild --list=is_component_build
```

You have to list your build directory as the first argument because the
available arguments and their default values are build-specific. For example,
setting Android as your target OS might expose new Android-specific build
arguments or use different default values.

*"GN args" as used on this page are **not** the command line arguments passed to
GN. They refer to the individual variables that are passed as part of the --args
command line flag and/or written to the args.gn file.*

## Common build variants

### Release build

The default GN build is debug. To do a release build:

```
is_debug = false
```

On Android, you can toggle ProGuard on/off with:

```
is_java_debug = false # Defaults to is_debug.
```

Trybots that run release builds have DCHECKs enabled, to catch potential bugs.

```
dcheck_always_on = true
```

### Component build

The [component
build](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/component_build.md)
links many parts of Chrome into separate shared libraries to avoid the long link
step at the end. It is the default when compiling debug non-iOS builds and most
developers use this mode for everyday builds and debugging. Startup is slower
and some linker optimizations won't work, so don't do benchmarks in this mode.
Some people like to turn it on for release builds to get both faster links and
reasonable runtime performance.

```
is_component_build = true
```

### Faster builds with no or minimal symbols

The symbol_level setting varies from 0 (no symbols or minimal symbols) to 2
(full symbols). Lower levels make debugging almost impossible, but the build
will be much faster. It can be useful in some cases where you just want a build
ASAP (many build bots do this).

```
symbol_level = 0
```

Alternately you can set symbol_level=1 which will build almost as fast as
symbol_level=0 but will give you additional information in call stacks.

### Disable Native Client

Most developers don't normally need to test Native Client capabilities and can
speed up the build by disabling it.

```
enable_nacl = false
```

### Remove WebCore symbols

WebCore has lots of templates that account for a large portion of the debugging
symbols. If you're not debugging WebCore, you can set the blink symbol level to
0 or 1:

```
blink_symbol_level=0
```

### Remove v8 symbols

v8 is often the second-largest source of debugging symbols. If you're not
debugging v8, you can set the v8 symbol level to 0 or 1:

```
v8_symbol_level=0
```

### Overriding the CPU architecture

By default, the GN build will match that of the host OS and CPU architecture. To
override:

```
target_cpu = "x86"
```

Possible values for the `target_cpu`:

*   Windows supports "`x86`" and "`x64`". Since building is only supported
            on 64-bit machines, the default will always be "`x64`".

*   Mac and desktop Linux supports only "`x64`". On desktop Linux you
            might also theoretically try any of the ARM or MIPS architecture
            strings form the Android section below, but these aren't supported
            or tested and you will also need a sysroot.

*   Chrome OS supports "`x86`" and "`x64`", but to build a 32-bit binary you
            will need to use a sysroot on a 64-bit machine.

*   If you specify an Android build (see below) the default CPU
            architecture will be "`arm`". You could try overriding it to "`arm64`",
            "`x86`", "`mipsel`", or "`mips64el`" but the GN builds for these aren't
            regularly tested.

### Goma

Googlers can use this for distributed builds. `goma_dir` is only required if you
use the Goma tools not in the depot_tools.

```
use_goma = true
goma_dir = "/home/me/somewhere/goma" # Optional
```

### Official Chrome build

This build requires that you are a Googler with src-internal checked out.

Use these args for official builds:

```
is_official_build = true
is_chrome_branded = true
is_debug = false
```

For 32-bit official builds, append this arg to the above set:

```
target_cpu = "x86"
```

On Windows and Mac you also need to add the following entry to your .gclient
file to automatically fetch the PGO profiles required to do an official build:

```
solutions = [
  {
    "name": "src",
    # ...
    "custom_vars": {
      "checkout_pgo_profiles": True,
    },
  },
],
```

### You can also set the following GN argument to disable PGO if needed:

```
chrome_pgo_phase = 0
```

### Windows

There is a `gn gen` argument (`--ide`) for producing Visual Studio project and
solution files:

```
$ gn gen out\mybuild --ide=vs
```

Projects are configured for VS 2019 by default.

See [this
page](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/windows_build_instructions.md)
for more information on configuring Chromium builds for Windows.

### Android build (from Linux)

This assumes you've already followed the [Android build
instructions](/developers/how-tos/android-build-instructions) to check out.

It is easy to use the same checkout on Linux to build both Android and desktop
Linux versions of Chrome. Your .gclient file must list Android, however, to get
the proper SDKs downloaded. This will happen automatically if you follow the
Android checkout instructions. To add this to an existing Linux checkout, add
`target_os` to your `.gclient` file (in the directory above src), and run `gclient
runhooks`.

```
solutions = [
  ...existing stuff in here...
]
target_os = [ 'android' ]  # Add this to get Android stuff checked out.
```

### Chrome OS build (from Linux)

This will build the Chrome OS variant of the browser that is distributed with
the operating system. You can run it on your Linux desktop for feature
development.

```
target_os = "chromeos"
```

Checkouts which are used to build Chrome OS builds must also have `'chromeos'`
added to the `target_os` list in the .gclient file. After making this change,
you will need to run `gclient sync` once.

```
solutions = [
   ...existing stuff in here...
]
target_os = ['chromeos']
# Or if you also build e.g. android, this might be
# target_os = ['android', 'chromeos']
```
