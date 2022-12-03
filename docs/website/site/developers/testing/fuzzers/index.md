---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: fuzzers
title: fuzzers
---

## cross_fuzz

cross_fuzz is a DOM fuzzer, a good stress test for Chromium and any other
browser.

To get cross_fuzz just add

"src/third_party/cross_fuzz":
"http://src.chromium.org/chrome/trunk/deps/third_party/cross_fuzz",

to your "custom_deps" section in your .gclient and run gclient runhooks.

Your gclient file should look like this:

solutions = \[

{ "name" : "src",

"url" : "http://src.chromium.org/chrome/trunk/src",

"custom_deps" : {

**"src/third_party/cross_fuzz":
"http://src.chromium.org/chrome/trunk/deps/third_party/cross_fuzz",**

...

}

},

\]

To run cross_fuzz just point the browser to the
third_party/cross_fuzz/cross_fuzz_randomized_20110105_seed.html page. Popup
blocker should be disabled.

For automated runs you may use something like this:

out/Release/chrome **--disable-popup-blocking** --no-first-run
--user-data-dir=$TEMPDIR \\

--allow-file-access-from-files --noerrdialogs --disable-hang-monitor \\

file://\`pwd\`/third_party/cross_fuzz/cross_fuzz_randomized_20110105_seed.html**#1234**

On Windows, you should use

**file://%cd%/third_party/cross_fuzz/cross_fuzz_randomized_20110105_seed.html**#1234****

instead.

**#1234** is the random seed. Replace it with your own seed or remove it from
the URL if you want cross fuzz to generate its own random seed.
