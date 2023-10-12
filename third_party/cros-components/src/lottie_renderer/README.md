# Lottie Renderer and Worker

See: go/cros-lottie-renderer

The `cros-lottie-renderer` component uses an offscreen canvas controlled by
a WebWorker thread in order to move Lottie rendering off the main thread, in
order to provide better performance and sandboxing.

This directory contains the `lottie_worker.js` script, which is basically a copy
of https://github.com/airbnb/lottie-web, modified to run on a worker thread
so that it doesn't rely on a DOM node for the canvas. This functionality has
been adopted into the official Lottie project, but is currently unavailable in
//google3/third_party/javascript/lottie, as we are lagging multiple versions
behind. Work to update the Lottie library is underway, and when complete the
`lottie_worker.js` script in this directory will be removed and replaced with
the official script.

In the interim, some extra modifications have been made to this library in order
to bypass JSC security exceptions. These changes can be found in the
`lottie_worker.DIFF` file if they need to be rolled back. The original file
can be found in [Chromium](https://crsrc.org/c/third_party/lottie/lottie_worker.js?q=lottie_worker.js). The file has been minified using the `minify_lottie.py`
script.
