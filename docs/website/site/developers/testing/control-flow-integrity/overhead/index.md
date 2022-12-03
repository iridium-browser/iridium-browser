---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
- - /developers/testing/control-flow-integrity
  - Control Flow Integrity
page_name: overhead
title: Overhead
---

## Code Size

As of June 23 2016, code size overhead for official Chrome is about 5% for calls
and 7.5% for calls and casts. The following is the binary size of official
Chrome built in various configurations:

*   Without CFI: 104398832 bytes
*   With CFI for virtual calls only: 109502448 bytes
*   With CFI for virtual calls and casts: 112021488 bytes

## Performance

Measured performance overhead for various benchmark suites in the CFI for
virtual calls only configuration:

*   blink_perf.layout: min -1.71%, median 2.81%, max 12.23%
*   smoothness.top_25_smooth: cannot measure due to noise

## Reproducing

The above figures were derived from official builds of r401651 of Chromium by
following these instructions.

1.  Build r273760 of LLVM
2.  Fetch r401651 of Chromium and apply
            <https://codereview.chromium.org/2099003002> to it
3.  Build three Chromium GN trees as shown below:

    **$ gn args out_gn/ra_official**

    **$ gn args out_gn/ra_official_cfi_callonly****

    **$ gn args out_gn/ra_official_cfi****

    ****$ cat out_gn/ra_official/args.gn******

    is_official_build = true**

    is_chrome_branded = true**

    is_debug = false**

    clang_use_chrome_plugins = false**

    clang_base_path = "/path/to/llvm/build"**

    ****$ cat out_gn/ra_official_cfi_callonly/args.gn******

    **is_official_build = true****

    **is_chrome_branded = true****

    **is_debug = false****

    **clang_use_chrome_plugins = false****

    **clang_base_path = "/path/to/llvm/build"****

    **is_cfi = true****

    ****use_cfi_cast = false******

    ******$ cat out_gn/ra_official_cfi/args.gn********

    is_official_build = true**

    is_chrome_branded = true**

    is_debug = false**

    clang_use_chrome_plugins = false**

    clang_base_path = "/path/to/llvm/build"**

    is_cfi = true**

    ****$ ninja -C out_gn/ra_official chrome******

    ****$ ninja -C out_gn/ra_official_cfi_callonly chrome******

    ****$ ninja -C out_gn/ra_official_cfi chrome******

    ********$ strip -o /tmp/chrome1********

    ********out_gn/ra_official/chrome************

    **$ stri****

    p -o /tmp/chrome2 out_gn/ra_official_cfi_callonly/chrome**

    **$ strip -o /tmp/chrome3 out_gn/ra_official_cfi/chrome****

    ******$ ls -l /tmp/chrome\[123\]********

4.  Run the following shell script from the chromium/src directory to
            obtain performance numbers:

**#!/bin/bash**

**for i in smoothness.top_25_smooth blink_perf.layout blink_perf.svg
blink_perf.css blink_perf.dom blink_perf.paint blink_perf.canvas
blink_perf.events blink_perf.parser blink_perf.bindings blink_perf.mutation
blink_perf.animation blink_perf.shadow_dom blink_perf.interactive
blink_perf.pywebsocket blink_perf.xml_http_request blink_perf.mutation.reference
blink_perf.interactive.reference speedometer dromaeo.domcoreattr
dromaeo.domcorequery dromaeo.domcoremodify dromaeo.cssqueryjquery
dromaeo.jslibattrjquery dromaeo.domcoretraverse dromaeo.jslibeventjquery
dromaeo.jslibstylejquery dromaeo.jslibmodifyjquery dromaeo.jslibattrprototype
dromaeo.jslibeventprototype dromaeo.jslibstyleprototype
dromaeo.jslibtraversejquery dromaeo.jslibmodifyprototype
dromaeo.jslibtraverseprototype browsermark octane ; do**

**xvfb-run -s "-screen 0 1024x768x24" ./tools/perf/run_benchmark --browser=exact
--browser-executable=out_gn/ra_official/chrome --results-label=lto
--pageset-repeat=50 $i**

**xvfb-run -s "-screen 0 1024x768x24" ./tools/perf/run_benchmark --browser=exact
--browser-executable=out_gn/ra_official_cfi_callonly/chrome
--results-label=ltocficall --pageset-repeat=50 $i**

**xvfb-run -s "-screen 0 1024x768x24" ./tools/perf/run_benchmark --browser=exact
--browser-executable=out_gn/ra_official_cfi/chrome --results-label=ltocfi
--pageset-repeat=50 $i**

**mv tools/perf/results.html bm-devirt6/$i.html**

**done**
