---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
page_name: eliminating-simple-text
title: Eliminating Simple Text
---

[eae](mailto:eae@chromium.org), [drott](mailto:drott@chromium.org),
[behdad](mailto:behdad@chromium.org), [kojii](mailto:kojii@chromium.org)
April 2, 2015

Blink currently has two separate text paths, the simple path and the complex
path. The simple path is an optimization and is used for a handful of scripts,
most notably latin-1, when no advanced typesetting features are used.

## Complex Text Performance

We'd like to eliminate the simple text path and have all text use the current
complex text path. Doing so would allow us to support kerning and ligatures
everywhere, have similar performance regardless of script and, more importantly,
greatly reduce code complexity and reduce bugs resulting from differences in the
two code paths.

In order to do that though we must first improve the performance of the complex
path to match, or exceed, the performance of the simple path. Early measurements
here or here suggests that hb_shape() itself is not anything nearly the
bottleneck at all, and a lot of churn happens in HarfBuzzShaper, which can and
should be streamlined.

### Ideas for improving complex text performance:

*   Replace current (broken) run cache with a word cache (see below for
            more ideas on how to cache width info), ala Firefox.
*   Normalize text directly on harfbuzz buffer, avoiding extra copy of
            string.
*   Add support for latin-1 text to shaper, avoiding upconverting
            latin-1 text to utf-16.
*   Remove overhead of deciding which code path to use.
*   Do not repeatedly setFontFeatures on each shaper construction
*   Do not create list of HarfBuzzRun’s, etc, just to free them after
            iterating over them once. Copy to output buffer directly from
            hb_buffer().
*   Switch to HarfBuzz-based itemization; this, if done properly, will
            result in improved performance; but is mainly a correctness feature.
*   Keep information about HarfBuzzRun segmentation in a parallel data
            structure, see below.

## Details

### Keeping Word Length Measurements / Thoughts on Caching

Word length measurements are a function of a styled font instance, language
information, script information, and a string of text and it’s surrounding text
or context.

length = f(font, language, script, text, context)

Instead of continuously calling constructTextRun and textWidth during layout,
then doing this all over again for selections, dirty rectangles and so on, we
can perhaps keep a separate text tree that contains the information required for
length measurements and the strings. Then RenderObjects would just reference
portions of this text tree, and can directly derive width information.

Instead of a “singleton word cache” somewhere with only an eviction heuristic or
a fixed size, we could have stricter lifecycle control over the objects in such
a tree - they would stay in sync with the RenderObjects that reference them.
When more text containing nodes are added to the DOM, new objects in the text
tree are created. When DOM Nodes are deleted, we no longer need portions of the
text tree.

Note that the size of a word is not always easy to define for languages that do
not necessarily use an explicit word separator such as a space character.

### HarfBuzzRun Segmentation

The parallel data structure as outlined above, should also keep information
about shaping run segmentation, and - if needed - recompute that on a finer
granularity, if text changes.

### Optimize hb_shape()

HarfBuzz’s OpenType processing is much faster than alternatives we have
measured. Still, the kerning feature in Roboto takes half of the shaping time.
That’s way too excessive. In general, comparing hb-shape’s performance with
--shaper=ot against --shaper=fallback shows how far off we are from a barebone
fastpath. Currently this number stands at about 10x, depending on what font and
what font_funcs implementation. More measurement and optimizations here are
possible.

### Benchmarks

We should restructure our telemetry benchmarks specifically for text measurement
performance, perhaps have a separate group of tests besides blink_perf.Layout.

In addition to that, we need to develop new test cases, to better cover real
world text-performance heavy use cases.

Out of that effort we could develop a publicly available text layout benchmark,
to compare Blink performance against other engines. The Sunspider, Kraken of
text layout.

### Unit Testing

We should create a native test harness for the Font.cpp and HarfBuzzShaper.cpp
logic.

In order to improve our shaping correctness and perhaps do performance tests on
a level that cuts out blink layout etc. we should try to create a test harness
for Font.cpp where we can instantiate fonts by name, size and font features.
Then we can test for how runs are formed, shaping results, etc.

## Eliminating Simple Text

Once the complex text path is as fast (or nearly as fast) as the current simple
text path we should force all text down the complex text path and (eventually)
eliminate the simple text path entirely. This will give us an extra performance
win as it allows all text path branching to be removed, including costly string
scans. It would also allow us to enable typesetting features, such as kerning
and ligatures, for all text. More importantly though it would allow us to have a
single unified text layout path which in turn would reduce complexity and
maintenance costs as well as as ensure consistent performance regardless of
script.
