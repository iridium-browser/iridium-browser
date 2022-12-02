---
breadcrumbs:
- - /developers
  - For Developers
page_name: gyp-environment-variables
title: GYP build parameters
---

Chromium no longer uses GYP but GN. The corresponding page is [GN build
configuration.](/developers/gn-build-configuration)

Hence, the following is **obsolete**.

GYP has many **environment variables** that configure the way Chrome is built.
By modifying these parameters, you can build chrome for different architectures,
or speed up the build process. Generally, we strive to have good defaults and
you shouldn't have to set many of these. The wiki also has some additional tips
on [common build
tasks](https://code.google.com/p/chromium/wiki/CommonBuildTasks).

## Specifying parameters

### Command-line

To specify build parameters for GYP, you can do so on the command line:

$ ./build/gyp_chromium -Dchromeos=1 -Dcomponent=shared_library

### Environment variable

Or with an environment variable:

$ export GYP_DEFINES="chromeos=1 component=shared_library" $
./build/gyp_chromium

### chromium.gyp_env file

Or in a chromium.gyp_env file in your chromium directory:

$ echo "{ 'GYP_DEFINES': 'chromeos=1 component=shared_library' }" &gt;
../chromium.gyp_env

### Some common file examples:

**Windows**:

{ 'GYP_DEFINES': 'component=shared_library' }

**Mac**:

{ 'GYP_DEFINES': 'fastbuild=1' }

**Linux**:

{ 'GYP_DEFINES': 'component=shared_library remove_webcore_debug_symbols=1' }

## ChromeOS on Linux:

{ 'GYP_DEFINES': 'chromeos=1 component=shared_library
remove_webcore_debug_symbols=1' }

## Android:

{ 'GYP_DEFINES': 'OS=android' }

## Recommended Parameters

For a full list of the main build variables and their defaults, see the
variables section in any \*.gyp or \*.gypi file, e.g.:

$ cat build/common.gypi

<table>
<tr>

<td>### Variable</td>

<td>### Explanation</td>

<td>### Linux</td>

<td>### Windows</td>

<td>### Mac</td>

</tr>
<tr>
<td>component=shared_library</td>
<td>If you dynamically link, you save a lot of time linking for a small time cost during startup. On Windows, this uses a DLL build and incremental linking, which makes linking much faster in Debug.</td>

<td><a href="/developers/gyp-environment-variables/menu_check.png"><img
alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>

<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
</tr>
<tr>
<td>fastbuild=1</td>
<td><b>Optional: removes all debug information, but builds much faster. </b>0 means all debug symbols (.pdb generated from .obj), 1 means link time symbols (.pdb generated from .lib) and 2 means no pdb at all (no debug information). In practice 0 and 1, will generate the exact same PE. Only 2 will generate a PE that could have it's .rdata to be ~40 bytes smaller.</td>

<td><a href="/developers/gyp-environment-variables/menu_check.png"><img
alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>

<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
</tr>
<tr>
<td>remove_webcore_debug_symbols=1</td>
<td>If you don't need to trace into WebKit, you can cut down the size and slowness of debug builds significantly by building WebKit without debug symbols. </td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a> </td>
</tr>
</table>

### Building a release build

To build Chrome in a release build, as well as building in the out/Release
directory, you may add the following flags:

<table>
<tr>

<td>### Variable</td>

<td>### Explanation</td>

<td>### Linux</td>

<td>### Windows</td>

<td>### Mac</td>

</tr>
<tr>
<td>buildtype=Official</td>
<td>Builds the heavily optimized official release build (invalid for builds in out/Debug). Other options include 'Dev', for development/testing. Official builds take <b>*drastically*</b> longer to build, and are most likely not what you want.</td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a> </td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a> </td>
</tr>
<tr>
<td>branding=Chrome</td>
<td>Changes the branding from 'Chromium' to 'Chrome', for official builds.</td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a> </td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
</tr>
</table>

### Use goma distributed build (Googlers only)

See [go/ma](https://sites.google.com/a/google.com/goma/) for more information on
how to set up goma.

### Use the icecc linker (Linux only)

[Icecc](https://github.com/icecc/icecream) is the distributed compiler with a
central scheduler to share build load. Currently, many external contributors use
it. e.g. Intel, Opera, Samsung.

<table>
<tr>

<td>### Variable</td>

<td>### Explanation</td>

<td>### Linux</td>

<td>### Windows</td>

<td>### Mac</td>

</tr>
<tr>
<td>linux_use_bundled_binutils=0</td>
<td>-B option is not supported. See <a href="https://github.com/icecc/icecream/commit/b2ce5b9cc4bd1900f55c3684214e409fa81e7a92">this commit</a> for more details.</td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
</tr>
<tr>
<td>linux_use_debug_fission=0</td>
<td><a href="http://gcc.gnu.org/wiki/DebugFission">Debug fission</a> is not supported. See <a href="https://github.com/icecc/icecream/issues/86">this bug</a> for more details.</td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
</tr>
<tr>
<td>clang=0</td>
<td>Icecc doesn't support clang yet.</td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
</tr>
</table>

### Build ChromeOS on Linux (Linux only)

To build ChromeOS on Linux, use the following gyp variables and the regular
chrome target will run ChromeOS instead of Chrome.

<table>
<tr>

<td>### Variable</td>

<td>### Explanation</td>

<td>### Linux</td>

<td>### Windows</td>

<td>### Mac</td>

</tr>
<tr>
<td>chromeos=1</td>
<td>Build for the ChromeOS platform instead of Linux.</td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
</tr>
</table>

### Build Android (Linux only)

To build ChromeOS on Linux, use the following gyp variables and the either the
chrome_shell_apk or webview_instrumentation_apk target.

<table>
<tr>

<td>### Variable</td>

<td>### Explanation</td>

<td>### Linux</td>

<td>### Windows</td>

<td>### Mac</td>

</tr>
<tr>
<td>OS=android</td>
<td>Build for the Android platform instead of Linux.</td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
</tr>
<tr>
<td>target_arch=ia32</td>
<td><b>Optional. </b>Use this if building for x86 targets.</td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
</tr>
<tr>
<td>target_arch=mipsel</td>
<td><b>Optional. </b>Use this if building for MIPS targets.</td>
<td><a href="/developers/gyp-environment-variables/menu_check.png"><img alt="image" src="/developers/gyp-environment-variables/menu_check.png"></a></td>
</tr>
</table>
