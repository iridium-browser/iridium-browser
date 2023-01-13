---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: autotest-graphics-documentation
title: Autotest Graphics Documentation
---

[TOC]

Keywords: This testing document describes how to run GLBench, TearTest, Piglit,
WebGLConformance and other tests and benchmarks on ChromeOS and ChromiumOS
devices.

## Source code location

Python sources are under src/third_party/autotest/files/client/site_tests

<table>
<tr>
<td>drwxr-xr-x</td>
<td>-</td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_GLAPICheck;h=a6adea1387bf225ae8fff9c75c6111c78ebc9152;hb=HEAD">graphics_GLAPICheck</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_GLAPICheck;h=a6adea1387bf225ae8fff9c75c6111c78ebc9152;hb=HEAD">tree</a> | <a href="http://git.chromium.org/gitweb/?p=autotest.git;a=history;f=client/site_tests/graphics_GLAPICheck;hb=HEAD">history</a></td>
</tr>
<tr>
<td>drwxr-xr-x</td>
<td>-</td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_GLBench;h=87b8a78d5908803418369ef74774b8e609247f44;hb=HEAD">graphics_GLBench</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_GLBench;h=87b8a78d5908803418369ef74774b8e609247f44;hb=HEAD">tree</a> | <a href="http://git.chromium.org/gitweb/?p=autotest.git;a=history;f=client/site_tests/graphics_GLBench;hb=HEAD">history</a></td>
</tr>
<tr>
<td>drwxr-xr-x</td>
<td>-</td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_O3DSelenium;h=110a6d915564be1c9c85d73f32ab5294b5e361f5;hb=HEAD">graphics_O3DSelenium</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_O3DSelenium;h=110a6d915564be1c9c85d73f32ab5294b5e361f5;hb=HEAD">tree</a> | <a href="http://git.chromium.org/gitweb/?p=autotest.git;a=history;f=client/site_tests/graphics_O3DSelenium;hb=HEAD">history</a></td>
</tr>
<tr>
<td>drwxr-xr-x</td>
<td>-</td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_SanAngeles;h=3164f5fa2b5688cdafe534493ca6e0f86807d3fe;hb=HEAD">graphics_SanAngeles</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_SanAngeles;h=3164f5fa2b5688cdafe534493ca6e0f86807d3fe;hb=HEAD">tree</a> | <a href="http://git.chromium.org/gitweb/?p=autotest.git;a=history;f=client/site_tests/graphics_SanAngeles;hb=HEAD">history</a></td>
</tr>
<tr>
<td>drwxr-xr-x</td>
<td>-</td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_TearTest;h=2d7e89034a73df1d66134a7ab9dc1ca0761f4f0b;hb=HEAD">graphics_TearTest</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_TearTest;h=2d7e89034a73df1d66134a7ab9dc1ca0761f4f0b;hb=HEAD">tree</a> | <a href="http://git.chromium.org/gitweb/?p=autotest.git;a=history;f=client/site_tests/graphics_TearTest;hb=HEAD">history</a></td>
</tr>
<tr>
<td>drwxr-xr-x</td>
<td>-</td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_WebGLConformance;h=ad84141374e0338c069895e2c195eaaf7e9f662d;hb=HEAD">graphics_WebGLConformance</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_WebGLConformance;h=ad84141374e0338c069895e2c195eaaf7e9f662d;hb=HEAD">tree</a> | <a href="http://git.chromium.org/gitweb/?p=autotest.git;a=history;f=client/site_tests/graphics_WebGLConformance;hb=HEAD">history</a></td>
</tr>
<tr>
<td>drwxr-xr-x</td>
<td>-</td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_WindowManagerGraphicsCapture;h=78e46205be3d01e44b4c8c8ab9adce5118632b9b;hb=HEAD">graphics_WindowManagerGraphicsCapture</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/site_tests/graphics_WindowManagerGraphicsCapture;h=78e46205be3d01e44b4c8c8ab9adce5118632b9b;hb=HEAD">tree</a> | <a href="http://git.chromium.org/gitweb/?p=autotest.git;a=history;f=client/site_tests/graphics_WindowManagerGraphicsCapture;hb=HEAD">history</a></td>
</tr>
</table>

Binary dependencies are under src/third_party/autotest/files/client/deps

<table>
<tr>
<td>drwxr-xr-x</td>
<td>-</td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/deps/glbench-images;h=635233de155100312539602b4accd574c4efdfb4;hb=HEAD">glbench-images</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/deps/glbench-images;h=635233de155100312539602b4accd574c4efdfb4;hb=HEAD">tree</a> | <a href="http://git.chromium.org/gitweb/?p=autotest.git;a=history;f=client/deps/glbench-images;hb=HEAD">history</a></td>
</tr>
<tr>
<td>drwxr-xr-x</td>
<td>-</td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/deps/glbench;h=47be7d689cf1aaa866cc02bf2fd392046d8ff8c5;hb=HEAD">glbench</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/deps/glbench;h=47be7d689cf1aaa866cc02bf2fd392046d8ff8c5;hb=HEAD">tree</a> | <a href="http://git.chromium.org/gitweb/?p=autotest.git;a=history;f=client/deps/glbench;hb=HEAD">history</a></td>
</tr>
<tr>
<td>drwxr-xr-x</td>
<td>-</td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/deps/piglit;h=4e06825e78483a5e7c525bf384680c3d087e888a;hb=HEAD">piglit</a></td>
<td><a href="http://git.chromium.org/gitweb/?p=autotest.git;a=tree;f=client/deps/piglit;h=4e06825e78483a5e7c525bf384680c3d087e888a;hb=HEAD">tree</a> | <a href="http://git.chromium.org/gitweb/?p=autotest.git;a=history;f=client/deps/piglit;hb=HEAD">history</a></td>
</tr>
</table>

## A note on building binaries

The ebuild system knows if a target knows either OpenGL or OpenGL ES, or even
both. If only pre-compiled binaries are used there should not be much of a
problem with running tests. When editing tests after using the cros_workon
script the default system behavior is to recompile the binaries once more
without invoking the ebuild system. Unfortunately this means USE="opengles"
flags are not honored. Instead the easy way is to specify the proper switch via
am environment variable **GRAPHICS_BACKEND=OPENGLES./run_remote_tests.sh** that
gets directly passed to the Makefile. This is somewhat hacky. To use the ebuild
system to generate the binaries one has to do a painfully slow **USE=opengles
emerge-tegra2_seaboard chromeos-base/autotest** followed by
**./run_remote_tests.sh --use_emerged \[...\]**. Don't forget the --use_emerged
flag as it is not default.

## Autotest graphics_GLAPICheck

- two different binaries can be made

-- GRAPHICS_BACKEND=OPENGL or OPENGLES

- runs 25 seconds

- sample results

- for OPENGL creates output below and parses some samples in python to see if
they are there and if SUCCEED: run to the end is there

GL_VERSION = 3.2.0 NVIDIA 195.36.24

GL_EXTENSIONS = GL_ARB_color_buffer_float GL_ARB_compatibility
GL_ARB_copy_buffer GL_ARB_depth_buffer_float GL_ARB_depth_clamp
GL_ARB_depth_texture GL_ARB_draw_buffers GL_ARB_draw_elements_base_vertex
\[...\] NV-GLX NV-CONTROL Generic

Event Extension SHAPE MIT-SHM XInputExtension XTEST BIG-REQUESTS SYNC XKEYBOARD
XC-MISC SECURITY XINERAMA XFIXES RENDER RANDR XINERAMA Composite DAMAGE

SUCCEED: run to the end

### Sample output

(cros-chroot) ihf@ql ~/trunk/src/scripts $ **GRAPHICS_BACKEND=OPENGL
./run_remote_tests.sh --board=${BOARD} --remote=172.22.71.194
graphics_GLAPICheck --use_emerged**

Initiating first contact with remote host

\[...\]

INFO : Test results:

---------------------------------------------

graphics_GLAPICheck PASS

graphics_GLAPICheck/graphics_GLAPICheck PASS

---------------------------------------------

Total PASS: 2/2 (100%)

No crashes detected during testing.

Elapsed time: 0m25s

## Autotest graphics_GLBench

- draws green/ref/purple/fractal rectangles

- runs about 5 minutes

This benchmark executes glbench, a graphics benchmark designed to time how long

various graphic intensive activities take, which includes measuring:

- fill rate

- blended

- opaque

-Z reject rate

-triangle rate

- no cull

- half cull (half triangles backface culled)

- full cull (mix of back face and degenerates)

- blend rate

- texture fetch

- nearest

- bilinear

- trilinear

- compute

- vertex shader

- pixel shader

- \*fragement shader to test ddx and ddy

- attribute fetch

- color depth stencil test

- \*state change

- texture upload

- read back

- does MD5 checksums of some but not all images

-- deps/glbench/src/checksums

-- can be re-generated with -save option

### Sample output

(cros-chroot) ihf@ql ~/trunk/src/scripts $ **GRAPHICS_BACKEND=OPENGLES
./run_remote_tests.sh --board=${BOARD} --remote=172.22.70.27 graphics_GLBench
--use_emerged**

\[...\]

INFO : Test results:

----------------------------------------------------

graphics_GLBench PASS

graphics_GLBench/graphics_GLBench PASS

1280x768_fps_compositing 11.18

1280x768_fps_no_fill_compositing 41.96

mpixels_sec_clear_color 490.74

mpixels_sec_clear_colordepth 323.88

mpixels_sec_clear_colordepthstencil 281.45

mpixels_sec_clear_depth 969.71

mpixels_sec_clear_depthstencil 647.34

mpixels_sec_fill_solid 285.1

mpixels_sec_fill_solid_blended 128.55

mpixels_sec_fill_solid_depth_neq 973.2

mpixels_sec_fill_solid_depth_never 1189.08

mpixels_sec_fill_tex_bilinear 149.56

mpixels_sec_fill_tex_nearest 203.7

mpixels_sec_fill_tex_trilinear_linear_01 164.63

mpixels_sec_fill_tex_trilinear_linear_04 nan

mpixels_sec_fill_tex_trilinear_linear_05 nan

mpixels_sec_fill_tex_trilinear_nearest_05 nan

mpixels_sec_pixel_read 6.72

mpixels_sec_pixel_read_2 6.72

mpixels_sec_pixel_read_3 6.74

mpixels_sec_varyings_shader_1 270.68

mpixels_sec_varyings_shader_2 144.33

mpixels_sec_varyings_shader_4 73.57

mpixels_sec_varyings_shader_8 24.29

mpixels_sec_yuv_shader_1 nan

mpixels_sec_yuv_shader_2 47.82

mpixels_sec_yuv_shader_3 93.36

mpixels_sec_yuv_shader_4 136.15

mtexel_sec_texture_update_teximage2d_1024 149.85

mtexel_sec_texture_update_teximage2d_128 80.92

mtexel_sec_texture_update_teximage2d_1536 146.05

mtexel_sec_texture_update_teximage2d_2048 149.47

mtexel_sec_texture_update_teximage2d_256 132.51

mtexel_sec_texture_update_teximage2d_32 10.29

mtexel_sec_texture_update_teximage2d_512 142.05

mtexel_sec_texture_update_teximage2d_768 147.56

mtexel_sec_texture_update_texsubimage2d_1024 150.94

mtexel_sec_texture_update_texsubimage2d_128 81.59

mtexel_sec_texture_update_texsubimage2d_1536 147.2

mtexel_sec_texture_update_texsubimage2d_2048 149.03

mtexel_sec_texture_update_texsubimage2d_256 130.59

mtexel_sec_texture_update_texsubimage2d_32 10.09

mtexel_sec_texture_update_texsubimage2d_512 113.54

mtexel_sec_texture_update_texsubimage2d_768 150.0

mtri_sec_triangle_setup 30.34

mtri_sec_triangle_setup_all_culled 49.89

mtri_sec_triangle_setup_half_culled 44.91

mvtx_sec_attribute_fetch_shader 149.51

mvtx_sec_attribute_fetch_shader_2_attr 149.34

mvtx_sec_attribute_fetch_shader_4_attr 141.36

mvtx_sec_attribute_fetch_shader_8_attr 106.71

us_swap_swap 19464.19

----------------------------------------------------

Total PASS: 2/2 (100%)

No crashes detected during testing.

Elapsed time: 8m58s

&gt;&gt;&gt; Details stored under /tmp/run_remote_tests.purz

The "nan" in this example are passed because of knownbad output images. For more
more details and failures look at

(cros-chroot) ihf@ql ~/trunk/src/scripts $ **more
/tmp/run_remote_tests.purz/graphics_GLBench/graphics_GLBench/summary.txt**

### Note on HW Qual failures

Sometimes when site_tests/suite_HWQual/control.auto is run a constraint is
violated and failure **mtri_sec_triangle_setup &gt;= 10** is thrown. This is due
to some x86 boards doing vertex shading in the CPU. This exception is considered
harmless until a waiver system is developed.

## Autotest graphics_O3DSelenium

- tries to run O3D tests inside of chrome

- failed to run and is known to be broken

## Autotest graphics_Piglit

- tests OpenGL implementation. Does not currently work on OpenGL ES only systems
like Tegra2

- about 1000-2000 targeted subtests

- usually a few hundred failures

- mostly care about counts of passes/failures over time like perf test

- runs about 12-18 minutes, but initial copying dep-piglit.tar.bz2 takes 5-10
minutes that seem like a hang. Need to fix this.

- generates html page with detailed results in

/tmp/run_remote_tests.\*/graphics_Piglit/graphics_Piglit/cros-driver/html/index.html

- can compare two test runs against each other via

./piglit-summary-html.py summary/compare results/baseline.results
results/current.results

- can be build locally from piglit.tar.gz via cmake .;make

- the numbers of passes does fluctuate by about 10 over time, so some
intermittency

- about 10 test binaries crash and are blocklisted to not be reported

- sometimes (1 in about 2 weeks of test runs) fbo_depth_sample_compare takes
gpu/system with it when it crashes

2011-05-04T11:06:02.995237-07:00 localhost kernel: \[ 75.994363\]
fbo-depth-sampl\[4250\]: segfault at 0 ip (null) sp 7fc7abac error 14 in
card0\[75f97000+4000\] 2011-05-04T11:06:03.040387-07:00 localhost
crash_reporter\[4251\]: Received crash notification for
fbo-depth-sample-compare\[4250\] sig 11 (developer build - not testing - always
dumping) 2011-05-04T11:06:03.234936-07:00 localhost crash_reporter\[4251\]:
Stored minidump to
/var/spool/crash/fbo_depth_sample_compare.20110504.110603.4250.dmp
2011-05-04T11:06:03.235566-07:00 localhost crash_reporter\[4251\]: Leaving core
file at /var/spool/crash/fbo_depth_sample_compare.20110504.110603.4250.core due
to developer image 2011-05-04T11:06:17.742377-07:00 localhost kernel: \[
90.742080\] \[drm:i915_hangcheck_elapsed\] \*ERROR\* Hangcheck timer elapsed...
GPU hung 2011-05-04T11:06:17.742439-07:00 localhost kernel: \[ 90.742100\]
render error detected, EIR: 0x00000000 2011-05-04T11:06:17.742471-07:00
localhost kernel: \[ 90.742168\] \[drm:i915_do_wait_request\] \*ERROR\*
i915_do_wait_request returns -5 (awaiting 29946 at 29941)
2011-05-04T11:06:18.330692-07:00 localhost crash_reporter\[4328\]: Received
crash notification for Xorg\[3498\] sig 6 (developer build - not testing -
always dumping)

### Sample output

(cros-chroot) ihf@ql ~/trunk/src/scripts $ **./run_remote_tests.sh
--board=${BOARD} --remote=172.22.70.69 graphics_Piglit --use_emerged**

\[...\]

INFO : Test results:

-------------------------------------

graphics_Piglit PASS

graphics_Piglit/graphics_Piglit PASS

count_subtests_fail 239

count_subtests_pass 1040

count_subtests_skip 687

count_subtests_warn 14

-------------------------------------

Total PASS: 2/2 (100%)

No crashes detected during testing.

Elapsed time: 13m31s

&gt;&gt;&gt; Details stored under /tmp/run_remote_tests.8gjT

Point browser to

/tmp/run_remote_tests.\*/graphics_Piglit/graphics_Piglit/cros-driver/html/index.html

## Autotest graphics_SanAngeles

- draws a bunch of very coarse 3d models

This test runs the San Angeles Observation GPU benchmark. This benchmark uses

a minimal and portable framework to generate a small demo program. It exercises

basic features of OpenGL like vertex arrays, color arrays, and lighting. It

also uses objects generated using procedural algorithms.

This test is a benchmark. It will fail if it fails to complete.

### Sample output

(cros-chroot) ihf@ql ~/trunk/src/scripts $ **GRAPHICS_BACKEND=OPENGL
./run_remote_tests.sh --board=${BOARD} --remote=172.22.70.69 graphics_SanAngeles
--use_emerged**

\[...\]

INFO : Test results:

---------------------------------------------

graphics_SanAngeles PASS

graphics_SanAngeles/graphics_SanAngeles PASS

frames_per_sec_rate_san_angeles 2.1

---------------------------------------------

Total PASS: 2/2 (100%)

No crashes detected during testing.

Elapsed time: 2m17s

&gt;&gt;&gt; Details stored under /tmp/run_remote_tests.vDwN

## Autotest graphics_TearTest

- copies glbench as dependency over

- runs deps/glbench/teartest

- This test will fail if there is tearing in the two vertical lines that are

scrolling horizontally.

This is a semi-automated test that displays vertical lines scrolling

horizontally and asks the user if tearing was observed. Three variants are

available:

\* using uniform update. This tests that glSwapInterval function performs as

expected.

\* using full texture update. This tests that CPU-GPU interaction is properly

synchronized in the driver.

\* using pixmap to texture extension. This tests that pixmap to texture

extension is properly synchronized.

### Sample output

(cros-chroot) ihf@ql ~/trunk/src/scripts $ **GRAPHICS_BACKEND=OPENGL
./run_remote_tests.sh --board=${BOARD} --remote=172.22.70.69 graphics_TearTest
--use_emerged**

\[...\] - some user interaction

INFO : Test results:

-----------------------------------------

graphics_TearTest PASS

graphics_TearTest/graphics_TearTest PASS

-----------------------------------------

Total PASS: 2/2 (100%)

No crashes detected during testing.

Elapsed time: 1m26s

&gt;&gt;&gt; Details stored under /tmp/run_remote_tests.SHxS

## Autotest graphics_WebGLConformance

This test runs the WebGL conformance tests:
<http://www.khronos.org/webgl/wiki/Testing/Conformance>

- currently uses release version 1.0.0

- opens copy of above website in chrome and executes the html page with hundreds
of targeted tests

- all suite tests must pass, but there is the ability to waive failures in the
python script

- javascript is patched to report individual results

- suite is currently blocked in autotest regression runs by hangs/crashes in
browser

### Sample output

Doesn't compile any code, so no GRAPHICS_BACKEND needed (but no harm in using
it). When the browser doesn't crash/hang it should look like this.

(cros-chroot) ihf@ql ~/trunk/src/scripts $ **./run_remote_tests.sh
--board=${BOARD} --remote=172.22.70.80 graphics_WebGLConformance/control**

\[...\]

INFO : Test results:

---------------------------------------------------------

graphics_WebGLConformance PASS

graphics_WebGLConformance/graphics_WebGLConformance PASS

count_tests_fail 352

count_tests_pass 5064

count_tests_timeout 2

waived_url_000
conformance/tex-image-and-sub-image-2d-with-array-buffer-view.html : 192
failures (192 waived)

waived_url_001 conformance/tex-image-with-format-and-type.html : 12 failures (12
waived)

waived_url_002 conformance/texture-npot.html : 12 failures (12 waived)

waived_url_003 conformance/glsl-conformance.html : 1 failures (1 waived)

waived_url_004 conformance/tex-image-and-sub-image-2d-with-image.html : 8
failures (8 waived)

waived_url_005 conformance/copy-tex-image-and-sub-image-2d.html : 34 failures
(34 waived)

waived_url_006 conformance/gl-clear.html : 4 failures (4 waived)

waived_url_007 conformance/more/functions/readPixelsBadArgs.html : 1 failures (1
waived)

waived_url_008 conformance/more/conformance/webGLArrays.html : 1 failures (1
waived)

waived_url_009 conformance/gl-teximage.html : 46 failures (46 waived)

waived_url_010 conformance/texture-active-bind.html : 4 failures (4 waived)

waived_url_011 conformance/read-pixels-test.html : 3 failures (3 waived)

waived_url_012 conformance/gl-object-get-calls.html : 2 failures (2 waived)

waived_url_013 conformance/point-size.html : 1 failures (1 waived)

waived_url_014 conformance/texture-formats-test.html : 4 failures (4 waived)

waived_url_015 conformance/texture-complete.html : 1 failures (1 waived)

waived_url_016 conformance/tex-image-and-sub-image-2d-with-video.html : 8
failures (8 waived)

waived_url_017 conformance/context-lost-restored.html : 2 failures (2 waived)

waived_url_018 conformance/tex-image-and-sub-image-2d-with-image-data.html : 16
failures (16 waived)

---------------------------------------------------------

Total PASS: 2/2 (100%)

Crashes detected during testing:

---------------------------------------------------------

chrome sig 6

graphics_WebGLConformance/graphics_WebGLConformance

---------------------------------------------------------

Total unique crashes: 1

&gt;&gt;&gt; Details stored under /tmp/run_remote_tests.iumX

## Autotest graphics_WindowManagerGraphicsCapture

- This test verifies the window manager can capture graphics from applications.

- This test fails if application screen shots cannot capture the screen output.

- draws less than 10 full screen color frames of a dot/sphere

- copies dep-glbench.tar.bz2 as dependency over

- runs deps/glbench/windowmanagertest

- converts/resizes images to 100x100 pixels

- runs perceptualdiff to compare with reference images

- runs about 30 seconds

- fairly stable test

### Sample Output

ihf@ql ~/trunk/src/scripts $ **GRAPHICS_BACKEND=OPENGL ./run_remote_tests.sh
--board=${BOARD} --remote=172.22.70.69 graphics_WindowManagerGraphicsCapture
--use_emerged**

\[...\]

INFO : Test results:

---------------------------------------------------------------------------------

graphics_WindowManagerGraphicsCapture PASS

graphics_WindowManagerGraphicsCapture/graphics_WindowManagerGraphicsCapture PASS

---------------------------------------------------------------------------------

Total PASS: 2/2 (100%)

No crashes detected during testing.

Elapsed time: 0m36s

&gt;&gt;&gt; Details stored under /tmp/run_remote_tests.Lb4p
