---
breadcrumbs:
- - /Home
  - Chromium
page_name: chromium-clobber-landmines
title: Chromium Clobber Landmines
---

[TOC]

## What are they?

Clobber landmines are an easy way to send a targeted clobber to a class of
builder machines (i.e. bot machines on build.chromium.org). This can be useful
in the following circumstances:

*   A build breakage is discovered and requires all the bots for a given
            operating system to clobber
*   You know you're making a change which will introduce binary
            incompatibility

Landmines have the property that if a bot 'rolls over' the revision containing
them (either forwards or backwards), it will trigger a clobber. This is quite
useful for trybots which can be assigned random base revisions, or may even just
switch between HEAD and LKGR multiple times per day. Simply clobbering the bot
manually once may not be enough to make sure that it's 'fixed' for good.

## How do I use them?

Simply add logic to
[`build/get_landmines.py`](https://code.google.com/p/chromium/codesearch#chromium/src/build/get_landmines.py)
in the
[`print_landmines`](https://code.google.com/p/chromium/codesearch#chromium/src/build/get_landmines.py&q=print_landmines)`()`
function. There are a variety of parameters which you can use to fine-tune the
effect of a landmine:

<table>
<tr>
<td> <b>Landmine parameter/function</b> </td>
<td> <b>Example values</b></td>
<td> <b>Description</b></td>
</tr>
<tr>
<td> platform()</td>
<td> 'win', 'mac', 'linux', 'ios', or 'android' </td>
<td> target platform being built.</td>
</tr>
<tr>
<td> gyp_defines()</td>
<td> {'component': 'static_library'} ...</td>
<td> dict()-ification of the GYP_DEFINES env var.</td>
</tr>
<tr>
<td> distributor() </td>
<td> 'goma', 'ib', or None</td>
<td> the distributed compile technology in use for this build.</td>
</tr>
<tr>
<td> builder()</td>
<td> 'make', 'ninja', 'xcode', or 'msvs'</td>
<td> the builder technology for this build. </td>
</tr>
<tr>
<td> target</td>
<td> 'Release', 'Debug'</td>
<td> the target that print_landmines() is generating landmines for.</td>
</tr>
</table>

To add a landmine for the given configuration, just `add()` a message in
`print_landmines()`, conditioned on the configuration. Try to make the reason
for the landmine as descriptive as possible, citing relevant revisions, CLs or
bugs.

### Example:

```none
def print_landmines(target):
  # ... Other landmines here
  if platform() == 'win' and builder() == 'ninja':
    add('Compile on cc_unittests fails due to symbols removed in r185063.')
```

## Frequently Asked Questions:

**Q: What happens if I delete/clean up/replace landmines in get_landmines()?**

A: This is OK (as in, it won't break anything), but note that it will cause bots
which match the configuration of the removed landmines to clobber again.

Sometimes this is done intentionally: if there's a frequently-clobbered
configuration it may be expedient to only have one landmine which is replaced
when a clobber is needed.

**Q: Can I include a landmine in a CL which does other things?**

A: If you know that your change will **definitely** require some bots to
clobber, then yes, you SHOULD include it with the rest of the changes.

**Q: How do I include a landmine for a change in a different repository (e.g.,
Blink, Skia)**

A: The change to `build/get_landmines.py` should be included in the
[roll](/developers/how-tos/get-the-code#Rolling_DEPS) (i.e., the CL that changes
DEPS to update the revision). You will need to contact the current
[gardener](/developers/tree-sheriffs#TOC-What-is-a-gardener-) of your component
(listed at the top left of the [BuildBot
waterfall](http://build.chromium.org/p/chromium/waterfall?reload=120)) and have
them apply the change in their roll. See for example
[19836002](https://codereview.chromium.org/19836002) for an example of adding a
landmine during a Blink roll. If a roll lands without a landmine (say thanks to
autoroll bot), it's ok to commit a separate CL with just the landmine, ideally
as close as possible after the roll, since this leaves little gap between them
(you do not need to roll back and have a manual roll+landmine CL, as this is
rarely necessary).

**Q: I have a question that's not here, whom do I bug?**

A: Please bug [iannucci@chromium.org](mailto:iannucci@chromium.org).

## Details, please...

For a given build:

1.  Check out some version of the code
2.  landmines.py runs with the current GYP environment
    1.  This calls get_landmines() for each POSSIBLE target (not just
                the one that's going to be built).
    2.  If &lt;build_dir&gt;/&lt;target&gt;/.landmines doesn't exist,
                it's written with the result of get_landmines(&lt;target&gt;)
    3.  Else
        1.  If the result of get_landmines(&lt;target&gt;) differs from
                    the content of the .landmines file, the diff is written out
                    to &lt;build_dir&gt;/&lt;target&gt;/.landmines_triggered .
        2.  Else &lt;build_dir&gt;/&lt;target&gt;/.landmines_triggered
                    is deleted
3.  compile.py runs with a --target passed to it
    1.  if &lt;build_dir&gt;/&lt;target&gt;/.landmines_triggered exists,
                compile.py prints the contents of the file and behaves as if
                --clobber was specified on the command line.
        1.  clobbering includes removing both .landmines and
                    .landmines_triggered

## Use cases

### Moving generated files

If you move [generated files](/developers/generated-files), then you need to
clobber the build, otherwise stale files may be used (if they are found earlier
during header search). Subtly, this shows up only on *later* CLs that change the
generated file in the new location, but don't overwrite the stale one. See for
example Issue
[381111](https://code.google.com/p/chromium/issues/detail?id=381111)
([comment](https://code.google.com/p/chromium/issues/detail?id=381111#c4)).
Further, if this change happens in a separate repository (e.g., Blink), then CLs
that change the generated files don't work until the repo has rolled and a
landmine has been added. I.e., the steps are:

1.  Submit Blink CL moving generated files.
2.  Roll Blink to Chromium, including landmine (or include in separate
            followup CL).
3.  Submit Blink CL changing generated files.
