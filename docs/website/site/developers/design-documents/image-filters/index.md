---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: image-filters
title: Filter Effects
---

Filter Effects in Chrome can be somewhat confusing. The
[SVG](http://www.w3.org/TR/SVG/filters.html) and
[CSS](http://www.w3.org/TR/filter-effects-1/) specs give us three different ways
of specifying filters (SVG-on-SVG, SVG-in-CSS, shorthand CSS), Chrome has two
rendering modes (GPU-accelerated or CPU), and two filter implementations (Blink
and Skia). This is an attempt to document the current situation, and where we're
headed.

One important thing to note is that we currently only trigger the GPU filter
path for source elements that are already in a composited layer, e.g., 2D
&lt;canvas&gt;, WebGL, &lt;video&gt;, 3D CSS. We don't really have a choice but
to apply these filters in the compositor, since only the compositor contains the
source textures to which the filters are to be applied. We do trigger the GPU
path for filters which are animated, but only for the duration of the animation,
after which we switch back to software (same as is done for animated opacity, 2D
transforms, etc). All other rendering of filters on web content uses the CPU.
Currently, SVG-on-SVG filters are only rendered using a CPU path.

Ways to author filters (in chronological order):

SVG-on-SVG filters: the oldest (spec-wise, and blink-wise). Syntax:
filter="url(#foo)" on the target SVG element, where "foo" refers to an SVG
filter graph. Applies a SVG filter to an SVG element, via a DOM attribute.
Animatable via SVG. First shipped in Chrome M6.

SVG-in-CSS filters (aka reference filters, aka url() filters). Specified by CSS
property -webkit-filter: url(#foo), where foo refers to the ID of an SVG filter
node. Renders an HTML element to a buffer, then applies an SVG filter DAG to it.
First appeared in Firefox 5 or so. Implemented in Chrome M25 on the software
path, and M31 on the GPU path (disabled in M32 due to ubercomp; re-enabled in
M33). Animatable via SVG.

CSS shorthand filters. Specified by (e.g.,) CSS property -webkit-filter:
grayscale(50%) hue-rotate(90deg). A simple linear chain of filters. This is
essentially a simplified subset of what is possible with the SVG DAG. Animatable
via CSS. Proposed by Apple (but first shipped in Chrome M19), shipped in Safari
6.

The Blink implementation uses a graph of FilterEffect nodes (in
platform/graphics/filters/FilterEffect.\*, and subclasses FE\*.) This renders
only on the CPU.

The Skia implementation uses a graph of SkImageFilter nodes (in
src/core/SkImageFilter.\*, and subclasses \*ImageFilter.cpp). This has CPU and
GPU modes.

Shipping now:

<table>
<tr>
<td>Syntax</td>
<td>CPU</td>
<td>GPU</td>
</tr>
<tr>
<td>SVG-on-SVG</td>
<td>filter="url(#foo)"</td>
<td>FilterEffect DAG, renderer process</td>
<td>none yet\*</td>
</tr>
<tr>
<td>SVG-in-CSS</td>
<td>-webkit-filter: url(#foo)</td>
<td>FilterEffect DAG, renderer process</td>
<td>SkImageFilter DAG, compositor thread</td>
</tr>
<tr>
<td>CSS shorthand</td>
<td>-webkit-filter: grayscale(50%)</td>
<td>FilterEffect chain, renderer process</td>
<td>SkImageFilter chain, compositor thread</td>
</tr>
</table>

(\*there was actually a prototype implementation under the "Accelerated SVG
Filters" flag, but it ran in the renderer process, and the resulting filtered
element had to incur a readback from the GPU to be (CPU) composited. It was
useful for testing the individual Skia GPU filters before we had the accelerated
SVG-in-CSS path, but it's now old and busted. I've removed the flag in M36.)

Future (paths which will change are in bold):

<table>
<tr>
<td>Syntax</td>
<td>CPU</td>
<td>GPU</td>
</tr>
<tr>
<td>SVG-on-SVG</td>
<td>filter="url(#foo)"</td>
<td>SkImageFilter DAG, impl thread (enabled in M37)</td>
<td>SkImageFilter DAG, compositor thread (enabled in M37 where Ganesh is enabled)</td>
</tr>
<tr>
<td>SVG-in-CSS</td>
<td>-webkit-filter: url(#foo)</td>
<td>SkImageFilter DAG, impl thread (enabled in M39)</td>
<td>SkImageFilter DAG, compositor thread</td>
</tr>
<tr>
<td>CSS shorthand</td>
<td>-webkit-filter: grayscale(50%)</td>
<td>SkImageFilter chain, impl thread (enabled in M39)</td>
<td>SkImageFilter chain, compositor thread</td>
</tr>
</table>

The GPU path for SVG-on-SVG filters (\*\*\*) can forced on via
--force-gpu-rasterization (the Ganesh GPU rasterization path, essentially).

Now that these changes are done, all software rasterization of filters is be
performed on the impl-thread using the Skia implementation, and compatible with
multi-threaded and GPU rasterization. The Blink implementation of filter
processing can be removed (including its bespoke ParallelJobs threading
library). As a side note, this will also make filters compatible with non-tiled
GPU-accelerated rendering of web content too (a la IE9+), if that proves
promising.
