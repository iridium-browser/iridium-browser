---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: gfx-test-notes
title: GFX test notes
---

## [GPU Rendering Benchmarks](/developers/design-documents/rendering-benchmarks)

Chromium has some [GPU Rendering
Benchmarks](/developers/design-documents/rendering-benchmarks) which use the
[Telemetry](/developers/telemetry) framework.

The `smoothness` test loads a set of 25 very common web pages, uses Telemetry to
scroll them, and then measures some performance metrics.

To run the test, you first need to [check out the full Chromium
source](https://code.google.com/p/chromium/wiki/UsingGit), and a **Chrome OS
Test Image.**

```none
cd chromium/src
tools/perf/run_measurement --browser=cros-chrome-guest --remote=${remote_ip} smoothness tools/perf/page_sets/top_25.json --allow-live-sites
```

For the Google and Facebook site tests, you can to provide credentials in a
~/.telemetry-credentials file:

```none
cp tools/telemetry/examples/credentials_example.json ~/.telemetry-credentials
vi ~/.telemetry-credentials
```

Add Google and Facebook credentials, but the example file format has a bug:
there should be a ',' between the "google" and "facebook" blocks.

The previous example tested using live sites. There are times where it is
preferable to test using pre-cached site data. Telemetry lets you do that using
a "web page replay" tool which captures a snapshot of a set of webpages (.wpr).

Several such snapshots have already been captured, such as the "top_25" test set
which we are running here. Since the page contents are copyright of the
respective web page owners, however, they are not available as part of the
publicly available chromium source tree.

However, if you have access to the Chrome (src-internal) tree, you can check out
the repository containing test data and run the test withough
"--allow-live-sites":

```none
# Prerequisite: To access the pre-recorded page sets, first set up a chrome src-internal password if not already set.
# See: <https://www.googlesource.com/new-password>
cd chromium/src/tools/perf
git clone https://chrome-internal.googlesource.com/chrome/tools/perf/data
cd chromium/src
tools/perf/run_measurement --browser=cros-chrome-guest --remote=${remote_ip} smoothness tools/perf/page_sets/top_25.json
```

The `run_measurement`, is useful for running arbitrary measurements on different
page sets. However, the performance bots run their benchmarks, using the
`run_benchmark` script. The equivalent benchmark to the above rrun_measurement
example is:

```none
cd src/tools/perf
./run_benchmark run --browser=cros-chrome-guest --remote=${remote_ip} smoothness_top25
```

The scripts `run_benchmark`/`run_measurement both launch the python script
test_runner.py`, which in turn starts up a local `webpagereplay.py` server.

Beware that the webpagereplay.py will fail to start if `/etc/resolv.conf`
contains the line "nameserver 127.0.0.1":

```none
webpagereplay.ReplayNotStartedError: Web Page Replay failed to start. Log output:
dnsproxy.DnsProxyException: Invalid nameserver: 127.0.0.1 (causes an infinte loop)
```

This can happen when running on a laptop running Ubuntu 12.04+, since by default
Ubuntu 12.04 uses the `resolvconf` which uses a local `dnsmasq` server to do
actual dns resolution. Thus, the `nameserver` in `/etc/resov.conf` will be
127.0.0.1. To temporarily work around this, first fetch the real dns nameserver
using `nm-tool`, and use this value to manually update the `nameserver` entry in
`/etc/resolv.conf`. Note that any manual changes to `/etc/resolv.conf` will get
overwritten by the system at boot - or whenever `resolvconf` gets run.

## Testing without Vsync

In `/sbin/session_manager_setup.sh`, add the following to the chrome parameter
list: `--disable-gpu-vsync`

## Experimental WebGL autotests

### WebGLManyPlanetsDeep

```none
./run_remote_tests.sh --board=${board} --remote=${remote_ip} graphics_WebGLManyPlanetsDeep
```

Example results on mali 2013-wk20 w/ kernel/chromeos-3.4:

```none
graphics_WebGLManyPlanetsDeep/graphics_WebGLManyPlanetsDeep   avg_fps                       42.5563759729
graphics_WebGLManyPlanetsDeep/graphics_WebGLManyPlanetsDeep   avg_render_time               0.00651348182884
```

Example results on mali 2012-wk45 w/ kernel/chromeos-3.4:

```none
graphics_WebGLManyPlanetsDeep/graphics_WebGLManyPlanetsDeep   avg_fps                       59.1172358698
graphics_WebGLManyPlanetsDeep/graphics_WebGLManyPlanetsDeep   avg_render_time               0.00622212837838
```

### WebGLAquarium

```none
./run_remote_tests.sh --board=${board} --remote=${remote_ip} graphics_WebGLAquarium
```

**mali-2012_wk45 on 3.4 on daisy @ CPU=1.4 MHz GPU=533MHz**

```none
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_0001_fishes                         34.7671616056
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_0010_fishes                         38.0083871349
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_0050_fishes                         36.1185803129
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_0100_fishes                         33.5180697395
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_0250_fishes                         29.4186162358
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_0500_fishes                         21.4609046677
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_1000_fishes                         14.0662410091
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_0001_fishes                 0.0115492735466
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_0010_fishes                 0.0120682393055
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_0050_fishes                 0.0137373693937
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_0100_fishes                 0.0159920819915
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_0250_fishes                 0.0222824848304
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_0500_fishes                 0.0312746594998
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_1000_fishes                 0.0478747122113
```

**mali-2013_wk20 on 3.4 on daisy @ CPU=1.4 MHz GPU=533MHz**

```none
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_0001_fishes                         26.237068422
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_0010_fishes                         28.0702920668
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_0050_fishes                         27.0979309871
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_0100_fishes                         26.2101456206
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_0250_fishes                         23.3153728582
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_0500_fishes                         21.0018305341
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_fps_1000_fishes                         15.375419297
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_0001_fishes                 0.0117292154996
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_0010_fishes                 0.0115656813221
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_0050_fishes                 0.0134754876296
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_0100_fishes                 0.0157825582097
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_0250_fishes                 0.0217831682377
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_0500_fishes                 0.0322741695138
graphics_WebGLAquarium/graphics_WebGLAquarium   avg_render_time_1000_fishes                 0.0508704025854
```

### Restoring OOBE to make Telemetry happy

Telemetry gets confused if it cannot figure how to log in to the machine. This
can happen, for instance, if you've logged in as another user and telemetry
can't find the OOBE (Out Of Box Experience). To fix this, try restoring the
oobe:

```none
sudo pkill -9 chrome && rm -rf /home/chronos/Local\ State /home/chronos/.oobe_completed && restart ui
```

## Enable tracing in mali-ddk

1.  Add --no-sandbox to chrome start in
            /sbin/sesssion_manager_startup.sh
2.  Change –enable-gpu-sandbox to –disable-gpu-sandbox
3.  Disable self.lCCFlags.append('-pedantic') in
            bldsys/toolchain_abstr.py for trace defines to build.
4.  Change the #if 0 to #if 1 in cros/cros_tracing.h
5.  Change the cros_trace_enabled to 1 in cros/cros_tracing.c
6.  Each time you boot the board, run this to enable the Mali tracing:

chmod a+wx /sys

chmod a+wx /sys/kernel

chmod a+wx /sys/kernel/debug

chmod a+wx /sys/kernel/debug/tracing/

chmod a+wx /sys/kernel/debug/tracing/trace_marker

Then include cros/cros_tracing.h in any files with functions you want to trace
and add:

CROS_TRACE_ENABLE();

CROS_TRACE_BLOCK("trace block title");

## Using valgrind on the gpu-process

Chromium OS includes an ebuild for valgrind.
So you can just gmerge it to your target:

```none
gmerge valgrind
```

Then you just do `valgrind myprog`.

For the gpu process, you need to pass a flag to chrome to give it the gpu
launcher.

Add the following in in `session_manager_setup.sh`

```none
--gpu-launcher="usr/bin/valgrind --trace-children=yes --error-limit=no"
```
