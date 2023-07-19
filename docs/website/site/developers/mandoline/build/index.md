---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/mandoline
  - Mandoline (deprecated according to https://codereview.chromium.org/1677293002/)
page_name: build
title: 'Mandoline: Build, Debug & Test Instructions'
---

### Mandoline can be built out of a normal chromium checkout.

### Desktop:

Build mandoline. The code exclusively uses gn, and does not build with
is_component_build=true:

`gn gen out/Debug` `ninja -C out/Debug mandoline:all` Run mandoline with these
flags: `out/Debug/`mandoline `http://google.com`

If you're running over chromoting run:

`ninja -C out/Debug osmesa`

`DISPLAY=:20 out/Debug/`mandoline [`http://google.com`](http://google.com/)
--override-use-gl-with-osmesa-for-tests

### Android:

Follow the instructions here to make sure you have the necessary android files:
<https://code.google.com/p/chromium/wiki/AndroidBuildInstructions>. You don't
need to do a build yet, but make sure you have the necessary resources.

Build mojo for android:

`gn gen out/android_Debug --args='`symbol_level=1 target_os="android"
use_aura=true'

`ninja -C out/android_Debug mandoline:all`

**Note:** You will need to re-run \`gn gen\` without |use_aura| flag if you want
to build chrome or content-shell, since those targets do not use aura on
android.

Run the script to start mandoline on your device:

`./mandoline/tools/android/run_mandoline.py http://www.google.com `

If you just want to install, you can run the install_mandoline.py script in the
same directory.

**Build and test on the Android emulator with x86 Image:**

Build mojo for android x86:

`gn gen out/android_x86_Debug --args='`symbol_level=1 target_os="android"
target_cpu="x86" use_aura=true'

`ninja -C out/android_x86_Debug mandoline:all`

Run the script to start mandoline on the Android emulator:

`./mandoline/tools/android/run_mandoline.py --target=x86 http://www.google.com `

**Note:** You will need to create and run a x86 AVD(Android Virtual Device)
first.

**Debugging on android:**

For this to work you need to root your device. For Googlers follow instructions
[here](https://wiki.corp.google.com/twiki/bin/view/Main/MojoBuildInstructions).

Pass `--gdb` to `run_mandoline.py`. It'll do everything you need, which is the
following:

This assumes you've set things up for android, eg:

`. build/android/envsetup.sh`

Once per reboot/attaching you need to do the following:

`adb forward tcp:5039 tcp:5039`

On desktop pass in --wait-for-debugger to the shell.

On device:

`$ adb shell`

`$ gdbserver --attach :5039 pid`

pid should have been spit out when running. If not, ps | grep mojo

Back on desktop:

`./third_party/android_tools/ndk/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-gdb`

In gdb type the following:

`target remote localhost:5039`

And you should be able to continue.

You may not have symbols. You may need to do (in gdb):

`set solib-search-path path-to-your-android-debug-build`

If you are still seeing a garbage stack after all this it could be a couple of
possible things. I've had the best luck by inserting BreakDebugger() where the
crash is happening. With that gdb should break before you crash. If at this
point you still don't see symbols try the following (in gdb):

`info sharedlibrary`

Look for the library that the crash is happening in. If you see 'No' in the
column next to the library the crash is occurring in then gdb didn't find the
symbols. The path may be mangled, which is why gdb can't find the symbols. For
example, instead of libcore_services.so you might have
temp-1234-libcore_services.so. If this is the case, create a link to the real
one, eg (again, from within your build directory):

`ln -s libcore_services.so temp-1234-libcore_services.so`

It's entirely possible the library you want is not at the root of the build
directory. If that is the case you'll need to copy/link things around so that
the library gdb is looking for can be found.

Also, make sure you run set solib-search-path after you hit the breakpoint you
added.

**Testing:**

Beyond the unit tests associated with Mojo and Mandoline code, there is a suite
of Mojo application tests.

These are Mojo applications hosting GTest suites and fixtures, run in the
shell/runner with a helper script:

./mojo/tools/apptest_runner.py out/Debug

./mojo/tools/apptest_runner.py out/android_Debug

To run a unittest on Android, use the helper script:

build/android/test_runner.py gtest --s html_viewer_unittests
--output-directory=out/android_Debug

**Performance tests:**

To run a performance test, use the run_benchmark script with
--browser=mandoline-debug or --browser=mandoline-release, like:

./tools/perf/run_benchmark --browser=mandoline-debug &lt;test_suite_name&gt;

--browser-{debug,release} assumes that the binary locates at ./out/{Debug,
Release}. If you use a different output directory, use --browser=exact
--browser-executable=path/to/mandoline/including/binary/name.

Use the run_benchmark script with the list command to show all available test
suites.

Telemetry runs tests with archived Web resources. To launch Mandoline manually
and let it load those Web resources, use the following commands: (Take
typcial_25 as example, change the wpr file path if you want to load pages from a
different page set.)

third_party/webpagereplay/replay.py --host=127.0.0.1 --port=54813
--ssl_port=54814 --no-dns_forwarding --use_closest_match --log_level=warning
tools/perf/page_sets/data/typical_25_002.wpr

mandoline --ignore-certificate-errors "--host-resolver-rules=MAP \*
127.0.0.1,EXCLUDE localhost," --testing-fixed-http-port=54813
--testing-fixed-https-port=54814 &lt;some_url&gt;

**Layout tests:**

Run by way:

`./third_party/WebKit/Tools/Scripts/run-webkit-tests \`

` --driver-name=layout_test_runner \`

`
--additional-driver-flag=--content-handlers=text/html,mojo:layout_test_html_viewer
\`

--additional-driver-flag=--js-flags=--expose-gc \\

` --debug --no-retry-failures \`

` fast/events/touch`

If you are using a build directory that is not the standard out/Debug or
out/Release, then you need to also specify --target and --build-directory flags.
(e.g. --build-directory=custom_out --target=Debug_gn).

**Bugs:**

Use
[Proj-Mandoline](https://code.google.com/p/chromium/issues/list?can=2&q=label%3Aproj-mandoline&colspec=ID+Pri+M+Week+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)
label. If the bug is performance-related, please also add Test-Performance
label.
